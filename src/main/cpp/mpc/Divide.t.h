/**
 * Copyright Stealth Software Technologies, Inc.
 */
namespace ff {
namespace mpc {

template<typename Large_T, typename Small_T>
DivideRandomness<Large_T, Small_T>::DivideRandomness(
    std::unique_ptr<RandomnessDispenser<
        PrefixOrRandomness<Small_T>,
        DoNotGenerateInfo>> prefixOrDispenser,
    std::unique_ptr<
        RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
        multiplyDispenser,
    std::unique_ptr<RandomnessDispenser<
        PosIntCompareRandomness<Large_T, Small_T>,
        DoNotGenerateInfo>> compareDispenser,
    std::unique_ptr<RandomnessDispenser<
        TypeCastTriple<Small_T>,
        TypeCastFromBitInfo<Small_T>>>
        smallPrimeTCTripleFromBitDispenser,
    std::unique_ptr<
        RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
        smallPrimeBeaverDispenser,
    std::unique_ptr<RandomnessDispenser<
        TypeCastTriple<Small_T>,
        TypeCastInfo<Small_T>>> smallPrimeTCTripleDispenser,

    std::unique_ptr<RandomnessDispenser<
        TypeCastTriple<Large_T>,
        TypeCastFromBitInfo<Large_T>>> endPrimeTCTripleDispenser) :
    prefixOrDispenser(std::move(prefixOrDispenser)),
    multiplyDispenser(std::move(multiplyDispenser)),
    compareDispenser(std::move(compareDispenser)),
    smallPrimeTCTripleFromBitDispenser(
        std::move(smallPrimeTCTripleFromBitDispenser)),
    smallPrimeBeaverDispenser(std::move(smallPrimeBeaverDispenser)),
    smallPrimeTCTripleDispenser(std::move(smallPrimeTCTripleDispenser)),
    endPrimeTCTripleDispenser(std::move(endPrimeTCTripleDispenser)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string Divide<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Divide large mod: ") + dec(this->info->modulus) +
      " small mod: " + dec(this->info->modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
Divide<FF_TYPES, Large_T, Small_T>::Divide(
    Large_T const & ms_x,
    Large_T const & ms_y,
    Large_T * const out,
    DivideInfo<Identity_T, Large_T, Small_T> const * const div_info,
    DivideRandomness<Large_T, Small_T> && rand) :
    randomness(std::move(rand)),
    sh_dividend(ms_x),
    sh_divisor(ms_y),
    sh_quotient(out),
    info(div_info),
    large_mult_info(
        info->revealer, BeaverInfo<Large_T>(this->info->modulus)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("handlePromise called");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("handleReceive called");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::init() {

  /* Initial Setting */
  log_debug("Divide init() called");
  log_debug("dividend share is %u", this->sh_dividend);
  log_debug("divisor share is %u", this->sh_divisor);
  log_debug("modulus is %u", this->info->modulus);

  if (this->getSelf() == *info->revealer) {
    sh_dividend++;
  }
  /* Local Computation */
  Large_T temp = 1;
  /* compute the powers of two */
  log_debug("[LC] compute powers of two");
  for (size_t i = 0; i < this->info->ell; i++) {
    log_debug("[i=%zu] %u ", i, temp);
    this->powtwos.push_back(temp);
    Large_T const two = 2;
    temp = modMul(temp, two, this->info->modulus);
  }
  /* compute the share of (t^i * y) */
  log_debug("[LC] compute [t^i*y]");
  for (size_t i = 0; i < this->info->ell; i++) {
    temp =
        modMul(this->powtwos[i], this->sh_divisor, this->info->modulus);
    log_debug("[t^%zu * y] %u ", i, temp);
    this->sh_tiy.push_back(temp);
  }
  /* Invoke Batched Compare */
  log_debug("[JC] prep batched comparison");
  this->compare_batch =
      std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  for (size_t i = 1; i < this->info->ell; i++) {
    this->compare_batch->children.emplace_back(
        new PosIntCompare<FF_TYPES, Large_T, Small_T>(
            this->sh_tiy[i - 1],
            this->sh_tiy[i],
            this->info->compareInfo,
            ::std::move(this->randomness.compareDispenser->get())));
  }
  this->invoke(std::move(this->compare_batch), this->getPeers());
  log_debug("Invoked Batched Comparison");
  this->state = awaitingBatchCompare;
  log_debug("End of Init()");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::divide_awaitingBatchCompare(
    ff::Fronctocol<FF_TYPES> & f) {
  Batch<FF_TYPES> & batch = static_cast<Batch<FF_TYPES> &>(f);
  log_debug("=> awaitingBatchCompare");
  this->sh_ais.push_back(0);
  for (size_t i = 0; i < this->info->ell - 1; i++) {
    this->sh_ais.push_back(
        static_cast<PosIntCompare<FF_TYPES, Large_T, Small_T> &>(
            *batch.children[i])
            .outputShare %
        2);
  }
  for (size_t i = 0; i < this->info->ell; i++) {
    log_debug("%u", this->sh_ais[i]);
  }
  /* Invoke Batched TypeCastFromBit */
  this->typecast_batch =
      std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  for (size_t i = 0; i < this->info->ell; i++) {
    this->typecast_batch->children.emplace_back(
        new TypeCastFromBit<FF_TYPES, Small_T>(
            this->sh_ais[i],
            this->info->smallModulus,
            this->info->revealer,
            this->randomness.smallPrimeTCTripleFromBitDispenser
                ->get()));
  }
  this->invoke(std::move(this->typecast_batch), this->getPeers());
  log_debug("Invoked Batched TypeCastFromBit");
  this->state = awaitingBatchTypecast;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::divide_awaitingBatchTypecast(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingBatchTypecast");
  Batch<FF_TYPES> & batch = static_cast<Batch<FF_TYPES> &>(f);
  for (size_t i = 0; i < this->info->ell; i++) {
    this->sh_ais_modp.push_back(
        static_cast<TypeCastFromBit<FF_TYPES, Small_T> &>(
            *batch.children[i])
            .outputBitShare);
    log_debug("typecast result %u", sh_ais_modp.back());
  }
  /* Invoke PrefixOR */
  std::unique_ptr<PrefixOr<FF_TYPES, Small_T>> pref(
      new PrefixOr<FF_TYPES, Small_T>(
          this->sh_ais_modp,
          new PrefixOrInfo<Identity_T, Small_T>(
              this->info->smallModulus,
              this->info->ell,
              this->info->revealer),
          ::std::move(this->randomness.prefixOrDispenser->get())));
  this->invoke(std::move(pref), this->getPeers());
  log_debug("PrefixOR invoked");
  this->state = awaitingPrefixOR;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::divide_awaitingPrefixOR(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingPrefixOR");
  PrefixOr<FF_TYPES, Small_T> & pref =
      static_cast<PrefixOr<FF_TYPES, Small_T> &>(f);
  /* [LC]: Compute [b_i] */
  for (size_t i = 0; i < this->info->ell; i++) {
    log_debug(
        "The %zu-th bit of output PrefixOr := %u",
        i,
        pref.orResults[i]);
    if (*this->info->revealer == this->getSelf()) {
      this->sh_bis.push_back(
          (1 + this->info->smallModulus - pref.orResults[i]) %
          this->info->smallModulus);
    } else {
      this->sh_bis.push_back(
          (this->info->smallModulus - pref.orResults[i]) %
          this->info->smallModulus);
    }
    log_debug("The [b_%zu] := %u", i, this->sh_bis[i]);
  }

  /* Invoke Batched TypeCastFromBit */
  this->typecast_batch =
      std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  for (size_t i = 0; i < this->info->ell; i++) {
    this->typecast_batch->children.emplace_back(
        new TypeCast<FF_TYPES, Small_T>(
            this->sh_bis[i],
            this->info->smallModulus,
            this->info->revealer,
            this->randomness.smallPrimeBeaverDispenser->get(),
            this->randomness.smallPrimeTCTripleDispenser->get()));
  }
  this->invoke(std::move(this->typecast_batch), this->getPeers());
  log_debug("Invoked Batched TypeCast");
  this->state = awaitingSecondBatchTypecast;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::
    divide_awaitingSecondBatchTypecast(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingBatchTypecast2");
  Batch<FF_TYPES> & batch = static_cast<Batch<FF_TYPES> &>(f);
  for (size_t i = 0; i < this->info->ell; i++) {
    this->sh_bis_bits.push_back(
        static_cast<TypeCast<FF_TYPES, Small_T> &>(*batch.children[i])
            .outputBitShare);
    log_debug("typecast result %u", sh_ais_modp.back());
  }

  /* Invoke Batched TypeCastFromBit */
  this->typecast_batch =
      std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  for (size_t i = 0; i < this->info->ell; i++) {
    this->typecast_batch->children.emplace_back(
        new TypeCastFromBit<FF_TYPES, Large_T>(
            this->sh_bis_bits[i],
            this->info->modulus,
            this->info->revealer,
            this->randomness.endPrimeTCTripleDispenser->get()));
  }
  this->invoke(std::move(this->typecast_batch), this->getPeers());
  log_debug("Invoked Batched TypeCastFromBit");
  this->state = awaitingThirdBatchTypecast;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::
    divide_awaitingThirdBatchTypecast(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingBatchTypecast3");
  Batch<FF_TYPES> & batch = static_cast<Batch<FF_TYPES> &>(f);
  for (size_t i = 0; i < this->info->ell; i++) {
    this->sh_bis_large_prime.push_back(
        static_cast<TypeCastFromBit<FF_TYPES, Large_T> &>(
            *batch.children[i])
            .outputBitShare);
    log_debug("typecast result %u", sh_bis_large_prime.back());
  }

  /* [LC]: Compute [w_0] and [c_0] */
  this->sh_wis.push_back(sh_dividend);
  this->sh_cis_bits.push_back(0x00);
  this->sh_cis.push_back(0);
  log_debug("sh_w[0] = %u", this->sh_wis[0]);
  log_debug("sh_c[0] = %u", this->sh_cis[0]);
  this->itr++; // iterator is now 1.
  /* Invoke Batched Multiply */
  this->mult_batch =
      std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  if (this->itr == 1) {
    this->w_rhs_product = 0;
  } else {
    this->mult_batch->children.emplace_back(
        new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
            this->sh_cis[this->itr - 1],
            this->sh_tiy[this->info->ell + 1 - this->itr],
            &this->w_rhs_product,
            this->randomness.multiplyDispenser->get(),
            &large_mult_info));
  } // computing [c_(i-1)]*[y]
  log_assert(this->randomness.multiplyDispenser != nullptr);
  this->mult_batch->children.emplace_back(
      new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
          this->sh_bis_large_prime[this->info->ell - this->itr],
          this->sh_tiy[this->info->ell - this->itr],
          &this->c_rhs_product,
          this->randomness.multiplyDispenser->get(),
          &large_mult_info)); // computing [c_(i-1)]*[y]
  this->invoke(std::move(this->mult_batch), this->getPeers());
  log_debug(
      "Invoked Batched Multiplication for iteration #%zu", this->itr);
  this->state = awaitingBatchMultiply;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::
    divide_awaitingBatchMultiply() {
  log_debug("Loop (%zu)", this->itr);
  log_debug("=> awaitingBatchMultiply");
  log_debug("RHS W value = %u", this->w_rhs_product);
  log_debug("RHS C value = %u", this->c_rhs_product);
  /* [LC]: compute [w_i] */
  this->sh_wis.push_back(modAdd(
      this->sh_wis[this->itr - 1],
      this->info->modulus - this->w_rhs_product,
      this->info->modulus));
  /* Invoke Compare with [w_i] and [RHS] */
  std::unique_ptr<PosIntCompare<FF_TYPES, Large_T, Small_T>> comp(
      new PosIntCompare<FF_TYPES, Large_T, Small_T>(
          this->sh_wis[this->itr],
          modAdd(
              this->sh_wis[this->itr],
              this->info->modulus - this->c_rhs_product,
              this->info->modulus),
          this->info->compareInfo,
          ::std::move(this->randomness.compareDispenser->get())));
  this->invoke(std::move(comp), this->getPeers());
  log_debug("Invoked Comparison for Interation #%zu", this->itr);
  this->state = awaitingCompare;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::divide_awaitingCompare(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingCompare");
  /* [LC]: Store [c_i] */
  PosIntCompare<FF_TYPES, Large_T, Small_T> & comp =
      static_cast<PosIntCompare<FF_TYPES, Large_T, Small_T> &>(f);
  this->sh_cis_bits.push_back(comp.outputShare % 2);

  log_debug("Loop (%zu)", this->itr);
  /* [LC]: compute [w_i] */
  /* Invoke Compare with [w_i] and [RHS] */
  std::unique_ptr<TypeCastFromBit<FF_TYPES, Large_T>> typecast(
      new TypeCastFromBit<FF_TYPES, Large_T>(
          this->sh_cis_bits[this->itr],
          this->info->modulus,
          this->info->revealer,
          this->randomness.endPrimeTCTripleDispenser->get()));
  this->invoke(std::move(typecast), this->getPeers());
  log_debug("Invoked TypeCast for Interation #%zu", this->itr);
  this->state = awaitingEndLoopTypecast;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::divide_awaitingEndLoopTypecast(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("=> awaitingEndLoopTypecast");
  TypeCastFromBit<FF_TYPES, Large_T> & tct =
      static_cast<TypeCastFromBit<FF_TYPES, Large_T> &>(f);
  this->sh_cis.push_back(tct.outputBitShare);

  log_debug(
      "Iteration #%zu Comparison outputs %u",
      this->itr,
      this->sh_cis[this->itr]);
  log_debug("Iteration #%zu Completed", this->itr);
  this->itr++; // increment the counter
  if (this->itr < this->info->ell + 1) {
    log_debug("Iteration #%zu Started", this->itr);
    /* Invoke Batched Multiply */
    this->mult_batch =
        std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
    this->mult_batch->children.emplace_back(
        new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
            this->sh_cis[this->itr - 1],
            this->sh_tiy[this->info->ell + 1 - this->itr],
            &this->w_rhs_product,
            this->randomness.multiplyDispenser->get(),
            &large_mult_info));
    log_assert(this->randomness.multiplyDispenser != nullptr);
    this->mult_batch->children.emplace_back(
        new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
            this->sh_bis_large_prime[this->info->ell - this->itr],
            this->sh_tiy[this->info->ell - this->itr],
            &this->c_rhs_product,
            this->randomness.multiplyDispenser->get(),
            &large_mult_info)); // computing [c_(i-1)]*[y]
    this->invoke(std::move(this->mult_batch), this->getPeers());
    log_debug(
        "Invoked Batched Multiplication for iteration #%zu", this->itr);
    this->state = awaitingBatchMultiply;
  } else {
    for (size_t i = 1; i < this->info->ell + 1; i++) {
      log_debug("sh_cis[%zu] := %u", i, this->sh_cis[i]);
    }
    log_debug("Iteration #%zu DID NOT Started", this->itr);
    for (size_t i = 1; i < this->info->ell + 1; i++) {
      log_debug("sh_quotient := %u", *this->sh_quotient);
      *this->sh_quotient = modAdd(
          *this->sh_quotient,
          modMul(
              this->sh_cis[i],
              this->powtwos[this->info->ell - i],
              this->info->modulus),
          this->info->modulus);
    }
    log_debug(
        "Division Completed with sh_quotient = %u", *this->sh_quotient);
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Divide<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("State MACHINE START AT %i", this->state);
  log_debug("Divide handleComplete Called");
  switch (this->state) {
    case awaitingBatchCompare: {
      this->divide_awaitingBatchCompare(f);
    } break;
    case awaitingBatchTypecast: {
      this->divide_awaitingBatchTypecast(f);
    } break;
    case awaitingPrefixOR: {
      this->divide_awaitingPrefixOR(f);
    } break;
    case awaitingSecondBatchTypecast: {
      this->divide_awaitingSecondBatchTypecast(f);
    } break;
    case awaitingThirdBatchTypecast: {
      this->divide_awaitingThirdBatchTypecast(f);
    } break;
    case awaitingBatchMultiply: {
      this->divide_awaitingBatchMultiply();
    } break;
    case awaitingCompare: {
      this->divide_awaitingCompare(f);
    } break;
    case awaitingEndLoopTypecast: {
      this->divide_awaitingEndLoopTypecast(f);
    } break;
    default:
      log_error("state machine in unexpected state");
  }
  log_debug("State MACHINE END AT %i", this->state);
}

} // namespace mpc
} // namespace ff

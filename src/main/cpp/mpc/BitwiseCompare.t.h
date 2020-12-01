/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<FF_TYPENAMES, typename Number_T>
std::string BitwiseCompare<FF_TYPES, Number_T>::name() {
  return std::string("Bitwise Compare small mod: ") +
      dec(this->prefInfo->s);
}

template<FF_TYPENAMES, typename Number_T>
BitwiseCompare<FF_TYPES, Number_T>::BitwiseCompare(
    std::vector<Number_T> & shares_of_a,
    const std::vector<Number_T> & b_in_the_clear,
    PrefixOrInfo<Identity_T, Number_T> const * const prefInfo,
    BitwiseCompareRandomness<Number_T> && randomness) :
    shares_of_a(shares_of_a),
    b_in_the_clear(b_in_the_clear),
    prefInfo(prefInfo),
    randomness(std::move(randomness)) {
  this->outputShare = 0;
  log_assert(
      this->prefInfo->lambda * this->prefInfo->lambda >
      this->shares_of_a.size());
  log_assert(this->shares_of_a.size() == this->b_in_the_clear.size());
}

template<FF_TYPENAMES, typename Number_T>
void BitwiseCompare<FF_TYPES, Number_T>::init() {
  log_debug("Calling init on BitwiseCompare");
  for (size_t i = 0; i < this->shares_of_a.size(); i++) {
    log_debug(
        "shares of a and value of b before XOR: %u, %u ",
        this->shares_of_a[i],
        this->b_in_the_clear[i]);
  }

  for (size_t i = 0; i < this->shares_of_a.size(); i++) {
    if (this->b_in_the_clear[i] == 1 &&
        *this->prefInfo->rev == this->getSelf()) {
      this->shares_of_a[i] =
          (this->prefInfo->s + 1 - this->shares_of_a[i]) %
          (this->prefInfo->s);
    } else if (this->b_in_the_clear[i] == 1) {
      this->shares_of_a[i] =
          (this->prefInfo->s - this->shares_of_a[i]) %
          (this->prefInfo->s);
    }
    log_debug("Starting share of a[%lu] = %u", i, this->shares_of_a[i]);
  }

  log_debug("this->lambda: %zu", this->lambda);

  std::unique_ptr<PrefixOr<FF_TYPES, Number_T>> pref(
      new PrefixOr<FF_TYPES, Number_T>(
          this->shares_of_a,
          this->prefInfo,
          std::move(this->randomness.randomness)));
  this->invoke(std::move(pref), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void BitwiseCompare<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in BitwiseCompare");
}

template<FF_TYPENAMES, typename Number_T>
void BitwiseCompare<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in PrefixOr");
}

template<FF_TYPENAMES, typename Number_T>
void BitwiseCompare<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete");
  // case logic
  switch (this->state) {
    case awaitingPrefixOr: {
      log_debug("Case awaitingPrefixOr");
      PrefixOr<FF_TYPES, Number_T> & pref =
          static_cast<PrefixOr<FF_TYPES, Number_T> &>(f);

      Number_T compare_val_mod_s = 0;
      log_debug(
          "Share of prefix or [%lu] = %u", 0UL, pref.orResults[0]);
      if (this->b_in_the_clear[0] == 1) {
        compare_val_mod_s = modAdd(
            compare_val_mod_s, pref.orResults[0], this->prefInfo->s);
      }
      for (size_t i = 1; i < this->shares_of_a.size(); i++) {
        log_debug(
            "Share of prefix or [%zu] = %u and b = %u",
            i,
            pref.orResults[i],
            this->b_in_the_clear[i]);
        if (this->b_in_the_clear[i] == 1) {
          compare_val_mod_s = modAdd(
              modAdd(
                  compare_val_mod_s,
                  pref.orResults[i],
                  this->prefInfo->s),
              this->prefInfo->s - pref.orResults[i - 1],
              this->prefInfo->s);
        }
      }

      this->equalityCheckValue = this->prefInfo->s -
          pref.orResults[this->shares_of_a.size() - 1];
      if (*this->prefInfo->rev == this->getSelf()) {
        this->equalityCheckValue += 1;
      }
      this->equalityCheckValue %= this->prefInfo->s;
      log_debug(
          "Invoking tcb on value: %u with beaver triple %u %u %u and "
          "tcTriple %u %u %u",
          compare_val_mod_s,
          this->randomness.beaver.a,
          this->randomness.beaver.b,
          this->randomness.beaver.c,
          this->randomness.tcTriple.r_0,
          this->randomness.tcTriple.r_1,
          this->randomness.tcTriple.r_2);

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

      batch->children.emplace_back(new TypeCast<FF_TYPES, Number_T>(
          compare_val_mod_s,
          this->prefInfo->s,
          this->prefInfo->rev,
          this->randomness.beaver,
          this->randomness.tcTriple));
      batch->children.emplace_back(new TypeCast<FF_TYPES, Number_T>(
          this->equalityCheckValue,
          this->prefInfo->s,
          this->prefInfo->rev,
          this->randomness.beaver2,
          this->randomness.tcTriple2));

      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingBatchTypeCastBit;
    } break;
    case awaitingBatchTypeCastBit: {
      outputShare =
          static_cast<TypeCast<FF_TYPES, Number_T> *>(
              static_cast<Batch<FF_TYPES> &>(f).children[0].get())
              ->outputBitShare;

      outputShare = static_cast<Boolean_t>(
          outputShare +
          2 *
              static_cast<TypeCast<FF_TYPES, Number_T> *>(
                  static_cast<Batch<FF_TYPES> &>(f).children[1].get())
                  ->outputBitShare);
      this->complete();
    } break;
    default:
      log_error("BitwiseCompare state machine in unexpected state");
  }
}

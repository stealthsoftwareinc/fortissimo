/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {

template<typename Identity_T, typename Large_T, typename Small_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::DecomposedBitSetInfo<Large_T, Small_T> & info) {
  bool success = true;
  success = success && msg.template read<Large_T>(info.p);
  success = success && msg.template read<Small_T>(info.s);
  uint64_t ell64 = 0UL;
  success = success && msg.template read<uint64_t>(ell64);
  info.ell = (size_t)ell64;

  return success;
}

template<typename Identity_T, typename Large_T, typename Small_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::DecomposedBitSetInfo<Large_T, Small_T> const & info) {
  bool success = true;
  success = success && msg.template write<Large_T>(info.p);
  success = success && msg.template write<Small_T>(info.s);
  success = success && msg.template write<uint64_t>((uint64_t)info.ell);

  return success;
}

template<typename Identity_T, typename Large_T, typename Small_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::DecomposedBitSet<Large_T, Small_T> & dbs) {
  bool success = true;
  success = success && msg.template read<Large_T>(dbs.r);

  uint64_t ell64 = 0UL;
  success = success && msg.template read<uint64_t>(ell64);
  size_t ell = (size_t)ell64;
  dbs.r_is.reserve(ell);

  Small_T ri_input;
  for (size_t i = 0; i < ell; i++) {
    success = success && msg.template read<Small_T>(ri_input);
    dbs.r_is.emplace_back(ri_input);
  }

  success = success && msg.template read<Boolean_t>(dbs.r_0);

  return success;
}

template<typename Identity_T, typename Large_T, typename Small_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::DecomposedBitSet<Large_T, Small_T> const & dbs) {
  bool success = true;
  success = success && msg.template write<Large_T>(dbs.r);

  success = success &&
      msg.template write<uint64_t>((uint64_t)dbs.r_is.size());

  for (size_t i = 0; i < dbs.r_is.size(); i++) {
    success = success && msg.template write<Small_T>(dbs.r_is[i]);
  }

  success = success && msg.template write<Boolean_t>(dbs.r_0);

  return success;
}

namespace mpc {

template<typename Large_T, typename Small_T>
DecomposedBitSet<Large_T, Small_T>::DecomposedBitSet(
    Large_T const & r, std::vector<Small_T> && r_is, Boolean_t r_0) :
    r(r), r_is(std::move(r_is)), r_0(r_0) {
}

template<typename Large_T, typename Small_T>
DecomposedBitSetInfo<Large_T, Small_T>::DecomposedBitSetInfo(
    const Large_T & p, const Small_T & s, const size_t ell) :
    p(p), s(s), ell(ell) {
}

template<typename Large_T, typename Small_T>
bool DecomposedBitSetInfo<Large_T, Small_T>::operator==(
    DecomposedBitSetInfo<Large_T, Small_T> const & other) const {
  return this->p == other.p && this->s == other.s &&
      this->ell == other.ell;
}

template<typename Large_T, typename Small_T>
void DecomposedBitSetInfo<Large_T, Small_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<DecomposedBitSet<Large_T, Small_T>> & vals) const {

  log_debug("Generating random dbs object");
  /** Step 1. Randomly create the "original" randomness instance. */
  Large_T const r = randomModP<Large_T>(this->p);

  log_debug(
      "prime: %s, r = %s, this->ell: %zu",
      dec(this->p).c_str(),
      dec(r).c_str(),
      this->ell);

  std::vector<Small_T> r_bits(this->ell);

  Boolean_t r_0 = static_cast<Boolean_t>(r % 2);
  Large_T r_copy = r;

  for (size_t i = 0; i < this->ell; i++) {
    r_bits[this->ell - 1 - i] = static_cast<Small_T>(r_copy % 2);
    r_copy /= 2;
  }

  /** Step 2. randomly secret share the original. */

  vals.resize(n_parties);

  for (size_t i = 1; i < n_parties; i++) {
    vals[i].r = randomModP<Large_T>(this->p);
  }

  for (size_t i = 1; i < n_parties; i++) {
    vals[i].r_is.resize(this->ell);
    for (size_t j = 0; j < this->ell; j++) {
      vals[i].r_is[j] = randomModP<Small_T>(this->s);
    }
  }

  for (size_t i = 1; i < n_parties; i++) {
    vals[i].r_0 = randomModP<Boolean_t>(2);
  }

  /** Step 3. The last secret share is computed from the priors. */

  vals[0].r = r;
  for (size_t i = 1; i < n_parties; i++) {
    vals[0].r = vals[0].r + (this->p - vals[i].r);
    vals[0].r %= this->p;
  }

  vals[0].r_is.resize(this->ell);
  for (size_t i = 0; i < this->ell; i++) {
    vals[0].r_is[i] = r_bits[i];
    for (size_t j = 1; j < n_parties; j++) {
      vals[0].r_is[i] += (this->s - vals[j].r_is[i]);
      vals[0].r_is[i] %= this->s;
    }
  }

  vals[0].r_0 = r_0;
  for (size_t i = 1; i < n_parties; i++) {
    vals[0].r_0 = vals[0].r_0 ^ vals[i].r_0;
  }

  if_debug {
    Large_T test_r_sum = 0;
    for (size_t i = 0; i < this->ell; i++) {
      Small_T test_r_partial_sum = vals[0].r_is[this->ell - 1 - i];
      for (size_t j = 1; j < n_parties; j++) {
        test_r_partial_sum =
            test_r_partial_sum + vals[j].r_is[this->ell - 1 - i];
      }
      test_r_partial_sum = test_r_partial_sum % this->s;
      Large_T test_r_partial_sum_large = Large_T(test_r_partial_sum);
      test_r_partial_sum_large = test_r_partial_sum_large << i;
      log_debug(
          "test adjusted bit %s",
          dec(test_r_partial_sum_large).c_str());
      test_r_sum += test_r_partial_sum_large;
    }
    log_debug("test sum %s", dec(test_r_sum).c_str());
    log_assert(test_r_sum == r);
    Boolean_t test_r_0 = vals[0].r_0;
    for (size_t j = 1; j < n_parties; j++) {
      test_r_0 = test_r_0 ^ vals[j].r_0;
    }
    log_assert(test_r_0 == r_0);
  }
}

template<typename Identity_T, typename Large_T, typename Small_T>
CompareInfo<Identity_T, Large_T, Small_T>::CompareInfo(
    const Large_T & modulus, Identity_T const * const revealer) :
    p(modulus),
    s(nextPrime(static_cast<Small_T>(approxLog2(modulus)) + 3)),
    ell(static_cast<size_t>(approxLog2(modulus)) + 1),
    lambda(
        static_cast<size_t>(std::sqrt(static_cast<double>(this->ell))) +
        1),
    lagrangePolynomialSet(generateLagrangePolynomialSet(
        this->ell, this->lambda, this->s)),
    revealer(revealer),
    prefInfo(
        this->s,
        this->lambda,
        this->ell,
        this->lagrangePolynomialSet,
        this->revealer) {
  log_debug("prefInfo s: %s", dec(this->prefInfo.s).c_str());
}

template<typename Identity_T, typename Large_T, typename Small_T>
CompareInfo<Identity_T, Large_T, Small_T>::CompareInfo(
    Large_T const & p,
    Small_T const & s,
    size_t const ell,
    size_t const lambda,
    std::vector<std::vector<Small_T>> const & lagrangePolynomialSet,
    const Identity_T * revealer) :
    p(p),
    s(s),
    ell(ell),
    lambda(lambda),
    lagrangePolynomialSet(lagrangePolynomialSet),
    revealer(revealer),
    prefInfo(
        this->s,
        this->lambda,
        this->ell,
        this->lagrangePolynomialSet,
        this->revealer) {
  log_debug("prefInfo s: %s", dec(this->prefInfo.s).c_str());
}

template<typename Large_T, typename Small_T>
CompareRandomness<Large_T, Small_T>::CompareRandomness(
    std::vector<ExponentSeries<Small_T>> && exponentSeries,
    std::unique_ptr<
        RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
        multiplyDispenser,
    TypeCastTriple<Small_T> && Tct1,
    TypeCastTriple<Small_T> && Tct2,
    DecomposedBitSet<Large_T, Small_T> && singleDbs) :
    exponentSeries(std::move(exponentSeries)),
    multiplyDispenser(std::move(multiplyDispenser)),
    Tct1(std::move(Tct1)),
    Tct2(std::move(Tct2)),
    singleDbs(std::move(singleDbs)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string Compare<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Compare large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
Compare<FF_TYPES, Large_T, Small_T>::Compare(
    Large_T const & share_of_x, // mod p
    Large_T const & share_of_y, // mod p
    CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo,
    CompareRandomness<Large_T, Small_T> && randomness) :
    share_of_x(share_of_x),
    share_of_y(share_of_y),
    compareInfo(compareInfo),
    randomness(std::move(randomness)) {
  log_assert(
      this->compareInfo->lambda * this->compareInfo->lambda >
      this->compareInfo->ell);
  this->dbSet = this->randomness.singleDbs;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Compare<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on Compare");

  std::unique_ptr<Reveal<FF_TYPES, Large_T>> reveal(
      new Reveal<FF_TYPES, Large_T>(
          ((this->compareInfo->p + this->share_of_x -
            this->share_of_y) %
               this->compareInfo->p * 2 +
           this->dbSet.r) %
              this->compareInfo->p,
          this->compareInfo->p,
          this->compareInfo->revealer));
  this->invoke(std::move(reveal), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Compare<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in Compare");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Compare<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in Compare");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void Compare<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete in Compare");
  // case logic
  switch (this->state) {
    case awaitingReveal: {
      log_debug("Case awaitingReveal in Compare");
      Reveal<FF_TYPES, Large_T> & reveal =
          static_cast<Reveal<FF_TYPES, Large_T> &>(f);
      Large_T c = reveal.openedValue;
      log_debug("Revealed value %s", dec(c).c_str());

      this->c_bits_mod_s.clear();
      this->c_bits_mod_s.resize(this->compareInfo->ell, 0);

      for (size_t i = 0; i < this->compareInfo->ell; i++) {
        this->c_bits_mod_s[this->compareInfo->ell - 1 - i] =
            static_cast<Small_T>(c % 2);
        c /= 2;
      }

      BeaverTriple<Small_T> beaver1 =
          this->randomness.multiplyDispenser->get();
      BeaverTriple<Small_T> beaver2 =
          this->randomness.multiplyDispenser->get();

      std::unique_ptr<BitwiseCompare<FF_TYPES, Small_T>> bitwiseCompare(
          new BitwiseCompare<FF_TYPES, Small_T>(
              this->dbSet.r_is,
              this->c_bits_mod_s,
              &this->compareInfo->prefInfo,
              BitwiseCompareRandomness<Small_T>(
                  PrefixOrRandomness<Small_T>(
                      std::move(this->randomness.exponentSeries),
                      std::move(this->randomness.multiplyDispenser)),
                  beaver1,
                  this->randomness.Tct1,
                  beaver2,
                  this->randomness.Tct2)));

      this->invoke(std::move(bitwiseCompare), this->getPeers());
      this->state = awaitingBitwiseCompare;

    } break;
    case awaitingBitwiseCompare: {
      log_debug("Case awaitingBitwiseCompare");
      if (this->getSelf() == *this->compareInfo->revealer) {
        log_debug(
            "XORing values: %u, %u, %u",
            static_cast<BitwiseCompare<FF_TYPES, Small_T> &>(f)
                .outputShare,
            (Boolean_t)this->dbSet.r_0,
            (Boolean_t)this->c_bits_mod_s[this->compareInfo->ell - 1]);
        outputShare =
            static_cast<BitwiseCompare<FF_TYPES, Small_T> &>(f)
                .outputShare ^
            (Boolean_t)this->dbSet.r_0 ^
            (Boolean_t)this->c_bits_mod_s[this->compareInfo->ell - 1];
      } else {
        outputShare =
            static_cast<BitwiseCompare<FF_TYPES, Small_T> &>(f)
                .outputShare ^
            (Boolean_t)this->dbSet.r_0;
      }
      this->complete();
    } break;
    default:
      log_error("Compare state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

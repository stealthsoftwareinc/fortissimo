/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {

template<typename Identity_T, typename Number_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::ExponentSeriesInfo<Number_T> & info) {
  bool success = msg.template read<Number_T>(info.p);
  uint64_t ell64 = 0;
  success = success && msg.template read<uint64_t>(ell64);
  info.ell = (size_t)ell64;
  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::ExponentSeriesInfo<Number_T> const & info) {
  bool success = msg.template write<Number_T>(info.p);
  success = success && msg.template write<uint64_t>((uint64_t)info.ell);
  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::ExponentSeries<Number_T> & es) {
  bool success = true;
  uint64_t ell_plus_1_64 = 0;
  success = msg.template read<uint64_t>(ell_plus_1_64);
  // since this is generic code for a vector of ArithmeticShare_t,
  // probably best to send over the real size and not the off-by-one parameter
  size_t ell_plus_one = ell_plus_1_64;

  es.reserve(ell_plus_one);
  Number_T a = 0;

  for (size_t i = 0; i < ell_plus_one; i++) {
    success = success && msg.template read<Number_T>(a);
    es.emplace_back(a);
  }

  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::ExponentSeries<Number_T> const & es) {
  bool success = true;

  // since this is generic code for a vector of ArithmeticShare_t,
  // probably best to send over the real size and not the off-by-one parameter
  success =
      success && msg.template write<uint64_t>((uint64_t)es.size());

  for (size_t i = 0; i < es.size(); i++) {
    success = success && msg.template write<Number_T>(es[i]);
  }

  return success;
}

namespace mpc {

template<typename Number_T>
void ExponentSeriesInfo<Number_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<ExponentSeries<Number_T>> & vals) const {

  Number_T r = 0;
  while (r == 0) {
    r = randomModP<Number_T>(this->p);
  }
  Number_T const r_inverse = modInvert(r, this->p);

  log_debug("r = %u", r);
  log_debug("r inverse = %u", r_inverse);
  log_debug("r * r inverse = %u", (r * r_inverse) % this->p);

  ExponentSeries<Number_T> last_e;
  last_e.reserve(this->ell + 1);
  Number_T r_pow = r;
  for (size_t i = 0; i < this->ell; i++) {
    last_e.emplace_back(r_pow);
    r_pow = modMul(r, r_pow, this->p);
  }
  last_e.emplace_back(r_inverse);

  vals.resize(n_parties);

  /** Step 2. randomly secret share the original. */
  for (size_t i = 1; i < n_parties; i++) {
    vals[i].resize(this->ell + 1);
    for (size_t j = 0; j < this->ell + 1; j++) {
      vals[i][j] = randomModP<Number_T>(this->p);
    }
  }

  /** Step 3. The last secret share is computed from the priors. */
  vals[0].resize(this->ell + 1);
  for (size_t i = 0; i < this->ell + 1; i++) {
    vals[0][i] = last_e[i];
    for (size_t j = 1; j < n_parties; j++) {
      vals[0][i] += (this->p - vals[j][i]);
    }
    vals[0][i] %= this->p;
  }

  r_pow = r;
  if_debug {
    for (size_t i = 0; i < this->ell; i++) {
      Number_T test_r_pow = vals[0][i];
      for (size_t j = 1; j < vals.size(); j++) {
        test_r_pow = test_r_pow + vals[j][i];
      }
      test_r_pow = test_r_pow % this->p;

      log_assert(test_r_pow == r_pow);
      r_pow = modMul(r, r_pow, this->p);
    }
    Number_T test_r_inv = vals[0][this->ell];
    for (size_t j = 1; j < vals.size(); j++) {
      test_r_inv = test_r_inv + vals[j][this->ell];
    }
    test_r_inv = test_r_inv % this->p;

    log_assert(test_r_inv == r_inverse);
  }
}

template<FF_TYPENAMES, typename Number_T>
std::string UnboundedFaninOr<FF_TYPES, Number_T>::name() {
  return std::string("Unbounded Fanin Or small mod: ") +
      dec(this->info->s);
}

template<FF_TYPENAMES, typename Number_T>
UnboundedFaninOr<FF_TYPES, Number_T>::UnboundedFaninOr(
    Number_T sov,
    size_t nv,
    Number_T * const out,
    ExponentSeries<Number_T> && e,
    BeaverTriple<Number_T> && b,
    ::std::vector<Number_T> const & lgp,
    UnboundedFaninOrInfo<Identity_T, Number_T> const * const i) :
    sumOfValues(sov),
    numValues(nv),
    output(out),
    es(::std::move(e)),
    beaver(::std::move(b)),
    lagrangePolynomial(lgp),
    info(i),
    multInfo(i->revealer, BeaverInfo<Number_T>(i->s)) {
}

template<FF_TYPENAMES, typename Number_T>
void UnboundedFaninOr<FF_TYPES, Number_T>::init() {
  log_debug("Dataowner UnboundedFaninOr init");

  for (auto e : this->es) {
    randomSeries.push_back(e);
  }
  this->initAfterRandomness();
}

template<FF_TYPENAMES, typename Number_T>
void UnboundedFaninOr<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected unbounded fanin or handle promise");
}

template<FF_TYPENAMES, typename Number_T>
void UnboundedFaninOr<FF_TYPES, Number_T>::initAfterRandomness() {
  log_debug("unbounded fanin or init after randomness");
  this->A = this->sumOfValues;
  if (*this->info->revealer == this->getSelf()) {
    this->A += 1;
  }

  std::unique_ptr<Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>>
      mf(new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
          this->A,
          this->randomSeries[this->randomSeries.size() - 1],
          &this->ATimesRinvShare,
          ::std::move(this->beaver),
          &this->multInfo));

  this->invoke(::std::move(mf), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void UnboundedFaninOr<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T & imsg) {
  log_error("Unexpected handleReceive in UnboundedFaninOr");
  (void)imsg;
}

template<FF_TYPENAMES, typename Number_T>
void UnboundedFaninOr<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("UnboundedFaninOr handle complete");
  switch (this->completedState) {
    case multiply: {
      log_debug("Case multiply");
      std::unique_ptr<Reveal<FF_TYPES, Number_T>> rf(
          new Reveal<FF_TYPES, Number_T>(
              this->ATimesRinvShare,
              this->info->s,
              this->info->revealer));

      this->invoke(std::move(rf), this->getPeers());
      this->completedState = reveal;
    } break;
    case reveal: {
      log_debug("Case reveal");

      Number_T A_times_rinv =
          static_cast<Reveal<FF_TYPES, Number_T> &>(f).openedValue;

      log_debug("A times r^inv = %u", A_times_rinv);

      Number_T A_times_rinv_pow = 1;
      Number_T polynomial_eval = 0;
      if (*this->info->revealer == this->getSelf()) {
        polynomial_eval = this->lagrangePolynomial[0];
      }
      log_debug(
          "lagrangePolynomial[0] = %u", this->lagrangePolynomial[0]);
      for (size_t i = 1; i < this->lagrangePolynomial.size(); i++) {
        log_debug(
            "lagrangePolynomial[%lu] = %u",
            i,
            this->lagrangePolynomial[i]);

        A_times_rinv_pow =
            modMul(A_times_rinv_pow, A_times_rinv, this->info->s);
        polynomial_eval += modMul(
            modMul(
                this->lagrangePolynomial[i],
                A_times_rinv_pow,
                this->info->s),
            this->randomSeries[i - 1],
            this->info->s);
        polynomial_eval %= this->info->s;

        log_debug("polynomial_eval = %lu", polynomial_eval);
        log_debug("randomSeries[%lu] = %u", i - 1, randomSeries[i - 1]);
      }

      *(this->output) = polynomial_eval;
      log_debug("Value of result share is %u", *output);
      this->complete();
    } break;
    default:
      log_error("UnboundedFaninOr state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

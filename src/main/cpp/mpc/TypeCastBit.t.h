/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<typename Identity_T, typename Number_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::TypeCastInfo<Number_T> & info) {
  return msg.template read<Number_T>(info.modulus);
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::TypeCastInfo<Number_T> const & info) {
  return msg.template write<Number_T>(info.modulus);
}

template<typename Identity_T, typename Number_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::TypeCastTriple<Number_T> & triple) {
  bool success = true;
  success = success && msg.template read<Number_T>(triple.r_0);
  success = success && msg.template read<Number_T>(triple.r_1);
  success = success && msg.template read<Boolean_t>(triple.r_2);
  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::TypeCastTriple<Number_T> const & triple) {
  bool success = true;
  success = success && msg.template write<Number_T>(triple.r_0);
  success = success && msg.template write<Number_T>(triple.r_1);
  success = success && msg.template write<Boolean_t>(triple.r_2);
  return success;
}

namespace mpc {

template<typename Number_T>
TypeCastTriple<Number_T>::TypeCastTriple(
    Number_T r_0, Number_T r_1, Boolean_t r_2) :
    r_0(r_0), r_1(r_1), r_2(r_2) {
}

template<typename Number_T>
TypeCastInfo<Number_T>::TypeCastInfo(const Number_T modulus) :
    modulus(modulus) {
}

template<typename Number_T>
void TypeCastInfo<Number_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<TypeCastTriple<Number_T>> & vals) const {

  /** Step 1. Randomly create the "original" randomness instance. */
  auto const r_2 = randomModP<Boolean_t>(2);

  Number_T const r_0 = (r_2 == 0) ? 1 : this->modulus - 1;
  Number_T const r_1 = (r_2 == 0) ? 0 : 1;

  TypeCastTriple<Number_T> const og(r_0, r_1, r_2);

  /** Step 2. randomly secret share the original. */
  vals.reserve(n_parties);
  for (size_t i = 1; i < n_parties; i++) {
    auto sr_2 = randomModP<Boolean_t>(2);

    auto sr_0 = randomModP<Number_T>(this->modulus);
    auto sr_1 = randomModP<Number_T>(this->modulus);

    vals.emplace_back(sr_0, sr_1, sr_2);
  }

  /** Step 3. The last secret share is computed from the priors. */
  auto lr_2 = og.r_2;
  auto lr_0 = og.r_0;
  auto lr_1 = og.r_1;
  for (TypeCastTriple<Number_T> & prev : vals) {
    lr_2 = lr_2 ^ prev.r_2;
    lr_0 = (lr_0 + (this->modulus - prev.r_0)) % this->modulus;
    lr_1 = (lr_1 + (this->modulus - prev.r_1)) % this->modulus;
  }
  vals.emplace_back(lr_0, lr_1, lr_2);

  if_debug {
    auto tr_2 = vals.front().r_2;
    auto tr_0 = vals.front().r_0;
    auto tr_1 = vals.front().r_1;
    for (size_t i = 1; i < vals.size(); i++) {
      tr_2 = tr_2 ^ vals[i].r_2;
      tr_0 = (tr_0 + vals[i].r_0) % this->modulus;
      tr_1 = (tr_1 + vals[i].r_1) % this->modulus;
    }

    log_assert(tr_0 == og.r_0);
    log_assert(tr_1 == og.r_1);
    log_assert(tr_2 == og.r_2);
  }
}

template<FF_TYPENAMES, typename Number_T>
std::string TypeCast<FF_TYPES, Number_T>::name() {
  return std::string("TypeCast modulus: ") + dec(this->modulus);
}

template<FF_TYPENAMES, typename Number_T>
TypeCast<FF_TYPES, Number_T>::TypeCast(
    const Number_T & arithmeticShareOfBit,
    const Number_T & modulus,
    const Identity_T * rev,
    const BeaverTriple<Number_T> & beaverTriple,
    const TypeCastTriple<Number_T> & tcTriple) :
    arithmeticShareOfBit(arithmeticShareOfBit),
    modulus(modulus),
    revealer(rev),
    beaverTriple(beaverTriple),
    tcTriple(tcTriple),
    multiplyInfo(rev, modulus) {
}

template<FF_TYPENAMES, typename Number_T>
void TypeCast<FF_TYPES, Number_T>::init() {
  log_debug(
      "Calling init on TypeCast with beaver triple %u %u %u and "
      "tcTriple %u %u %u and value %u",
      this->beaverTriple.a,
      this->beaverTriple.b,
      this->beaverTriple.c,
      this->tcTriple.r_0,
      this->tcTriple.r_1,
      this->tcTriple.r_2,
      this->arithmeticShareOfBit);

  std::unique_ptr<Fronctocol<FF_TYPES>> multiply(
      new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
          this->arithmeticShareOfBit,
          this->tcTriple.r_0,
          &this->multiplyResult,
          std::move(this->beaverTriple),
          &this->multiplyInfo));

  this->multiplyResult = 0;
  this->invoke(std::move(multiply), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void TypeCast<FF_TYPES, Number_T>::handlePromise(
    Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in TypeCastBit");
}

template<FF_TYPENAMES, typename Number_T>
void TypeCast<FF_TYPES, Number_T>::handleReceive(IncomingMessage_T &) {
  log_error("Unexpected handleReceive in TypeCastBit");
}

template<FF_TYPENAMES, typename Number_T>
void TypeCast<FF_TYPES, Number_T>::handleComplete(
    Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete on TypeCastBit");
  // case logic
  switch (this->state) {
    case awaitingMultiply: {
      log_debug("Case awaitingMultiply");
      std::unique_ptr<Fronctocol<FF_TYPES>> reveal(
          new Reveal<FF_TYPES, Number_T>(
              (this->multiplyResult + this->tcTriple.r_1) %
                  this->modulus,
              this->modulus,
              this->revealer));
      log_debug(
          "Sending to reveal: %u",
          (this->multiplyResult + this->tcTriple.r_1) % this->modulus);
      this->invoke(std::move(reveal), this->getPeers());
      this->state = awaitingReveal;
    } break;
    case awaitingReveal: {
      log_debug("Case awaitingReveal");
      auto ov =
          static_cast<Reveal<FF_TYPES, Number_T> &>(f).openedValue;
      log_debug("opened value is %u", ov);
      log_assert(ov == 0 || ov == 1);
      if (this->getSelf() == *this->revealer) {
        log_debug(
            "Type cast to %u in TypeCastBit from %u, %u",
            static_cast<Boolean_t>(ov) ^ this->tcTriple.r_2,
            ov,
            this->tcTriple.r_2);
        outputBitShare =
            static_cast<Boolean_t>(ov ^ this->tcTriple.r_2);
      } else {
        log_debug("Type cast to %u in TypeCastBit", this->tcTriple.r_2);
        outputBitShare = this->tcTriple.r_2;
      }
      this->complete();
    } break;
    default:
      log_error("TypeCastBit state machine in unexpected state");
  }
}
} // namespace mpc

/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {

template<typename Identity_T, typename Number_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::BeaverInfo<Number_T> & info) {
  return msg.template read<Number_T>(info.modulus);
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::BeaverInfo<Number_T> const & info) {
  return msg.template write<Number_T>(info.modulus);
}

template<typename Identity_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> &, mpc::BooleanBeaverInfo &) {
  return true;
}

template<typename Identity_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> &, mpc::BooleanBeaverInfo const &) {
  return true;
}

template<typename Identity_T, typename Number_t>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg,
    mpc::BeaverTriple<Number_t> & beaver) {
  bool success = true;
  success = success && msg.template read<Number_t>(beaver.a);
  success = success && msg.template read<Number_t>(beaver.b);
  success = success && msg.template read<Number_t>(beaver.c);

  return success;
}

template<typename Identity_T, typename Number_t>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg,
    mpc::BeaverTriple<Number_t> const & beaver) {
  bool success = true;
  success = success && msg.template write<Number_t>(beaver.a);
  success = success && msg.template write<Number_t>(beaver.b);
  success = success && msg.template write<Number_t>(beaver.c);

  return success;
}

namespace mpc {

template<typename Number_t>
BeaverTriple<Number_t>::BeaverTriple(
    Number_t a, Number_t b, Number_t c) :
    a(a), b(b), c(c) {
}

template<typename Number_T>
void BeaverInfo<Number_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<BeaverTriple<Number_T>> & vals) const {
  Number_T a = randomModP<Number_T>(this->modulus);
  Number_T b = randomModP<Number_T>(this->modulus);
  Number_T c = modMul<Number_T>(a, b, this->modulus);

  vals.clear();
  vals.reserve(n_parties);

  ::std::vector<Number_T> a_shares;
  ::std::vector<Number_T> b_shares;
  ::std::vector<Number_T> c_shares;
  a_shares.reserve(n_parties);
  b_shares.reserve(n_parties);
  c_shares.reserve(n_parties);

  arithmeticSecretShare(n_parties, this->modulus, a, a_shares);
  arithmeticSecretShare(n_parties, this->modulus, b, b_shares);
  arithmeticSecretShare(n_parties, this->modulus, c, c_shares);

  for (size_t i = 0; i < n_parties; i++) {
    vals.emplace_back(a_shares[i], b_shares[i], c_shares[i]);
  }
}

template<FF_TYPENAMES, typename Number_T, typename Info_T>
std::string Multiply<FF_TYPES, Number_T, Info_T>::name() {
  return std::string("Multiply mod: ") + dec(this->info->info.modulus);
}

template<FF_TYPENAMES, typename Number_T, typename Info_T>
void Multiply<FF_TYPES, Number_T, Info_T>::init() {
  this->numOutstandingMessages = 0;
  this->revealed_d =
      modSub(this->myShare_x, this->beaver.a, this->info->info.modulus);
  this->revealed_e =
      modSub(this->myShare_y, this->beaver.b, this->info->info.modulus);

  this->getPeers().forEach([this](Identity_T const & other) {
    if (this->getSelf() != other) {
      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(other));
      omsg->template write<Number_T>(this->revealed_d);
      omsg->template write<Number_T>(this->revealed_e);
      this->send(std::move(omsg));
      this->numOutstandingMessages++;
    }
  });
}

template<FF_TYPENAMES, typename Number_T, typename Info_T>
void Multiply<FF_TYPES, Number_T, Info_T>::computeResultShare() {
  /*
   * z = x * y = (a+d)*(b+e) = a*b + b*d + a*e + d*e = c + b*d + a*e + d*e
   * We hold shares of a,b,c, but d,e are public.
   * Only one party (arbitrarily, the "revealer") includes d*e in their share
   */
  *this->myShare_z = modAdd(
      modAdd(
          modMul(
              this->beaver.b,
              this->revealed_d,
              this->info->info.modulus),
          modMul(
              this->beaver.a,
              this->revealed_e,
              this->info->info.modulus),
          this->info->info.modulus),
      this->beaver.c,
      this->info->info.modulus);

  if (*this->info->revealer == this->getSelf()) {
    log_debug("adding d * e");
    *this->myShare_z = modAdd(
        *this->myShare_z,
        modMul(
            this->revealed_d,
            this->revealed_e,
            this->info->info.modulus),
        this->info->info.modulus);
  }

  this->complete();
}

template<FF_TYPENAMES, typename Number_T, typename Info_T>
void Multiply<FF_TYPES, Number_T, Info_T>::handleReceive(
    IncomingMessage_T & imsg) {
  Number_T temp_val = 0;
  imsg.template read<Number_T>(temp_val);
  this->revealed_d =
      modAdd(this->revealed_d, temp_val, this->info->info.modulus);
  imsg.template read<Number_T>(temp_val);
  this->revealed_e =
      modAdd(this->revealed_e, temp_val, this->info->info.modulus);

  this->numOutstandingMessages--;
  if (this->numOutstandingMessages == 0) {
    this->computeResultShare();
  }
}

template<FF_TYPENAMES, typename Number_t, typename Info_t>
void Multiply<FF_TYPES, Number_t, Info_t>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("  Multiply Fronctocol unexpected "
            "handle promise");
}

template<FF_TYPENAMES, typename Number_t, typename Info_t>
void Multiply<FF_TYPES, Number_t, Info_t>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Multiply Fronctocol received unexpected "
            "handle complete");
}

template<FF_TYPENAMES>
class Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>
    : public Fronctocol<FF_TYPES> {
public:
  virtual std::string name() override {
    return std::string("Multiply Boolean");
  }

  // share of z, for z = x * y, where shares of x and y are below
  Boolean_t * const myShare_z;

  Boolean_t const myShare_x;
  Boolean_t const myShare_y;

  BeaverTriple<Boolean_t> beaver;

  MultiplyInfo<Identity_T, BooleanBeaverInfo> const * const info;

  Multiply(
      Boolean_t const & ms_x,
      Boolean_t const & ms_y,
      Boolean_t * const ms_z,
      BeaverTriple<Boolean_t> && b,
      MultiplyInfo<Identity_T, BooleanBeaverInfo> const * const i) :
      myShare_z(ms_z),
      myShare_x(ms_x),
      myShare_y(ms_y),
      beaver(::std::move(b)),
      info(i) {
  }

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  Boolean_t revealed_d = 0;
  Boolean_t revealed_e = 0;
  size_t numOutstandingMessages = 0;

  void computeResultShare();
};

template<FF_TYPENAMES>
void Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("  Multiply Fronctocol unexpected "
            "handle promise");
}

template<FF_TYPENAMES>
void Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Multiply Fronctocol received unexpected "
            "handle complete");
}

template<FF_TYPENAMES>
void Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>::init() {
  this->numOutstandingMessages = 0;
  this->revealed_d = this->myShare_x ^ this->beaver.a;
  this->revealed_e = this->myShare_y ^ this->beaver.b;

  this->getPeers().forEach([this](Identity_T const & other) {
    if (this->getSelf() != other) {
      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(other));
      omsg->template write<Boolean_t>(this->revealed_d);
      omsg->template write<Boolean_t>(this->revealed_e);
      this->send(std::move(omsg));
      this->numOutstandingMessages++;
    }
  });
}

template<FF_TYPENAMES>
void Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>::
    computeResultShare() {
  /*
   * z = x * y = (a+d)*(b+e) = a*b + b*d + a*e + d*e = c + b*d + a*e + d*e
   * We hold shares of a,b,c, but d,e are public.
   * Only one party (arbitrarily, the "revealer") includes d*e in their share
   */
  *this->myShare_z = (this->beaver.b & this->revealed_d) ^
      (this->beaver.a & this->revealed_e) ^ this->beaver.c;

  if (*this->info->revealer == this->getSelf()) {
    log_debug("(xor mult) adding d * e");
    *this->myShare_z =
        *this->myShare_z ^ (this->revealed_d & this->revealed_e);
  }

  this->complete();
}

template<FF_TYPENAMES>
void Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>::handleReceive(
    IncomingMessage_T & imsg) {
  Boolean_t temp_val = 0;
  imsg.template read<Boolean_t>(temp_val);
  this->revealed_d = this->revealed_d ^ temp_val;
  imsg.template read<Boolean_t>(temp_val);
  this->revealed_e = this->revealed_e ^ temp_val;

  this->numOutstandingMessages--;
  if (this->numOutstandingMessages == 0) {
    this->computeResultShare();
  }
}

} // namespace mpc
} // namespace ff

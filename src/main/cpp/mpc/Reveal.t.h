/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
std::string Reveal<FF_TYPES, Number_T>::name() {
  return std::string("Reveal modulus: ") + dec(this->modulus);
}

template<FF_TYPENAMES>
class Reveal<FF_TYPES, Boolean_t> : public Fronctocol<FF_TYPES> {
public:
  std::string name() override {
    return std::string("Reveal Boolean");
  }
  Boolean_t openedValue;

  Reveal(Boolean_t share, const Boolean_t &, const Identity_T * rev) :
      openedValue(share), revealer(rev) {
  }

  Reveal(Boolean_t share, const Identity_T * rev) :
      openedValue(share), revealer(rev) {
  }

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  const Identity_T * revealer;
  size_t numOutstandingMessages =
      0; // used for tracking "state machine" behavior
};

template<FF_TYPENAMES, typename Number_T>
void Reveal<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Reveal Fronctocol received unexpected "
            "handle complete");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void Reveal<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Reveal Fronctocol received unexpected "
            "handlePromise");
  this->abort();
}

template<FF_TYPENAMES>
void Reveal<FF_TYPES, Boolean_t>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Reveal Fronctocol received unexpected "
            "handle complete");
  this->abort();
}

template<FF_TYPENAMES>
void Reveal<FF_TYPES, Boolean_t>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Reveal Fronctocol received unexpected "
            "handlePromise");
  this->abort();
}

/*
 * We initially store each party's share in "openedValue", and then
 * overwrite it with the true opened value
 */

template<FF_TYPENAMES, typename Number_T>
void Reveal<FF_TYPES, Number_T>::init() {
  this->numOutstandingMessages = 0;
  this->getPeers().forEach([this](Identity_T const & other) {
    if (this->getSelf() != other) {
      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(other));

      if_debug {
        omsg->template write<Number_T>(this->modulus);
      }

      omsg->template write<Number_T>(this->openedValue);
      this->send(std::move(omsg));
      this->numOutstandingMessages++;
    }
  });
}

template<FF_TYPENAMES>
void Reveal<FF_TYPES, Boolean_t>::init() {
  this->numOutstandingMessages = 0;
  this->getPeers().forEach([this](Identity_T const & other) {
    if (this->getSelf() != other) {
      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(other));

      omsg->template write<Boolean_t>(this->openedValue);
      this->send(std::move(omsg));
      this->numOutstandingMessages++;
    }
  });
}

template<FF_TYPENAMES, typename Number_T>
void Reveal<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T & imsg) {
  if_debug {
    Number_T other_mod = 0;
    imsg.template read<Number_T>(other_mod);
    log_assert(this->modulus == other_mod);
  }

  Number_T temp_val = 0;
  imsg.template read<Number_T>(temp_val);
  this->openedValue =
      modAdd(this->openedValue, temp_val, this->modulus);
  this->numOutstandingMessages--;

  if (this->numOutstandingMessages == 0) {
    this->complete();
  }
}

template<FF_TYPENAMES>
void Reveal<FF_TYPES, Boolean_t>::handleReceive(
    IncomingMessage_T & imsg) {
  Boolean_t temp_val = 0;
  imsg.template read<Boolean_t>(temp_val);
  this->openedValue = this->openedValue ^ temp_val;
  this->numOutstandingMessages--;

  if (this->numOutstandingMessages == 0) {
    this->complete();
  }
}

} // namespace mpc
} // namespace ff

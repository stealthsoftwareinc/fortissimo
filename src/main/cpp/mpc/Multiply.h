/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * Beaver triple multiplication,
 * specific implementation adapted from
 * https://eprint.iacr.org/2012/642.pdf p. 36
 *
 * Variants for elements shared additively mod p
 * and elements shared additively as XOR shares
 */

#ifndef FF_MPC_MULTIPLY_H_
#define FF_MPC_MULTIPLY_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */

#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <mpc/ModUtils.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_t>
struct BeaverTriple {
  Number_t a;
  Number_t b;
  Number_t c;

  BeaverTriple<Number_t>(Number_t a, Number_t b, Number_t c);
  BeaverTriple<Number_t>() = default;

  template<typename Info_T>
  BeaverTriple<Number_t>(Info_T const &) : BeaverTriple<Number_t>() {
  }

  static std::string name() {
    return std::string("Beaver Triple");
  }
};

template<typename Number_T>
struct BeaverInfo {
  Number_T modulus;

  size_t instanceSize() const {
    // TODO(KIM): actually exceeding this size is not harmful, but it
    // would be better to get the correct size for a bignum...
    return 3 * numberLen(this->modulus);
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<BeaverTriple<Number_T>> & vals) const;

  bool operator==(BeaverInfo<Number_T> const & other) const {
    return this->modulus == other.modulus;
  }

  bool operator!=(BeaverInfo<Number_T> const & other) const {
    return !(*this == other);
  }

  BeaverInfo(Number_T const & modulus) : modulus(modulus) {
  }
  BeaverInfo() = default;
};

struct BooleanBeaverInfo {

  size_t instanceSize() const {
    return 3 * sizeof(Boolean_t);
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<BeaverTriple<Boolean_t>> & vals) const;

  bool operator==(BooleanBeaverInfo const &) const {
    return 1;
  }

  bool operator!=(BooleanBeaverInfo const &) const {
    return 0;
  }
};

template<typename Identity_T, typename Info_T>
struct MultiplyInfo {
  Identity_T const * const revealer;
  Info_T const info;

  MultiplyInfo(Identity_T const * const r, Info_T const & i) :
      revealer(r), info(i) {
  }
};

template<
    FF_TYPENAMES,
    typename Number_T,
    typename Info_T = BeaverInfo<Number_T>>
class Multiply : public Fronctocol<FF_TYPES> {
public:
  virtual std::string name() override;
  // share of z, for z = x * y, where shares of x and y are below
  Number_T * const myShare_z;

  Number_T const myShare_x;
  Number_T const myShare_y;

  BeaverTriple<Number_T> beaver;

  MultiplyInfo<Identity_T, Info_T> const * const info;

  Multiply(
      Number_T const & ms_x,
      Number_T const & ms_y,
      Number_T * const out,
      BeaverTriple<Number_T> && b,
      MultiplyInfo<Identity_T, Info_T> const * const i) :
      myShare_z(out),
      myShare_x(ms_x),
      myShare_y(ms_y),
      beaver(b),
      info(i) {
  }

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  Number_T revealed_d = 0;
  Number_T revealed_e = 0;
  size_t numOutstandingMessages = 0;

  void computeResultShare();
};

} // namespace mpc
} // namespace ff

#include <mpc/Multiply.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_MULTIPLY_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_TYPE_CAST_BIT_H_
#define FF_MPC_TYPE_CAST_BIT_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/Reveal.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
struct TypeCastInfo;

template<typename Number_T>
struct TypeCastFromBitInfo;

/**
 * A TypeCastTriple is a triplet of numbers r_0, r_1, r_2 where r_0 and r_1 are
 * shared mod p, and r_2 with XOR. with 1/2 probability they take the
 * values (1, 0, 0) or (-1, 1, 1).
 */
template<typename Number_T>
struct TypeCastTriple {
  Number_T r_0 = 0U;
  Number_T r_1 = 0U;
  Boolean_t r_2 = 0x00;

  TypeCastTriple<Number_T>(Number_T r_0, Number_T r_1, Boolean_t r_2);
  TypeCastTriple<Number_T>() = default;

  TypeCastTriple<Number_T>(TypeCastInfo<Number_T> const &) :
      TypeCastTriple<Number_T>() {
  }
  TypeCastTriple<Number_T>(TypeCastFromBitInfo<Number_T> const &) :
      TypeCastTriple<Number_T>() {
  }

  static std::string name() {
    return std::string("Type Cast Triple");
  }
};

template<typename Number_T>
struct TypeCastInfo {
  Number_T modulus = 0U;

  size_t instanceSize() const {
    return (2 * sizeof(Number_T) + sizeof(Boolean_t));
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<TypeCastTriple<Number_T>> & vals) const;

  bool operator==(TypeCastInfo<Number_T> const & other) const {
    return this->modulus == other.modulus;
  }

  bool operator!=(TypeCastInfo<Number_T> const & other) const {
    return !(*this == other);
  }

  TypeCastInfo<Number_T>(const Number_T modulus);
  TypeCastInfo<Number_T>() = default;
};

template<FF_TYPENAMES, typename Number_T>
class TypeCast : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  Boolean_t outputBitShare;

  /*
   * Takes in a list of shares of xvals mod some small prime s
   * satisfying s > xvals.size()+1. Computes this parties share of
   * the "or" of all values and stores it in orResult
   */
  TypeCast(
      const Number_T & arithmeticShareOfBit,
      const Number_T & modulus,
      const Identity_T * rev,
      const BeaverTriple<Number_T> & beaverTriple,
      const TypeCastTriple<Number_T> & tcTriple);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum TypeCastState { awaitingMultiply, awaitingReveal };
  TypeCastState state = awaitingMultiply;

  const Number_T arithmeticShareOfBit;
  const Number_T modulus;

  const Identity_T * revealer;

  BeaverTriple<Number_T> beaverTriple;
  const TypeCastTriple<Number_T> tcTriple;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> multiplyInfo;

  Number_T multiplyResult;
};

} // namespace mpc

#include <mpc/TypeCastBit.t.h>

} // namespace ff

#define LOG_UNCLUDE

#include <ff/logging.h>

#endif //FF_MPC_TYPE_CAST_BIT_H_

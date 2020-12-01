/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_BITWISE_COMPARE_H_
#define FF_MPC_BITWISE_COMPARE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <mpc/Batch.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
struct BitwiseCompareRandomness {
  PrefixOrRandomness<Number_T> randomness;
  BeaverTriple<Number_T> beaver;
  TypeCastTriple<Number_T> tcTriple;
  BeaverTriple<Number_T> beaver2;
  TypeCastTriple<Number_T> tcTriple2;

  BitwiseCompareRandomness(
      PrefixOrRandomness<Number_T> && randomness,
      const BeaverTriple<Number_T> & beaver,
      const TypeCastTriple<Number_T> & tcTriple,
      const BeaverTriple<Number_T> & beaver2,
      const TypeCastTriple<Number_T> & tcTriple2) :
      randomness(std::move(randomness)),
      beaver(beaver),
      tcTriple(tcTriple),
      beaver2(beaver2),
      tcTriple2(tcTriple2) {
  }
};

template<FF_TYPENAMES, typename Number_T>
class BitwiseCompare : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;
  Boolean_t outputShare;

  /*
   * Takes in a list of shares of xvals mod some small prime s
   * satisfying s > xvals.size()+1. Computes this parties share of
   * the "or" of all values and stores it in orResult
   */
  BitwiseCompare(
      std::vector<Number_T> & shares_of_a,
      const std::vector<Number_T> & b_in_the_clear,
      PrefixOrInfo<Identity_T, Number_T> const * const prefInfo,
      BitwiseCompareRandomness<Number_T> && randomness);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum BitwiseCompareState {
    awaitingPrefixOr,
    awaitingBatchTypeCastBit,
  };
  BitwiseCompareState state = awaitingPrefixOr;

  std::vector<Number_T> shares_of_a;
  const std::vector<Number_T> b_in_the_clear;

  PrefixOrInfo<Identity_T, Number_T> const * const prefInfo;
  BitwiseCompareRandomness<Number_T> randomness;

  Number_T equalityCheckValue;
};

#include <mpc/BitwiseCompare.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_BITWISE_COMPARE_H_

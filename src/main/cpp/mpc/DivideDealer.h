/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_DIVIDE_HOUSE_H_
#define FF_MPC_DIVIDE_HOUSE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <list>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <ff/Promise.h>
#include <mpc/Compare.h>
#include <mpc/Divide.h>
#include <mpc/ModConvUp.h>
#include <mpc/Multiply.h>
#include <mpc/PosIntCompare.h>
#include <mpc/PrefixOr.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class DivideRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  DivideRandomnessHouse(
      DivideInfo<Identity_T, Large_T, Small_T> const * const info);

private:
  DivideInfo<Identity_T, Large_T, Small_T> const * const info;

  size_t numDealersRemaining = 0;
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class DivideRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      DivideRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      divideDispenser;

  DivideRandomnessPatron(
      DivideInfo<Identity_T, Large_T, Small_T> const * const info,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateOutputDispenser();

  enum DividePatronPromiseState {
    awaitingPrefixOr,
    awaitingMultiply,
    awaitingCompare,
    awaitingTct,
    awaitingMultiply2,
    awaitingTct2,
    awaitingTct3
  };
  DividePatronPromiseState state = awaitingPrefixOr;

  DivideInfo<Identity_T, Large_T, Small_T> const * const info;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;

  size_t numPrefixOrsNeeded;
  size_t numBeaverTriplesNeeded;
  size_t numComparesNeeded;
  size_t numStartTCTriplesFromBitNeeded;
  size_t numStartTCTriplesNeeded;
  size_t numEndTCTriplesNeeded;

  std::unique_ptr<RandomnessDispenser<
      PrefixOrRandomness<Small_T>,
      DoNotGenerateInfo>>
      prefixOrDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      multiplyDispenser;
  /* Additional Vars for Batch Compare */
  std::unique_ptr<RandomnessDispenser<
      PosIntCompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;
  /* Vars for typecastfrombit */

  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastFromBitInfo<Small_T>>>
      smallPrimeTCTripleFromBitDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      smallPrimeBeaverDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastInfo<Small_T>>>
      smallPrimeTCTripleDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Large_T>,
      TypeCastFromBitInfo<Large_T>>>
      endPrimeTCTripleDispenser;
};

} // namespace mpc
} // namespace ff

#include <mpc/DivideDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_DIVIDE_HOUSE_H_

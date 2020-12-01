/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_ZIP_ADJACENT_DEALER_H_
#define FF_MPC_ZIP_ADJACENT_DEALER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Promise.h>
#include <mpc/Compare.h>
#include <mpc/CompareDealer.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/ZipAdjacent.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class ZipAdjacentRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  ZipAdjacentRandomnessHouse(
      ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info);

private:
  ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info;

  size_t numDealersRemaining = 0;
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class ZipAdjacentRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      ZipAdjacentRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      zipAdjacentDispenser;

  ZipAdjacentRandomnessPatron(
      ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateOutputDispenser();

  enum ZipAdjacentPatronPromiseState {
    awaitingCompare,
    awaitingTct,
    awaitingArithmeticMultiply,
    awaitingXORMultiply
  };
  ZipAdjacentPatronPromiseState state = awaitingCompare;

  ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;

  size_t numComparesNeeded;
  size_t numTCTsNeeded;
  size_t numArithmeticBeaverTriplesNeeded;
  size_t numXORBeaverTriplesNeeded;

  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Large_T>,
      TypeCastFromBitInfo<Large_T>>>
      endPrimeTCTripleDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      arithmeticMultiplyDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORMultiplyDispenser;
  /* Additional Vars for Batch Compare */
  /* Vars for typecastfrombit */
};

} // namespace mpc
} // namespace ff

#include <mpc/ZipAdjacentDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_ZIP_ADJACENT_DEALER_H_

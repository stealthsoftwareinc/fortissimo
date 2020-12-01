/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_COMPARE_HOUSE_H_
#define FF_MPC_COMPARE_HOUSE_H_

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

#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class CompareRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  CompareRandomnessHouse(
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo);

private:
  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;

  size_t numDealersRemaining = 0;
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class CompareRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;

  CompareRandomnessPatron(
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateOutputDispenser();

  enum ComparePatronPromiseState {
    awaitingExponentSeries,
    awaitingMultiply,
    awaitingTct,
    awaitingDbs
  };
  ComparePatronPromiseState promiseState = awaitingExponentSeries;

  size_t numMultiplies;

  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;
  size_t BeaverTriplesNeeded;

  BeaverInfo<Small_T> beaverInfo;
  DecomposedBitSetInfo<Large_T, Small_T> dbsInfo;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          ExponentSeries<Small_T>,
          ExponentSeriesInfo<Small_T>>>>
      exponentPromiseDispenser;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>>
      fullMultiplyPromiseDispenser;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          TypeCastTriple<Small_T>,
          TypeCastInfo<Small_T>>>>
      fullTctPromiseDispenser;
  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          DecomposedBitSet<Large_T, Small_T>,
          DecomposedBitSetInfo<Large_T, Small_T>>>>
      fullDbsPromiseDispenser;

  std::vector<std::unique_ptr<RandomnessDispenser<
      ExponentSeries<Small_T>,
      ExponentSeriesInfo<Small_T>>>>
      fullExponentDispensers;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      fullMultiplyDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      littleMultiplyDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastInfo<Small_T>>>
      fullTctDispenser;
  std::unique_ptr<RandomnessDispenser<
      DecomposedBitSet<Large_T, Small_T>,
      DecomposedBitSetInfo<Large_T, Small_T>>>
      fullDbsDispenser;
};

} // namespace mpc
} // namespace ff

#include <mpc/CompareDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_COMPARE_HOUSE_H_

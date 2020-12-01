/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_POS_INT_COMPARE_HOUSE_H_
#define FF_MPC_POS_INT_COMPARE_HOUSE_H_

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
#include <mpc/PosIntCompare.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class PosIntCompareRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  PosIntCompareRandomnessHouse(
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo);

private:
  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;

  size_t numDealersRemaining = 0;
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class PosIntCompareRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      PosIntCompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      posIntCompareDispenser;

  PosIntCompareRandomnessPatron(
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateOutputDispenser();

  enum ComparePatronPromiseState { awaitingCompare, awaitingMultiply };
  size_t numCompares = 3;
  size_t numMultiplies = 2;

  ComparePatronPromiseState promiseState = awaitingCompare;

  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          CompareRandomness<Large_T, Small_T>,
          DoNotGenerateInfo>>>
      fullComparePromiseDispenser;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      fullMultiplyPromiseDispenser;

  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      fullCompareDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      fullMultiplyDispenser;
};

} // namespace mpc
} // namespace ff

#include <mpc/PosIntCompareDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_POS_INT_COMPARE_HOUSE_H_

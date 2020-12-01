/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_PREFIX_OR_DEALER_H_
#define FF_MPC_PREFIX_OR_DEALER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Promise.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
class PrefixOrRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  PrefixOrRandomnessHouse(
      PrefixOrInfo<Identity_T, Number_T> const * const info);

private:
  PrefixOrInfo<Identity_T, Number_T> const * const info;

  size_t numDealersRemaining = 0;
};

template<FF_TYPENAMES, typename Number_T>
class PrefixOrRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      PrefixOrRandomness<Number_T>,
      DoNotGenerateInfo>>
      prefixOrDispenser;

  PrefixOrRandomnessPatron(
      PrefixOrInfo<Identity_T, Number_T> const * const info,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateOutputDispenser();

  size_t BeaverTriplesNeeded;

  PrefixOrInfo<Identity_T, Number_T> const * const info;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;
  size_t numPromisesRemaining;

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;

  std::vector<std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          ExponentSeries<Number_T>,
          ExponentSeriesInfo<Number_T>>>>>
      esFullPromiseDispensers;

  std::vector<std::unique_ptr<RandomnessDispenser<
      ExponentSeries<Number_T>,
      ExponentSeriesInfo<Number_T>>>>
      esFullDispensers;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>>>
      fullMultiplyPromiseDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      fullMultiplyDispenser;
};

} // namespace mpc
} // namespace ff

#include <mpc/PrefixOrDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_PREFIX_OR_DEALER_H_

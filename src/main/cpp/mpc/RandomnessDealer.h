/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_RANDOMNESS_DEALER_H_
#define FF_MPC_RANDOMNESS_DEALER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* logging Configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

/**
 * The RandomnessHouse class specializes for a particular type of
 * correlated randomness and creates randomness instances for
 * RandomnessPatrons to consume.
 */
template<FF_TYPENAMES, typename Rand_T, typename Info_T>
class RandomnessHouse : public Fronctocol<FF_TYPES> {
  std::string name() override;

  /**
   * Metadata of randomness (prime modulos etc.) and number of randomness
   * instances to generate. These are read from the mpcs messages.
   *
   * Each patron must send the exact same info. The max numDesired is
   * taken from all patrons. If numDesired does not match across all patrons
   * a warning will be generated when in debug mode.
   */
  Info_T info;
  size_t numDesired = 0;

  /**
   * Count of patrons which want randomness. Calculated during ``init()``.
   *
   * numReceived counts how many patrons have synced info and numDesired
   * yet. The house cannot begin dealing until numParties == numReceived.
   */
  size_t numParties = 0;
  size_t numReceived = 0;

public:
  void init() override;
  void handleReceive(IncomingMessage_T & msg) override;
  void handleComplete(Fronctocol<FF_TYPES> & f) override;
  void handlePromise(Fronctocol<FF_TYPES> & f) override;
};

/**
 * The RandomnessPatron is an implementation of a RandomnessGenerator
 * which requests randomness instances from a trusted dealer,
 * known as the RandomnessHouse.
 */
template<FF_TYPENAMES, typename Rand_T, typename Info_T>
class RandomnessPatron
    : public RandomnessGenerator<FF_TYPES, Rand_T, Info_T> {
public:
  std::string name() override;

  Identity_T const dealer;

  RandomnessPatron(
      Identity_T const & dealer, size_t n, Info_T const & i) :
      RandomnessGenerator<FF_TYPES, Rand_T, Info_T>(n, i),
      dealer(dealer) {
  }

  void init() override;
  void handleReceive(IncomingMessage_T & im) override;
  void handleComplete(Fronctocol<FF_TYPES> & f) override;
  void handlePromise(Fronctocol<FF_TYPES> & f) override;

private:
  size_t batchesTotal = 0;
  size_t batchSize = 0;
  size_t batchesReceived = 0;
};

#include <mpc/RandomnessDealer.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_RANDOMNESS_DEALER_H_

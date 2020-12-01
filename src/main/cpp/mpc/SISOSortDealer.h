/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_SISO_SORT_HOUSE_H_
#define FF_MPC_SISO_SORT_HOUSE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/QuicksortDealer.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Waksman.h>
#include <mpc/simplePrime.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class SISOSortRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  SISOSortRandomnessHouse(
      const size_t listSize,
      const Large_T modulus,
      const Identity_T * revealer,
      const Identity_T * dealerIdentity);

  SISOSortRandomnessHouse(
      const size_t listSize,
      const Large_T modulus,
      const Large_T keyModulus,
      const Identity_T * revealer,
      const Identity_T * dealerIdentity);

private:
  const size_t listSize;
  const Large_T modulus;
  const Large_T keyModulus;
  const Identity_T * revealer;
  const Identity_T * dealerIdentity;

  size_t d;
  Small_T small_modulus;
  size_t bitsPerPrime;
  size_t sqrt_ell;

  std::vector<std::vector<Small_T>>
      emptyLagrangePolynomialSet; // dummy input to QuicksortHouse

  size_t numSubDealers = 5;

  const size_t NULL_KEY_COUNT = 0; // dummy input to QuicksortHouse
};

#include <mpc/SISOSortDealer.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_SISO_SORT_HOUSE_H_

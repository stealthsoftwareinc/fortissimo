/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * General implementation of a shared input shared output sort
 * Using Waksman shuffle followed by a O(log n) round quicksort implementation
 *
 * Each party's shares are input by reference as an ObservationList, a struct defined in
 * mpc/Quicksort.h. It's a vector of Observations, which consist of uint32_t Key, uint32
 * ArithmeticPayload, and uint8_t XORPayload vectors.
 *
 * Usage notes:
 *
 * All key entries (after reconstruction) must lie in the interval [0,modulus/2] for correct
 * behavior
 *
 * The comparison is big-endian on KeyCols, i.e. KeyCols[0] is the most significant "digit"
 *
 * duplicated key entries (when reconstructed) will cause leakage from the sort.
 */

#ifndef FF_MPC_SISO_SORT_H
#define FF_MPC_SISO_SORT_H

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Promise.h>
#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/ObservationList.h>
#include <mpc/Quicksort.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Waksman.h>
#include <mpc/lagrange.h>
#include <mpc/simplePrime.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class SISOSort : public Fronctocol<FF_TYPES> {
public:
  virtual std::string name() override;

  ObservationList<Large_T> & sharedList;

  SISOSort(
      ObservationList<Large_T> & sharedList,
      Large_T modulus,
      const Identity_T * revealer,
      const Identity_T * dealerIdentity);

  SISOSort(
      ObservationList<Large_T> & sharedList,
      Large_T modulus,
      Large_T keyModulus,
      const Identity_T * revealer,
      const Identity_T * dealerIdentity);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  void setup();
  void batchMultiplyForSwaps();

  enum SISOSortState { awaitingWaksman, awaitingQuicksort };
  SISOSortState state = awaitingWaksman;
  size_t numPromisesRemaining = 4;

  std::unique_ptr<CompareInfo<Identity_T, Large_T, Small_T>>
      compareInfo;

  Large_T modulus;
  Large_T keyModulus;

  std::vector<std::vector<Small_T>> lagrangePolynomialSet;

  const Identity_T * revealer;

  size_t d;
  size_t expandedListSize;
  Small_T smallModulus;
  size_t bitsPerPrime;
  size_t sqrtEll;

  BeaverInfo<Large_T> beaverInfo;
  BeaverInfo<Large_T> keyBeaverInfo;
  WaksmanInfo<Large_T> waksmanInfo;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>>
      multiplyPromiseDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      mrd;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>>
      multiplyKeyPromiseDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      mrd_key;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      XORMultiplyPromiseDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORmrd;

  std::unique_ptr<Promise<
      FF_TYPES,
      RandomnessDispenser<WaksmanBits<Large_T>, WaksmanInfo<Large_T>>>>
      waksmanPromiseDispenser;

  const Identity_T * dealerIdentity;
};

#include <mpc/SISOSort.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_PREFIX_OR_H_

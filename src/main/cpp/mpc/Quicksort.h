/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * This is quicksort with the recursion "unpacked"
 * At each stage of the recursion, we have a linked list
 * of block indices, and we split each block in half in
 * the standard quicksort way (Hoare partition scheme)
 */

#ifndef FF_MPC_QUICKSORT_H_
#define FF_MPC_QUICKSORT_H_

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
#include <ff/Promise.h>
#include <mpc/Batch.h>
#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Reveal.h>
#include <mpc/TypeCastBit.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

const size_t DO_NOT_COMPARE = SIZE_MAX;

/*
 * Needed only in Quicksort.cpp
 * Holds an ordered pair tracking the indices
 * of a block
 */
struct LoHiPair {
  size_t lo;
  size_t hi;
  LoHiPair(size_t l, size_t h) : lo(l), hi(h) {
  }
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class QuickSortFronctocol : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;
  ObservationList<Large_T> * inputList;

  /*
   * Run Quicksort on an ObservationList list by passing a
   * pointer to list into the constructor and calling init();
   * Once the Quicksort Fronctocol completes, list will point
   * to the sorted list
   */
  QuickSortFronctocol(
      ObservationList<Large_T> * list,
      const CompareInfo<Identity_T, Large_T, Small_T> & compareInfo,
      const Identity_T * revealer,
      const Identity_T * dealerIdentity);

private:
  enum QuickSortState {
    awaitingBatchedReveal,
    awaitingBatchedMultiply,
    awaitingBatchedCompare
  };
  enum QuickSortPromiseState {
    awaitingExponentSeries,
    awaitingMultiply,
    awaitingXORMultiply,
    awaitingTct,
    awaitingDbs
  };
  size_t numMultiplies;
  QuickSortState state = awaitingBatchedCompare;
  QuickSortPromiseState promiseState = awaitingExponentSeries;

  // for each element of the list
  std::vector<size_t> pivots;

  // holds elements[i][j] < pivots[i][j]
  std::vector<Boolean_t> fullComparisonShares;

  // holds elements[i] < pivots[i], based on lexicographical ordering up
  // to keyCol[j]
  std::vector<Boolean_t> partialComparisonsOutput;

  // holds elements[i] < pivots[i] for each i
  std::vector<Boolean_t> comparisons;
  MultiplyInfo<Identity_T, BooleanBeaverInfo> multiplyInfo;

  // for each block of the list, pivot, lo, hi
  std::list<LoHiPair> blockInfo;

  CompareInfo<Identity_T, Large_T, Small_T> compareInfo;

  size_t maxNumberCompares;
  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;
  size_t BeaverTriplesNeeded;
  size_t XORBeaverTriplesNeeded;

  BeaverInfo<Small_T> beaverInfo;
  DecomposedBitSetInfo<Large_T, Small_T> dbsInfo;

  void buildCompareDispenser();
  void runComparisons();

  size_t testVar = 2;

  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;

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
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      XORMultiplyPromiseDispenser;

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
  std::vector<std::unique_ptr<RandomnessDispenser<
      ExponentSeries<Small_T>,
      ExponentSeriesInfo<Small_T>>>>
      littleExponentDispensers;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      fullMultiplyDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORMultiplyDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      littleMultiplyDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastInfo<Small_T>>>
      fullTctDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastInfo<Small_T>>>
      littleTctDispenser;
  std::unique_ptr<RandomnessDispenser<
      DecomposedBitSet<Large_T, Small_T>,
      DecomposedBitSetInfo<Large_T, Small_T>>>
      fullDbsDispenser;
  std::unique_ptr<RandomnessDispenser<
      DecomposedBitSet<Large_T, Small_T>,
      DecomposedBitSetInfo<Large_T, Small_T>>>
      littleDbsDispenser;

  const Identity_T * revealIdentity;
  const Identity_T * dealerIdentity;
};

} // namespace mpc
} // namespace ff

#include <mpc/Quicksort.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_QUICKSORT_H_

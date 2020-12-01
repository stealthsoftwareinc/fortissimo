/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * This is quicksort with the recursion "unpacked"
 * At each stage of the recursion, we have a linked list
 * of block indices, and we split each block in half in
 * the standard quicksort way (Hoare partition scheme)
 */

#ifndef FF_MPC_QUICKSORT_DEALER_H_
#define FF_MPC_QUICKSORT_DEALER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class QuicksortRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  /*
   * Run Quicksort on an ObservationList list by passing a
   * pointer to list into the constructor and calling init();
   * Once the Quicksort Fronctocol completes, list will point
   * to the sorted list
   */
  QuicksortRandomnessHouse(
      const CompareInfo<Identity_T, Large_T, Small_T> & compareInfo,
      size_t numElements,
      size_t numKeyCols);

private:
  const CompareInfo<Identity_T, Large_T, Small_T> compareInfo;
  size_t numElements;
  size_t numKeyCols;

  size_t numDealersRemaining;
};

#include <mpc/QuicksortDealer.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_QUICKSORT_DEALER_H_

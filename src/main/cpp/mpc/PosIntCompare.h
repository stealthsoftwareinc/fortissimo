/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * Comparison of two values a,b in [0, p),
 * returns 1 if a > b over Z, 0 if a < b, 2 if a == b
 */

#ifndef FF_MPC_POS_INT_COMPARE_H_
#define FF_MPC_POS_INT_COMPARE_H_

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
#include <mpc/Batch.h>
#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Large_T, typename Small_T>
struct PosIntCompareRandomness {
  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser; // size 3
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      beaverDispenser; // size 2

  PosIntCompareRandomness(
      std::unique_ptr<RandomnessDispenser<
          CompareRandomness<Large_T, Small_T>,
          DoNotGenerateInfo>> compareDispenser, // size 3
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Boolean_t>,
          BooleanBeaverInfo>> beaverDispenser);
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class PosIntCompare : public Fronctocol<FF_TYPES> {
public:
  Boolean_t outputShare;
  /*
   */
  PosIntCompare(
      Large_T const & share_of_x, // mod p
      Large_T const & share_of_y, // mod p
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo,
      PosIntCompareRandomness<Large_T, Small_T> && randomness);

  std::string name() override;

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  void initAfterRandomness();

  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum CompareState {
    awaitingBatchedCompare,
    awaitingBatchedXORMultiply
  };
  CompareState state = awaitingBatchedCompare;

  // 2: a == 2 && (b==c)

  // 1: (a == 1 && b == c) || (b == 0 && c == 1)

  // 0: (a == 0 && b == c) || (b == 1 && c == 0)

  // (a & ((b^c^1)%2)*3) ^ (c & (b ^ c)) FINAL ANSWER

  const Large_T share_of_x;
  const Large_T share_of_y;

  Boolean_t firstMultOutput = 0x00;
  Boolean_t secondMultOutput = 0x00;

  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;

  PosIntCompareRandomness<Large_T, Small_T> randomness;
};

} // namespace mpc
} // namespace ff

#include <mpc/PosIntCompare.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_COMPARE_H_

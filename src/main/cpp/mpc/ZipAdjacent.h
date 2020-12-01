/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_ZIP_ADJACENT_H_
#define FF_MPC_ZIP_ADJACENT_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/Batch.h>
#include <mpc/Compare.h>
#include <mpc/ModConvUp.h>
#include <mpc/Multiply.h>
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Large_T, typename Small_T>
struct ZipAdjacentRandomness {
  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Large_T>,
      TypeCastFromBitInfo<Large_T>>>
      tctDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      arithmeticBeaverDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORBeaverDispenser;

  ZipAdjacentRandomness(
      std::unique_ptr<RandomnessDispenser<
          CompareRandomness<Large_T, Small_T>,
          DoNotGenerateInfo>> compareDispenser,
      std::unique_ptr<RandomnessDispenser<
          TypeCastTriple<Large_T>,
          TypeCastFromBitInfo<Large_T>>> tctDispenser,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Large_T>,
          BeaverInfo<Large_T>>> arithmeticBeaverDispenser,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Boolean_t>,
          BooleanBeaverInfo>> XORBeaverDispenser) :
      compareDispenser(std::move(compareDispenser)),
      tctDispenser(std::move(tctDispenser)),
      arithmeticBeaverDispenser(std::move(arithmeticBeaverDispenser)),
      XORBeaverDispenser(std::move(XORBeaverDispenser)) {
  }
};

template<typename Identity_T, typename Large_T, typename Small_T>
struct ZipAdjacentInfo {
  const size_t batchSize;
  const size_t numArithmeticPayloadCols;
  const size_t numXORPayloadCols;

  CompareInfo<Identity_T, Large_T, Small_T> const compareInfo;
  MultiplyInfo<Identity_T, BeaverInfo<Large_T>> const multiplyInfo;
  MultiplyInfo<Identity_T, BooleanBeaverInfo> const booleanMultiplyInfo;

  ZipAdjacentInfo(
      const size_t batchSize,
      const size_t numArithmeticPayloadCols,
      const size_t numXORPayloadCols,
      const Large_T modulus,
      Identity_T const * const revealer);
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class ZipAdjacent : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;
  ObservationList<Large_T> zippedAdjacentPairs;

  /*
   * Takes in a sorted ObservationList and processes it
   * Assume for now that numKeyCols = 1
   * TO-DO: re-write SISOSort to force numKeyCols = 1.
   */

  ZipAdjacent(
      const ObservationList<Large_T> & inputList,
      ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info,
      ZipAdjacentRandomness<Large_T, Small_T> && randomness);

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  ObservationList<Large_T> const &
      inputList; // sorted, ready for processing
  ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info;
  ZipAdjacentRandomness<Large_T, Small_T> randomness;

  enum BatchEvalState {
    awaitingCompare,
    awaitingTypeCastFromBit,
    awaitingMultiply
  };

  BatchEvalState state = awaitingCompare;

  size_t numMultipliesRemaining = 2;

  std::vector<Boolean_t> compareResults;
  std::vector<Large_T> typeCastFromBitResults;

  std::vector<std::vector<Large_T>> arithmeticMultiplyResults;
  std::vector<std::vector<Boolean_t>> XORMultiplyResults;
};

} // namespace mpc
} // namespace ff

#include <mpc/ZipAdjacent.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_ZIP_ADJACENT_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_MOD_CONV_UP_DEALER_H_
#define FF_MPC_MOD_CONV_UP_DEALER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */

#include <ff/Fronctocol.h>
#include <mpc/ModConvUp.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
class ModConvUpRandomnessHouse : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  ModConvUpRandomnessHouse(ModConvUpInfo<
                           Identity_T,
                           SmallNumber_T,
                           MediumNumber_T,
                           LargeNumber_T> const * const info);

private:
  ModConvUpInfo<
      Identity_T,
      SmallNumber_T,
      MediumNumber_T,
      LargeNumber_T> const * const info;

  size_t numDealersRemaining = 6;
};

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
class ModConvUpRandomnessPatron : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

  std::unique_ptr<RandomnessDispenser<
      ModConvUpRandomness<SmallNumber_T, MediumNumber_T, LargeNumber_T>,
      DoNotGenerateInfo>>
      modConvUpDispenser;
  std::unique_ptr<RandomnessDispenser<
      BitwiseCompareRandomness<SmallNumber_T>,
      DoNotGenerateInfo>>
      bitwiseDispenser;

  ModConvUpRandomnessPatron(
      ModConvUpInfo<
          Identity_T,
          SmallNumber_T,
          MediumNumber_T,
          LargeNumber_T> const * const info,
      Identity_T const * const dealerIdentity,
      const size_t dispenserSize);

private:
  void generateBitwiseOutputDispenser();
  void generateOutputDispenser();

  size_t BeaverTriplesNeeded;

  ModConvUpInfo<
      Identity_T,
      SmallNumber_T,
      MediumNumber_T,
      LargeNumber_T> const * const info;
  const Identity_T * dealerIdentity;
  size_t dispenserSize;
  size_t numPromisesRemaining;

  size_t numBitwiseComparesNeeded = 2;
  size_t numSmallPrimeBeaverTriplesNeeded =
      2 * numBitwiseComparesNeeded;
  size_t numSmallPrimeTCTsNeeded = 2 * numBitwiseComparesNeeded;
  size_t numEndPrimeTCTsNeeded = 3;
  size_t numXORBeaverTriplesNeeded = 2;
  size_t numModConvUpAuxNeeded = 1;

  enum dispenserState {
    awaitingPrefixOr,
    awaitingMultiply,
    awaitingStartTCT,
    awaitingEndTCT,
    awaitingXORMultiply,
    awaitingModConvUpAux
  };

  dispenserState state;

  std::unique_ptr<RandomnessDispenser<
      PrefixOrRandomness<SmallNumber_T>,
      DoNotGenerateInfo>>
      fullPrefixOrDispenser;

  std::unique_ptr<RandomnessDispenser<
      BeaverTriple<SmallNumber_T>,
      BeaverInfo<SmallNumber_T>>>
      fullStartPrimeMultiplyDispenser;

  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<SmallNumber_T>,
      TypeCastInfo<SmallNumber_T>>>
      fullStartTCTDispenser;

  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<LargeNumber_T>,
      TypeCastFromBitInfo<LargeNumber_T>>>
      fullEndPrimeTCTDispenser;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      fullXORMultiplyDispenser;

  std::unique_ptr<RandomnessDispenser<
      ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>,
      ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T>>>
      fullModConvUpAuxDispenser;
};

} // namespace mpc
} // namespace ff

#include <mpc/ModConvUpDealer.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_MOD_CONV_UP_DEALER_H_

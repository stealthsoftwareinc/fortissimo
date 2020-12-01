/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * This modulus conversion protocol converts shares mod p
 * to shares mod q, for any prime q > mp, where m is the number
 * of players
 */

#ifndef FF_MPC_MOD_CONV_UP_H_
#define FF_MPC_MOD_CONV_UP_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */

#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <mpc/BitwiseCompare.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/Reveal.h>
#include <mpc/TypeCastBit.h>
#include <mpc/simplePrime.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

/**
 * A TypeCastFromBitTriple is a triplet of numbers r_0, r_1, r_2 where r_0 and r_1 are
 * shared mod p, and r_2 with XOR. with 1/2 probability they take the
 * values (1, 0, 0) or (0, 1, 1).
 *
 * We already have TypeCast, so we re-use TypeCast as TypeCastFromBit, and pass the
 * TypeCastFromBitInfo object instead of TypeCastInfo to generate the randomness correctly
 */
template<typename Number_T>
struct TypeCastFromBitInfo {
  Number_T modulus = 0U;

  size_t instanceSize() const {
    return (2 * sizeof(Number_T) + sizeof(Boolean_t));
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<TypeCastTriple<Number_T>> & vals) const;

  bool operator==(TypeCastFromBitInfo<Number_T> const & other) const;

  bool operator!=(TypeCastFromBitInfo<Number_T> const & other) const {
    return !(*this == other);
  }

  TypeCastFromBitInfo<Number_T>(const Number_T modulus);
  TypeCastFromBitInfo<Number_T>() = default;
};

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
struct ModConvUpAuxInfo;

/**
 * A ModConvUpAux holds shares of r,x mod q, with r = x mod p,
 * bits of x mod s (for s a small prime), and XOR shares of LSB(r)
 * r uniform on [0,q), and x close-ish to uniform on [0,p)
 * (note: we do not need uniformity of x for security)
 */
template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
struct ModConvUpAux {
  LargeNumber_T r = 0U; // mod q
  LargeNumber_T x = 0U; // mod q
  std::vector<SmallNumber_T> bits_of_x; // mod s
  Boolean_t LSB_of_r = 0x00;

  ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>(
      LargeNumber_T const & r,
      LargeNumber_T const & x,
      std::vector<SmallNumber_T> && bits_of_x,
      Boolean_t LSB_of_r);
  ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>() =
      default;

  ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>(
      ModConvUpAuxInfo<
          SmallNumber_T,
          MediumNumber_T,
          LargeNumber_T> const &) :
      ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>() {
  }

  static std::string name() {
    return std::string("Mod Conv Up Aux");
  }
};

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
struct ModConvUpAuxInfo {
  LargeNumber_T endModulus = 0U; // q
  MediumNumber_T startModulus = 0U; // p
  SmallNumber_T smallModulus = 0U; // s = ceil log p
  size_t x_bitLength = 0LU;

  size_t instanceSize() const {
    return (
        (x_bitLength)*numberLen(this->smallModulus) +
        2 * numberLen(this->endModulus) + sizeof(Boolean_t));
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<
          ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>> &
          vals) const;

  bool operator==(ModConvUpAuxInfo<
                  SmallNumber_T,
                  MediumNumber_T,
                  LargeNumber_T> const & other) const;

  bool operator!=(ModConvUpAuxInfo<
                  SmallNumber_T,
                  MediumNumber_T,
                  LargeNumber_T> const & other) const {
    return !(*this == other);
  }

  ModConvUpAuxInfo(
      LargeNumber_T const & endModulus,
      MediumNumber_T const & startModulus);

  ModConvUpAuxInfo() = default;
};

template<FF_TYPENAMES, typename Number_T>
class TypeCastFromBit : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  Number_T outputBitShare;

  /*
   * Converts from XOR shares to mod p shares of either 0 or 1
   */
  TypeCastFromBit(
      const Boolean_t XORShareOfBit,
      const Number_T modulus,
      const Identity_T * rev,
      const TypeCastTriple<Number_T> & tcTriple);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  const Boolean_t XORShareOfBit;
  const Number_T modulus;

  const Identity_T * revealer;

  const TypeCastTriple<Number_T> tcTriple;

  Boolean_t revealResult;
};

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
struct ModConvUpInfo {
  const ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T>
      mcuaInfo;
  PrefixOrInfo<Identity_T, SmallNumber_T> prefInfo;

  MultiplyInfo<Identity_T, BooleanBeaverInfo> multiplyInfo;

  ModConvUpInfo<
      Identity_T,
      SmallNumber_T,
      MediumNumber_T,
      LargeNumber_T>(
      const LargeNumber_T endModulus,
      const MediumNumber_T startModulus,
      Identity_T const * const revealer);
};

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
struct ModConvUpRandomness {

  std::unique_ptr<RandomnessDispenser<
      BitwiseCompareRandomness<SmallNumber_T>,
      DoNotGenerateInfo>>
      bitwiseDispenser;
  std::unique_ptr<RandomnessDispenser<
      BeaverTriple<LargeNumber_T>,
      BeaverInfo<LargeNumber_T>>>
      endPrimeBeaverDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<LargeNumber_T>,
      TypeCastFromBitInfo<LargeNumber_T>>>
      endPrimeTCTripleDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORBeaverDispenser;
  ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T> mcua;

  ModConvUpRandomness<SmallNumber_T, MediumNumber_T, LargeNumber_T>(
      std::unique_ptr<RandomnessDispenser<
          BitwiseCompareRandomness<SmallNumber_T>,
          DoNotGenerateInfo>> bitwiseDispenser,
      std::unique_ptr<RandomnessDispenser<
          TypeCastTriple<LargeNumber_T>,
          TypeCastFromBitInfo<LargeNumber_T>>>
          endPrimeTCTripleDispenser,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Boolean_t>,
          BooleanBeaverInfo>> XORBeaverDispenser,
      const ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T> &
          mcua);
};

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
class ModConvUp : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  LargeNumber_T outputShare;

  /*
   * mod p to mod q, p*num_parties < q
   */
  ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>(
      const MediumNumber_T inputShare,
      ModConvUpInfo<
          Identity_T,
          SmallNumber_T,
          MediumNumber_T,
          LargeNumber_T> const * const info,
      ModConvUpRandomness<
          SmallNumber_T,
          MediumNumber_T,
          LargeNumber_T> && randomness);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  // This begins as "reveal"
  enum ModConvState {
    awaitingReveal,
    awaitingFirstBitwiseCompare,
    awaitingSecondBitwiseCompare,
    awaitingFirstXORMultiply,
    awaitingLastXORMultiply,
    awaitingFirstTypeCast,
    awaitingSecondTypeCast,
    awaitingThirdTypeCast,
    preparingFakeXORMultiply
  };
  ModConvState state = awaitingReveal;

  const MediumNumber_T inputShare;
  ModConvUpInfo<
      Identity_T,
      SmallNumber_T,
      MediumNumber_T,
      LargeNumber_T> const * const info;
  ModConvUpRandomness<SmallNumber_T, MediumNumber_T, LargeNumber_T>
      randomness;

  Boolean_t LSB_sum_shares_plus_r;
  LargeNumber_T t;
  LargeNumber_T q_tilde;

  std::vector<SmallNumber_T> tBitsMod_s;

  MultiplyInfo<Identity_T, BooleanBeaverInfo> * multiplyInfo;

  Boolean_t firstStartPrimeCarryBit = 0x00;
  Boolean_t secondStartPrimeCarryBit = 0x00;
  Boolean_t endPrimeCarryBit = 0x00;
  Boolean_t andResult = 0x00;

  LargeNumber_T firstStartPrimeCarryBitArithmetic;
  LargeNumber_T andResultArithmetic;
  LargeNumber_T endPrimeCarryBitArithmetic;

  Boolean_t fakeAndResult = 0x00;
};

} // namespace mpc
#include <mpc/ModConvUp.t.h>

} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_MOD_CONV_UP_H_

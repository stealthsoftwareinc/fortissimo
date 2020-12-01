/**
 * Copyright Stealth Software Technologies, Inc.
 */
#ifndef FF_MPC_Divide_H_
#define FF_MPC_Divide_H_

/* C and POSIX Headers */
/* C++ Headers */
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <mpc/Batch.h>
#include <mpc/BitwiseCompare.h>
#include <mpc/Compare.h>
#include <mpc/CompareDealer.h>
#include <mpc/ModConvUp.h>
#include <mpc/Multiply.h>
#include <mpc/PosIntCompare.h>
#include <mpc/PosIntCompareDealer.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/simplePrime.h>
#include <mpc/templates.h>

/* Logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Identity_T, typename Large_T, typename Small_T>
struct DivideInfo {
  const Identity_T * revealer;
  size_t const ell;
  size_t const lambda;
  const std::vector<std::vector<Small_T>> lagrangePolynomialSet;
  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;
  Large_T modulus;
  Small_T smallModulus;
  DivideInfo(
      Identity_T const * const r,
      Large_T modulus,
      size_t ell,
      size_t lambda,
      const std::vector<std::vector<Small_T>> & lagrangePolynomialSet,
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo) :
      revealer(r),
      ell(ell),
      lambda(lambda),
      lagrangePolynomialSet(lagrangePolynomialSet),
      compareInfo(compareInfo),
      modulus(modulus),
      smallModulus(nextPrime<Small_T>(static_cast<Small_T>(ell) + 2)) {
  }
};

// TO-DO: Templatize (so multiply and TCT can be bignum)
template<typename Large_T, typename Small_T>
struct DivideRandomness {
  std::unique_ptr<RandomnessDispenser<
      PrefixOrRandomness<Small_T>,
      DoNotGenerateInfo>>
      prefixOrDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>
      multiplyDispenser;
  /* Additional Vars for Batch Compare */
  std::unique_ptr<RandomnessDispenser<
      PosIntCompareRandomness<Large_T, Small_T>,
      DoNotGenerateInfo>>
      compareDispenser;
  /* Vars for typecastfrombit */

  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastFromBitInfo<Small_T>>>
      smallPrimeTCTripleFromBitDispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      smallPrimeBeaverDispenser;
  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Small_T>,
      TypeCastInfo<Small_T>>>
      smallPrimeTCTripleDispenser;

  std::unique_ptr<RandomnessDispenser<
      TypeCastTriple<Large_T>,
      TypeCastFromBitInfo<Large_T>>>
      endPrimeTCTripleDispenser;

  DivideRandomness(
      std::unique_ptr<RandomnessDispenser<
          PrefixOrRandomness<Small_T>,
          DoNotGenerateInfo>> prefixOrDispenser,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Large_T>,
          BeaverInfo<Large_T>>> multiplyDispenser,
      std::unique_ptr<RandomnessDispenser<
          PosIntCompareRandomness<Large_T, Small_T>,
          DoNotGenerateInfo>> compareDispenser,
      std::unique_ptr<RandomnessDispenser<
          TypeCastTriple<Small_T>,
          TypeCastFromBitInfo<Small_T>>>
          smallPrimeTCTripleFromBitDispenser,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Small_T>,
          BeaverInfo<Small_T>>> smallPrimeBeaverDispenser,
      std::unique_ptr<RandomnessDispenser<
          TypeCastTriple<Small_T>,
          TypeCastInfo<Small_T>>> smallPrimeTCTripleDispenser,

      std::unique_ptr<RandomnessDispenser<
          TypeCastTriple<Large_T>,
          TypeCastFromBitInfo<Large_T>>> endPrimeTCTripleDispenser);
};

template<FF_TYPENAMES, typename Large_T, typename Small_T>
class Divide : public Fronctocol<FF_TYPES> {

public:
  std::string name() override;

  DivideRandomness<Large_T, Small_T> randomness;
  /* Vars for Batched Multiply */
  /* Vars for Comparison */

  // Constructor or can be replaced with init()
  // TODO: test with serialization constructor (make sure change t.h)
  Divide(
      Large_T const & ms_x,
      Large_T const & ms_y,
      Large_T * const out,
      DivideInfo<Identity_T, Large_T, Small_T> const * const div_info,
      DivideRandomness<Large_T, Small_T> && randomness);

  void init() override; // Initialization
  void divide_awaitingBatchCompare(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingBatchTypecast(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingSecondBatchTypecast(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingThirdBatchTypecast(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingPrefixOR(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingBatchMultiply();
  void divide_awaitingCompare(ff::Fronctocol<FF_TYPES> & f);
  void divide_awaitingEndLoopTypecast(ff::Fronctocol<FF_TYPES> & f);
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

private:
  Large_T sh_dividend = 0;
  Large_T sh_divisor = 0;
  Large_T * const sh_quotient; // dividend = q*divisor + r
  DivideInfo<Identity_T, Large_T, Small_T> const * const info;
  size_t itr = 0;
  std::unique_ptr<Batch<FF_TYPES>> compare_batch;
  std::unique_ptr<Batch<FF_TYPES>> typecast_batch;
  std::unique_ptr<Batch<FF_TYPES>> mult_batch;

  std::vector<BeaverTriple<Large_T>> beaver = {};
  //std:vector<CompareCorrelatedRand<Number_t>> correl_compare;
  std::vector<Large_T> powtwos = {}; // 2^i
  std::vector<Large_T> sh_tiy =
      {}; // share of a_i s.t. a_i = 2^i*divisor
  std::vector<Boolean_t> sh_ais = {}; // xor share of a_i mod 2
  std::vector<Small_T> sh_ais_modp = {}; // additive share of a_i mode p
  std::vector<Large_T> sh_xis = {}; // share of a_i mod p
  std::vector<Small_T> sh_bis =
      {}; // share of b_i s.t. b_i = 1 - PrefixOR a_i
  std::vector<Boolean_t> sh_bis_bits = {};
  std::vector<Large_T> sh_bis_large_prime = {};
  std::vector<Large_T> sh_wis = {}; // share of w_i
  std::vector<Boolean_t> sh_cis_bits =
      {}; // share of c_i before typeCast
  std::vector<Large_T> sh_cis = {}; // share of c_i
  Large_T w_rhs_product = 0;
  Large_T c_rhs_product = 0;
  MultiplyInfo<Identity_T, BeaverInfo<Large_T>> large_mult_info; // ???

  /*Correlated Randomness for Batched Comparison, PrefixOR, Batched Multiplications */
  enum div_state {
    awaitingBatchCompare,
    awaitingBatchTypecast,
    awaitingPrefixOR,
    awaitingSecondBatchTypecast,
    awaitingThirdBatchTypecast,
    awaitingBatchMultiply,
    awaitingCompare,
    awaitingEndLoopTypecast
  };
  div_state state;
};

} // namespace mpc
} // namespace ff

#include <mpc/Divide.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_MULTIPLY_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_COMPARE_H_
#define FF_MPC_COMPARE_H_

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
#include <mpc/ModUtils.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/simplePrime.h>
#include <mpc/templates.h>

#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Large_T, typename Small_T>
struct DecomposedBitSetInfo;

template<typename Large_T, typename Small_T>
struct DecomposedBitSet {
  Large_T r;
  std::vector<Small_T> r_is;
  Boolean_t r_0;

  DecomposedBitSet(
      Large_T const & r, std::vector<Small_T> && r_is, Boolean_t r_0);

  DecomposedBitSet() = default;

  DecomposedBitSet(DecomposedBitSetInfo<Large_T, Small_T> const &) :
      DecomposedBitSet() {
  }

  static std::string name() {
    return std::string("Decomposed Bit Set");
  }
};

template<typename Large_T, typename Small_T>
struct DecomposedBitSetInfo {
  Large_T p;
  Small_T s;
  size_t ell;

  size_t instanceSize() const {
    return numberLen(this->s) * (this->ell) + numberLen(this->p) +
        sizeof(Boolean_t) + sizeof(size_t);
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<DecomposedBitSet<Large_T, Small_T>> & vals) const;

  bool operator==(DecomposedBitSetInfo const & other) const;
  bool operator!=(DecomposedBitSetInfo const & other) const {
    return !(*this == other);
  }

  DecomposedBitSetInfo(
      const Large_T & p, const Small_T & s, size_t ell);
  DecomposedBitSetInfo() = default;
};

// p: some prime
// s: some prime > 1 + ceil(log2(p))
// ell: ceil(log2(p))
// lambda: ceil(sqrt(ell))
template<typename Identity_T, typename Large_T, typename Small_T>
struct CompareInfo {
  const Large_T p;
  const Small_T s;
  const size_t ell;
  const size_t lambda;
  const std::vector<std::vector<Small_T>> lagrangePolynomialSet;
  const Identity_T * revealer;
  const PrefixOrInfo<Identity_T, Small_T> prefInfo;

  CompareInfo(
      Large_T const & p,
      Small_T const & s,
      size_t const ell,
      size_t const lambda,
      std::vector<std::vector<Small_T>> const & lagrangePolynomialSet,
      const Identity_T * revealer);
  CompareInfo(const Large_T & p, Identity_T const * const revealer);
  CompareInfo() = default;
};

template<typename Large_T, typename Small_T>
struct CompareRandomness {
  std::vector<ExponentSeries<Small_T>> exponentSeries;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Small_T>, BeaverInfo<Small_T>>>
      multiplyDispenser;
  TypeCastTriple<Small_T> Tct1;
  TypeCastTriple<Small_T> Tct2;

  DecomposedBitSet<Large_T, Small_T> singleDbs;

  CompareRandomness(
      std::vector<ExponentSeries<Small_T>> && exponentSeries,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Small_T>,
          BeaverInfo<Small_T>>> multiplyDispenser,
      TypeCastTriple<Small_T> && Tct1,
      TypeCastTriple<Small_T> && Tct2,
      DecomposedBitSet<Large_T, Small_T> && singleDbs);

  CompareRandomness(CompareRandomness &&) = default;
  CompareRandomness(const CompareRandomness &) = delete;
};

template<FF_TYPENAMES, typename Large_T, typename Small_T = SmallNum>
class Compare : public Fronctocol<FF_TYPES> {
public:
  virtual std::string name() override;

  Boolean_t outputShare;
  /*
   */
  Compare(
      Large_T const & share_of_x, // mod p
      Large_T const & share_of_y, // mod p
      CompareInfo<Identity_T, Large_T, Small_T> const * const
          compareInfo,
      CompareRandomness<Large_T, Small_T> && randomness);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  void initAfterRandomness();

  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum CompareState { awaitingReveal, awaitingBitwiseCompare };
  CompareState state = awaitingReveal;

  const Large_T share_of_x;
  const Large_T share_of_y;

  CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo;
  CompareRandomness<Large_T, Small_T> randomness;

  DecomposedBitSet<Large_T, Small_T> dbSet;

  std::vector<Small_T> c_bits_mod_s;
};

} // namespace mpc
} // namespace ff

#include <mpc/Compare.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_COMPARE_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_WAKSMAN_H_
#define FF_MPC_WAKSMAN_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <ff/Promise.h>
#include <mpc/Batch.h>
#include <mpc/Multiply.h>
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/Reveal.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
struct WaksmanInfo;

template<typename Number_T>
struct WaksmanBits {
  std::vector<Number_T> arithmeticBitShares;
  std::vector<Number_T> keyBitShares;
  std::vector<Boolean_t> XORBitShares;

  WaksmanBits(
      std::vector<Number_T> && arithmeticBitShares,
      std::vector<Number_T> && keyBitShares,
      std::vector<Boolean_t> && XORBitShares);
  WaksmanBits() = default;

  WaksmanBits(WaksmanInfo<Number_T> const &) : WaksmanBits() {
  }

  static std::string name() {
    return std::string("Waksman Bits");
  }
};

template<typename Number_T>
void swap(std::vector<Number_T> & bits, size_t i, size_t j);
size_t toggle(size_t current_d, size_t i);

constexpr Boolean_t UNCOLORED = 255;
constexpr Boolean_t RED = 0;
constexpr Boolean_t BLUE = 1;

template<typename Number_T>
struct WaksmanInfo {
  Number_T p;
  Number_T keyModulus;
  size_t n; // network operates on n elements
  size_t d; // 2^d >= n (for now, assume 2^d = n?)
  size_t
      w_of_n; // = approx n * d, but easier to pass it in as a constant

  size_t instanceSize() const {
    return (numberLen(this->p) + numberLen(this->keyModulus) +
            sizeof(Boolean_t)) *
        (w_of_n) +
        sizeof(size_t);
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<WaksmanBits<Number_T>> & vals) const;

  bool operator==(WaksmanInfo const & other) const;

  bool operator!=(WaksmanInfo const & other) const {
    return !(*this == other);
  }

  WaksmanInfo(
      const Number_T p,
      const Number_T keyModulus,
      const size_t n,
      const size_t d,
      const size_t w_of_n);
  WaksmanInfo() = default;
};

template<FF_TYPENAMES, typename Number_T>
class WaksmanShuffle : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  ObservationList<Number_T> & sharedList;

  /*
   */
  WaksmanShuffle(
      ObservationList<Number_T> & sharedList,
      Number_T modulus,
      Number_T keyModulus,
      WaksmanBits<Number_T> & swapBitShares,
      size_t d,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>> mrd,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>> mrd_key,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Boolean_t>,
          BooleanBeaverInfo>> XORmrd,
      const Identity_T * revealer);

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  void batchMultiplyForSwaps();
  void launchFinalReveal();

  enum WaksmanState { leftHalf, rightHalf, awaitingFinalReveal };
  WaksmanState state = leftHalf;
  Boolean_t awaitingArithmeticMultiplications = true;

  Number_T modulus;
  Number_T keyModulus;
  WaksmanBits<Number_T> swapBitShares;

  size_t d; // total depth, i.e. n = 2^d
  size_t depth = 0;
  size_t waksmanVectorCounter = 0;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      mrd;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      mrd_key;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> multiplyInfo;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> multiplyKeyInfo;

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      XORmrd;

  MultiplyInfo<Identity_T, BooleanBeaverInfo> XORmultiplyInfo;
  ::std::unique_ptr<Batch<FF_TYPES>> XORmultiplyBatch;

  BeaverInfo<Number_T> info;
  BeaverInfo<Number_T> info_key;
  const Identity_T * revealer;
  std::vector<Number_T> batchedMultiplyResults;

  std::vector<Number_T> batchedKeyMultiplyResults;

  std::vector<Boolean_t> batchedXORMultiplyResults;

  std::vector<Boolean_t> batchedRevealResults;
};

} // namespace mpc
} // namespace ff

#include <mpc/Waksman.t.h>

#include <ff/logging.h>

#endif // FF_MPC_WAKSMAN_H_

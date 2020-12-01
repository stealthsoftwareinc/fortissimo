/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_PREFIX_OR_H_
#define FF_MPC_PREFIX_OR_H_

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
#include <mpc/ModUtils.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/lagrange.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Identity_T, typename Number_T>
struct PrefixOrInfo {

  const Number_T s;
  const size_t lambda; // such that lambda^2 > inputVals.size()
  const size_t ell;
  const std::vector<std::vector<Number_T>> lagrangePolynomialSet;
  const Identity_T * rev;

  PrefixOrInfo(
      const Number_T s,
      const size_t lambda, // such that lambda^2 > inputVals.size()
      const size_t ell,
      const std::vector<std::vector<Number_T>> & lagrangePolynomialSet,
      const Identity_T * rev);

  PrefixOrInfo(
      const Number_T s, const size_t ell, const Identity_T * revealer);
};

template<typename Number_T>
struct PrefixOrRandomness {

  std::vector<ExponentSeries<Number_T>> exponentSeries;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      multiplyDispenser;

  PrefixOrRandomness(
      std::vector<ExponentSeries<Number_T>> && exponentSeries,
      std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>> multiplyDispenser) :
      exponentSeries(std::move(exponentSeries)),
      multiplyDispenser(std::move(multiplyDispenser)) {
  }
};

template<FF_TYPENAMES, typename Number_T>
class PrefixOr : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  std::vector<Number_T> orResults;

  /*
   * Takes in a list of shares of xvals mod some small prime s
   * satisfying s > xvals.size()+1. Computes this parties share of
   * the "or" of all values and stores it in orResult
   */
  PrefixOr(
      const std::vector<Number_T> & inputVals,
      PrefixOrInfo<Identity_T, Number_T> const * const info,
      PrefixOrRandomness<Number_T> && randomness);

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  void initAfterRandomness();

  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum PrefixState {
    awaitingFirstBatchedUnboundedFaninOr,
    awaitingFirstBatchedMultiply,
    awaitingSecondBatchedUnboundedFaninOr,
    awaitingSecondBatchedMultiply
  };
  PrefixState state = awaitingFirstBatchedUnboundedFaninOr;

  std::vector<Number_T> inputVals;
  PrefixOrInfo<Identity_T, Number_T> const * const info;
  PrefixOrRandomness<Number_T> randomness;

  size_t exponentSeriesIndex = 0;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> multiplyInfo;
  UnboundedFaninOrInfo<Identity_T, Number_T> unboundedFaninOrInfo;

  std::vector<Number_T> fronctocolResults;
  std::vector<Number_T> fronctocolResults2;
  std::vector<Number_T> y_values;
  std::vector<Number_T> w_values;
};

} // namespace mpc
} // namespace ff

#include <mpc/PrefixOr.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_PREFIX_OR_H_

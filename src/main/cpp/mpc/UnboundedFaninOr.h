/**
 * Copyright Stealth Software Technologies, Inc.
 *
 * Computes the unbounded-fanin-or of a set of values
 * As a preprocessing step (before calling this fronctocol)
 * The caller sums the shares of all values, and passes a
 * single value to this fronctocol
 */

#ifndef FF_MPC_UNBOUNDED_FANIN_OR_H_
#define FF_MPC_UNBOUNDED_FANIN_OR_H_

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
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/Reveal.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
struct ExponentSeriesInfo;

template<typename Number_T>
struct ExponentSeries : public ::std::vector<Number_T> {
  ExponentSeries(ExponentSeriesInfo<Number_T> const &) :
      ::std::vector<Number_T>() {
  }

  ExponentSeries() = default;

  static std::string name() {
    return std::string("Exponent Series");
  }
};

template<typename Number_T>
struct ExponentSeriesInfo {
  Number_T p = 0;
  size_t ell = 0;

  size_t instanceSize() const {
    return numberLen(p) * (ell + 1) + sizeof(uint64_t);
  }

  void generate(
      size_t n_parties,
      size_t,
      std::vector<ExponentSeries<Number_T>> & vals) const;

  bool operator==(ExponentSeriesInfo const & other) const {
    return this->ell == other.ell && this->p == other.p;
  }

  bool operator!=(ExponentSeriesInfo const & other) const {
    return !(*this == other);
  }
  ExponentSeriesInfo(const Number_T & p, const size_t ell) :
      p(p), ell(ell) {
  }
  ExponentSeriesInfo() = default;
};

template<typename Identity_T, typename Number_T>
struct UnboundedFaninOrInfo {
  Number_T const s;
  Identity_T const * const revealer;

  UnboundedFaninOrInfo(Number_T const & s, Identity_T const * const r) :
      s(s), revealer(r) {
  }
};

template<FF_TYPENAMES, typename Number_T>
class UnboundedFaninOr : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  Number_T sumOfValues;
  size_t numValues;
  Number_T * const output;
  ExponentSeries<Number_T> const es;
  BeaverTriple<Number_T> beaver;

  const ::std::vector<Number_T> lagrangePolynomial;

  UnboundedFaninOrInfo<Identity_T, Number_T> const * const info;

  /*
   * Takes in a list of shares of xvals mod some small prime s
   * satisfying s > xvals.size()+1. Computes this parties share of
   * the "or" of all values and stores it in orResult
   */
  UnboundedFaninOr(
      Number_T sumOfValues,
      size_t nv,
      Number_T * const out,
      ExponentSeries<Number_T> && e,
      BeaverTriple<Number_T> && b,
      ::std::vector<Number_T> const & lgp,
      UnboundedFaninOrInfo<Identity_T, Number_T> const * const i);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;

  void handlePromise(ff::Fronctocol<FF_TYPES> & fronctocol) override;

private:
  void initAfterRandomness();

  // This begins as "multiply" and becomes "reveal" once multiplication is done
  enum FaninState { multiply, reveal };
  FaninState completedState = multiply;

  std::vector<Number_T> randomSeries;

  Number_T A = 0;
  Number_T ATimesRinvShare = 0;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> multInfo;
};

} // namespace mpc
} // namespace ff

#include <mpc/UnboundedFaninOr.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_UNBOUNDED_FANIN_OR_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_MOD_UTIL_H_
#define FF_MPC_MOD_UTIL_H_

#include <cstdint>

/** Logging config */
#include <ff/logging.h>

namespace ff {
namespace mpc {

/**
 * Multiply a * b, over a modulus p.
 * It casts as necessary to avoid overflow.
 *
 * implementations exist for ff::mpc::LargeNum ff::mpc::SmallNum
 */
template<typename Number_T>
Number_T
modMul(Number_T const & a, Number_T const & b, Number_T const & p);

template<typename Number_T>
Number_T
modAdd(Number_T const & a, Number_T const & b, Number_T const & p);

template<typename Number_T>
Number_T
modSub(Number_T const & a, Number_T const & b, Number_T const & p) {
  log_assert(b < p);
  return modAdd(a, p - b, p);
}

template<typename Number_T>
std::size_t approxLog2(Number_T val);

/*
 * Inverts a mod s
 */
template<typename Number_T>
Number_T modInvert(Number_T const & num, Number_T const & mod);

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_MOD_UTIL_H_

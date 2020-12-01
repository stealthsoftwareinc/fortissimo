/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_SIMPLE_PRIME_H_
#define FF_MPC_SIMPLE_PRIME_H_

/* C and POSIX Headers */

/* C++ Headers */

/* 3rd Party Headers */

/* Fortissimo Headers */

#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
bool isPrime(Number_T value);

template<typename Number_T>
Number_T nextPrime(Number_T value);

template<>
bool isPrime(LargeNum value);

} // namespace mpc
} // namespace ff

#include <mpc/simplePrime.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_SIMPLE_PRIME_H_

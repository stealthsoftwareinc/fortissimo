/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * Generates the Lagrange coeffs we need for PrefixOr
 * i.e. returns a polynomial of degree ell
 * with f(1) = 0 and f(2)=f(3)=...=f(ell+1) = 1
 * Helper functions in lagrange.cpp
 */

#ifndef FF_MPC_LAGRANGE_H_
#define FF_MPC_LAGRANGE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <vector>

/* Fortissimo Headers */
#include <mpc/ModUtils.h>

/* polynomial is stored as a "little-endian" std::vector<Number_T>,
i.e. coeffs[0] is the constant term*/

namespace ff {
namespace mpc {

template<typename Number_T>
void computeLagrangeCoeffsForPrefixOr(
    Number_T const & s,
    Number_T const & ell,
    std::vector<Number_T> & ref);

template<typename Number_T>
std::vector<Number_T> computeLagrangeCoeffsForPrefixOr(
    Number_T const & s, Number_T const & ell);

template<typename Number_T>
void polyAdd(
    std::vector<Number_T> & left_term,
    std::vector<Number_T> & right_term,
    Number_T const & s,
    Number_T const & degree);

template<typename Number_T>
void polyMultByScalar(
    std::vector<Number_T> & coeffs,
    Number_T const & scalar,
    Number_T const & s,
    Number_T const & degree);

/* more or less hidden functions, exposed only for testing */
template<typename Number_T>
void computeProductOneToEllPlusOne(
    Number_T const & s,
    Number_T const & ell,
    std::vector<Number_T> & ret); // product (x-i) from one to ell

} // namespace mpc
} // namespace ff

#include <mpc/lagrange.t.h>

#endif //FF_MPC_LAGRANGE_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#include <ff/logging.h>
#include <mpc/lagrange.h>

#include <string>

namespace ff {
namespace mpc {

template<typename Number_T>
void divideByDegreeOnePolynomial(
    std::vector<Number_T> & coeffs,
    std::vector<Number_T> & overwritten_output,
    Number_T const & s,
    Number_T const & a,
    Number_T const & ell); // divides by (x-a) mod s

template<typename Number_T>
Number_T multiplyDifferentXValues(
    Number_T const & s, Number_T const & ell, Number_T const & i);

template<typename Number_T>
std::vector<Number_T> computeLagrangeCoeffsForPrefixOr(
    Number_T const & s, Number_T const & ell) {
  std::vector<Number_T> ret;
  computeLagrangeCoeffsForPrefixOr(s, ell, ret);
  return ret;
}

template<typename Number_T>
void computeLagrangeCoeffsForPrefixOr(
    Number_T const & s,
    Number_T const & ell,
    std::vector<Number_T> & coeffs_to_return) {
  coeffs_to_return = std::vector<Number_T>((size_t)ell + 1, 0);

  std::vector<Number_T> coeffs_of_product;
  computeProductOneToEllPlusOne(s, ell, coeffs_of_product);
  std::vector<Number_T> intermediate_term(
      static_cast<size_t>(ell + 1), 0);

  for (Number_T i = 2; i < ell + 2; i++) {
    divideByDegreeOnePolynomial(
        coeffs_of_product, intermediate_term, s, i, ell);
    polyMultByScalar(
        intermediate_term,
        modInvert(multiplyDifferentXValues(s, ell, i), s),
        s,
        ell);
    polyAdd(coeffs_to_return, intermediate_term, s, ell);
  }
}

template<typename Number_T>
void computeProductOneToEllPlusOne(
    Number_T const & s,
    Number_T const & ell,
    std::vector<Number_T> & coeffs_to_return) {
  coeffs_to_return = std::vector<Number_T>(
      (size_t)ell + 2,
      1); // at round i, we want to set the coefficient of x^i term to 1.
  coeffs_to_return[0] = s - 1;

  for (Number_T i = 2; i < ell + 2; i++) {
    for (Number_T j = i - 1; j != 0; j--) {
      coeffs_to_return[static_cast<size_t>(j)] =
          (s - i) * coeffs_to_return[static_cast<size_t>(j)] +
          coeffs_to_return[static_cast<size_t>(j - 1)];
      coeffs_to_return[static_cast<size_t>(j)] %= s;
    }
    coeffs_to_return[0] *= (s - i);
    coeffs_to_return[0] %= s;
  }
}

/*
Divides by (x-a)
*/
template<typename Number_T>
void divideByDegreeOnePolynomial(
    std::vector<Number_T> & coeffs,
    std::vector<Number_T> & output,
    Number_T const & s,
    Number_T const & a,
    Number_T const & ell) {
  output[static_cast<size_t>(ell)] = 1;
  for (size_t i = static_cast<size_t>(ell - 1); i + 1 != 0; i--) {
    output[i] = coeffs[i + 1] + a * output[i + 1];
    output[i] %= s;
  }
}

template<typename Number_T>
void polyAdd(
    std::vector<Number_T> & left_term,
    std::vector<Number_T> & right_term,
    Number_T const & s,
    Number_T const & degree) {
  for (size_t i = 0; i < static_cast<size_t>(degree + 1); i++) {
    left_term[i] = modAdd(left_term[i], right_term[i], s);
  }
}

template<typename Number_T>
void polyMultByScalar(
    std::vector<Number_T> & coeffs,
    Number_T const & scalar,
    Number_T const & s,
    Number_T const & degree) {
  for (size_t i = 0; i < static_cast<size_t>(degree + 1); i++) {
    coeffs[i] = modMul(coeffs[i], scalar, s);
  }
}

template<typename Number_T>
Number_T multiplyDifferentXValues(
    Number_T const & s, Number_T const & ell, Number_T const & a) {
  Number_T product = 1;

  for (Number_T i = 1; i < ell + 2; i++) {
    if (i != a) {
      product = (product * ((s + a - i) % s)) % s;
    }
  }

  return product;
}

} // namespace mpc
} // namespace ff

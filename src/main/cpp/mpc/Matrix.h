/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_MATRIX_H_
#define FF_MPC_MATRIX_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <string>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <mpc/ModUtils.h>
#include <mpc/templates.h>

/* Logging config */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<typename Number_T>
class Matrix {
private:
  std::vector<Number_T> buffer;
  size_t numRows;
  size_t numColumns;

public:
  Matrix(
      ::std::vector<Number_T> && buf, size_t const nr, size_t const nc);
  Matrix(
      ::std::vector<Number_T> const & buf,
      size_t const nr,
      size_t const nc);

  Matrix(size_t const nr, size_t const nc);
  Matrix(size_t const id); // creates a square identity matrix.
  Matrix(Matrix const &) = default;
  Matrix(Matrix &&) = default;

  Matrix & operator=(Matrix const &) = default;
  Matrix & operator=(Matrix &&) = default;

  Number_T & at(size_t const i, size_t const j);
  Number_T const & at(size_t const i, size_t const j) const;

  size_t getNumRows() const;
  size_t getNumColumns() const;

  std::string Print() const;

  // Matrix Utility functions:
  //   - Returns M^-1
  //     (NOTE: Requires 'modulus' to know how to find multiplicative inverse).
  Matrix<Number_T> Inverse(const Number_T & modulus) const;
  //   - Sets M -> M^-1.
  //     (NOTE: Requires 'modulus' to know how to find multiplicative inverse).
  void Invert(const Number_T & modulus);
  //   - Returns M^T (Transpose of M).
  Matrix<Number_T> Transpose() const;
  //   - Sets M -> M^T (Transpose of M).
  void DoTranspose();
  //   - Returns det(M).
  //     (NOTE: Requires 'modulus' to know how to find multiplicative inverse).
  Number_T Det(const Number_T & modulus) const;
  //   - Returns trace(M).
  Number_T Trace() const;
  //   - Given input matrix 'b', forms augmented matrix: M | b, and then
  //     simulates the (row-reduce) operations that transform M into Id,
  //     performing those same operations on b.
  //     NOTE: The two most common use-cases are for when b = Id (and then
  //     the output will have b = M^-1), and when b = y (a vector), and
  //     then the output b will be the solution for 'x' in: Mx = y.
  void
  MakeIdentity(const Number_T & modulus, Matrix<Number_T> & b) const;
};

template<typename Number_T>
void plainMatrixMult(
    Matrix<Number_T> const * const A,
    Matrix<Number_T> const * const B,
    Matrix<Number_T> * const C,
    Number_T const p);

} // namespace mpc
} // namespace ff

#include <mpc/Matrix.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_MATRIX_H_

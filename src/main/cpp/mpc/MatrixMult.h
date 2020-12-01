/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_MATRIX_MULT_H_
#define FF_MPC_MATRIX_MULT_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>

#include <mpc/Batch.h>
#include <mpc/Matrix.h>
#include <mpc/ModUtils.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging config */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
class MatrixMult : public Batch<FF_TYPES> {
  ::std::vector<Number_T> products;

public:
  ::std::string name() override;

  // Input matrixes
  Matrix<Number_T> const * const A;
  Matrix<Number_T> const * const B;

  // Output matrix
  Matrix<Number_T> * const C;

  MultiplyInfo<Identity_T, BeaverInfo<Number_T>> const multInfo;

  ::std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      beaverTriples;

  MatrixMult(
      Matrix<Number_T> const * const A,
      Matrix<Number_T> const * const B,
      Matrix<Number_T> * const C,
      MultiplyInfo<Identity_T, BeaverInfo<Number_T>> mi,
      ::std::unique_ptr<RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>> bts) :
      Batch<FF_TYPES>(),
      A(A),
      B(B),
      C(C),
      multInfo(mi),
      beaverTriples(::std::move(bts)) {
  }

  void onInit() override;
  void onComplete() override;
};

} // namespace mpc
} // namespace ff

#include <mpc/MatrixMult.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_MATRIX_MULT_H_

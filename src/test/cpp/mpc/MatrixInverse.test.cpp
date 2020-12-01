/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mpc/Matrix.h>
#include <mpc/Randomness.h>

void generate_random_matrix(
    uint32_t const prime, ff::mpc::Matrix<uint32_t> * M) {

  for (size_t i = 0; i < M->getNumRows(); i++) {
    for (size_t j = 0; j < M->getNumColumns(); j++) {
      M->at(i, j) = ff::mpc::randomModP<uint32_t>(prime);
    }
  }
}

TEST(RowReduction, row_reduce) {
  uint32_t const prime = 97;
  size_t const size = 5;
  ff::mpc::Matrix<uint32_t> b(size, 1); // n x 1 (n rows, 1 col)

  for (size_t i = 0; i < size; i++) {
    b.at(i, 0) = ff::mpc::randomModP<uint32_t>(prime);
  }

  ff::mpc::Matrix<uint32_t> M(size, size);
  uint32_t det = 0;
  while (det == 0) {
    generate_random_matrix(prime, &M);
    det = M.Det(prime);
  }

  ff::mpc::Matrix<uint32_t> new_b(b);

  M.MakeIdentity(prime, new_b);

  ff::mpc::Matrix<uint32_t> oldb(b.getNumRows(), b.getNumColumns());
  ff::mpc::plainMatrixMult(&M, &new_b, &oldb, prime);

  for (size_t i = 0; i < oldb.getNumRows(); i++) {
    EXPECT_EQ(oldb.at(i, 0) % prime, b.at(i, 0) % prime);
  }
}

TEST(RowReduction, inverse) {
  uint32_t const prime = 97;
  size_t const size = 5;
  ff::mpc::Matrix<uint32_t> M(size, size);
  uint32_t det = 0;
  while (det == 0) {
    generate_random_matrix(prime, &M);
    det = M.Det(prime);
  }

  ff::mpc::Matrix<uint32_t> M_inverse(M.Inverse(prime));
  ff::mpc::Matrix<uint32_t> ExpectedIdentity(M.getNumRows());
  ff::mpc::Matrix<uint32_t> ActualIdentity(M.getNumRows());

  ff::mpc::plainMatrixMult(&M, &M_inverse, &ExpectedIdentity, prime);

  for (size_t i = 0; i < ExpectedIdentity.getNumRows(); i++) {
    for (size_t j = 0; j < ExpectedIdentity.getNumColumns(); j++) {
      EXPECT_EQ(ExpectedIdentity.at(i, j), ActualIdentity.at(i, j))
          << "i: " << i << ", j: " << j << std::endl;
    }
  }
}

TEST(RowReduction, UpperTriangularDeterminant) {
  uint32_t const prime = 97;
  size_t const size = 5;
  ff::mpc::Matrix<uint32_t> M(size, size);
  uint32_t det = 0;
  while (det == 0) {
    generate_random_matrix(prime, &M);
    det = M.Det(prime);
  }

  uint32_t expectedDeterminant = 1U;
  for (size_t i = 0; i < size; i++) {
    for (size_t j = 0; j <= i; j++) {
      if (j == i) {
        expectedDeterminant *= M.at(i, j);
        expectedDeterminant %= prime;
      } else {
        M.at(i, j) = 0;
      }
    }
  }
  auto actualDeterminant = M.Det(prime);
  EXPECT_EQ(expectedDeterminant, actualDeterminant);
}

TEST(RowReduction, determinant) {
  uint32_t const prime = 97;
  size_t const size = 5;

  ff::mpc::Matrix<uint32_t> M(size, size);
  generate_random_matrix(prime, &M);

  auto detM = M.Det(prime);
  if (detM == 0) {
    EXPECT_TRUE(true);
  } else {
    ff::mpc::Matrix<uint32_t> MInverse(M.Inverse(prime));
    auto detMInverse = MInverse.Det(prime);
    EXPECT_EQ(ff::mpc::modMul(detM, detMInverse, prime), 1U);
  }
}

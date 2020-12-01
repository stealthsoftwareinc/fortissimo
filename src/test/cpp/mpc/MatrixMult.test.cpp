
/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/MatrixMult.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

ArithmeticShare_t const P = 97;

TEST(MatrixMult, Matrix) {
  Matrix<ArithmeticShare_t> m(
      randomModP<size_t>(2 * P), randomModP(2 * P));

  ArithmeticShare_t const seed = randomModP(P);
  auto init_rand = std::bind(
      std::uniform_int_distribution<ArithmeticShare_t>(0, P),
      std::default_random_engine(seed));

  for (size_t i = 0; i < m.getNumRows(); i++) {
    for (size_t j = 0; j < m.getNumColumns(); j++) {
      m.at(i, j) = init_rand();
    }
  }

  auto check_rand = std::bind(
      std::uniform_int_distribution<ArithmeticShare_t>(0, P),
      std::default_random_engine(seed));

  for (size_t i = 0; i < m.getNumRows(); i++) {
    for (size_t j = 0; j < m.getNumColumns(); j++) {
      ASSERT_EQ(check_rand(), m.at(i, j));
    }
  }
}

TEST(MatrixMult, PlainMultConst) {
  Matrix<uint32_t> A({{5, 8, 3, 2, 0, 4}}, 3, 2);
  Matrix<uint32_t> B({{6, 3, 2, 5}}, 2, 2);

  Matrix<uint32_t> C_const({{46, 55, 22, 19, 8, 20}}, 3, 2);

  Matrix<uint32_t> C_calc(3, 2);

  plainMatrixMult(&A, &B, &C_calc, P);

  for (size_t i = 0; i < C_const.getNumRows(); i++) {
    for (size_t j = 0; j < C_const.getNumColumns(); j++) {
      EXPECT_EQ(C_const.at(i, j), C_calc.at(i, j));
    }
  }
}

TEST(MatrixMult, PlainMult) {
  // Choosing A, B, and C at random make matrixes of size AxB and BxC
  // Result should be a matrix of AxC
  size_t const a = randomModP(2 * P);
  size_t const b = randomModP(2 * P);
  size_t const c = randomModP(2 * P);

  log_info("A:= %zux%zu B:= %zux%zu C:= %zux%zu", a, b, b, c, a, c);

  Matrix<ArithmeticShare_t> A(a, b);
  Matrix<ArithmeticShare_t> B(b, c);

  for (size_t i = 0; i < A.getNumRows(); i++) {
    for (size_t j = 0; j < A.getNumColumns(); j++) {
      A.at(i, j) = randomModP(P);
    }
  }

  for (size_t i = 0; i < B.getNumRows(); i++) {
    for (size_t j = 0; j < B.getNumColumns(); j++) {
      B.at(i, j) = randomModP(P);
    }
  }

  Matrix<ArithmeticShare_t> C(a, c);

  plainMatrixMult(&A, &B, &C, P);
}

template<typename Number_T>
void testCipherMult(Number_T const prime) {
  // Choosing A, B, and C at random make matrixes of size AxB and BxC
  // Result should be a matrix of AxC

  LogTimer timer = log_time_start("CipherMult");

  size_t const a = randomModP<size_t>(16) + 1;
  size_t const b = randomModP<size_t>(16) + 1;
  size_t const c = randomModP<size_t>(16) + 1;

  log_info("A:= %zux%zu B:= %zux%zu C:= %zux%zu", a, b, b, c, a, c);

  Matrix<Number_T> A(a, b);
  Matrix<Number_T> B(b, c);

  log_time_update(timer, "allocated matrices");
  log_info("generating random matrices");

  for (size_t i = 0; i < A.getNumRows(); i++) {
    for (size_t j = 0; j < A.getNumColumns(); j++) {
      A.at(i, j) = randomModP<Number_T>(prime);
    }
  }

  for (size_t i = 0; i < B.getNumRows(); i++) {
    for (size_t j = 0; j < B.getNumColumns(); j++) {
      B.at(i, j) = randomModP<Number_T>(prime);
    }
  }

  Matrix<Number_T> C(a, c);

  log_time_update(timer, "generated random matrices");
  log_info("plaintext multiply");

  plainMatrixMult(&A, &B, &C, prime);

  log_time_update(timer, "multiplied in plaintext");
  log_info("secret sharing");

  // secret share A and B for alice and bob.
  Matrix<Number_T> alice_A(a, b);
  Matrix<Number_T> bob_A(a, b);

  for (size_t i = 0; i < A.getNumRows(); i++) {
    for (size_t j = 0; j < A.getNumColumns(); j++) {
      std::vector<Number_T> shares;
      arithmeticSecretShare<Number_T>(2, prime, A.at(i, j), shares);
      alice_A.at(i, j) = shares[0];
      bob_A.at(i, j) = shares[1];
    }
  }

  Matrix<Number_T> alice_B(b, c);
  Matrix<Number_T> bob_B(b, c);

  for (size_t i = 0; i < B.getNumRows(); i++) {
    for (size_t j = 0; j < B.getNumColumns(); j++) {
      std::vector<Number_T> shares;
      arithmeticSecretShare<Number_T>(2, prime, B.at(i, j), shares);
      alice_B.at(i, j) = shares[0];
      bob_B.at(i, j) = shares[1];
    }
  }

  Matrix<Number_T> alice_C(a, c);
  Matrix<Number_T> bob_C(a, c);

  log_time_update(timer, "secret shared matrices");
  log_info("generating beaver triples");

  std::string revealer("alice");
  MultiplyInfo<std::string, BeaverInfo<Number_T>> mult_info(
      &revealer, BeaverInfo<Number_T>(prime));

  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      alice_beavers(new RandomnessDispenser<
                    BeaverTriple<Number_T>,
                    BeaverInfo<Number_T>>(BeaverInfo<Number_T>(prime)));
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Number_T>, BeaverInfo<Number_T>>>
      bob_beavers(new RandomnessDispenser<
                  BeaverTriple<Number_T>,
                  BeaverInfo<Number_T>>(BeaverInfo<Number_T>(prime)));

  std::vector<BeaverTriple<Number_T>> triple_shares;
  for (size_t i = 0; i < a * b * c; i++) {
    mult_info.info.generate(2, i, triple_shares);
    alice_beavers->insert(triple_shares[0]);
    bob_beavers->insert(triple_shares[1]);
  }

  log_time_update(timer, "generated beaver triples");
  log_info("ciphertext multiply");

  std::map<std::string, std::unique_ptr<Fronctocol>> tests;

  tests["alice"] =
      std::unique_ptr<Fronctocol>(new MatrixMult<TEST_TYPES, Number_T>(
          &alice_A,
          &alice_B,
          &alice_C,
          mult_info,
          std::move(alice_beavers)));
  tests["bob"] =
      std::unique_ptr<Fronctocol>(new MatrixMult<TEST_TYPES, Number_T>(
          &bob_A, &bob_B, &bob_C, mult_info, std::move(bob_beavers)));

  EXPECT_TRUE(runTests(tests));

  log_time_update(timer, "multiplied in ciphertext");
  log_info("checking result");

  Matrix<Number_T> unshared_C(a, c);

  for (size_t i = 0; i < C.getNumRows(); i++) {
    for (size_t j = 0; j < C.getNumColumns(); j++) {
      unshared_C.at(i, j) =
          modAdd(alice_C.at(i, j), bob_C.at(i, j), prime);
    }
  }

  for (size_t i = 0; i < C.getNumRows(); i++) {
    for (size_t j = 0; j < C.getNumColumns(); j++) {
      EXPECT_EQ(C.at(i, j), unshared_C.at(i, j));
    }
  }

  log_time_update(timer, "checked results");
}

TEST(MatrixMult, CipherMultSmallNum) {
  SmallNum const prime = 65521;
  testCipherMult<SmallNum>(prime);
}

TEST(MatrixMult, CipherMultLargeNum) {
  // 1536 group from https://tools.ietf.org/html/rfc3526
  LargeNum const prime = largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));
  testCipherMult<LargeNum>(prime);
}

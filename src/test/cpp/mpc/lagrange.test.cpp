/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <string>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/ModUtils.h>
#include <mpc/Randomness.h>
#include <mpc/lagrange.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

template<typename Number_T>
Number_T
evaluate(std::vector<Number_T> & coeffs, Number_T val, Number_T s);

TEST(LagrangeTester, modular_inversion_mod_11) {
  SmallNum s = 11;
  for (SmallNum i = 1; i < s; i++) {
    EXPECT_EQ(1, (i * modInvert(i, s) % s));
  }
}

TEST(LagrangeTester, modular_inversion_mod_65521) {
  SmallNum s = 65521;
  for (SmallNum i = 1; i < s; i++) {
    EXPECT_EQ(1, (i * modInvert(i, s) % s));
  }
}

TEST(LagrangeTester, modular_inversion_mod_bignum) {
  // 1536 group from https://tools.ietf.org/html/rfc3526
  const LargeNum s = largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));

  for (LargeNum i = 1; i < 2048; i++) {
    LargeNum val = static_cast<LargeNum>(1) + randomModP(s - 1);
    EXPECT_EQ(static_cast<LargeNum>(1), (val * modInvert(val, s) % s));
  }
}

TEST(LagrangeTester, product_x_minus_i_small) {
  SmallNum s = 11;
  SmallNum ell = 4;

  std::vector<SmallNum> true_coeffs = std::vector<SmallNum>(6);
  true_coeffs[0] = 1;
  true_coeffs[1] = 10;
  true_coeffs[2] = 6;
  true_coeffs[3] = 8;
  true_coeffs[4] = 7;
  true_coeffs[5] = 1;

  std::vector<SmallNum> coeffs;
  computeProductOneToEllPlusOne(s, ell, coeffs);
  for (SmallNum i = 0; i < ell + 2; i++) {
    EXPECT_EQ(true_coeffs[i], coeffs[i]);
  }
}

TEST(LagrangeTester, lagrange_poly_mod_11) {
  SmallNum s = 11;
  SmallNum ell = 7;
  std::vector<SmallNum> coeffs =
      computeLagrangeCoeffsForPrefixOr(s, ell);

  EXPECT_EQ(0, evaluate(coeffs, (SmallNum)1, s));
  for (SmallNum i = 2; i < ell + 1; i++) {
    EXPECT_EQ(1, evaluate(coeffs, i, s));
  }
}

TEST(LagrangeTester, lagrange_poly_mod_509) {
  SmallNum s = 509;
  SmallNum ell = 500;
  std::vector<SmallNum> coeffs =
      computeLagrangeCoeffsForPrefixOr(s, ell);

  EXPECT_EQ(0, evaluate(coeffs, (SmallNum)1, s));
  for (SmallNum i = 2; i < ell + 1; i++) {
    EXPECT_EQ(1, evaluate(coeffs, i, s));
  }
}

TEST(LagrangeTester, lagrange_poly_mod_bignum) {
  // 1536 group from https://tools.ietf.org/html/rfc3526
  const LargeNum s = largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));
  LargeNum ell = 128;
  std::vector<LargeNum> coeffs =
      computeLagrangeCoeffsForPrefixOr(s, ell);

  EXPECT_EQ(
      static_cast<LargeNum>(0),
      evaluate(coeffs, static_cast<LargeNum>(1), s));
  for (LargeNum i = 2; i < ell + 1; i++) {
    EXPECT_EQ(static_cast<LargeNum>(1), evaluate(coeffs, i, s));
  }
}

// s is a prime
template<typename Number_T>
Number_T
evaluate(std::vector<Number_T> & coeffs, Number_T val, Number_T s) {
  Number_T ans = coeffs[0];
  Number_T val_pow = 1;
  for (size_t i = 1; i < coeffs.size(); i++) {
    val_pow = modMul(val_pow, val, s);
    Number_T t = modMul(val_pow, coeffs[i], s);
    ans = modAdd(ans, t, s);
  }
  return ans;
}

/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <map>
#include <memory>
#include <string>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/UnboundedFaninOr.h>
#include <mpc/lagrange.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

template<typename Number_T>
Number_T sum(std::vector<Number_T> & vec);

TEST(Compare, unbounded_fanin_or_smallnum) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 65521;

  SmallNum share_income = 0;
  SmallNum share_univ1 = 0;
  SmallNum share_univ2 = 0;

  std::string revealer("income");

  std::vector<SmallNum> og_vals = {
      randomModP(2U), randomModP(2U), randomModP(2U)};
  size_t const ell = og_vals.size();

  // vals transposed the wrong way.
  std::vector<std::vector<SmallNum>> vals_T;

  for (size_t i = 0; i < og_vals.size(); i++) {
    vals_T.emplace_back(0);
    arithmeticSecretShare(3, p, og_vals[i], vals_T[i]);
  }

  std::vector<SmallNum> vals1 = {
      vals_T[0][0], vals_T[1][0], vals_T[2][0]};
  std::vector<SmallNum> vals2 = {
      vals_T[0][1], vals_T[1][1], vals_T[2][1]};
  std::vector<SmallNum> vals3 = {
      vals_T[0][2], vals_T[1][2], vals_T[2][2]};

  std::vector<SmallNum> polynomial = computeLagrangeCoeffsForPrefixOr(
      p, static_cast<SmallNum>(vals1.size()));

  UnboundedFaninOrInfo<std::string, SmallNum> info(p, &revealer);
  ExponentSeriesInfo<SmallNum> exp_info(p, ell);

  std::vector<ExponentSeries<SmallNum>> es;
  exp_info.generate(3, 0, es);

  std::vector<BeaverTriple<SmallNum>> beavers;
  BeaverInfo<SmallNum> beaver_info(info.s);
  beaver_info.generate(3, 0, beavers);

  test["income"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, SmallNum>(
          sum(vals1),
          ell,
          &share_income,
          std::move(es[0]),
          std::move(beavers[0]),
          polynomial,
          &info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, SmallNum>(
          sum(vals2),
          ell,
          &share_univ1,
          std::move(es[1]),
          std::move(beavers[1]),
          polynomial,
          &info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, SmallNum>(
          sum(vals3),
          ell,
          &share_univ2,
          std::move(es[2]),
          std::move(beavers[2]),
          polynomial,
          &info));

  log_debug("launching tests");
  EXPECT_TRUE(runTests(test));

  log_debug("Vals %u, %u, %u,", share_univ1, share_univ2, share_income);

  SmallNum correct_result = og_vals[0];
  for (size_t i = 1; i < og_vals.size(); i++) {
    correct_result = correct_result | og_vals[i];
  }

  EXPECT_EQ(
      correct_result, (share_univ1 + share_univ2 + share_income) % p);
};

TEST(Compare, unbounded_fanin_or_largenum) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const LargeNum p = largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));

  LargeNum share_income = 0;
  LargeNum share_univ1 = 0;
  LargeNum share_univ2 = 0;

  std::string revealer("income");

  std::vector<LargeNum> og_vals = {
      randomModP(2U), randomModP(2U), randomModP(2U)};
  size_t const ell = og_vals.size();

  // vals transposed the wrong way.
  std::vector<std::vector<LargeNum>> vals_T;

  for (size_t i = 0; i < og_vals.size(); i++) {
    vals_T.emplace_back(0);
    arithmeticSecretShare(3, p, og_vals[i], vals_T[i]);
  }

  std::vector<LargeNum> vals1 = {
      vals_T[0][0], vals_T[1][0], vals_T[2][0]};
  std::vector<LargeNum> vals2 = {
      vals_T[0][1], vals_T[1][1], vals_T[2][1]};
  std::vector<LargeNum> vals3 = {
      vals_T[0][2], vals_T[1][2], vals_T[2][2]};

  std::vector<LargeNum> polynomial = computeLagrangeCoeffsForPrefixOr(
      p, static_cast<LargeNum>(vals1.size()));

  UnboundedFaninOrInfo<std::string, LargeNum> info(p, &revealer);
  ExponentSeriesInfo<LargeNum> exp_info(p, ell);

  std::vector<ExponentSeries<LargeNum>> es;
  exp_info.generate(3, 0, es);

  std::vector<BeaverTriple<LargeNum>> beavers;
  BeaverInfo<LargeNum> beaver_info(info.s);
  beaver_info.generate(3, 0, beavers);

  test["income"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, LargeNum>(
          sum(vals1),
          ell,
          &share_income,
          std::move(es[0]),
          std::move(beavers[0]),
          polynomial,
          &info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, LargeNum>(
          sum(vals2),
          ell,
          &share_univ1,
          std::move(es[1]),
          std::move(beavers[1]),
          polynomial,
          &info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new UnboundedFaninOr<TEST_TYPES, LargeNum>(
          sum(vals3),
          ell,
          &share_univ2,
          std::move(es[2]),
          std::move(beavers[2]),
          polynomial,
          &info));

  log_debug("launching tests");
  EXPECT_TRUE(runTests(test));

  log_debug("Vals %u, %u, %u,", share_univ1, share_univ2, share_income);

  LargeNum correct_result = og_vals[0];
  for (size_t i = 1; i < og_vals.size(); i++) {
    correct_result = correct_result | og_vals[i];
  }

  EXPECT_EQ(
      correct_result, (share_univ1 + share_univ2 + share_income) % p);
};

template<typename Number_T>
Number_T sum(std::vector<Number_T> & vec) {
  Number_T val = 0;
  for (size_t i = 0; i < vec.size(); i++) {
    val += vec[i];
  }
  return val;
}

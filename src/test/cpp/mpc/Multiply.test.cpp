/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/Multiply.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Multiply, uint32_arithmetic_multiply) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string revealer("income");
  const uint32_t p = (1U << 31) - 1; // mersenne prime
  MultiplyInfo<std::string, BeaverInfo<uint32_t>> mult_info(
      &revealer, BeaverInfo<uint32_t>(p));

  const uint32_t x = randomModP(p);
  const uint32_t y = randomModP(p);

  std::vector<uint32_t> xs;
  arithmeticSecretShare(3, p, x, xs);
  std::vector<uint32_t> ys;
  arithmeticSecretShare(3, p, y, ys);

  std::vector<BeaverTriple<uint32_t>> beavers;
  mult_info.info.generate(3, 1, beavers);

  uint32_t income_share = 0;
  uint32_t univ1_share = 0;
  uint32_t univ2_share = 0;

  test["income"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint32_t, BeaverInfo<uint32_t>>(
          xs[0],
          ys[0],
          &income_share,
          std::move(beavers[0]),
          &mult_info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint32_t, BeaverInfo<uint32_t>>(
          xs[1],
          ys[1],
          &univ1_share,
          std::move(beavers[1]),
          &mult_info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint32_t, BeaverInfo<uint32_t>>(
          xs[2],
          ys[2],
          &univ2_share,
          std::move(beavers[2]),
          &mult_info));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug("%u, %u, %u", income_share, univ1_share, univ2_share);
  EXPECT_EQ(
      modMul(x, y, p),
      modAdd(modAdd(income_share, univ1_share, p), univ2_share, p));
};

TEST(Multiply, uint64_arithmetic_multiply) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string revealer("income");
  const uint64_t p = (1UL << 61) - 1; // mersenne prime
  MultiplyInfo<std::string, BeaverInfo<uint64_t>> mult_info(
      &revealer, BeaverInfo<uint64_t>(p));

  const uint64_t x = randomModP(p);
  const uint64_t y = randomModP(p);

  std::vector<uint64_t> xs;
  arithmeticSecretShare(3, p, x, xs);
  std::vector<uint64_t> ys;
  arithmeticSecretShare(3, p, y, ys);

  std::vector<BeaverTriple<uint64_t>> beavers;
  mult_info.info.generate(3, 1, beavers);

  uint64_t income_share = 0;
  uint64_t univ1_share = 0;
  uint64_t univ2_share = 0;

  test["income"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint64_t, BeaverInfo<uint64_t>>(
          xs[0],
          ys[0],
          &income_share,
          std::move(beavers[0]),
          &mult_info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint64_t, BeaverInfo<uint64_t>>(
          xs[1],
          ys[1],
          &univ1_share,
          std::move(beavers[1]),
          &mult_info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, uint64_t, BeaverInfo<uint64_t>>(
          xs[2],
          ys[2],
          &univ2_share,
          std::move(beavers[2]),
          &mult_info));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug("%u, %u, %u", income_share, univ1_share, univ2_share);
  EXPECT_EQ(
      modMul(x, y, p),
      modAdd(modAdd(income_share, univ1_share, p), univ2_share, p));
};

TEST(Multiply, large_arithmetic_multiply) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string revealer("income");

  // 1536 group from https://tools.ietf.org/html/rfc3526
  const LargeNum p = largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));

  log_info("using prime %s", dec(p).c_str());

  MultiplyInfo<std::string, BeaverInfo<LargeNum>> mult_info(
      &revealer, BeaverInfo<LargeNum>(p));

  const LargeNum x = randomModP(p);
  const LargeNum y = randomModP(p);

  std::vector<LargeNum> xs;
  arithmeticSecretShare(3, p, x, xs);
  std::vector<LargeNum> ys;
  arithmeticSecretShare(3, p, y, ys);

  std::vector<BeaverTriple<LargeNum>> beavers;
  mult_info.info.generate(3, 1, beavers);

  LargeNum income_share = 0;
  LargeNum univ1_share = 0;
  LargeNum univ2_share = 0;

  test["income"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, LargeNum, BeaverInfo<LargeNum>>(
          xs[0],
          ys[0],
          &income_share,
          std::move(beavers[0]),
          &mult_info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, LargeNum, BeaverInfo<LargeNum>>(
          xs[1],
          ys[1],
          &univ1_share,
          std::move(beavers[1]),
          &mult_info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, LargeNum, BeaverInfo<LargeNum>>(
          xs[2],
          ys[2],
          &univ2_share,
          std::move(beavers[2]),
          &mult_info));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug(
      "%s, %s, %s",
      dec(income_share).c_str(),
      dec(univ1_share).c_str(),
      dec(univ2_share).c_str());
  EXPECT_EQ(
      (x * y) % p, (income_share + univ1_share + univ2_share) % p);
};

TEST(Multiply, boolean_multiply) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string revealer("income");
  MultiplyInfo<std::string, BooleanBeaverInfo> mult_info(
      &revealer, BooleanBeaverInfo());

  const Boolean_t x = randomModP<Boolean_t>(2);
  const Boolean_t y = randomModP<Boolean_t>(2);

  std::vector<Boolean_t> xs;
  xorSecretShare(3, x, xs);
  std::vector<Boolean_t> ys;
  xorSecretShare(3, y, ys);

  std::vector<BeaverTriple<Boolean_t>> beavers;
  mult_info.info.generate(3, 1, beavers);

  Boolean_t income_share = 0;
  Boolean_t univ1_share = 0;
  Boolean_t univ2_share = 0;

  test["income"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, Boolean_t, BooleanBeaverInfo>(
          xs[0],
          ys[0],
          &income_share,
          std::move(beavers[0]),
          &mult_info));

  test["univ1"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, Boolean_t, BooleanBeaverInfo>(
          xs[1],
          ys[1],
          &univ1_share,
          std::move(beavers[1]),
          &mult_info));

  test["univ2"] = std::unique_ptr<Fronctocol>(
      new Multiply<TEST_TYPES, Boolean_t, BooleanBeaverInfo>(
          xs[2],
          ys[2],
          &univ2_share,
          std::move(beavers[2]),
          &mult_info));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug("%u, %u, %u", income_share, univ1_share, univ2_share);
  EXPECT_EQ(
      (income_share + univ1_share + univ2_share) % 2, (x * y) % 2);
};

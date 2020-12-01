/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <deque>
#include <map>
#include <memory>

/* 3rd Party Headers */
#include <gtest/gtest.h>
#include <openssl/rand.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/BitwiseCompare.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Runnable.h>

/* Logging Configuration */
#include <ff/logging.h>

using Boolean_t = uint8_t;
using ArithmeticShare_t = uint32_t;

using namespace ff::mpc;

TEST(Compare, bitwise_compare_2parties) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const ArithmeticShare_t p = 97;
  const size_t numElements = 7;

  std::string const * const revealer = new std::string("income");
  std::string const * const dealer = new std::string("dealer");

  PrefixOrInfo<std::string> info(p, numElements, revealer);

  BeaverTriple<ArithmeticShare_t> value_beaver_income =
      BeaverTriple<ArithmeticShare_t>(2, 3, 6);
  BeaverTriple<ArithmeticShare_t> value_beaver_univ1 =
      BeaverTriple<ArithmeticShare_t>(0, 0, 0);

  TypeCastTriple value_tct_income = TypeCastTriple(96, 1, 1);
  TypeCastTriple value_tct_univ1 = TypeCastTriple(0, 0, 0);

  BeaverTriple<ArithmeticShare_t> value_beaver2_income =
      BeaverTriple<ArithmeticShare_t>(2, 3, 6);
  BeaverTriple<ArithmeticShare_t> value_beaver2_univ1 =
      BeaverTriple<ArithmeticShare_t>(0, 0, 0);

  TypeCastTriple value_tct2_income = TypeCastTriple(96, 1, 1);
  TypeCastTriple value_tct2_univ1 = TypeCastTriple(0, 0, 0);

  Boolean_t result_income = 0;
  Boolean_t result_univ1 = 0;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new PrefixOrRandomnessHouse<TEST_TYPES>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  std::vector<ArithmeticShare_t> vals_income = {
      11, 44, 23, 38, 19, 34, 82};
  std::vector<ArithmeticShare_t> vals_univ1 = {
      86, 54, 74, 60, 79, 63, 15};

  // 0 1 0 1 1 0 0

  std::vector<ArithmeticShare_t> b_in_the_clear = {0, 0, 1, 1, 1, 0, 0};

  // a XOR b = 0, 1, 1, 0, 0, 0, 0
  // prefixOR should equal 1, 1, 1, 0, 0, 0, 0
  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new PrefixOrRandomnessPatron<TEST_TYPES>(
                &info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {

          PrefixOrRandomness pref_randomness(
              static_cast<PrefixOrRandomnessPatron<TEST_TYPES> &>(f)
                  .prefixOrDispenser->get());
          BitwiseCompareRandomness randomness(
              std::move(pref_randomness),
              value_beaver_income,
              value_tct_income,
              value_beaver2_income,
              value_tct2_income);

          std::unique_ptr<BitwiseCompare<TEST_TYPES>> bc(
              new BitwiseCompare<TEST_TYPES>(
                  vals_income,
                  b_in_the_clear,
                  &info,
                  std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(bc), ps);

        } else {
          log_debug("finishing mpc income");
          result_income =
              static_cast<BitwiseCompare<TEST_TYPES> &>(f).outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t univ1_num_fronctocols_remaining = 2;
  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ1");
        std::unique_ptr<Fronctocol> patron(
            new PrefixOrRandomnessPatron<TEST_TYPES>(
                &info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (univ1_num_fronctocols_remaining == 1) {

          PrefixOrRandomness pref_randomness(
              static_cast<PrefixOrRandomnessPatron<TEST_TYPES> &>(f)
                  .prefixOrDispenser->get());
          BitwiseCompareRandomness randomness(
              std::move(pref_randomness),
              value_beaver_univ1,
              value_tct_univ1,
              value_beaver2_univ1,
              value_tct2_univ1);

          std::unique_ptr<BitwiseCompare<TEST_TYPES>> bc(
              new BitwiseCompare<TEST_TYPES>(
                  vals_univ1,
                  b_in_the_clear,
                  &info,
                  std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(bc), ps);

        } else {
          log_debug("finishing mpc univ1");
          result_univ1 =
              static_cast<BitwiseCompare<TEST_TYPES> &>(f).outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug("result_income, univ1 %u, %u", result_income, result_univ1);

  EXPECT_EQ(0, (result_income ^ result_univ1));
}

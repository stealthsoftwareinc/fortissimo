/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <map>
#include <memory>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/BitwiseCompare.h>
#include <mpc/Multiply.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Compare, bitwise_compare) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 97;
  const size_t numElements = 7;

  std::string const revealer("income");
  std::string const dealer("dealer");

  PrefixOrInfo<std::string, SmallNum> info(p, numElements, &revealer);

  BeaverTriple<SmallNum> value_beaver_income =
      BeaverTriple<SmallNum>(2, 3, 6);
  BeaverTriple<SmallNum> value_beaver_univ1 =
      BeaverTriple<SmallNum>(0, 0, 0);
  BeaverTriple<SmallNum> value_beaver_univ2 =
      BeaverTriple<SmallNum>(0, 0, 0);

  auto value_tct_income = TypeCastTriple<SmallNum>(96, 1, 1);
  auto value_tct_univ1 = TypeCastTriple<SmallNum>(0, 0, 0);
  auto value_tct_univ2 = TypeCastTriple<SmallNum>(0, 0, 0);

  BeaverTriple<SmallNum> value_beaver2_income =
      BeaverTriple<SmallNum>(2, 3, 6);
  BeaverTriple<SmallNum> value_beaver2_univ1 =
      BeaverTriple<SmallNum>(0, 0, 0);
  BeaverTriple<SmallNum> value_beaver2_univ2 =
      BeaverTriple<SmallNum>(0, 0, 0);

  auto value_tct2_income = TypeCastTriple<SmallNum>(96, 1, 1);
  auto value_tct2_univ1 = TypeCastTriple<SmallNum>(0, 0, 0);
  auto value_tct2_univ2 = TypeCastTriple<SmallNum>(0, 0, 0);

  Boolean_t result_income = 0;
  Boolean_t result_univ1 = 0;
  Boolean_t result_univ2 = 0;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new PrefixOrRandomnessHouse<TEST_TYPES, SmallNum>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  std::vector<SmallNum> vals_income = {11, 44, 23, 38, 19, 34, 82};
  std::vector<SmallNum> vals_univ1 = {42, 28, 52, 78, 80, 53, 7};
  std::vector<SmallNum> vals_univ2 = {44, 26, 22, 79, 96, 10, 8};

  // 0 1 0 1 1 0 0

  std::vector<SmallNum> b_in_the_clear = {0, 0, 1, 1, 1, 0, 0};

  // a XOR b = 0, 1, 1, 0, 0, 0, 0
  // prefixOR should equal 1, 1, 1, 0, 0, 0, 0
  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new PrefixOrRandomnessPatron<TEST_TYPES, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {

          PrefixOrRandomness<SmallNum> pref_randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());
          BitwiseCompareRandomness<SmallNum> randomness(
              std::move(pref_randomness),
              value_beaver_income,
              value_tct_income,
              value_beaver2_income,
              value_tct2_income);

          std::unique_ptr<BitwiseCompare<TEST_TYPES, SmallNum>> bc(
              new BitwiseCompare<TEST_TYPES, SmallNum>(
                  vals_income,
                  b_in_the_clear,
                  &info,
                  std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(bc), ps);

        } else {
          log_debug("finishing mpc income");
          result_income =
              static_cast<BitwiseCompare<TEST_TYPES, SmallNum> &>(f)
                  .outputShare;
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
            new PrefixOrRandomnessPatron<TEST_TYPES, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (univ1_num_fronctocols_remaining == 1) {

          PrefixOrRandomness<SmallNum> pref_randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());
          BitwiseCompareRandomness<SmallNum> randomness(
              std::move(pref_randomness),
              value_beaver_univ1,
              value_tct_univ1,
              value_beaver2_univ1,
              value_tct2_univ1);

          std::unique_ptr<BitwiseCompare<TEST_TYPES, SmallNum>> bc(
              new BitwiseCompare<TEST_TYPES, SmallNum>(
                  vals_univ1,
                  b_in_the_clear,
                  &info,
                  std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(bc), ps);

        } else {
          log_debug("finishing mpc univ1");
          result_univ1 =
              static_cast<BitwiseCompare<TEST_TYPES, SmallNum> &>(f)
                  .outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t univ2_num_fronctocols_remaining = 2;
  test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ2");
        std::unique_ptr<Fronctocol> patron(
            new PrefixOrRandomnessPatron<TEST_TYPES, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ2_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (univ2_num_fronctocols_remaining == 1) {

          PrefixOrRandomness<SmallNum> pref_randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());
          BitwiseCompareRandomness<SmallNum> randomness(
              std::move(pref_randomness),
              value_beaver_univ2,
              value_tct_univ2,
              value_beaver2_univ2,
              value_tct2_univ2);

          std::unique_ptr<BitwiseCompare<TEST_TYPES, SmallNum>> bc(
              new BitwiseCompare<TEST_TYPES, SmallNum>(
                  vals_univ2,
                  b_in_the_clear,
                  &info,
                  std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(bc), ps);

        } else {
          log_debug("finishing mpc univ2");
          result_univ2 =
              static_cast<BitwiseCompare<TEST_TYPES, SmallNum> &>(f)
                  .outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug(
      "result_income, univ1, univ2 %u, %u, %u",
      result_income,
      result_univ1,
      result_univ2);

  EXPECT_EQ(0, (result_income ^ result_univ1 ^ result_univ2));
}

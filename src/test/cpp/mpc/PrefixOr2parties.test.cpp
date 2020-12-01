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

#include <mpc/PrefixOr.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Runnable.h>
#include <mpc/UnboundedFaninOr.h>

/* Logging Configuration */
#include <ff/logging.h>

using Boolean_t = uint8_t;
using ArithmeticShare_t = uint32_t;

using namespace ff::mpc;

TEST(Compare, prefix_or_2parties) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const ArithmeticShare_t p = 97;
  const size_t numElements = 7;

  std::string const * const revealer = new std::string("income");
  std::string const * const dealer = new std::string("dealer");

  PrefixOrInfo<std::string> info(p, numElements, revealer);

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new PrefixOrRandomnessHouse<TEST_TYPES>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  std::vector<ArithmeticShare_t> vals_income = {
      11, 43, 23, 38, 18, 33, 82};
  std::vector<ArithmeticShare_t> vals_univ1 = {
      86, 54, 74, 60, 79, 65, 15};

  // 0 0 0 1 0 1 0

  std::vector<ArithmeticShare_t> results_income = {};
  std::vector<ArithmeticShare_t> results_univ1 = {};

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
          PrefixOrRandomness randomness(
              static_cast<PrefixOrRandomnessPatron<TEST_TYPES> &>(f)
                  .prefixOrDispenser->get());

          std::unique_ptr<PrefixOr<TEST_TYPES>> prefix(
              new PrefixOr<TEST_TYPES>(
                  vals_income, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(prefix), ps);
        } else {
          log_debug("finishing mpc income");
          results_income =
              static_cast<PrefixOr<TEST_TYPES> &>(f).orResults;
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
          PrefixOrRandomness randomness(
              static_cast<PrefixOrRandomnessPatron<TEST_TYPES> &>(f)
                  .prefixOrDispenser->get());

          std::unique_ptr<PrefixOr<TEST_TYPES>> prefix(
              new PrefixOr<TEST_TYPES>(
                  vals_univ1, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(prefix), ps);
        } else {
          log_debug("finishing mpc univ1");
          results_univ1 =
              static_cast<PrefixOr<TEST_TYPES> &>(f).orResults;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  for (size_t i = 0; i < 3; i++) {
    log_debug("i = %lu", i);
    EXPECT_EQ(0, (results_income[i] + results_univ1[i]) % p);
  }

  for (size_t i = 3; i < 7; i++) {
    log_debug("i = %lu", i);
    EXPECT_EQ(1, (results_income[i] + results_univ1[i]) % p);
  }
};

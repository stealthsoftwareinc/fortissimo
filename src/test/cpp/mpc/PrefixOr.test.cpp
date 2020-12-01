/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/PrefixOr.h>
#include <mpc/PrefixOrDealer.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Compare, prefix_or) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 97;
  const size_t numElements = 7;

  std::string const revealer("income");
  std::string const dealer("dealer");

  PrefixOrInfo<std::string, SmallNum> info(p, numElements, &revealer);

  test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new PrefixOrRandomnessHouse<TEST_TYPES, SmallNum>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  std::vector<SmallNum> vals_income = {11, 43, 23, 38, 18};
  std::vector<SmallNum> vals_univ1 = {42, 28, 52, 78, 80};
  std::vector<SmallNum> vals_univ2 = {44, 26, 22, 79, 96};

  // 0 0 0 1 0 1 0

  std::vector<SmallNum> results_income;
  std::vector<SmallNum> results_univ1;
  std::vector<SmallNum> results_univ2;

  size_t income_num_fronctocols_remaining = 2;
  test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
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
          PrefixOrRandomness<SmallNum> randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());

          std::unique_ptr<PrefixOr<TEST_TYPES, SmallNum>> prefix(
              new PrefixOr<TEST_TYPES, SmallNum>(
                  vals_income, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(prefix), ps);
        } else {
          log_debug("finishing mpc income");
          results_income =
              static_cast<PrefixOr<TEST_TYPES, SmallNum> &>(f)
                  .orResults;
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
          PrefixOrRandomness<SmallNum> randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());

          std::unique_ptr<PrefixOr<TEST_TYPES, SmallNum>> prefix(
              new PrefixOr<TEST_TYPES, SmallNum>(
                  vals_univ1, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(prefix), ps);
        } else {
          log_debug("finishing mpc univ1");
          results_univ1 =
              static_cast<PrefixOr<TEST_TYPES, SmallNum> &>(f)
                  .orResults;
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
          PrefixOrRandomness<SmallNum> randomness(
              static_cast<
                  PrefixOrRandomnessPatron<TEST_TYPES, SmallNum> &>(f)
                  .prefixOrDispenser->get());

          std::unique_ptr<PrefixOr<TEST_TYPES, SmallNum>> prefix(
              new PrefixOr<TEST_TYPES, SmallNum>(
                  vals_univ2, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(prefix), ps);
        } else {
          log_debug("finishing mpc univ2");
          results_univ2 =
              static_cast<PrefixOr<TEST_TYPES, SmallNum> &>(f)
                  .orResults;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  for (size_t i = 0; i < 3; i++) {
    log_debug("i = %lu", i);
    EXPECT_EQ(
        0,
        (results_income[i] + results_univ1[i] + results_univ2[i]) % p);
  }

  for (size_t i = 3; i < 5; i++) {
    log_debug("i = %lu", i);
    EXPECT_EQ(
        1,
        (results_income[i] + results_univ1[i] + results_univ2[i]) % p);
  }
};

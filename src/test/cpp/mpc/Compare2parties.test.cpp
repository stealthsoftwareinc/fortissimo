/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C++ Headers */
#include <deque>
#include <map>
#include <memory>

/* 3rd Party Headers */
#include <gtest/gtest.h>
#include <openssl/rand.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/Compare.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Runnable.h>

#include <mpc/CompareDealer.h>
#include <mpc/ComparePatron.h>

/* Logging Configuration */
#include <ff/logging.h>

using Boolean_t = uint8_t;
using ArithmeticShare_t = uint32_t;

using namespace ff::mpc;

TEST(Compare, compare_greater_than_2parties) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const ArithmeticShare_t p = 97;
  const size_t numElements = 7;
  const size_t lambda = 3;

  Boolean_t result_income = 0;
  Boolean_t result_univ1 = 0;

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;
  std::vector<std::vector<ArithmeticShare_t>> lagrangePolynomialSet =
      std::vector<std::vector<ArithmeticShare_t>>();

  size_t block_size = lambda;
  while (block_size < numElements) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<ArithmeticShare_t>(block_size)));
    log_debug("block_size: %lu", block_size);
    block_size += lambda;
  }
  log_debug("block_size: %lu", numElements);
  UnboundedFaninOrRandomnessNeeds.emplace_back(numElements);
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      p, static_cast<ArithmeticShare_t>(numElements)));

  block_size = 1;
  while ((block_size - 1) < lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<ArithmeticShare_t>(block_size)));
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  std::string * dealer = new std::string("dealer");
  std::string * revealer = new std::string("income");
  CompareInfo<std::string> * info = new CompareInfo<std::string>(
      p, p, numElements, lambda, lagrangePolynomialSet, revealer);

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new CompareRandomnessHouse<TEST_TYPES>(info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  ArithmeticShare_t x_income = 30;
  ArithmeticShare_t x_univ1 = 75;

  ArithmeticShare_t y_income = 1;
  ArithmeticShare_t y_univ1 = 6;

  // 105 equiv 8 > 7 mod 97
  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new CompareRandomnessPatron<TEST_TYPES>(info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {
          CompareRandomness randomness(
              static_cast<CompareRandomnessPatron<TEST_TYPES> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES>> c(
              new Compare<TEST_TYPES>(
                  x_income, y_income, info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc income");
          result_income =
              static_cast<Compare<TEST_TYPES> &>(f).outputShare;
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
            new CompareRandomnessPatron<TEST_TYPES>(info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        if (univ1_num_fronctocols_remaining == 1) {
          CompareRandomness randomness(
              static_cast<CompareRandomnessPatron<TEST_TYPES> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES>> c(
              new Compare<TEST_TYPES>(
                  x_univ1, y_univ1, info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ1");
          result_univ1 =
              static_cast<Compare<TEST_TYPES> &>(f).outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug("result_income, univ1 %u, %u", result_income, result_univ1);

  EXPECT_EQ(1, (result_income ^ result_univ1));

  delete info;
  delete dealer;
}

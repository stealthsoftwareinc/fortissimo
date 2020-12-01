/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C++ Headers */
#include <deque>
#include <map>
#include <memory>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>
#include <openssl/rand.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/Batch.h>
#include <mpc/Compare.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>

#include <mpc/CompareDealer.h>
#include <mpc/ComparePatron.h>

/* Logging Configuration */
#include <ff/logging.h>

using Boolean_t = uint8_t;
using ArithmeticShare_t = uint32_t;

using namespace ff::mpc;

TEST(Compare, batch_compare) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const ArithmeticShare_t p = 509;
  const size_t batchSize = 100;

  std::string * dealer = new std::string("dealer");
  std::string * revealer = new std::string("income");

  CompareInfo<std::string> const compareInfo(p, revealer);

  std::vector<Boolean_t> result_income(batchSize);
  std::vector<Boolean_t> result_univ1(batchSize);
  std::vector<Boolean_t> result_univ2(batchSize);

  std::vector<ArithmeticShare_t> x_income =
      std::vector<ArithmeticShare_t>();
  std::vector<ArithmeticShare_t> x_univ1 =
      std::vector<ArithmeticShare_t>();
  std::vector<ArithmeticShare_t> x_univ2 =
      std::vector<ArithmeticShare_t>();

  std::vector<ArithmeticShare_t> y_income =
      std::vector<ArithmeticShare_t>();
  std::vector<ArithmeticShare_t> y_univ1 =
      std::vector<ArithmeticShare_t>();
  std::vector<ArithmeticShare_t> y_univ2 =
      std::vector<ArithmeticShare_t>();

  for (size_t i = 0; i < batchSize; i++) {
    x_income.emplace_back(randomModP<ArithmeticShare_t>(p / 6));
    x_univ1.emplace_back(randomModP<ArithmeticShare_t>(p / 6));
    x_univ2.emplace_back(randomModP<ArithmeticShare_t>(p / 6));

    y_income.emplace_back(randomModP<ArithmeticShare_t>(p / 6));
    y_univ1.emplace_back(randomModP<ArithmeticShare_t>(p / 6));
    y_univ2.emplace_back(randomModP<ArithmeticShare_t>(p / 6));
  }

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new CompareRandomnessHouse<TEST_TYPES>(&compareInfo));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new CompareRandomnessPatron<TEST_TYPES>(
                &compareInfo, dealer, batchSize));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {
          std::unique_ptr<
              RandomnessDispenser<CompareRandomness, DoNotGenerateInfo>>
              compareDispenser = std::move(
                  static_cast<CompareRandomnessPatron<TEST_TYPES> &>(f)
                      .compareDispenser);

          std::unique_ptr<Batch<TEST_TYPES>> batch(
              new Batch<TEST_TYPES>());

          for (size_t i = 0; i < batchSize; i++) {
            batch->children.emplace_back(new Compare<TEST_TYPES>(
                x_income.at(i),
                y_income.at(i),
                &compareInfo,
                compareDispenser->get()));
          }

          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(batch), ps);
        } else {
          log_debug("finishing mpc income");
          Batch<TEST_TYPES> & batch =
              static_cast<Batch<TEST_TYPES> &>(f);
          for (size_t i = 0; i < batchSize; i++) {
            result_income.at(i) = static_cast<Compare<TEST_TYPES> &>(
                                      *batch.children.at(i))
                                      .outputShare;
          }
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
            new CompareRandomnessPatron<TEST_TYPES>(
                &compareInfo, dealer, batchSize));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        if (univ1_num_fronctocols_remaining == 1) {
          std::unique_ptr<
              RandomnessDispenser<CompareRandomness, DoNotGenerateInfo>>
              compareDispenser = std::move(
                  static_cast<CompareRandomnessPatron<TEST_TYPES> &>(f)
                      .compareDispenser);

          std::unique_ptr<Batch<TEST_TYPES>> batch(
              new Batch<TEST_TYPES>());

          for (size_t i = 0; i < batchSize; i++) {
            batch->children.emplace_back(new Compare<TEST_TYPES>(
                x_univ1.at(i),
                y_univ1.at(i),
                &compareInfo,
                compareDispenser->get()));
          }

          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(batch), ps);
        } else {
          log_debug("finishing mpc income");
          Batch<TEST_TYPES> & batch =
              static_cast<Batch<TEST_TYPES> &>(f);
          for (size_t i = 0; i < batchSize; i++) {
            result_univ1.at(i) = static_cast<Compare<TEST_TYPES> &>(
                                     *batch.children.at(i))
                                     .outputShare;
          }
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
            new CompareRandomnessPatron<TEST_TYPES>(
                &compareInfo, dealer, batchSize));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ2_num_fronctocols_remaining--;
        log_debug(
            "univ2_num_fronctocols_remaining %lu",
            univ2_num_fronctocols_remaining);
        if (univ2_num_fronctocols_remaining == 1) {
          std::unique_ptr<
              RandomnessDispenser<CompareRandomness, DoNotGenerateInfo>>
              compareDispenser = std::move(
                  static_cast<CompareRandomnessPatron<TEST_TYPES> &>(f)
                      .compareDispenser);

          std::unique_ptr<Batch<TEST_TYPES>> batch(
              new Batch<TEST_TYPES>());

          for (size_t i = 0; i < batchSize; i++) {
            batch->children.emplace_back(new Compare<TEST_TYPES>(
                x_univ2.at(i),
                y_univ2.at(i),
                &compareInfo,
                compareDispenser->get()));
          }

          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(batch), ps);
        } else {
          log_debug("finishing mpc income");
          Batch<TEST_TYPES> & batch =
              static_cast<Batch<TEST_TYPES> &>(f);
          for (size_t i = 0; i < batchSize; i++) {
            result_univ2.at(i) = static_cast<Compare<TEST_TYPES> &>(
                                     *batch.children.at(i))
                                     .outputShare;
          }
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  for (size_t i = 0; i < batchSize; i++) {
    if (((x_income.at(i) + x_univ1.at(i) + x_univ2.at(i)) % p) >
        ((y_income.at(i) + y_univ1.at(i) + y_univ2.at(i)) % p)) {
      EXPECT_EQ(
          1,
          (result_income.at(i) ^ result_univ1.at(i) ^
           result_univ2.at(i)));
    }
    if (((x_income.at(i) + x_univ1.at(i) + x_univ2.at(i)) % p) <
        ((y_income.at(i) + y_univ1.at(i) + y_univ2.at(i)) % p)) {
      EXPECT_EQ(
          0,
          (result_income.at(i) ^ result_univ1.at(i) ^
           result_univ2.at(i)));
    }
    if (((x_income.at(i) + x_univ1.at(i) + x_univ2.at(i)) % p) ==
        ((y_income.at(i) + y_univ1.at(i) + y_univ2.at(i)) % p)) {
      EXPECT_EQ(
          2,
          (result_income.at(i) ^ result_univ1.at(i) ^
           result_univ2.at(i)));
    }
  }

  delete revealer;
  delete dealer;
}

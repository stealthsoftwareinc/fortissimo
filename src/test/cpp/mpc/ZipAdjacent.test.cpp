/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C++ Headers */
#include <algorithm>
#include <cstdint>
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
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/ZipAdjacent.h>
#include <mpc/ZipAdjacentDealer.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

using testnum_t = uint64_t;

TEST(Zip, zip_adjacent) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const size_t batchSize = 102;
  const size_t numParties = 2;

  const size_t numArithmeticPayloadCols = 1;

  const testnum_t modulus = 65521;

  const size_t numXORPayloadCols = 0;

  std::string dealer("dealer");
  std::string revealer("income");

  std::vector<ff::mpc::ObservationList<testnum_t>> oLists;
  oLists.resize(numParties);
  // This only works if numParties == 2 (which is the only value it should take)

  std::vector<testnum_t> keys(batchSize, 0);
  std::vector<std::vector<testnum_t>> keysShare(
      2, std::vector<testnum_t>(batchSize, 0));

  for (size_t i = 0; i < batchSize; i++) {
    keys.at(i) = randomModP<testnum_t>(batchSize) + 1;
  }

  std::sort(keys.begin(), keys.end());

  for (size_t i = 0; i < batchSize; i++) {
    keysShare.at(0).at(i) = randomModP<testnum_t>(modulus);
    keysShare.at(1).at(i) =
        (keys.at(i) + modulus - keysShare.at(0).at(i)) % modulus;
  }

  std::vector<testnum_t> keyCounts(batchSize + 1, 0);

  for (size_t i = 0; i < numParties; i++) {
    oLists.at(i).numKeyCols = 2;
    oLists.at(i).numArithmeticPayloadCols = numArithmeticPayloadCols;
    oLists.at(i).numXORPayloadCols = numXORPayloadCols; // 0
    for (size_t j = 0; j < batchSize; j++) {
      ff::mpc::Observation<testnum_t> o;
      o.keyCols = {keysShare.at(i).at(j),
                   static_cast<testnum_t>(j % 2)};
      std::vector<testnum_t> payloadCols(numArithmeticPayloadCols, 0);
      payloadCols.at(0) = o.keyCols.at(0);

      o.arithmeticPayloadCols = payloadCols;
      oLists.at(i).elements.push_back(o);
    }
  }

  for (size_t i = 0; i < batchSize; i++) {
    keyCounts.at(static_cast<size_t>(keys.at(i)))++;
  }

  std::vector<ff::mpc::ObservationList<testnum_t>> outputLists;
  outputLists.resize(numParties);

  log_debug("prime: %u", modulus);
  log_debug("payloadLength %zu", numArithmeticPayloadCols);

  ZipAdjacentInfo<std::string, testnum_t, testnum_t> info(
      batchSize,
      numArithmeticPayloadCols,
      numXORPayloadCols,
      modulus,
      &revealer);

  test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new ff::mpc::ZipAdjacentRandomnessHouse<
                TEST_TYPES,
                testnum_t,
                testnum_t>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  size_t income1_num_fronctocols_remaining = 2;
  test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income1");
        std::unique_ptr<Fronctocol> patron(
            new ff::mpc::ZipAdjacentRandomnessPatron<
                TEST_TYPES,
                testnum_t,
                testnum_t>(&info, &dealer, 1));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income1_num_fronctocols_remaining--;
        log_debug(
            "income1_num_fronctocols_remaining %lu",
            income1_num_fronctocols_remaining);
        if (income1_num_fronctocols_remaining == 1) {
          std::unique_ptr<RandomnessDispenser<
              ZipAdjacentRandomness<testnum_t, testnum_t>,
              DoNotGenerateInfo>>
              zipDispenser =
                  std::move(static_cast<ZipAdjacentRandomnessPatron<
                                TEST_TYPES,
                                testnum_t,
                                testnum_t> &>(f)
                                .zipAdjacentDispenser);

          std::unique_ptr<ZipAdjacent<TEST_TYPES, testnum_t, testnum_t>>
              batchEval(
                  new ZipAdjacent<TEST_TYPES, testnum_t, testnum_t>(
                      oLists.at(0), &info, zipDispenser->get()));

          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(batchEval), ps);
        } else {
          log_debug("finishing mpc income1");
          ZipAdjacent<TEST_TYPES, testnum_t, testnum_t> & zip =
              static_cast<
                  ZipAdjacent<TEST_TYPES, testnum_t, testnum_t> &>(f);

          for (size_t i = 0; i < 2 * (batchSize - 1); i++) {
            log_debug(
                "share of key[%zu] = %u",
                i,
                (zip.zippedAdjacentPairs.elements.at(i)
                     .arithmeticPayloadCols.at(0)));
          }
          outputLists.at(0) = std::move(zip.zippedAdjacentPairs);

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
            new ff::mpc::ZipAdjacentRandomnessPatron<
                TEST_TYPES,
                testnum_t,
                testnum_t>(&info, &dealer, 1));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        log_debug(
            "univ1_num_fronctocols_remaining %lu",
            univ1_num_fronctocols_remaining);
        if (univ1_num_fronctocols_remaining == 1) {
          std::unique_ptr<RandomnessDispenser<
              ZipAdjacentRandomness<testnum_t, testnum_t>,
              DoNotGenerateInfo>>
              zipDispenser =
                  std::move(static_cast<ZipAdjacentRandomnessPatron<
                                TEST_TYPES,
                                testnum_t,
                                testnum_t> &>(f)
                                .zipAdjacentDispenser);

          std::unique_ptr<ZipAdjacent<TEST_TYPES, testnum_t, testnum_t>>
              batchEval(
                  new ZipAdjacent<TEST_TYPES, testnum_t, testnum_t>(
                      oLists.at(1), &info, zipDispenser->get()));

          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(batchEval), ps);
        } else {
          log_debug("finishing mpc univ1");
          ZipAdjacent<TEST_TYPES, testnum_t, testnum_t> & zip =
              static_cast<
                  ZipAdjacent<TEST_TYPES, testnum_t, testnum_t> &>(f);

          for (size_t i = 0; i < 2 * (batchSize - 1); i++) {
            log_debug(
                "share of key[%zu] = %u",
                i,
                (zip.zippedAdjacentPairs.elements.at(i)
                     .arithmeticPayloadCols.at(0)));
          }

          outputLists.at(1) = std::move(zip.zippedAdjacentPairs);
          log_debug(
              "outputLists.elements.size() == %zu",
              outputLists.at(1).elements.size());
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug("Done with tests");
  log_debug(
      "outputLists.elements.size() == %zu",
      outputLists.at(1).elements.size());

  for (size_t i = 0; i < batchSize - 1; i++) {
    log_debug(
        "key[%zu]=%u, shares %u %u",
        2 * i,
        (outputLists.at(0).elements.at(2 * i).arithmeticPayloadCols.at(
             0) +
         outputLists.at(1).elements.at(2 * i).arithmeticPayloadCols.at(
             0)) %
            modulus,
        outputLists.at(0).elements.at(2 * i).arithmeticPayloadCols.at(
            0),
        outputLists.at(1).elements.at(2 * i).arithmeticPayloadCols.at(
            0));

    log_debug(
        "key[%zu]=%u, shares %u %u",
        2 * i + 1,
        (outputLists.at(0)
             .elements.at(2 * i + 1)
             .arithmeticPayloadCols.at(0) +
         outputLists.at(1)
             .elements.at(2 * i + 1)
             .arithmeticPayloadCols.at(0)) %
            modulus,
        outputLists.at(0)
            .elements.at(2 * i + 1)
            .arithmeticPayloadCols.at(0),
        outputLists.at(1)
            .elements.at(2 * i + 1)
            .arithmeticPayloadCols.at(0));

    EXPECT_EQ(
        (outputLists.at(0).elements.at(2 * i).arithmeticPayloadCols.at(
             0) +
         outputLists.at(1).elements.at(2 * i).arithmeticPayloadCols.at(
             0)) %
            modulus,
        (outputLists.at(0)
             .elements.at(2 * i + 1)
             .arithmeticPayloadCols.at(0) +
         outputLists.at(1)
             .elements.at(2 * i + 1)
             .arithmeticPayloadCols.at(0)) %
            modulus);
  }

  std::vector<ArithmeticShare_t> testKeyCounts(batchSize + 1, 0);

  for (size_t i = 0; i < batchSize - 1; i++) {
    testKeyCounts.at(static_cast<size_t>(
        (outputLists.at(0).elements.at(2 * i).arithmeticPayloadCols.at(
             0) +
         outputLists.at(1).elements.at(2 * i).arithmeticPayloadCols.at(
             0)) %
        modulus))++;
  }

  for (size_t i = 1; i < batchSize; i++) {
    if (keyCounts.at(i) > 0) {
      keyCounts.at(i)--;
    }
    EXPECT_EQ(keyCounts.at(i), testKeyCounts.at(i));
  }
}

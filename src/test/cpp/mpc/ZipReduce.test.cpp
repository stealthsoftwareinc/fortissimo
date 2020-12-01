/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C++ Headers */
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/AbstractZipReduceFactory.h>
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/ZipAdjacent.h>
#include <mpc/ZipAdjacentDealer.h>
#include <mpc/ZipReduce.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

using testnum_t = uint64_t;

class DoublePayloadCompute
    : public ZipReduceFronctocol<TEST_TYPES, testnum_t> {
public:
  std::string name() override {
    return std::string("Double Payload");
  }
  /*
   * Takes in two Arithmetic vectors (from two observations)
   * and doubles both shares
   */
  DoublePayloadCompute(testnum_t left, testnum_t right);

  void init() override;
  void handleReceive(IncomingMessage & imsg) override;
  void handleComplete(Fronctocol & f) override;
  void handlePromise(Fronctocol & f) override;

private:
  testnum_t left;
  testnum_t right;
};

class DoublePayloadComputeFactory
    : public ff::mpc::ZipReduceFactory<TEST_TYPES, testnum_t> {

public:
  DoublePayloadComputeFactory() {
  }

  std::unique_ptr<ff::mpc::ZipReduceFronctocol<TEST_TYPES, testnum_t>>
  generate(
      std::unique_ptr<ff::mpc::Observation<testnum_t>> o1,
      std::unique_ptr<ff::mpc::Observation<testnum_t>> o2) override;
};

DoublePayloadCompute::DoublePayloadCompute(
    testnum_t left, testnum_t right) :
    left(left), right(right) {
  log_debug("DoublePayloadCompute constructor");
}

void DoublePayloadCompute::init() {

  this->output.arithmeticPayloadCols = {2 * left, 2 * right};
  this->complete();
}

void DoublePayloadCompute::handlePromise(Fronctocol &) {
  log_error("Unexpected handlePromise in DoublePayloadCompute");
}

void DoublePayloadCompute::handleReceive(IncomingMessage &) {
  log_error("Unexpected handleReceive in DoublePayloadCompute");
}

void DoublePayloadCompute::handleComplete(Fronctocol &) {
  log_error("Unexpected handleComplete in DoublePayloadCompute");
}

std::unique_ptr<ff::mpc::ZipReduceFronctocol<TEST_TYPES, testnum_t>>
DoublePayloadComputeFactory::generate(
    std::unique_ptr<ff::mpc::Observation<testnum_t>> o1,
    std::unique_ptr<ff::mpc::Observation<testnum_t>> o2) {
  log_debug("Calling generate");
  log_debug(
      "vals %u and %u",
      o1->arithmeticPayloadCols.at(0),
      o2->arithmeticPayloadCols.at(0));
  testnum_t left = o1->arithmeticPayloadCols.at(0);
  testnum_t right = o2->arithmeticPayloadCols.at(0);
  std::unique_ptr<ff::mpc::ZipReduceFronctocol<TEST_TYPES, testnum_t>>
      eval(new DoublePayloadCompute(left, right));
  return eval;
}

TEST(Zip, zip_reduce) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const size_t batchSize =
      102; // NOTE: We require batchSize * 2 < modulus
  const size_t numParties = 2;

  const size_t numArithmeticPayloadCols = 1;

  const testnum_t modulus = 65521;

  const size_t numXORPayloadCols = 0;

  std::string dealer("dealer");
  std::string revealer("income");

  std::vector<ff::mpc::ObservationList<testnum_t>> oLists;
  oLists.resize(numParties);
  // This only works if numParties == 2 (which is the only value it should take)

  DoublePayloadComputeFactory factory1;

  DoublePayloadComputeFactory factory2;

  std::vector<testnum_t> keys(batchSize, 0);
  std::vector<std::vector<testnum_t>> keysShare(
      2, std::vector<testnum_t>(batchSize, 0));

  for (size_t i = 0; i < batchSize; i++) {
    keys.at(i) = 1 + randomModP<testnum_t>(batchSize);
    log_debug("keys.at[%zu]=%u", i, keys.at(i));
  }

  std::sort(keys.begin(), keys.end());

  for (size_t i = 0; i < batchSize; i++) {
    log_debug("keys.at[%zu]=%u", i, keys.at(i));
  }

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
    log_debug(
        "reconstructed keys.at[%zu]=%u",
        i,
        (keysShare.at(0).at(i) + keysShare.at(1).at(i)) % modulus);
  }

  for (size_t i = 0; i < batchSize; i++) {
    keyCounts.at(keys.at(i))++;
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

  size_t income1_num_fronctocols_remaining = 3;
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
        if (income1_num_fronctocols_remaining == 2) {
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
        } else if (income1_num_fronctocols_remaining == 1) {

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

          std::unique_ptr<ZipReduce<TEST_TYPES, testnum_t>> reduce(
              new ZipReduce<TEST_TYPES, testnum_t>(
                  std::move(zip.zippedAdjacentPairs), factory1));

          log_debug(
              "val %u",
              zip.zippedAdjacentPairs.elements.at(0)
                  .arithmeticPayloadCols.at(0));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(reduce), ps);
        } else {
          log_debug("finishing mpc income1");

          ZipReduce<TEST_TYPES, testnum_t> & zip =
              static_cast<ZipReduce<TEST_TYPES, testnum_t> &>(f);

          outputLists.at(0) = std::move(zip.outputList);

          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t univ1_num_fronctocols_remaining = 3;
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
        if (univ1_num_fronctocols_remaining == 2) {
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
        } else if (univ1_num_fronctocols_remaining == 1) {

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

          std::unique_ptr<ZipReduce<TEST_TYPES, testnum_t>> reduce(
              new ZipReduce<TEST_TYPES, testnum_t>(
                  std::move(zip.zippedAdjacentPairs), factory2));

          log_debug(
              "val %u",
              zip.zippedAdjacentPairs.elements.at(0)
                  .arithmeticPayloadCols.at(0));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(reduce), ps);
        } else {
          log_debug("finishing mpc univ1");

          ZipReduce<TEST_TYPES, testnum_t> & zip =
              static_cast<ZipReduce<TEST_TYPES, testnum_t> &>(f);

          for (size_t i = 0; i < (batchSize - 1); i++) {
            log_debug(
                "val[%zu] = %u",
                i,
                zip.outputList.elements.at(i).arithmeticPayloadCols.at(
                    0));
          }

          outputLists.at(1) = std::move(zip.outputList);

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
        "key[%zu]=%u, shares: %u, %u",
        i,
        (outputLists.at(0).elements.at(i).arithmeticPayloadCols.at(0) +
         outputLists.at(1).elements.at(i).arithmeticPayloadCols.at(0)) %
            modulus,
        outputLists.at(0).elements.at(i).arithmeticPayloadCols.at(0),
        outputLists.at(1).elements.at(i).arithmeticPayloadCols.at(0));

    EXPECT_EQ(
        ((outputLists.at(0).elements.at(i).arithmeticPayloadCols.at(0) +
          outputLists.at(1).elements.at(i).arithmeticPayloadCols.at(
              0)) %
         modulus) %
            2,
        0);
  }

  std::vector<testnum_t> testKeyCounts(batchSize + 1, 0);

  for (size_t i = 0; i < batchSize - 1; i++) {
    testKeyCounts.at(
        ((outputLists.at(0).elements.at(i).arithmeticPayloadCols.at(0) +
          outputLists.at(1).elements.at(i).arithmeticPayloadCols.at(
              0)) %
         modulus) /
        2)++;
  }

  for (size_t i = 1; i < batchSize; i++) {
    if (keyCounts.at(i) > 0) {
      keyCounts.at(i)--;
    }
    EXPECT_EQ(keyCounts.at(i), testKeyCounts.at(i));
  }
}

/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C++ Headers */
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
#include <mpc/Compare.h>
#include <mpc/ObservationList.h>
#include <mpc/Quicksort.h>
#include <mpc/QuicksortDealer.h>
#include <mpc/Randomness.h>
#include <mpc/lagrange.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using testnum_t = uint64_t;

using WaksmanBits_t = std::vector<testnum_t>;

using namespace ff::mpc;

TEST(SISO_Sort, quicksort) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string dealerName("dealer");
  std::string revealName("income");

  const size_t NUM_ARITHMETIC_PAYLOADS = 1;
  const size_t NUM_XOR_PAYLOADS = 8;
  //const size_t NUM_PAYLOADS = 1;
  const size_t NUM_KEYS = 4;
  const size_t NUM_PARTIES = 3;
  const testnum_t modulus = 299099; // a 19 bit prime
  //const testnum_t modulus = 31; // a 5 bit prime
  const testnum_t small_modulus = 47;
  const size_t bitsPerPrime = 19;
  //const size_t bitsPerPrime = 5;
  const size_t sqrt_ell = 5;
  //const size_t sqrt_ell = 3;
  const size_t MAX_LIST_SIZE = 64;
  const size_t KEY_WITH_VAL = 1;
  std::vector<testnum_t> arithmeticPayloads;
  std::vector<testnum_t> keys;
  std::vector<Boolean_t> XORPayloads;

  std::vector<testnum_t> permutation =
      std::vector<testnum_t>(MAX_LIST_SIZE);
  for (size_t i = 0; i < MAX_LIST_SIZE; i++) {
    permutation[i] = static_cast<testnum_t>(i);
  }
  for (size_t i = 0; i < MAX_LIST_SIZE - 1; i++) {
    size_t j = randomModP<size_t>(MAX_LIST_SIZE - i);
    testnum_t temp = permutation[j];
    permutation[j] = permutation[i];
    permutation[i] = temp;
  }

  std::vector<ObservationList<testnum_t> *> oLists =
      std::vector<ObservationList<testnum_t> *>(NUM_PARTIES);
  for (size_t i = 0; i < NUM_PARTIES; i++) {
    oLists.at(i) = new ObservationList<testnum_t>();
    std::vector<Observation<testnum_t>> o_elements(0);
    for (size_t j = 0; j < MAX_LIST_SIZE; j++) {
      arithmeticPayloads =
          std::vector<testnum_t>(NUM_ARITHMETIC_PAYLOADS, 0);
      keys = std::vector<testnum_t>(NUM_KEYS, 0);
      XORPayloads = std::vector<Boolean_t>(NUM_XOR_PAYLOADS, 0x00);

      keys.at(KEY_WITH_VAL) =
          static_cast<testnum_t>(i * MAX_LIST_SIZE + permutation[j]);
      keys.at(KEY_WITH_VAL) %= modulus;

      Observation<testnum_t> o;
      o.keyCols = keys;
      o.arithmeticPayloadCols = arithmeticPayloads;
      o.XORPayloadCols = XORPayloads;

      o_elements.push_back(o);
    }
    oLists.at(i)->elements = o_elements;
    oLists.at(i)->numKeyCols = NUM_KEYS;
    oLists.at(i)->numArithmeticPayloadCols = NUM_ARITHMETIC_PAYLOADS;
    oLists.at(i)->numXORPayloadCols = NUM_XOR_PAYLOADS;
  }

  for (size_t i = 0; i < oLists.at(0)->elements.size(); i++) {
    testnum_t test_val = oLists.at(0)->elements.at(i).keyCols.at(0) +
        oLists.at(1)->elements.at(i).keyCols.at(0) +
        oLists.at(2)->elements.at(i).keyCols.at(0);
    test_val %= modulus;
    log_debug("%u", test_val);
  }

  for (size_t i = 0; i < oLists.at(0)->elements.size(); i++) {
    testnum_t test_val =
        oLists.at(0)->elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(0)->elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(0)->elements.at(i).keyCols.at(KEY_WITH_VAL);
    test_val %= modulus;
    log_debug("%u", test_val);
  }

  std::vector<std::vector<testnum_t>> lagrangePolynomialSet =
      std::vector<std::vector<testnum_t>>();
  size_t block_size = sqrt_ell;
  while (block_size < bitsPerPrime) {
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        small_modulus, static_cast<testnum_t>(block_size)));
    //log_debug("block_size: %lu", block_size);
    block_size += sqrt_ell;
  }
  //log_debug("block_size: %lu", this->compareInfo.ell);
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      small_modulus, static_cast<testnum_t>(bitsPerPrime)));

  block_size = 1;
  while ((block_size - 1) < sqrt_ell) {
    //log_debug("block_size: %lu", block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        small_modulus, static_cast<testnum_t>(block_size)));
    block_size++;
  }

  CompareInfo<std::string, testnum_t, testnum_t> compareInfo =
      CompareInfo<std::string, testnum_t, testnum_t>(
          modulus,
          small_modulus,
          bitsPerPrime,
          sqrt_ell,
          lagrangePolynomialSet,
          &revealName);

  ObservationList<testnum_t> income_output =
      ObservationList<testnum_t>();
  ObservationList<testnum_t> univ1_output =
      ObservationList<testnum_t>();
  ObservationList<testnum_t> univ2_output =
      ObservationList<testnum_t>();

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");
        std::unique_ptr<
            QuicksortRandomnessHouse<TEST_TYPES, testnum_t, testnum_t>>
            rd(new QuicksortRandomnessHouse<
                TEST_TYPES,
                testnum_t,
                testnum_t>(compareInfo, MAX_LIST_SIZE, NUM_KEYS));
        self->invoke(std::move(rd), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) mutable {
        log_debug("Dealer done");
        self->complete();
      }));

  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> income_sort(
            new QuickSortFronctocol<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(0), compareInfo, &revealName, &dealerName));
        self->invoke(std::move(income_sort), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc income");
        income_output = *static_cast<QuickSortFronctocol<
            TEST_TYPES,
            testnum_t,
            testnum_t> &>(f)
                             .inputList;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ1");
        std::unique_ptr<Fronctocol> univ1_sort(
            new QuickSortFronctocol<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(1), compareInfo, &revealName, &dealerName));
        self->invoke(std::move(univ1_sort), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ1");
        univ1_output = *static_cast<QuickSortFronctocol<
            TEST_TYPES,
            testnum_t,
            testnum_t> &>(f)
                            .inputList;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ2");
        std::unique_ptr<Fronctocol> univ2_sort(
            new QuickSortFronctocol<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(2), compareInfo, &revealName, &dealerName));
        self->invoke(std::move(univ2_sort), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ2");
        univ2_output = *static_cast<QuickSortFronctocol<
            TEST_TYPES,
            testnum_t,
            testnum_t> &>(f)
                            .inputList;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  testnum_t test_val = 0;

  for (size_t i = 0; i < income_output.elements.size(); i++) {
    test_val = oLists.at(0)->elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(1)->elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(2)->elements.at(i).keyCols.at(KEY_WITH_VAL);
    test_val %= modulus;
    test_val += static_cast<testnum_t>(3 * modulus - 3 * MAX_LIST_SIZE);
    test_val %= modulus;
    test_val /= 3;
    EXPECT_EQ(i, test_val);
  }
  delete oLists.at(0);
  delete oLists.at(1);
  delete oLists.at(2);
}

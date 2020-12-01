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
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/SISOSort.h>
#include <mpc/SISOSortDealer.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using testnum_t = uint64_t;

using WaksmanBits_t = std::vector<testnum_t>;

using namespace ff::mpc;

TEST(SISO_Sort, SISO_sort) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string * dealerName = new std::string("dealer");

  const size_t NUM_ARITHMETIC_PAYLOADS = 1;
  const size_t NUM_XOR_PAYLOADS = 8;
  //const size_t NUM_PAYLOADS = 1;
  const size_t NUM_KEYS = 4;
  const size_t NUM_PARTIES = 3;
  const testnum_t modulus = 299099; // a 19 bit prime
  const testnum_t key_modulus = 2003;
  const size_t LIST_SIZE = 100;
  const size_t KEY_WITH_VAL = 1;
  const size_t SECOND_KEY_WITH_VAL = 2;
  std::vector<testnum_t> arithmeticPayloads;
  std::vector<testnum_t> keys;
  std::vector<Boolean_t> XORPayloads;

  std::vector<ObservationList<testnum_t>> oLists =
      std::vector<ObservationList<testnum_t>>(NUM_PARTIES);
  for (size_t i = 0; i < NUM_PARTIES; i++) {
    oLists.at(i) = ObservationList<testnum_t>();
    std::vector<Observation<testnum_t>> o_elements(0);
    for (size_t j = 0; j < LIST_SIZE; j++) {
      arithmeticPayloads =
          std::vector<testnum_t>(NUM_ARITHMETIC_PAYLOADS, 0);
      keys = std::vector<testnum_t>(NUM_KEYS, 0);
      XORPayloads = std::vector<Boolean_t>(NUM_XOR_PAYLOADS, 0x00);

      keys.at(KEY_WITH_VAL) =
          static_cast<testnum_t>(i * LIST_SIZE + j / 10);
      keys.at(KEY_WITH_VAL) %= modulus;
      keys.at(SECOND_KEY_WITH_VAL) =
          static_cast<testnum_t>(i * LIST_SIZE + (j % 10));
      keys.at(SECOND_KEY_WITH_VAL) %= modulus;

      Observation<testnum_t> o;
      o.keyCols = keys;
      o.arithmeticPayloadCols = arithmeticPayloads;
      o.XORPayloadCols = XORPayloads;

      o_elements.push_back(o);
    }
    oLists.at(i).elements = o_elements;
    oLists.at(i).numKeyCols = NUM_KEYS;
    oLists.at(i).numArithmeticPayloadCols = NUM_ARITHMETIC_PAYLOADS;
    oLists.at(i).numXORPayloadCols = NUM_XOR_PAYLOADS;
  }

  for (size_t i = 0; i < oLists.at(0).elements.size(); i++) {
    testnum_t test_val = oLists.at(0).elements.at(i).keyCols.at(0) +
        oLists.at(1).elements.at(i).keyCols.at(0) +
        oLists.at(2).elements.at(i).keyCols.at(0);
    test_val %= modulus;
    log_debug("%s", dec(test_val).c_str());
  }

  for (size_t i = 0; i < oLists.at(0).elements.size(); i++) {
    testnum_t test_val =
        oLists.at(0).elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(1).elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(2).elements.at(i).keyCols.at(KEY_WITH_VAL);
    test_val %= modulus;
    log_debug("%s", dec(test_val).c_str());
  }

  std::string * revealer = new std::string("income");

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");
        std::unique_ptr<
            SISOSortRandomnessHouse<TEST_TYPES, testnum_t, testnum_t>>
            rd(new SISOSortRandomnessHouse<
                TEST_TYPES,
                testnum_t,
                testnum_t>(
                LIST_SIZE, modulus, key_modulus, revealer, dealerName));
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
            new SISOSort<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(0),
                modulus,
                key_modulus,
                revealer,
                dealerName));
        self->invoke(std::move(income_sort), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) {
        log_debug("finishing mpc income");
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ1");
        std::unique_ptr<Fronctocol> univ1_sort(
            new SISOSort<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(1),
                modulus,
                key_modulus,
                revealer,
                dealerName));
        self->invoke(std::move(univ1_sort), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) {
        log_debug("finishing mpc univ1");
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ2");
        std::unique_ptr<Fronctocol> univ2_sort(
            new SISOSort<TEST_TYPES, testnum_t, testnum_t>(
                oLists.at(2),
                modulus,
                key_modulus,
                revealer,
                dealerName));
        self->invoke(std::move(univ2_sort), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) {
        log_debug("finishing mpc univ2");
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  testnum_t test_val = 0;
  testnum_t second_test_val = 0;

  log_debug("finishing tests");

  std::vector<testnum_t> output;

  for (size_t i = 0; i < oLists.at(0).elements.size(); i++) {
    test_val = oLists.at(0).elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(1).elements.at(i).keyCols.at(KEY_WITH_VAL) +
        oLists.at(2).elements.at(i).keyCols.at(KEY_WITH_VAL);
    test_val %= key_modulus;
    test_val -= static_cast<testnum_t>(3 * LIST_SIZE);
    test_val /= 3;

    second_test_val =
        oLists.at(0).elements.at(i).keyCols.at(SECOND_KEY_WITH_VAL) +
        oLists.at(1).elements.at(i).keyCols.at(SECOND_KEY_WITH_VAL) +
        oLists.at(2).elements.at(i).keyCols.at(SECOND_KEY_WITH_VAL);
    second_test_val %= key_modulus;
    second_test_val -= static_cast<testnum_t>(3 * LIST_SIZE);
    second_test_val /= 3;
    log_debug(
        "First val: %s, second val:%s",
        dec(test_val).c_str(),
        dec(second_test_val).c_str());
    EXPECT_EQ(i, 10 * test_val + second_test_val);
  }
}

template<typename Number_T>
bool observationCmp(
    Observation<Number_T> const & l, Observation<Number_T> const & r) {
  log_assert(l.keyCols.size() == r.keyCols.size());

  for (size_t i = 0; i < l.keyCols.size(); i++) {
    if (l.keyCols[i] == r.keyCols[i]) {
      continue;
    } else {
      return l.keyCols[i] < r.keyCols[i];
    }
  }
  return false;
}

template<typename Number_T>
bool observationEq(
    Observation<Number_T> const & l, Observation<Number_T> const & r) {
  log_assert(l.keyCols.size() == r.keyCols.size());

  for (size_t i = 0; i < l.keyCols.size(); i++) {
    if (l.keyCols[i] != r.keyCols[i]) {
      return false;
    }
  }
  return true;
}

const std::vector<std::string> PARTY_NAMES = {
    {"alice", "bob", "chelsea", "david", "farrah", "gabby", "eve"}};

template<typename Large_T, typename Small_T>
void testSISOParams(
    size_t const n_parties,
    size_t const n_records,
    size_t const n_keys,
    size_t const n_arith,
    size_t const n_xor,
    Large_T const prime) {
  LOG_ORGANIZATION = std::string("test");

  ObservationList<Large_T> og;
  og.numKeyCols = n_keys;
  og.numArithmeticPayloadCols = n_arith;
  og.numXORPayloadCols = n_xor;

  // Generate the original observation list
  og.elements.reserve(n_records);
  for (size_t i = 0; i < n_records; i++) {
    og.elements.emplace_back();
    // generate random original key cols
    for (size_t j = 0; j < n_keys; j++) {
      og.elements[i].keyCols.emplace_back(
          randomModP<Large_T>(prime / 3));
    }

    // Assure there are no duplicate keys.
    for (size_t j = 0; j < i; j++) {
      if (observationEq(og.elements[i], og.elements[j])) {
        i--;
        continue;
      }
    }

    // generate random original arithmetic payload cols
    for (size_t j = 0; j < n_arith; j++) {
      og.elements[i].arithmeticPayloadCols.emplace_back(
          randomModP<Large_T>(prime));
    }

    // generate random original XOR payload cols
    for (size_t j = 0; j < n_xor; j++) {
      og.elements[i].XORPayloadCols.emplace_back(
          randomModP<Boolean_t>(2));
    }
  }

  // Secret share the original observation list.
  std::vector<ObservationList<Large_T>> input_shares(n_parties);
  for (size_t i = 0; i < n_parties; i++) {
    input_shares[i].numKeyCols = n_keys;
    input_shares[i].numArithmeticPayloadCols = n_arith;
    input_shares[i].numXORPayloadCols = n_xor;

    input_shares[i].elements.resize(n_records);
  }

  for (size_t i = 0; i < n_records; i++) {
    // secret share the keys.
    for (size_t j = 0; j < n_keys; j++) {
      std::vector<Large_T> key_shares;
      arithmeticSecretShare(
          n_parties, prime, og.elements[i].keyCols[j], key_shares);

      for (size_t k = 0; k < n_parties; k++) {
        input_shares[k].elements[i].keyCols.push_back(key_shares[k]);
      }
    }

    // secret share the arithmetic payloads
    for (size_t j = 0; j < n_arith; j++) {
      std::vector<Large_T> arith_shares;
      arithmeticSecretShare(
          n_parties,
          prime,
          og.elements[i].arithmeticPayloadCols[j],
          arith_shares);

      for (size_t k = 0; k < n_parties; k++) {
        input_shares[k].elements[i].arithmeticPayloadCols.push_back(
            arith_shares[k]);
      }
    }

    // secret share the XOR payloads
    for (size_t j = 0; j < n_xor; j++) {
      std::vector<Boolean_t> xor_shares;
      xorSecretShare(
          n_parties, og.elements[i].XORPayloadCols[j], xor_shares);

      for (size_t k = 0; k < n_parties; k++) {
        input_shares[k].elements[i].XORPayloadCols.push_back(
            xor_shares[k]);
      }
    }
  }

  // Generate the fronctocols
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  std::string dealer_id("dealer");
  std::string revealer_id(PARTY_NAMES[0]);

  // Add the dealer
  test[dealer_id] = std::unique_ptr<
      SISOSortRandomnessHouse<TEST_TYPES, Large_T, Small_T>>(
      new SISOSortRandomnessHouse<TEST_TYPES, Large_T, Small_T>(
          n_records, prime, &revealer_id, &dealer_id));

  // Add the dataowners
  ASSERT_TRUE(n_parties <= PARTY_NAMES.size());
  for (size_t i = 0; i < n_parties; i++) {
    test[PARTY_NAMES[i]] = std::unique_ptr<Fronctocol>(
        new SISOSort<TEST_TYPES, Large_T, Small_T>(
            input_shares[i], prime, &revealer_id, &dealer_id));
  }

  log_info(
      "Running SISOSort test with %zu parties, %zu records",
      n_parties,
      n_records);
  log_info(
      "    %zu keys, %zu arithmetic columns, and %zu XOR columns per "
      "record",
      n_keys,
      n_arith,
      n_xor);
  log_info("    Prime is %s", ff::mpc::dec(prime).c_str());
  LogTimer lt = log_time_start("SISO Sorting");

  EXPECT_TRUE(runTests(test));

  LOG_ORGANIZATION = std::string("test");
  log_time_update(lt, "SISO Sort complete");

  // Reconstruct the result
  ObservationList<Large_T> reconst;
  reconst.numKeyCols = n_keys;
  reconst.numArithmeticPayloadCols = n_arith;
  reconst.numXORPayloadCols = n_xor;
  reconst.elements.resize(n_records);

  for (size_t i = 0; i < n_records; i++) {
    // Reconstruct keys.
    for (size_t j = 0; j < n_keys; j++) {
      Large_T key = input_shares[0].elements[i].keyCols[j];
      for (size_t k = 1; k < n_parties; k++) {
        key =
            modAdd(key, input_shares[k].elements[i].keyCols[j], prime);
      }
      reconst.elements[i].keyCols.push_back(key);
    }

    // Reconstruct Arithmetic payload.
    for (size_t j = 0; j < n_arith; j++) {
      Large_T arith =
          input_shares[0].elements[i].arithmeticPayloadCols[j];
      for (size_t k = 1; k < n_parties; k++) {
        arith = modAdd(
            arith,
            input_shares[k].elements[i].arithmeticPayloadCols[j],
            prime);
      }
      reconst.elements[i].arithmeticPayloadCols.push_back(arith);
    }

    // Reconstruct XOR payload.
    for (size_t j = 0; j < n_xor; j++) {
      Boolean_t xor_share =
          input_shares[0].elements[i].XORPayloadCols[j];
      for (size_t k = 1; k < n_parties; k++) {
        xor_share =
            xor_share ^ input_shares[k].elements[i].XORPayloadCols[j];
      }
      reconst.elements[i].XORPayloadCols.push_back(xor_share);
    }
  }

  // Sort the original ObservationList.
  std::sort(
      og.elements.begin(), og.elements.end(), observationCmp<Large_T>);

  // Compare the sorted original to the sorted reconstructed.
  for (size_t i = 0; i < n_records; i++) {
    for (size_t j = 0; j < n_keys; j++) {
      EXPECT_EQ(
          og.elements.at(i).keyCols.at(j),
          reconst.elements.at(i).keyCols.at(j));
    }

    for (size_t j = 0; j < n_arith; j++) {
      EXPECT_EQ(
          og.elements.at(i).arithmeticPayloadCols.at(j),
          reconst.elements.at(i).arithmeticPayloadCols.at(j));
    }

    for (size_t j = 0; j < n_xor; j++) {
      EXPECT_EQ(
          og.elements.at(i).XORPayloadCols.at(j),
          reconst.elements.at(i).XORPayloadCols.at(j));
    }
  }
}

TEST(SISO_Sort, SISOSort_with_random_parameters_uint32_uint32) {
  const uint32_t prime = (1U << 31) - 1; // mersenne prime
  for (size_t i = 0; i < 5; i++) {
    size_t n_parties = 2 + randomModP<size_t>(PARTY_NAMES.size() - 2);
    size_t n_records = 3 + randomModP<size_t>(18UL);
    size_t n_keys = 1 + randomModP<size_t>(4UL);
    size_t n_arith = randomModP<size_t>(7UL);
    size_t n_xor = randomModP<size_t>(7UL);

    testSISOParams<uint32_t, uint32_t>(
        n_parties, n_records, n_keys, n_arith, n_xor, prime);
    log_info("==========");
  }
}

TEST(SISO_Sort, SISOSort_with_random_parameters_uint64_uint64) {
  const uint64_t prime = (1ULL << 61) - 1; // mersenne prime
  for (size_t i = 0; i < 5; i++) {
    size_t n_parties = 2 + randomModP<size_t>(PARTY_NAMES.size() - 2);
    size_t n_records = 3 + randomModP<size_t>(18UL);
    size_t n_keys = 1 + randomModP<size_t>(4UL);
    size_t n_arith = randomModP<size_t>(7UL);
    size_t n_xor = randomModP<size_t>(7UL);

    testSISOParams<uint64_t, uint64_t>(
        n_parties, n_records, n_keys, n_arith, n_xor, prime);
    log_info("==========");
  }
}

TEST(SISO_Sort, SISOSort_with_random_parameters_largenum_uint32) {
  const LargeNum prime = (LargeNum(1) << 89) - 1; // mersenne prime
  for (size_t i = 0; i < 5; i++) {
    size_t n_parties = 2 + randomModP<size_t>(PARTY_NAMES.size() - 2);
    size_t n_records = 3 + randomModP<size_t>(18UL);
    size_t n_keys = 1 + randomModP<size_t>(4UL);
    size_t n_arith = randomModP<size_t>(7UL);
    size_t n_xor = randomModP<size_t>(7UL);

    testSISOParams<LargeNum, uint32_t>(
        n_parties, n_records, n_keys, n_arith, n_xor, prime);
    log_info("==========");
  }
}

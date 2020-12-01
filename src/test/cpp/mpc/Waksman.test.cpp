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
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <ff/Promise.h>
#include <mpc/Multiply.h>
#include <mpc/ObservationList.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>
#include <mpc/Waksman.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(SISO_Sort, waksman_shuffle) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  BooleanBeaverInfo booleanInfo = BooleanBeaverInfo();

  const size_t NUM_ARITHMETIC_PAYLOADS = 1;
  const size_t NUM_XOR_PAYLOADS = 8;
  const size_t NUM_KEYS = 4;
  const size_t NUM_PARTIES = 3;
  const uint64_t modulus = (1ULL << 61) - 1; // mersenne prime
  const uint64_t keyModulus = 2003;
  const size_t MAX_LIST_SIZE = 1000;
  const size_t DEPTH_OF_WAKSMAN =
      10; // s.t. 2^DEPTH_OF_WAKSMAN >= MAX_LIST_SIZE
  const size_t PAYLOAD_WITH_VAL =
      0; // not yet implemented testing end values
  //const size_t SWAPS_IN_WAKSMAN_NETWORK =
  //   MAX_LIST_SIZE * (DEPTH_OF_WAKSMAN - 1) + 1;
  const size_t ADJUSTED_LIST_SIZE = (1 << DEPTH_OF_WAKSMAN);
  const size_t SWAPS_IN_WAKSMAN_NETWORK =
      ADJUSTED_LIST_SIZE * (DEPTH_OF_WAKSMAN - 1) + 1;

  const Boolean_t TEST_BOOLEAN_VALUE = 0xb3;
  WaksmanInfo<uint64_t> info(
      modulus,
      keyModulus,
      ADJUSTED_LIST_SIZE,
      DEPTH_OF_WAKSMAN,
      SWAPS_IN_WAKSMAN_NETWORK);
  std::vector<uint64_t> payloads;
  std::vector<uint64_t> keys;
  std::vector<Boolean_t> payloadsXOR;

  size_t BeaverTriplesNeeded =
      ((DEPTH_OF_WAKSMAN - 1) * ADJUSTED_LIST_SIZE + 1) *
      (NUM_ARITHMETIC_PAYLOADS);
  size_t BeaverTriplesKeyNeeded =
      ((DEPTH_OF_WAKSMAN - 1) * ADJUSTED_LIST_SIZE + 1) * (NUM_KEYS);
  size_t XORBeaverTriplesNeeded =
      ((DEPTH_OF_WAKSMAN - 1) * ADJUSTED_LIST_SIZE + 1) *
      (NUM_XOR_PAYLOADS + 1);

  std::unique_ptr<Promise<RandomnessDispenser<
      WaksmanBits<uint64_t>,
      WaksmanInfo<uint64_t>>>>
      waksman_income;
  std::unique_ptr<Promise<RandomnessDispenser<
      WaksmanBits<uint64_t>,
      WaksmanInfo<uint64_t>>>>
      waksman_univ1;
  std::unique_ptr<Promise<RandomnessDispenser<
      WaksmanBits<uint64_t>,
      WaksmanInfo<uint64_t>>>>
      waksman_univ2;

  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_income;
  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_univ1;
  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_univ2;

  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_income_key;
  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_univ1_key;
  std::unique_ptr<Promise<RandomnessDispenser<
      BeaverTriple<uint64_t>,
      BeaverInfo<uint64_t>>>>
      beaver_univ2_key;

  std::unique_ptr<Promise<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      beaver_incomeXOR;
  std::unique_ptr<Promise<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      beaver_univ1XOR;
  std::unique_ptr<Promise<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      beaver_univ2XOR;

  std::vector<ObservationList<uint64_t>> oLists =
      std::vector<ObservationList<uint64_t>>(NUM_PARTIES);
  for (size_t i = 0; i < NUM_PARTIES; i++) {
    oLists.at(i) = ObservationList<uint64_t>();
    std::vector<Observation<uint64_t>> o_elements(0);
    for (size_t j = 0; j < MAX_LIST_SIZE; j++) {
      payloads = std::vector<uint64_t>(NUM_ARITHMETIC_PAYLOADS);
      keys = std::vector<uint64_t>(NUM_KEYS);

      payloads.at(PAYLOAD_WITH_VAL) =
          static_cast<uint64_t>(i * MAX_LIST_SIZE + j);

      payloadsXOR = std::vector<Boolean_t>(NUM_XOR_PAYLOADS);
      payloadsXOR.at(0) = TEST_BOOLEAN_VALUE;

      Observation<uint64_t> o;
      o.keyCols = keys;
      o.arithmeticPayloadCols = payloads;
      o.XORPayloadCols = payloadsXOR;

      o_elements.push_back(o);
    }
    oLists.at(i).elements = o_elements;
    oLists.at(i).numKeyCols = NUM_KEYS;
    oLists.at(i).numArithmeticPayloadCols = NUM_ARITHMETIC_PAYLOADS;
    oLists.at(i).numXORPayloadCols = NUM_XOR_PAYLOADS;
  }

  ObservationList<uint64_t> income_output = ObservationList<uint64_t>();
  ObservationList<uint64_t> univ1_output = ObservationList<uint64_t>();
  ObservationList<uint64_t> univ2_output = ObservationList<uint64_t>();

  size_t numDealers = 4;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [](Fronctocol * self) {
        log_debug("Starting dealer test");
        std::unique_ptr<Fronctocol> rd(new RandomnessHouse<
                                       TEST_TYPES,
                                       WaksmanBits<uint64_t>,
                                       WaksmanInfo<uint64_t>>());

        self->invoke(std::move(rd), self->getPeers());

        std::unique_ptr<Fronctocol> rd2(new RandomnessHouse<
                                        TEST_TYPES,
                                        BeaverTriple<uint64_t>,
                                        BeaverInfo<uint64_t>>());

        self->invoke(std::move(rd2), self->getPeers());

        std::unique_ptr<Fronctocol> rd3(new RandomnessHouse<
                                        TEST_TYPES,
                                        BeaverTriple<uint64_t>,
                                        BeaverInfo<uint64_t>>());

        self->invoke(std::move(rd3), self->getPeers());

        std::unique_ptr<Fronctocol> rd4(new RandomnessHouse<
                                        TEST_TYPES,
                                        BeaverTriple<Boolean_t>,
                                        BooleanBeaverInfo>());

        self->invoke(std::move(rd4), self->getPeers());
      },
      [&, numDealers](Fronctocol &, Fronctocol * self) mutable {
        numDealers--;
        log_debug("Dealer promise completed");
        if (numDealers == 0) {
          log_debug("Dealer done");
          self->complete();
        }
      }));

  std::string * starter = new std::string("income");

  size_t income_num_awaiting = 4;
  WaksmanBits<uint64_t> income_bits;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_income_dispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_key_income_dispenser;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        log_debug("Adding WaksmanBits randomness generator");
        std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
            WaksmanBits<uint64_t>,
            WaksmanInfo<uint64_t>>>>
            waksman_drg(new RandomnessPatron<
                        TEST_TYPES,
                        WaksmanBits<uint64_t>,
                        WaksmanInfo<uint64_t>>("dealer", 1, info));
        waksman_income =
            self->promise(std::move(waksman_drg), self->getPeers());
        self->await(*waksman_income);
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc income");
        income_output =
            static_cast<WaksmanShuffle<TEST_TYPES, uint64_t> &>(f)
                .sharedList;
        self->complete();
      },
      failTestOnReceive,
      [&,
       income_num_awaiting](Fronctocol & f, Fronctocol * self) mutable {
        income_num_awaiting--;
        log_debug("Promise completed");
        if (income_num_awaiting == 3) {
          std::unique_ptr<RandomnessDispenser<
              WaksmanBits<uint64_t>,
              WaksmanInfo<uint64_t>>>
              wrd = waksman_income->getResult(f);
          income_bits = wrd->get();
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesNeeded,
                  BeaverInfo<uint64_t>(modulus)));
          beaver_income =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_income);
        } else if (income_num_awaiting == 2) {
          beaver_income_dispenser = beaver_income->getResult(f);

          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesKeyNeeded,
                  BeaverInfo<uint64_t>(keyModulus)));
          beaver_income_key =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_income_key);
        } else if (income_num_awaiting == 1) {
          beaver_key_income_dispenser = beaver_income_key->getResult(f);
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<Boolean_t>,
                         BooleanBeaverInfo>(
                  "dealer", XORBeaverTriplesNeeded, booleanInfo));
          beaver_incomeXOR =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_incomeXOR);
        } else {
          log_debug("Launching shuffle on income server");
          std::unique_ptr<WaksmanShuffle<TEST_TYPES, uint64_t>> shuffle(
              new WaksmanShuffle<TEST_TYPES, uint64_t>(
                  oLists.at(0),
                  modulus,
                  keyModulus,
                  income_bits,
                  DEPTH_OF_WAKSMAN,
                  std::move(beaver_income_dispenser),
                  std::move(beaver_key_income_dispenser),
                  beaver_incomeXOR->getResult(f),
                  starter));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(shuffle), ps);
        }
      }));

  size_t univ1_num_awaiting = 4;
  WaksmanBits<uint64_t> univ1_bits;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_univ1_dispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_key_univ1_dispenser;
  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ1");
        log_debug("Adding WaksmanBits randomness generator");
        std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
            WaksmanBits<uint64_t>,
            WaksmanInfo<uint64_t>>>>
            waksman_drg(new RandomnessPatron<
                        TEST_TYPES,
                        WaksmanBits<uint64_t>,
                        WaksmanInfo<uint64_t>>("dealer", 1, info));
        waksman_univ1 =
            self->promise(std::move(waksman_drg), self->getPeers());
        self->await(*waksman_univ1);
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ1");
        univ1_output =
            static_cast<WaksmanShuffle<TEST_TYPES, uint64_t> &>(f)
                .sharedList;
        self->complete();
      },
      failTestOnReceive,
      [&,
       univ1_num_awaiting](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_awaiting--;
        log_debug("Promise completed");
        if (univ1_num_awaiting == 3) {
          std::unique_ptr<RandomnessDispenser<
              WaksmanBits<uint64_t>,
              WaksmanInfo<uint64_t>>>
              wrd = waksman_univ1->getResult(f);
          univ1_bits = wrd->get();
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesNeeded,
                  BeaverInfo<uint64_t>(modulus)));
          beaver_univ1 =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ1);
        } else if (univ1_num_awaiting == 2) {
          beaver_univ1_dispenser = beaver_univ1->getResult(f);

          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesKeyNeeded,
                  BeaverInfo<uint64_t>(keyModulus)));
          beaver_univ1_key =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ1_key);
        } else if (univ1_num_awaiting == 1) {
          beaver_key_univ1_dispenser = beaver_univ1_key->getResult(f);
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<Boolean_t>,
                         BooleanBeaverInfo>(
                  "dealer", XORBeaverTriplesNeeded, booleanInfo));
          beaver_univ1XOR =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ1XOR);
        } else {
          log_debug("Launching shuffle on univ1 server");
          std::unique_ptr<WaksmanShuffle<TEST_TYPES, uint64_t>> shuffle(
              new WaksmanShuffle<TEST_TYPES, uint64_t>(
                  oLists.at(1),
                  modulus,
                  keyModulus,
                  univ1_bits,
                  DEPTH_OF_WAKSMAN,
                  std::move(beaver_univ1_dispenser),
                  std::move(beaver_key_univ1_dispenser),
                  beaver_univ1XOR->getResult(f),
                  starter));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(shuffle), ps);
        }
      }));

  size_t univ2_num_awaiting = 4;
  WaksmanBits<uint64_t> univ2_bits;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_univ2_dispenser;
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<uint64_t>, BeaverInfo<uint64_t>>>
      beaver_key_univ2_dispenser;
  test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc univ2");
        log_debug("Adding WaksmanBits randomness generator");
        std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
            WaksmanBits<uint64_t>,
            WaksmanInfo<uint64_t>>>>
            waksman_drg(new RandomnessPatron<
                        TEST_TYPES,
                        WaksmanBits<uint64_t>,
                        WaksmanInfo<uint64_t>>("dealer", 1, info));
        waksman_univ2 =
            self->promise(std::move(waksman_drg), self->getPeers());
        self->await(*waksman_univ2);
      },
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ2");
        univ2_output =
            static_cast<WaksmanShuffle<TEST_TYPES, uint64_t> &>(f)
                .sharedList;
        self->complete();
      },
      failTestOnReceive,
      [&,
       univ2_num_awaiting](Fronctocol & f, Fronctocol * self) mutable {
        univ2_num_awaiting--;
        log_debug("Promise completed");
        if (univ2_num_awaiting == 3) {
          std::unique_ptr<RandomnessDispenser<
              WaksmanBits<uint64_t>,
              WaksmanInfo<uint64_t>>>
              wrd = waksman_univ2->getResult(f);
          univ2_bits = wrd->get();
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesNeeded,
                  BeaverInfo<uint64_t>(modulus)));
          beaver_univ2 =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ2);
        } else if (univ2_num_awaiting == 2) {
          beaver_univ2_dispenser = beaver_univ2->getResult(f);

          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<uint64_t>,
              BeaverInfo<uint64_t>>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<uint64_t>,
                         BeaverInfo<uint64_t>>(
                  "dealer",
                  BeaverTriplesKeyNeeded,
                  BeaverInfo<uint64_t>(keyModulus)));
          beaver_univ2_key =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ2_key);
        } else if (univ2_num_awaiting == 1) {
          beaver_key_univ2_dispenser = beaver_univ2_key->getResult(f);
          std::unique_ptr<PromiseFronctocol<RandomnessDispenser<
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>>>
              beaver_drg(new RandomnessPatron<
                         TEST_TYPES,
                         BeaverTriple<Boolean_t>,
                         BooleanBeaverInfo>(
                  "dealer", XORBeaverTriplesNeeded, booleanInfo));
          beaver_univ2XOR =
              self->promise(std::move(beaver_drg), self->getPeers());
          self->await(*beaver_univ2XOR);
        } else {
          log_debug("Launching shuffle on univ2 server");
          std::unique_ptr<WaksmanShuffle<TEST_TYPES, uint64_t>> shuffle(
              new WaksmanShuffle<TEST_TYPES, uint64_t>(
                  oLists.at(2),
                  modulus,
                  keyModulus,
                  univ2_bits,
                  DEPTH_OF_WAKSMAN,
                  std::move(beaver_univ2_dispenser),
                  std::move(beaver_key_univ2_dispenser),
                  beaver_univ2XOR->getResult(f),
                  starter));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(shuffle), ps);
        }
      }));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug("BACK");

  for (size_t i = 0; i < income_output.elements.size(); i++) {
    EXPECT_EQ(
        TEST_BOOLEAN_VALUE,
        income_output.elements.at(i).XORPayloadCols.at(0) ^
            univ1_output.elements.at(i).XORPayloadCols.at(0) ^
            univ2_output.elements.at(i).XORPayloadCols.at(0));
  }

  uint64_t test_val =
      income_output.elements.at(2 * MAX_LIST_SIZE / 3).keyCols.at(0) +
      univ1_output.elements.at(2 * MAX_LIST_SIZE / 3).keyCols.at(0) +
      univ2_output.elements.at(2 * MAX_LIST_SIZE / 3).keyCols.at(0);
  test_val %= keyModulus;
  EXPECT_EQ(0, test_val);
  for (size_t i = 0; i < income_output.elements.size(); i++) {
    test_val = income_output.elements.at(i).arithmeticPayloadCols.at(
                   PAYLOAD_WITH_VAL) +
        univ1_output.elements.at(i).arithmeticPayloadCols.at(
            PAYLOAD_WITH_VAL) +
        univ2_output.elements.at(i).arithmeticPayloadCols.at(
            PAYLOAD_WITH_VAL);
    test_val %= modulus;
    test_val = static_cast<uint64_t>(
        test_val + (3 * modulus - 3 * MAX_LIST_SIZE));
    test_val %= modulus;
    test_val /= 3;
    log_debug("%lu", test_val);
  }
}

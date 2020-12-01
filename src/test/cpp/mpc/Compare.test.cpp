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
#include <mpc/CompareDealer.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

const std::vector<std::string> NAMES = {
    {"alice", "bob", "chelsea", "david", "eve", "farrah"}};

template<typename Large_T, typename Small_T>
void testCompare(size_t const nparties, Large_T const p) {
  log_assert(nparties > 1);
  log_assert(nparties < NAMES.size());

  std::string const dealer("dealer");
  std::string const revealer(NAMES[1]);

  CompareInfo<std::string, Large_T, Small_T> info(p, &revealer);

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new CompareRandomnessHouse<TEST_TYPES, Large_T, Small_T>(
                &info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  Large_T const x = randomModP<Large_T>(p / 2);
  Large_T const y = randomModP<Large_T>(p / 2);

  std::vector<Large_T> xs;
  std::vector<Large_T> ys;

  arithmeticSecretShare<Large_T>(nparties, p, x, xs);
  arithmeticSecretShare<Large_T>(nparties, p, y, ys);

  std::vector<Boolean_t> results(nparties, 0);

  for (size_t i = 0; i < nparties; i++) {
    size_t * num_remaining = new size_t(2);
    test[NAMES[i]] = std::unique_ptr<Fronctocol>(new Tester(
        [&info, &dealer](Fronctocol * self) {
          self->invoke(
              std::unique_ptr<Fronctocol>(
                  new CompareRandomnessPatron<
                      TEST_TYPES,
                      Large_T,
                      Small_T>(&info, &dealer, 1UL)),
              self->getPeers());
        },
        [i, num_remaining, &info, &dealer, &xs, &ys, &results](
            Fronctocol & f, Fronctocol * self) {
          (*num_remaining)--;
          if (*num_remaining == 1) {
            CompareRandomness<Large_T, Small_T> randomness(
                static_cast<CompareRandomnessPatron<
                    TEST_TYPES,
                    Large_T,
                    Small_T> &>(f)
                    .compareDispenser->get());

            PeerSet ps(self->getPeers());
            ps.remove(dealer);
            self->invoke(
                std::unique_ptr<Fronctocol>(
                    new Compare<TEST_TYPES, Large_T, Small_T>(
                        xs[i], ys[i], &info, std::move(randomness))),
                ps);
          } else {
            results[i] =
                static_cast<Compare<TEST_TYPES, Large_T, Small_T> &>(f)
                    .outputShare;
            delete num_remaining;
            self->complete();
          }
        },
        failTestOnReceive,
        failTestOnPromise));
  }

  EXPECT_TRUE(runTests(test));

  Boolean_t result = results[0];
  for (size_t i = 1; i < nparties; i++) {
    result = (Boolean_t)(result ^ results[i]);
  }

  log_debug(
      "x=%s, y=%s, result = %hhu",
      dec(x).c_str(),
      dec(y).c_str(),
      result);

  if (x > y) {
    EXPECT_EQ(1, result);
  }
  if (x == y) {
    EXPECT_EQ(2, result);
  }
  if (x < y) {
    EXPECT_EQ(0, result);
  }
}

TEST(Compare, compare_2_to_6_parties_uint32_uint32) {
  for (size_t nparties = 2; nparties < 6; nparties++) {
    for (size_t i = 0; i < 5; i++) {
      testCompare<uint32_t, uint32_t>(
          nparties, ((uint32_t)1 << 31) - 1);
    }
  }
}

TEST(Compare, compare_2_to_6_parties_largenum_uint32) {
  for (size_t nparties = 2; nparties < 6; nparties++) {
    for (size_t i = 0; i < 5; i++) {
      testCompare<LargeNum, uint32_t>(
          nparties, (LargeNum(1) << 89) - 1);
    }
  }
}

TEST(Compare, compare_2_to_6_parties_uint64_uint64) {
  for (size_t nparties = 2; nparties < 6; nparties++) {
    for (size_t i = 0; i < 5; i++) {
      testCompare<uint64_t, uint64_t>(
          nparties, ((uint64_t)1 << 61) - 1);
    }
  }
}

TEST(Compare, compare_greater_than) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 97;
  const size_t numElements = 7;
  const size_t lambda = 3;

  Boolean_t result_income = 0;
  Boolean_t result_univ1 = 0;
  Boolean_t result_univ2 = 0;

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;
  std::vector<std::vector<SmallNum>> lagrangePolynomialSet;

  size_t block_size = lambda;
  while (block_size < numElements) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<SmallNum>(block_size)));
    log_debug("block_size: %lu", block_size);
    block_size += lambda;
  }

  log_debug("block_size: %lu", numElements);
  UnboundedFaninOrRandomnessNeeds.emplace_back(numElements);
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      p, static_cast<SmallNum>(numElements)));

  block_size = 1;
  while ((block_size - 1) < lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<SmallNum>(block_size)));
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  std::string const dealer("dealer");
  std::string const revealer("income");

  CompareInfo<std::string, SmallNum, SmallNum> info(
      p, p, numElements, lambda, lagrangePolynomialSet, &revealer);

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new CompareRandomnessHouse<TEST_TYPES, SmallNum, SmallNum>(
                &info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  SmallNum x_income = 30;
  SmallNum x_univ1 = 35;
  SmallNum x_univ2 = 40;

  SmallNum y_income = 1;
  SmallNum y_univ1 = 2;
  SmallNum y_univ2 = 4;

  // 105 equiv 8 > 7 mod 97
  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_income, y_income, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc income");
          result_income =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
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
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        if (univ1_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_univ1, y_univ1, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ1");
          result_univ1 =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
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
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ2_num_fronctocols_remaining--;
        log_debug(
            "univ2_num_fronctocols_remaining %lu",
            univ2_num_fronctocols_remaining);
        if (univ2_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_univ2, y_univ2, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ2");
          result_univ2 =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
                  .outputShare;
          log_debug("result_univ2 %u", result_univ2);
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

  EXPECT_EQ(1, (result_income ^ result_univ1 ^ result_univ2));
}

TEST(Compare, compare_less_than) {

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 97;
  const size_t numElements = 7;
  const size_t lambda = 3;

  Boolean_t result_income = 0;
  Boolean_t result_univ1 = 0;
  Boolean_t result_univ2 = 0;

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;
  std::vector<std::vector<SmallNum>> lagrangePolynomialSet =
      std::vector<std::vector<SmallNum>>();

  size_t block_size = lambda;
  while (block_size < numElements) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<SmallNum>(block_size)));
    log_debug("block_size: %lu", block_size);
    block_size += lambda;
  }
  log_debug("block_size: %lu", numElements);
  UnboundedFaninOrRandomnessNeeds.emplace_back(numElements);
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      p, static_cast<SmallNum>(numElements)));

  block_size = 1;
  while ((block_size - 1) < lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        p, static_cast<SmallNum>(block_size)));
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  std::string dealer("dealer");
  std::string revealer("income");
  CompareInfo<std::string, SmallNum, SmallNum> info(
      p, p, numElements, lambda, lagrangePolynomialSet, &revealer);

  test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(
            new CompareRandomnessHouse<TEST_TYPES, SmallNum, SmallNum>(
                &info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  SmallNum x_income = 30;
  SmallNum x_univ1 = 35;
  SmallNum x_univ2 = 40;

  SmallNum y_income = 59;
  SmallNum y_univ1 = 82;
  SmallNum y_univ2 = 66;

  // 105 equiv 8 < 207 equiv 13 mod 97
  size_t income_num_fronctocols_remaining = 2;
  test["income"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        if (income_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_income, y_income, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc income");
          result_income =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
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
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        if (univ1_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_univ1, y_univ1, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ1");
          result_univ1 =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
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
            new CompareRandomnessPatron<TEST_TYPES, SmallNum, SmallNum>(
                &info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ2_num_fronctocols_remaining--;
        if (univ2_num_fronctocols_remaining == 1) {
          CompareRandomness<SmallNum, SmallNum> randomness(
              static_cast<CompareRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum> &>(f)
                  .compareDispenser->get());

          std::unique_ptr<Compare<TEST_TYPES, SmallNum, SmallNum>> c(
              new Compare<TEST_TYPES, SmallNum, SmallNum>(
                  x_univ2, y_univ2, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove(dealer);
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ2");
          result_univ2 =
              static_cast<Compare<TEST_TYPES, SmallNum, SmallNum> &>(f)
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

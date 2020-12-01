/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <map>
#include <memory>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/Compare.h>
#include <mpc/Divide.h>
#include <mpc/DivideDealer.h>
#include <mpc/PrefixOr.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Divide, arithmetic_divide_uint32) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  size_t num_parties = 4;
  const uint32_t p = (1U << 31) - 1;
  log_assert(isPrime<uint32_t>(p));
  log_info("%s", dec(p).c_str());

  /* Plain Input Setup */
  const uint32_t dividend = randomModP<uint32_t>(p);
  // approximate sqrt of prime ((1 << 31) - 1) (actually sqrt(1<<32))
  const uint32_t divisor = 1 + randomModP<uint32_t>(65536);

  log_debug("Dividend: %u", dividend);
  log_debug("Divisor: %u", divisor);

  /****************************/
  /* START: Prep for PrefixOR */
  /****************************/
  std::string * dealer = new std::string("dealer");
  std::string * revealer = new std::string("Party1");
  CompareInfo<std::string, uint32_t, uint32_t> const compareInfo(
      p, revealer);

  PrefixOrInfo<std::string, uint32_t> prefInfo(
      p, compareInfo.ell, revealer);

  const DivideInfo<std::string, uint32_t, uint32_t> div_info(
      revealer,
      p,
      compareInfo.ell,
      prefInfo.lambda,
      prefInfo.lagrangePolynomialSet,
      &compareInfo);

  /* Creates Shares of Inputs */
  std::vector<uint32_t> sh_dividend;
  arithmeticSecretShare(num_parties, p, dividend, sh_dividend);
  std::vector<uint32_t> sh_divisor;
  arithmeticSecretShare(num_parties, p, divisor, sh_divisor);
  uint32_t temp_dividend = 0;
  uint32_t temp_divisor = 0;
  for (size_t i = 0; i < sh_dividend.size(); i++) {
    log_debug("<= Party %zu's input shares =>", i + 1);
    log_debug("Input Dividend Share: %u", sh_dividend.at(i));
    log_debug("Input Divisor Share: %u", sh_divisor.at(i));
    temp_dividend = modAdd(sh_dividend.at(i), temp_dividend, p);
    temp_divisor = modAdd(sh_divisor.at(i), temp_divisor, p);
  }
  log_assert(temp_dividend == dividend);
  log_assert(temp_divisor == divisor);

  uint32_t outshare1 = 0;
  uint32_t outshare2 = 0;
  uint32_t outshare3 = 0;
  uint32_t outshare4 = 0;

  /*********************************/
  /* START: Prep for Batch Compare */
  /*********************************/
  size_t numDealers = 1;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer for divide");
        std::unique_ptr<Fronctocol> rd3(
            new DivideRandomnessHouse<TEST_TYPES, uint32_t, uint32_t>(
                &div_info));
        self->invoke(std::move(rd3), self->getPeers());
      },
      [&, numDealers](Fronctocol &, Fronctocol * self) mutable {
        numDealers--;
        if (numDealers == 0) {
          self->complete();
        }
      }));
  /*******************************/
  /* END: Prep for Batch Compare */
  /*******************************/

  size_t p1_num_fronctocols_remaining = 2;
  test[*revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 1");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint32_t, uint32_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p1_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p1_num_fronctocols_remaining);
        if (p1_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint32_t, uint32_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint32_t,
                                uint32_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint32_t, uint32_t>(
                  sh_dividend.at(0),
                  sh_divisor.at(0),
                  &outshare1,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p2_num_fronctocols_remaining = 2;
  test["Party2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 2");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint32_t, uint32_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p2_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p2_num_fronctocols_remaining);
        if (p2_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint32_t, uint32_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint32_t,
                                uint32_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint32_t, uint32_t>(
                  sh_dividend.at(1),
                  sh_divisor.at(1),
                  &outshare2,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p3_num_fronctocols_remaining = 2;
  test["Party3"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 3");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint32_t, uint32_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p3_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p3_num_fronctocols_remaining);
        if (p3_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint32_t, uint32_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint32_t,
                                uint32_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint32_t, uint32_t>(
                  sh_dividend.at(2),
                  sh_divisor.at(2),
                  &outshare3,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p4_num_fronctocols_remaining = 2;
  test["Party4"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 4");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint32_t, uint32_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p4_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p4_num_fronctocols_remaining);
        if (p4_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint32_t, uint32_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint32_t,
                                uint32_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint32_t, uint32_t>(
                  sh_dividend.at(3),
                  sh_divisor.at(3),
                  &outshare4,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug(
      "%u, %u, %u, %u", outshare1, outshare2, outshare3, outshare4);
  EXPECT_EQ(
      (dividend / divisor) % p,
      modAdd(
          modAdd(modAdd(outshare1, outshare2, p), outshare3, p),
          outshare4,
          p));
};

TEST(Divide, arithmetic_divide_uint64) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  size_t num_parties = 4;
  const uint64_t p = (1ULL << 61) - 1;
  log_assert(isPrime<uint64_t>(p));
  log_info("%s", dec(p).c_str());

  /* Plain Input Setup */
  const uint64_t dividend = randomModP<uint64_t>(p);
  // approximate sqrt of prime ((1 << 61) - 1) (actually sqrt(1<<62))
  const uint64_t divisor = 1 + randomModP<uint64_t>(2147483648);

  log_debug("Dividend: %lu", dividend);
  log_debug("Divisor: %lu", divisor);

  /****************************/
  /* START: Prep for PrefixOR */
  /****************************/
  std::string * dealer = new std::string("dealer");
  std::string * revealer = new std::string("Party1");
  CompareInfo<std::string, uint64_t, uint64_t> const compareInfo(
      p, revealer);

  PrefixOrInfo<std::string, uint64_t> prefInfo(
      p, compareInfo.ell, revealer);

  const DivideInfo<std::string, uint64_t, uint64_t> div_info(
      revealer,
      p,
      compareInfo.ell,
      prefInfo.lambda,
      prefInfo.lagrangePolynomialSet,
      &compareInfo);

  /* Creates Shares of Inputs */
  std::vector<uint64_t> sh_dividend;
  arithmeticSecretShare(num_parties, p, dividend, sh_dividend);
  std::vector<uint64_t> sh_divisor;
  arithmeticSecretShare(num_parties, p, divisor, sh_divisor);
  uint64_t temp_dividend = 0;
  uint64_t temp_divisor = 0;
  for (size_t i = 0; i < sh_dividend.size(); i++) {
    log_debug("<= Party %zu's input shares =>", i + 1);
    log_debug("Input Dividend Share: %u", sh_dividend.at(i));
    log_debug("Input Divisor Share: %u", sh_divisor.at(i));
    temp_dividend = modAdd(sh_dividend.at(i), temp_dividend, p);
    temp_divisor = modAdd(sh_divisor.at(i), temp_divisor, p);
  }
  log_assert(temp_dividend == dividend);
  log_assert(temp_divisor == divisor);

  uint64_t outshare1 = 0;
  uint64_t outshare2 = 0;
  uint64_t outshare3 = 0;
  uint64_t outshare4 = 0;

  /*********************************/
  /* START: Prep for Batch Compare */
  /*********************************/
  size_t numDealers = 1;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer for divide");
        std::unique_ptr<Fronctocol> rd3(
            new DivideRandomnessHouse<TEST_TYPES, uint64_t, uint64_t>(
                &div_info));
        self->invoke(std::move(rd3), self->getPeers());
      },
      [&, numDealers](Fronctocol &, Fronctocol * self) mutable {
        numDealers--;
        if (numDealers == 0) {
          self->complete();
        }
      }));
  /*******************************/
  /* END: Prep for Batch Compare */
  /*******************************/

  size_t p1_num_fronctocols_remaining = 2;
  test[*revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 1");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint64_t, uint64_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p1_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p1_num_fronctocols_remaining);
        if (p1_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint64_t, uint64_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint64_t,
                                uint64_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint64_t, uint64_t>(
                  sh_dividend.at(0),
                  sh_divisor.at(0),
                  &outshare1,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p2_num_fronctocols_remaining = 2;
  test["Party2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 2");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint64_t, uint64_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p2_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p2_num_fronctocols_remaining);
        if (p2_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint64_t, uint64_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint64_t,
                                uint64_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint64_t, uint64_t>(
                  sh_dividend.at(1),
                  sh_divisor.at(1),
                  &outshare2,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p3_num_fronctocols_remaining = 2;
  test["Party3"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 3");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint64_t, uint64_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p3_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p3_num_fronctocols_remaining);
        if (p3_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint64_t, uint64_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint64_t,
                                uint64_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint64_t, uint64_t>(
                  sh_dividend.at(2),
                  sh_divisor.at(2),
                  &outshare3,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p4_num_fronctocols_remaining = 2;
  test["Party4"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 4");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, uint64_t, uint64_t>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p4_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p4_num_fronctocols_remaining);
        if (p4_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<uint64_t, uint64_t>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                uint64_t,
                                uint64_t> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, uint64_t, uint64_t>(
                  sh_dividend.at(3),
                  sh_divisor.at(3),
                  &outshare4,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug(
      "%u, %u, %u, %u", outshare1, outshare2, outshare3, outshare4);
  EXPECT_EQ(
      (dividend / divisor) % p,
      modAdd(
          modAdd(modAdd(outshare1, outshare2, p), outshare3, p),
          outshare4,
          p));
};

TEST(Divide, arithmetic_divide_big_num) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  size_t num_parties = 4;

  const LargeNum pg(2UL << 35);
  const LargeNum p = nextPrime<LargeNum>(pg * pg);
  // 1536 group from https://tools.ietf.org/html/rfc3526
  /* largeNumFromHex(
      std::string("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
                  "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
                  "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
                  "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
                  "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
                  "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
                  "83655D23DCA3AD961C62F356208552BB9ED529077096966D"
                  "670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"));
    */

  log_assert(isPrime<LargeNum>(p));
  log_info("%s", dec(p).c_str());

  // Plain Input Setup
  const LargeNum dividend = randomModP<LargeNum>(p);
  // approx sqrt of p
  const LargeNum divisor = LargeNum(1) + randomModP<LargeNum>(pg);

  log_debug("Dividend: %u", dividend);
  log_debug("Divisor: %u", divisor);

  // ****************************|
  // * START: Prep for PrefixOR *|
  // ****************************|
  std::string * dealer = new std::string("dealer");
  std::string * revealer = new std::string("Party1");
  CompareInfo<std::string, LargeNum, SmallNum> const compareInfo(
      p, revealer);

  PrefixOrInfo<std::string, SmallNum> prefInfo(
      compareInfo.s, compareInfo.ell, revealer);

  const DivideInfo<std::string, LargeNum, SmallNum> div_info(
      revealer,
      p,
      compareInfo.ell,
      prefInfo.lambda,
      prefInfo.lagrangePolynomialSet,
      &compareInfo);

  // Creates Shares of Inputs
  std::vector<LargeNum> sh_dividend;
  arithmeticSecretShare(num_parties, p, dividend, sh_dividend);
  std::vector<LargeNum> sh_divisor;
  arithmeticSecretShare(num_parties, p, divisor, sh_divisor);
  LargeNum temp_dividend = 0;
  LargeNum temp_divisor = 0;
  for (size_t i = 0; i < sh_dividend.size(); i++) {
    log_debug("<= Party %zu's input shares =>", i + 1);
    log_debug("Input Dividend Share: %u", sh_dividend.at(i));
    log_debug("Input Divisor Share: %u", sh_divisor.at(i));
    temp_dividend = (sh_dividend.at(i) + temp_dividend) % p;
    temp_divisor = (sh_divisor.at(i) + temp_divisor) % p;
  }
  log_assert(temp_dividend == dividend);
  log_assert(temp_divisor == divisor);

  //std::vector<BeaverTriple<SmallNum>> beavers;
  //div_info.info.generate(3, 1, beavers);

  LargeNum outshare1 = 0;
  LargeNum outshare2 = 0;
  LargeNum outshare3 = 0;
  LargeNum outshare4 = 0;

  //*********************************|
  //* START: Prep for Batch Compare *|
  //*********************************|
  size_t numDealers = 1;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        // BatchCompare randomness house

        log_debug("Starting dealer for divide");
        std::unique_ptr<Fronctocol> rd3(
            new DivideRandomnessHouse<TEST_TYPES, LargeNum, SmallNum>(
                &div_info));
        self->invoke(std::move(rd3), self->getPeers());
      },
      [&, numDealers](Fronctocol &, Fronctocol * self) mutable {
        numDealers--;
        if (numDealers == 0) {
          self->complete();
        }
      }));
  //*******************************|
  //* END: Prep for Batch Compare *|
  //*******************************|

  size_t p1_num_fronctocols_remaining = 2;
  test[*revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 1");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, LargeNum, SmallNum>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p1_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p1_num_fronctocols_remaining);
        if (p1_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<LargeNum, SmallNum>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                LargeNum,
                                SmallNum> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, LargeNum, SmallNum>(
                  sh_dividend.at(0),
                  sh_divisor.at(0),
                  &outshare1,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p2_num_fronctocols_remaining = 2;
  test["Party2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 2");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, LargeNum, SmallNum>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p2_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p2_num_fronctocols_remaining);
        if (p2_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<LargeNum, SmallNum>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                LargeNum,
                                SmallNum> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, LargeNum, SmallNum>(
                  sh_dividend.at(1),
                  sh_divisor.at(1),
                  &outshare2,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p3_num_fronctocols_remaining = 2;
  test["Party3"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 3");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, LargeNum, SmallNum>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p3_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p3_num_fronctocols_remaining);
        if (p3_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<LargeNum, SmallNum>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                LargeNum,
                                SmallNum> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, LargeNum, SmallNum>(
                  sh_dividend.at(2),
                  sh_divisor.at(2),
                  &outshare3,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  size_t p4_num_fronctocols_remaining = 2;
  test["Party4"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc 4");
        std::unique_ptr<Fronctocol> patron(
            new DivideRandomnessPatron<TEST_TYPES, LargeNum, SmallNum>(
                &div_info, dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        p4_num_fronctocols_remaining--;
        log_debug(
            "Calling handlecomplete num remaining %zu",
            p4_num_fronctocols_remaining);
        if (p4_num_fronctocols_remaining == 1) {
          log_debug("a2");
          std::unique_ptr<RandomnessDispenser<
              DivideRandomness<LargeNum, SmallNum>,
              DoNotGenerateInfo>>
              divideDispenser =
                  std::move(static_cast<DivideRandomnessPatron<
                                TEST_TYPES,
                                LargeNum,
                                SmallNum> &>(f)
                                .divideDispenser);

          std::unique_ptr<Fronctocol> ff(
              new Divide<TEST_TYPES, LargeNum, SmallNum>(
                  sh_dividend.at(3),
                  sh_divisor.at(3),
                  &outshare4,
                  &div_info,
                  divideDispenser->get()));

          log_debug("About to invoke??");
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(ff), ps);
        } else {
          log_debug("a3");
          log_debug("finishing mpc income");
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));
  log_debug("%u, %u, %u", outshare1, outshare2, outshare3);
  EXPECT_EQ(
      (outshare1 + outshare2 + outshare3 + outshare4) % p,
      (dividend / divisor) % p);
};

//*/

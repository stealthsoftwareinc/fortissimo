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
#include <mpc/ModConvUp.h>
#include <mpc/ModConvUpDealer.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(ModConvUp, mod_conversion_SmallNum_SmallNum_SmallNum) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum startModulus = 2707;
  const SmallNum endModulus = 22073;

  auto share_income = randomModP<SmallNum>(startModulus);
  auto share_univ1 = randomModP<SmallNum>(startModulus);
  auto share_univ2 = randomModP<SmallNum>(startModulus);

  SmallNum output_income = 0;
  SmallNum output_univ1 = 0;
  SmallNum output_univ2 = 0;

  std::string dealer{"dealer"};
  std::string revealer{"income"};

  ModConvUpInfo<std::string, SmallNum, SmallNum, SmallNum> info(
      endModulus, startModulus, &revealer);

  test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("Starting dealer test");

        std::unique_ptr<Fronctocol> rd2(new ModConvUpRandomnessHouse<
                                        TEST_TYPES,
                                        SmallNum,
                                        SmallNum,
                                        SmallNum>(&info));
        self->invoke(std::move(rd2), self->getPeers());
      },
      [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

  size_t income_num_fronctocols_remaining = 2;
  test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc income");
        std::unique_ptr<Fronctocol> patron(
            new ModConvUpRandomnessPatron<
                TEST_TYPES,
                SmallNum,
                SmallNum,
                SmallNum>(&info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        income_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (income_num_fronctocols_remaining == 1) {
          ModConvUpRandomness<SmallNum, SmallNum, SmallNum> randomness(
              static_cast<ModConvUpRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  SmallNum> &>(f)
                  .modConvUpDispenser->get());

          std::unique_ptr<
              ModConvUp<TEST_TYPES, SmallNum, SmallNum, SmallNum>>
              c(new ModConvUp<TEST_TYPES, SmallNum, SmallNum, SmallNum>(
                  share_income, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc income");
          output_income = static_cast<ModConvUp<
              TEST_TYPES,
              SmallNum,
              SmallNum,
              SmallNum> &>(f)
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
            new ModConvUpRandomnessPatron<
                TEST_TYPES,
                SmallNum,
                SmallNum,
                SmallNum>(&info, &dealer, 1UL));
        self->invoke(std::move(patron), self->getPeers());
      },
      [&](Fronctocol & f, Fronctocol * self) mutable {
        univ1_num_fronctocols_remaining--;
        log_debug("Handle complete");
        if (univ1_num_fronctocols_remaining == 1) {
          ModConvUpRandomness<SmallNum, SmallNum, SmallNum> randomness(
              static_cast<ModConvUpRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  SmallNum> &>(f)
                  .modConvUpDispenser->get());

          std::unique_ptr<
              ModConvUp<TEST_TYPES, SmallNum, SmallNum, SmallNum>>
              c(new ModConvUp<TEST_TYPES, SmallNum, SmallNum, SmallNum>(
                  share_univ1, &info, std::move(randomness)));
          PeerSet ps(self->getPeers());
          ps.remove("dealer");
          self->invoke(std::move(c), ps);
        } else {
          log_debug("finishing mpc univ1");
          output_univ1 = static_cast<ModConvUp<
              TEST_TYPES,
              SmallNum,
              SmallNum,
              SmallNum> &>(f)
                             .outputShare;
          self->complete();
        }
      },
      failTestOnReceive,
      failTestOnPromise));

  if (randomModP<size_t>(2) == 0) {
    log_info("with univ2");
    size_t univ2_num_fronctocols_remaining = 2;
    test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
        [&](Fronctocol * self) {
          log_debug("starting mpc univ2");
          std::unique_ptr<Fronctocol> patron(
              new ModConvUpRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  SmallNum>(&info, &dealer, 1UL));
          self->invoke(std::move(patron), self->getPeers());
        },
        [&](Fronctocol & f, Fronctocol * self) mutable {
          univ2_num_fronctocols_remaining--;
          log_debug("Handle complete");
          if (univ2_num_fronctocols_remaining == 1) {
            ModConvUpRandomness<SmallNum, SmallNum, SmallNum>
                randomness(static_cast<ModConvUpRandomnessPatron<
                               TEST_TYPES,
                               SmallNum,
                               SmallNum,
                               SmallNum> &>(f)
                               .modConvUpDispenser->get());

            std::unique_ptr<
                ModConvUp<TEST_TYPES, SmallNum, SmallNum, SmallNum>>
                c(new ModConvUp<
                    TEST_TYPES,
                    SmallNum,
                    SmallNum,
                    SmallNum>(
                    share_univ2, &info, std::move(randomness)));
            PeerSet ps(self->getPeers());
            ps.remove("dealer");
            self->invoke(std::move(c), ps);
          } else {
            log_debug("finishing mpc univ2");
            output_univ2 = static_cast<ModConvUp<
                TEST_TYPES,
                SmallNum,
                SmallNum,
                SmallNum> &>(f)
                               .outputShare;
            self->complete();
          }
        },
        failTestOnReceive,
        failTestOnPromise));
  } else {
    share_univ2 = 0;
  }

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug(
      "Vals %u, %u, %u,", output_income, output_univ1, output_univ2);

  EXPECT_EQ(
      (share_income + share_univ1 + share_univ2) % startModulus,
      static_cast<uint32_t>(
          (static_cast<uint64_t>(output_income) +
           static_cast<uint64_t>(output_univ1) +
           static_cast<uint64_t>(output_univ2)) %
          static_cast<uint64_t>(endModulus)));
};

TEST(ModConvUp, mod_conversion_SmallNum_SmallNum_LargeNum) {
  for (size_t i = 0; i < 10; i++) {
    std::map<std::string, std::unique_ptr<Fronctocol>> test;

    const SmallNum startModulus = 2707;
    const SmallNum endModulus = 22073;

    EXPECT_EQ(true, isPrime<SmallNum>(startModulus));
    EXPECT_EQ(true, isPrime<SmallNum>(endModulus));

    //* toggling between random inputs
    SmallNum share_income = randomModP<SmallNum>(startModulus);
    SmallNum share_univ1 = randomModP<SmallNum>(startModulus);
    SmallNum share_univ2 = randomModP<SmallNum>(startModulus);
    //*/

    /* and fixed inputs
  SmallNum share_income = 0;
  SmallNum share_univ1 = 0;
  SmallNum share_univ2 = 0;
  //*/

    LargeNum output_income = 0;
    LargeNum output_univ1 = 0;
    LargeNum output_univ2 = 0;

    std::string dealer{"dealer"};
    std::string revealer{"income"};

    ModConvUpInfo<std::string, SmallNum, SmallNum, LargeNum> info(
        endModulus, startModulus, &revealer);

    test[dealer] = std::unique_ptr<Fronctocol>(new Tester(
        [&](Fronctocol * self) {
          log_debug("Starting dealer test");

          std::unique_ptr<Fronctocol> rd2(new ModConvUpRandomnessHouse<
                                          TEST_TYPES,
                                          SmallNum,
                                          SmallNum,
                                          LargeNum>(&info));
          self->invoke(std::move(rd2), self->getPeers());
        },
        [&](Fronctocol &, Fronctocol * self) { self->complete(); }));

    size_t income_num_fronctocols_remaining = 2;
    test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
        [&](Fronctocol * self) {
          log_debug("starting mpc income");
          std::unique_ptr<Fronctocol> patron(
              new ModConvUpRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  LargeNum>(&info, &dealer, 1UL));
          self->invoke(std::move(patron), self->getPeers());
        },
        [&](Fronctocol & f, Fronctocol * self) mutable {
          income_num_fronctocols_remaining--;
          log_debug("Handle complete");
          if (income_num_fronctocols_remaining == 1) {
            ModConvUpRandomness<SmallNum, SmallNum, LargeNum>
                randomness(static_cast<ModConvUpRandomnessPatron<
                               TEST_TYPES,
                               SmallNum,
                               SmallNum,
                               LargeNum> &>(f)
                               .modConvUpDispenser->get());

            std::unique_ptr<
                ModConvUp<TEST_TYPES, SmallNum, SmallNum, LargeNum>>
                c(new ModConvUp<
                    TEST_TYPES,
                    SmallNum,
                    SmallNum,
                    LargeNum>(
                    share_income, &info, std::move(randomness)));
            PeerSet ps(self->getPeers());
            ps.remove("dealer");
            self->invoke(std::move(c), ps);
          } else {
            log_debug("finishing mpc income");
            output_income = static_cast<ModConvUp<
                TEST_TYPES,
                SmallNum,
                SmallNum,
                LargeNum> &>(f)
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
              new ModConvUpRandomnessPatron<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  LargeNum>(&info, &dealer, 1UL));
          self->invoke(std::move(patron), self->getPeers());
        },
        [&](Fronctocol & f, Fronctocol * self) mutable {
          univ1_num_fronctocols_remaining--;
          log_debug("Handle complete");
          if (univ1_num_fronctocols_remaining == 1) {
            ModConvUpRandomness<SmallNum, SmallNum, LargeNum>
                randomness(static_cast<ModConvUpRandomnessPatron<
                               TEST_TYPES,
                               SmallNum,
                               SmallNum,
                               LargeNum> &>(f)
                               .modConvUpDispenser->get());

            std::unique_ptr<
                ModConvUp<TEST_TYPES, SmallNum, SmallNum, LargeNum>>
                c(new ModConvUp<
                    TEST_TYPES,
                    SmallNum,
                    SmallNum,
                    LargeNum>(
                    share_univ1, &info, std::move(randomness)));
            PeerSet ps(self->getPeers());
            ps.remove("dealer");
            self->invoke(std::move(c), ps);
          } else {
            log_debug("finishing mpc univ1");
            output_univ1 = static_cast<ModConvUp<
                TEST_TYPES,
                SmallNum,
                SmallNum,
                LargeNum> &>(f)
                               .outputShare;
            self->complete();
          }
        },
        failTestOnReceive,
        failTestOnPromise));

    if (randomModP<size_t>(2) == 0) {
      log_info("with univ2");
      size_t univ2_num_fronctocols_remaining = 2;
      test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
          [&](Fronctocol * self) {
            log_debug("starting mpc univ2");
            std::unique_ptr<Fronctocol> patron(
                new ModConvUpRandomnessPatron<
                    TEST_TYPES,
                    SmallNum,
                    SmallNum,
                    LargeNum>(&info, &dealer, 1UL));
            self->invoke(std::move(patron), self->getPeers());
          },
          [&](Fronctocol & f, Fronctocol * self) mutable {
            univ2_num_fronctocols_remaining--;
            log_debug("Handle complete");
            if (univ2_num_fronctocols_remaining == 1) {
              ModConvUpRandomness<SmallNum, SmallNum, LargeNum>
                  randomness(static_cast<ModConvUpRandomnessPatron<
                                 TEST_TYPES,
                                 SmallNum,
                                 SmallNum,
                                 LargeNum> &>(f)
                                 .modConvUpDispenser->get());

              std::unique_ptr<
                  ModConvUp<TEST_TYPES, SmallNum, SmallNum, LargeNum>>
                  c(new ModConvUp<
                      TEST_TYPES,
                      SmallNum,
                      SmallNum,
                      LargeNum>(
                      share_univ2, &info, std::move(randomness)));
              PeerSet ps(self->getPeers());
              ps.remove("dealer");
              self->invoke(std::move(c), ps);
            } else {
              log_debug("finishing mpc univ2");
              output_univ2 = static_cast<ModConvUp<
                  TEST_TYPES,
                  SmallNum,
                  SmallNum,
                  LargeNum> &>(f)
                                 .outputShare;
              self->complete();
            }
          },
          failTestOnReceive,
          failTestOnPromise));
    } else {
      share_univ2 = 0;
    }

    EXPECT_TRUE(runTests(test));

    log_debug(
        "Share Vals %s, %s, %s,",
        dec(share_income).c_str(),
        dec(share_univ1).c_str(),
        dec(share_univ2).c_str());
    log_debug(
        "Output Vals %s, %s, %s,",
        dec(output_income).c_str(),
        dec(output_univ1).c_str(),
        dec(output_univ2).c_str());

    EXPECT_EQ(
        (share_income + share_univ1 + share_univ2) % startModulus,
        static_cast<uint32_t>(
            (static_cast<uint64_t>(output_income) +
             static_cast<uint64_t>(output_univ1) +
             static_cast<uint64_t>(output_univ2)) %
            static_cast<uint64_t>(endModulus)));
  }
};

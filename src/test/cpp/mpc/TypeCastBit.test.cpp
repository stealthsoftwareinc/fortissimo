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
#include <mpc/Multiply.h>
#include <mpc/TypeCastBit.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(TypeCastBit, small_type_cast_bit) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const SmallNum p = 97;

  SmallNum si = 29;
  SmallNum su1 = 33;
  SmallNum su2 = 36;

  Boolean_t output_income = 0;
  Boolean_t output_univ1 = 0;
  Boolean_t output_univ2 = 0;

  std::string const revealer("income");

  test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&si, &revealer, &p](Fronctocol * self) {
        log_debug("starting mpc income");

        std::unique_ptr<TypeCast<TEST_TYPES, SmallNum>> tc(
            new TypeCast<TEST_TYPES, SmallNum>(
                si,
                p,
                &revealer,
                BeaverTriple<SmallNum>(2, 3, 6),
                TypeCastTriple<SmallNum>(96, 1, 1)));
        self->invoke(std::move(tc), self->getPeers());
      },
      [&output_income](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc income");
        output_income = static_cast<TypeCast<TEST_TYPES, SmallNum> &>(f)
                            .outputBitShare;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&su1, &revealer, &p](Fronctocol * self) {
        log_debug("starting mpc univ1");

        std::unique_ptr<TypeCast<TEST_TYPES, SmallNum>> tc(
            new TypeCast<TEST_TYPES, SmallNum>(
                su1,
                p,
                &revealer,
                BeaverTriple<SmallNum>(0, 0, 0),
                TypeCastTriple<SmallNum>(0, 0, 0)));
        self->invoke(std::move(tc), self->getPeers());
      },
      [&output_univ1](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ1");
        output_univ1 = static_cast<TypeCast<TEST_TYPES, SmallNum> &>(f)
                           .outputBitShare;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ2"] = std::unique_ptr<Fronctocol>(new Tester(
      [&su2, &revealer, &p](Fronctocol * self) {
        log_debug("starting mpc univ2");

        std::unique_ptr<TypeCast<TEST_TYPES, SmallNum>> tc(
            new TypeCast<TEST_TYPES, SmallNum>(
                su2,
                p,
                &revealer,
                BeaverTriple<SmallNum>(0, 0, 0),
                TypeCastTriple<SmallNum>(0, 0, 0)));
        self->invoke(std::move(tc), self->getPeers());
      },
      [&output_univ2](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ2");
        output_univ2 = static_cast<TypeCast<TEST_TYPES, SmallNum> &>(f)
                           .outputBitShare;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug(
      "Vals %u, %u, %u,", output_income, output_univ1, output_univ2);

  EXPECT_EQ(1, (output_income ^ output_univ1 ^ output_univ2));
};

TEST(TypeCastBit, small_type_cast_bit_2parties) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  const ArithmeticShare_t p = 97;

  ArithmeticShare_t si = 29;
  ArithmeticShare_t su1 = 69;

  Boolean_t output_income = 0;
  Boolean_t output_univ1 = 0;

  std::string const revealer("income");

  test[revealer] = std::unique_ptr<Fronctocol>(new Tester(
      [&si, &revealer, &p](Fronctocol * self) {
        log_debug("starting mpc income");

        std::unique_ptr<TypeCast<TEST_TYPES, ArithmeticShare_t>> tc(
            new TypeCast<TEST_TYPES, ArithmeticShare_t>(
                si,
                p,
                &revealer,
                BeaverTriple<ArithmeticShare_t>(2, 3, 6),
                TypeCastTriple<ArithmeticShare_t>(96, 1, 1)));
        self->invoke(std::move(tc), self->getPeers());
      },
      [&output_income](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc income");
        output_income =
            static_cast<TypeCast<TEST_TYPES, ArithmeticShare_t> &>(f)
                .outputBitShare;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  test["univ1"] = std::unique_ptr<Fronctocol>(new Tester(
      [&su1, &revealer, &p](Fronctocol * self) {
        log_debug("starting mpc univ1");

        std::unique_ptr<TypeCast<TEST_TYPES, ArithmeticShare_t>> tc(
            new TypeCast<TEST_TYPES, ArithmeticShare_t>(
                su1,
                p,
                &revealer,
                BeaverTriple<ArithmeticShare_t>(0, 0, 0),
                TypeCastTriple<ArithmeticShare_t>(0, 0, 0)));
        self->invoke(std::move(tc), self->getPeers());
      },
      [&output_univ1](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing mpc univ1");
        output_univ1 =
            static_cast<TypeCast<TEST_TYPES, ArithmeticShare_t> &>(f)
                .outputBitShare;
        self->complete();
      },
      failTestOnReceive,
      failTestOnPromise));

  log_debug("launching tests");

  EXPECT_TRUE(runTests(test));

  log_debug("Vals %u, %u,", output_income, output_univ1);

  EXPECT_EQ(1, (output_income ^ output_univ1));
};

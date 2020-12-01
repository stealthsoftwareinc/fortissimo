/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>
#include <string>
#include <utility>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <mpc/Compare.h>
#include <mpc/Multiply.h>
#include <mpc/PosIntCompare.h>
#include <mpc/PosIntCompareDealer.h>
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Compare, PosIntCompare_compiles) {
  std::vector<std::vector<SmallNum>> lagrange;
  std::string party("alice");
  CompareInfo<std::string, LargeNum, SmallNum> info(
      LargeNum(97), 31, 7, 3, lagrange, &party);

  std::unique_ptr<RandomnessDispenser<
      CompareRandomness<LargeNum, SmallNum>,
      DoNotGenerateInfo>>
      compare_rands(nullptr);
  std::unique_ptr<
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
      xor_mult_rands(nullptr);

  PosIntCompareRandomness<LargeNum, SmallNum> pos_int_rand(
      std::move(compare_rands), std::move(xor_mult_rands));

  PosIntCompareRandomnessHouse<TEST_TYPES, LargeNum, SmallNum> house(
      &info);
  PosIntCompareRandomnessPatron<TEST_TYPES, LargeNum, SmallNum> patron(
      &info, &party, 0);

  PosIntCompare<TEST_TYPES, LargeNum, SmallNum> pos_compare(
      0, 0, &info, std::move(pos_int_rand));

  IncomingMessage imsg(party, nullptr, 0);

  /*
  pos_compare.init();
  pos_compare.handleReceive(imsg);
  pos_compare.handleComplete(house);
  house.init();
  house.handleReceive(imsg);
  house.handleComplete(pos_compare);
  patron.init();
  patron.handleReceive(imsg);
  patron.handleComplete(pos_compare);
  */
}

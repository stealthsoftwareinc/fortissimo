/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <ff/Util.h>

TEST(Util, uint64_to_from_buffer) {
  auto test_to_from = [](uint64_t val) -> bool {
    uint8_t buf[sizeof(uint64_t)];
    uint64_t rval;

    ff::uint64_to_buffer(val, buf);
    rval = ff::buffer_to_uint64(buf);

    return rval == val;
  };

  EXPECT_TRUE(test_to_from(0l));
  EXPECT_TRUE(test_to_from(1l));
  EXPECT_TRUE(test_to_from(2l));
  EXPECT_TRUE(test_to_from(3l));
  EXPECT_TRUE(test_to_from(4l));
  EXPECT_TRUE(test_to_from(5l));
  EXPECT_TRUE(test_to_from(6l));
  EXPECT_TRUE(test_to_from(7l));
  EXPECT_TRUE(test_to_from(8l));
  EXPECT_TRUE(test_to_from(9l));

  EXPECT_TRUE(test_to_from(0xf0f0f0f0f0f0f0f0l));
  EXPECT_TRUE(test_to_from(0x0f0f0f0f0f0f0f0fl));
  EXPECT_TRUE(test_to_from(72057594037927936l));

  EXPECT_TRUE(test_to_from(UINT64_MAX));
}

TEST(Util, fronctocolId_to_from_buffer) {
  auto test_to_from = [](uint64_t val) -> bool {
    uint8_t buf[sizeof(uint64_t)];
    uint64_t rval;

    fronctocolId_to_buffer(val, buf);
    rval = buffer_to_fronctocolId(buf);

    return rval == val;
  };

  EXPECT_TRUE(test_to_from(0l));
  EXPECT_TRUE(test_to_from(1l));
  EXPECT_TRUE(test_to_from(2l));
  EXPECT_TRUE(test_to_from(3l));
  EXPECT_TRUE(test_to_from(4l));
  EXPECT_TRUE(test_to_from(5l));
  EXPECT_TRUE(test_to_from(6l));
  EXPECT_TRUE(test_to_from(7l));
  EXPECT_TRUE(test_to_from(8l));
  EXPECT_TRUE(test_to_from(9l));

  EXPECT_TRUE(test_to_from(0xf0f0f0f0f0f0f0f0l));
  EXPECT_TRUE(test_to_from(0x0f0f0f0f0f0f0f0fl));
  EXPECT_TRUE(test_to_from(72057594037927936l));

  EXPECT_TRUE(test_to_from(UINT64_MAX));
}

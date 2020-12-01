/* C and POSIX Headers */

/* C++ Headers */
#include <cstdlib>
#include <ctime>
#include <string>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

/* Logging configuration */
#include <ff/logging.h>

TEST(Message, read_write_rand_int) {
  srand((unsigned int)time(nullptr));

  int x = rand();
  OutgoingMessage omsg("alice");
  EXPECT_EQ(true, omsg.write(x));

  size_t const len = omsg.length();
  IncomingMessage imsg(std::string("bob"), omsg.takeBuffer(), len);
  int y;
  EXPECT_EQ(true, imsg.read(y));

  EXPECT_EQ(x, y);
}

TEST(Message, read_write_selected_values_named_types) {
  uint8_t const a = 123;
  uint16_t const b = 12345;
  uint32_t const c = 1234567890;
  uint64_t const d = 123456781234567L;
  int8_t const e = 123;
  int16_t const f = 12345;
  int32_t const g = 1234567890;
  int64_t const h = 123456781234567L;
  int8_t const i = -123;
  int16_t const j = -12345;
  int32_t const k = -1234567890;
  int64_t const l = -123456781234567L;
  std::string const m("Hi I'm Alice");

  OutgoingMessage omsg("alice");

  EXPECT_EQ(true, omsg.write<uint8_t>(a));
  EXPECT_EQ(true, omsg.write<uint16_t>(b));
  EXPECT_EQ(true, omsg.write<uint32_t>(c));
  EXPECT_EQ(true, omsg.write<uint64_t>(d));
  EXPECT_EQ(true, omsg.write<int8_t>(e));
  EXPECT_EQ(true, omsg.write<int16_t>(f));
  EXPECT_EQ(true, omsg.write<int32_t>(g));
  EXPECT_EQ(true, omsg.write<int64_t>(h));
  EXPECT_EQ(true, omsg.write<int8_t>(i));
  EXPECT_EQ(true, omsg.write<int16_t>(j));
  EXPECT_EQ(true, omsg.write<int32_t>(k));
  EXPECT_EQ(true, omsg.write<int64_t>(l));
  EXPECT_EQ(true, omsg.write<std::string>(m));

  size_t const len = omsg.length();
  IncomingMessage imsg(std::string("bob"), omsg.takeBuffer(), len);

  uint8_t r_a;
  uint16_t r_b;
  uint32_t r_c;
  uint64_t r_d;
  int8_t r_e;
  int16_t r_f;
  int32_t r_g;
  int64_t r_h;
  int8_t r_i;
  int16_t r_j;
  int32_t r_k;
  int64_t r_l;
  std::string r_m;

  EXPECT_EQ(true, imsg.read<uint8_t>(r_a));
  EXPECT_EQ(true, imsg.read<uint16_t>(r_b));
  EXPECT_EQ(true, imsg.read<uint32_t>(r_c));
  EXPECT_EQ(true, imsg.read<uint64_t>(r_d));
  EXPECT_EQ(true, imsg.read<int8_t>(r_e));
  EXPECT_EQ(true, imsg.read<int16_t>(r_f));
  EXPECT_EQ(true, imsg.read<int32_t>(r_g));
  EXPECT_EQ(true, imsg.read<int64_t>(r_h));
  EXPECT_EQ(true, imsg.read<int8_t>(r_i));
  EXPECT_EQ(true, imsg.read<int16_t>(r_j));
  EXPECT_EQ(true, imsg.read<int32_t>(r_k));
  EXPECT_EQ(true, imsg.read<int64_t>(r_l));
  EXPECT_EQ(true, imsg.read<std::string>(r_m));

  EXPECT_EQ(a, r_a);
  EXPECT_EQ(b, r_b);
  EXPECT_EQ(c, r_c);
  EXPECT_EQ(d, r_d);
  EXPECT_EQ(e, r_e);
  EXPECT_EQ(f, r_f);
  EXPECT_EQ(g, r_g);
  EXPECT_EQ(h, r_h);
  EXPECT_EQ(i, r_i);
  EXPECT_EQ(j, r_j);
  EXPECT_EQ(k, r_k);
  EXPECT_EQ(l, r_l);
  EXPECT_EQ(m, r_m);
}

TEST(Message, read_write_selected_values_inferred_types) {
  uint8_t const a = 123;
  uint16_t const b = 12345;
  uint32_t const c = 1234567890;
  uint64_t const d = 123456781234567L;
  int8_t const e = 123;
  int16_t const f = 12345;
  int32_t const g = 1234567890;
  int64_t const h = 123456781234567L;
  int8_t const i = -123;
  int16_t const j = -12345;
  int32_t const k = -1234567890;
  int64_t const l = -123456781234567L;
  std::string const m("Hi I'm Alice");

  OutgoingMessage omsg("alice");

  EXPECT_EQ(true, omsg.write(a));
  EXPECT_EQ(true, omsg.write(b));
  EXPECT_EQ(true, omsg.write(c));
  EXPECT_EQ(true, omsg.write(d));
  EXPECT_EQ(true, omsg.write(e));
  EXPECT_EQ(true, omsg.write(f));
  EXPECT_EQ(true, omsg.write(g));
  EXPECT_EQ(true, omsg.write(h));
  EXPECT_EQ(true, omsg.write(i));
  EXPECT_EQ(true, omsg.write(j));
  EXPECT_EQ(true, omsg.write(k));
  EXPECT_EQ(true, omsg.write(l));
  EXPECT_EQ(true, omsg.write(m));

  size_t const len = omsg.length();
  IncomingMessage imsg(std::string("bob"), omsg.takeBuffer(), len);

  uint8_t r_a;
  uint16_t r_b;
  uint32_t r_c;
  uint64_t r_d;
  int8_t r_e;
  int16_t r_f;
  int32_t r_g;
  int64_t r_h;
  int8_t r_i;
  int16_t r_j;
  int32_t r_k;
  int64_t r_l;
  std::string r_m;

  EXPECT_EQ(true, imsg.read(r_a));
  EXPECT_EQ(true, imsg.read(r_b));
  EXPECT_EQ(true, imsg.read(r_c));
  EXPECT_EQ(true, imsg.read(r_d));
  EXPECT_EQ(true, imsg.read(r_e));
  EXPECT_EQ(true, imsg.read(r_f));
  EXPECT_EQ(true, imsg.read(r_g));
  EXPECT_EQ(true, imsg.read(r_h));
  EXPECT_EQ(true, imsg.read(r_i));
  EXPECT_EQ(true, imsg.read(r_j));
  EXPECT_EQ(true, imsg.read(r_k));
  EXPECT_EQ(true, imsg.read(r_l));
  EXPECT_EQ(true, imsg.read(r_m));

  EXPECT_EQ(a, r_a);
  EXPECT_EQ(b, r_b);
  EXPECT_EQ(c, r_c);
  EXPECT_EQ(d, r_d);
  EXPECT_EQ(e, r_e);
  EXPECT_EQ(f, r_f);
  EXPECT_EQ(g, r_g);
  EXPECT_EQ(h, r_h);
  EXPECT_EQ(i, r_i);
  EXPECT_EQ(j, r_j);
  EXPECT_EQ(k, r_k);
  EXPECT_EQ(l, r_l);
  EXPECT_EQ(m, r_m);
}

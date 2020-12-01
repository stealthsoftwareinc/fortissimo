/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>

/* 3rd Party Headers */
#include <sst/catalog/bignum.hpp>

/* Fortissimo Headers */
#include <mpc/ModUtils.h>
#include <mpc/templates.h>

/* Logging utilities */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<>
sst::bignum modMul(
    sst::bignum const & a,
    sst::bignum const & b,
    sst::bignum const & p) {
  sst::bignum ret;

  if (1 !=
      BN_mod_mul(
          ret.peek(),
          a.peek(),
          b.peek(),
          p.peek(),
          sst::bignum::ctx())) {
    log_error("mod multiply failed");
  }

  return ret;
}

template<>
uint32_t
modMul(uint32_t const & a, uint32_t const & b, uint32_t const & p) {
  return (uint32_t)(((uint64_t)a * (uint64_t)b) % (uint64_t)p);
}

template<>
uint64_t
modMul(uint64_t const & a, uint64_t const & b, uint64_t const & p) {
  return (uint64_t)(((__uint128_t)a * (__uint128_t)b) % (__uint128_t)p);
}

template<>
sst::bignum modAdd(
    sst::bignum const & a,
    sst::bignum const & b,
    sst::bignum const & p) {
  sst::bignum ret;

  if (1 !=
      BN_mod_add(
          ret.peek(),
          a.peek(),
          b.peek(),
          p.peek(),
          sst::bignum::ctx())) {
    log_error("mod add failed");
  }

  return ret;
}

template<>
uint32_t
modAdd(uint32_t const & a, uint32_t const & b, uint32_t const & p) {
  return (uint32_t)(((uint64_t)a + (uint64_t)b) % (uint64_t)p);
}

template<>
uint64_t
modAdd(uint64_t const & a, uint64_t const & b, uint64_t const & p) {
  return (uint64_t)(((__uint128_t)a + (__uint128_t)b) % (__uint128_t)p);
}

template<>
size_t approxLog2(uint32_t val) {
  return static_cast<size_t>(std::floor(std::log2(val)) + 1);
}

template<>
size_t approxLog2(uint64_t val) {
  return static_cast<size_t>(std::floor(std::log2(val)) + 1);
}

template<>
size_t approxLog2(sst::bignum val) {
  return static_cast<size_t>(BN_num_bits(val.peek()));
}

template<>
sst::bignum
modInvert(sst::bignum const & num, sst::bignum const & mod) {
  sst::bignum ret;
  if (nullptr ==
      BN_mod_inverse(
          ret.peek(), num.peek(), mod.peek(), sst::bignum::ctx())) {
    log_error("mod invert failed");
  }

  return ret;
}

template<>
uint32_t modInvert(uint32_t const & a, uint32_t const & modulus) {
  uint32_t r_zero = modulus;
  uint32_t r_one = a;
  uint32_t t_zero = 0;
  uint32_t t_one = 1;
  uint32_t r_two, t_two, q;

  while (r_one != 0) {
    q = r_zero / r_one;
    r_two = r_zero - q * r_one;
    t_two = t_zero - q * t_one;
    r_zero = r_one;
    t_zero = t_one;
    r_one = r_two;
    t_one = t_two;
  }
  t_zero += modulus;
  t_zero %= modulus;
  return t_zero;
}

template<>
uint64_t modInvert(uint64_t const & a, uint64_t const & modulus) {
  uint64_t r_zero = modulus;
  uint64_t r_one = a;
  uint64_t t_zero = 0;
  uint64_t t_one = 1;
  uint64_t r_two, t_two, q;

  while (r_one != 0) {
    q = r_zero / r_one;
    r_two = r_zero - q * r_one;
    t_two = t_zero - q * t_one;
    r_zero = r_one;
    t_zero = t_one;
    r_one = r_two;
    t_one = t_two;
  }
  t_zero += modulus;
  t_zero %= modulus;
  return t_zero;
}

} // namespace mpc
} // namespace ff

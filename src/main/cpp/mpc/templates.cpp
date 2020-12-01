/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <string>

/* 3rd Party Headers */
#include <openssl/bn.h>
#include <sst/catalog/bignum.hpp>

/* Fortissimo Headers */
#include <mpc/templates.h>

namespace ff {
namespace mpc {

template<>
std::string dec(sst::bignum const & num) {
  return num.to_string();
}

template<>
std::string dec(uint32_t const & num) {
  return std::to_string(num);
}

template<>
std::string dec(uint64_t const & num) {
  return std::to_string(num);
}

LargeNum largeNumFromHex(std::string const & str) {
  LargeNum num;
  auto * bnp = num.peek();

  BN_hex2bn(&bnp, str.c_str());

  return num;
}

template<>
size_t numberLen(sst::bignum const & num) {
  return sizeof(uint16_t) + (size_t)BN_num_bytes(num.peek());
}

template<>
size_t numberLen(uint32_t const &) {
  return sizeof(uint32_t);
}

template<>
size_t numberLen(uint64_t const &) {
  return sizeof(uint64_t);
}

} // namespace mpc
} // namespace ff

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#include <cstdint>
#include <cstdlib>

#include <limits>

#include <ff/Util.h>

namespace ff {

void uint64_to_buffer(uint64_t const in, uint8_t * out) {
  for (size_t i = 0; i < sizeof(uint64_t); i++) {
    size_t const shift = (sizeof(uint64_t) - 1 - i) *
        std::numeric_limits<uint8_t>::digits;
    out[i] = (uint8_t)((in >> shift) & 0xFF);
  }
}

uint64_t buffer_to_uint64(uint8_t const * in) {
  uint64_t out = 0;
  for (size_t i = 0; i < sizeof(uint64_t); i++) {
    size_t const shift = (sizeof(uint64_t) - 1 - i) *
        std::numeric_limits<uint8_t>::digits;
    out = out | (uint64_t)((uint64_t)in[i] << shift);
  }

  return out;
}

template<>
::std::string
identity_to_string<::std::string>(std::string const & id) {
  return ::std::string(id);
}

template<>
::std::string
identity_to_string<unsigned char>(unsigned char const & id) {
  return ::std::to_string(id);
}

template<>
::std::string
identity_to_string<unsigned short>(unsigned short const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<unsigned>(unsigned const & id) {
  return ::std::to_string(id);
}

template<>
::std::string
identity_to_string<unsigned long>(unsigned long const & id) {
  return ::std::to_string(id);
}

template<>
::std::string
identity_to_string<unsigned long long>(unsigned long long const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<char>(char const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<short>(short const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<int>(int const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<long>(long const & id) {
  return ::std::to_string(id);
}

template<>
::std::string identity_to_string<long long>(long long const & id) {
  return ::std::to_string(id);
}

} // namespace ff

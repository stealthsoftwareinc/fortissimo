/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_UTIL_H_
#define FF_UTIL_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <string>

/* 3rd Party Headers */

/* Fortissimo Headers */

namespace ff {

/* Convert buffers (must be sized for 64 bits) into uint64_t and back */
void uint64_to_buffer(uint64_t const, uint8_t *);
uint64_t buffer_to_uint64(uint8_t const *);

using fronctocolId_t = uint64_t;
constexpr fronctocolId_t FRONCTOCOLID_INVALID = UINT64_MAX;

#define fronctocolId_to_buffer(fid, buf) \
  ::ff::uint64_to_buffer((fid), (buf))
#define buffer_to_fronctocolId(buf) ::ff::buffer_to_uint64(buf)

/* Convert an Identity_T to a string. Used for error messages. */
template<typename Identity_T>
::std::string identity_to_string(Identity_T const & id);

} // namespace ff

#endif // FF_UTIL_H_

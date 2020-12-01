/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <string>

/* 3rd Party Headers */
#include <sst/catalog/bignum.hpp>

/* Fortissimo Headers */

/**
 * These macros list out the four repeated template types which all
 * Fronctocols are templated upon.
 */

#ifndef FF_TYPENAMES
#define FF_TYPENAMES \
  typename Identity_T, typename PeerSet_T, typename IncomingMessage_T, \
      typename OutgoingMessage_T
#endif

#ifndef FF_TYPES
#define FF_TYPES \
  Identity_T, PeerSet_T, IncomingMessage_T, OutgoingMessage_T
#endif

#ifndef FF_MPC_TEMPLATES_H_
#define FF_MPC_TEMPLATES_H_

using ArithmeticShare_t = uint32_t;
using Boolean_t = uint8_t;

namespace ff {
namespace mpc {

using LargeNum = sst::bignum;
using SmallNum = uint32_t;

template<typename Number_T>
std::string dec(Number_T const & num);

LargeNum largeNumFromHex(std::string const & str);

template<typename Number_T>
size_t numberLen(Number_T const & num);

} // namespace mpc
} // namespace ff

#endif

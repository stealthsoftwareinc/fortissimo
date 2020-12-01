/**
 * Copyright Stealth Software Technologies, Inc.
 */

#include <mpc/simplePrime.h>
#include <mpc/templates.h>

#include <openssl/bn.h>
#include <sst/catalog/bignum.hpp>

#include <ff/logging.h>

namespace ff {
namespace mpc {

template<>
bool isPrime<LargeNum>(LargeNum value) {
  return (
      BN_is_prime_ex(
          value.peek(), BN_prime_checks, sst::bignum::ctx(), nullptr) ==
      1);
}

} // namespace mpc
} // namespace ff
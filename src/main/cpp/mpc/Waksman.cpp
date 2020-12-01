/**
 * Copyright Stealth Software Technologies, Inc.
 */
/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <vector>
/* 3rd Party Headers */

/* Fortissimo Headers */
#include <mpc/Randomness.h>
#include <mpc/Waksman.h>
#include <mpc/templates.h>

/* Logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

size_t toggle(size_t current_d, size_t i) {
  size_t outer_residue = i % (1 << current_d);
  size_t outer_quotient = i >> current_d;
  size_t inner_residue = outer_quotient % 2;
  size_t inner_quotient = outer_quotient >> 1;

  log_debug(
      "Calling toggle on %zu, %zu, and returning %lu ",
      current_d,
      i,
      (((inner_quotient << 1) + (inner_residue ^ 1)) << current_d) +
          outer_residue);

  return (((inner_quotient << 1) + (inner_residue ^ 1)) << current_d) +
      outer_residue;
}

} // namespace mpc
} // namespace ff

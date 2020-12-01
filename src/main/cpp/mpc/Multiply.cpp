/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <mpc/Multiply.h>
#include <mpc/templates.h>

/* Logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

void BooleanBeaverInfo::generate(
    size_t n_parties,
    size_t,
    std::vector<BeaverTriple<Boolean_t>> & vals) const {
  /** Step 1. Randomly create the "original" randomness instance. */
  Boolean_t const a = randomByte();
  Boolean_t const b = randomByte();
  Boolean_t const c = (a & b);

  BeaverTriple<Boolean_t> const og(a, b, c);

  /** Step 2. randomly secret share the original. */
  vals.reserve(n_parties);
  for (size_t i = 1; i < n_parties; i++) {
    Boolean_t sa = randomByte();
    Boolean_t sb = randomByte();
    Boolean_t sc = randomByte();

    vals.emplace_back(sa, sb, sc);
  }

  /** Step 3. The last secret share is computed from the priors. */
  Boolean_t la = og.a;
  Boolean_t lb = og.b;
  Boolean_t lc = og.c;
  for (BeaverTriple<Boolean_t> & prev : vals) {
    la = la ^ prev.a;
    lb = lb ^ prev.b;
    lc = lc ^ prev.c;
  }
  vals.emplace_back(la, lb, lc);

  if_debug {
    Boolean_t ta = vals[0].a;
    Boolean_t tb = vals[0].b;
    Boolean_t tc = vals[0].c;
    for (size_t i = 1; i < vals.size(); i++) {
      ta = ta ^ vals[i].a;
      tb = tb ^ vals[i].b;
      tc = tc ^ vals[i].c;
    }

    log_assert(ta == og.a);
    log_assert(tb == og.b);
    log_assert(tc == og.c);
  }
}

} // namespace mpc
} // namespace ff

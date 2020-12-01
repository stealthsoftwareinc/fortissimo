/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 * Reconstructs a value from additive shares (either XOR or mod p)
 */

#ifndef FF_MPC_REVEAL_H_
#define FF_MPC_REVEAL_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>
#include <utility>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
class Reveal : public Fronctocol<FF_TYPES> {
public:
  std::string name() override;

  Number_T openedValue;

  Reveal(
      Number_T const & share,
      Number_T const & mod,
      const Identity_T * rev) :
      openedValue(share), modulus(mod), revealer(rev) {
  }

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(ff::Fronctocol<FF_TYPES> & f) override;
  void handlePromise(ff::Fronctocol<FF_TYPES> & f) override;

private:
  Number_T modulus;
  const Identity_T * revealer;
  size_t numOutstandingMessages =
      0; // used for tracking "state machine" behavior
};

} // namespace mpc
} // namespace ff

#include <mpc/Reveal.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif //FF_MPC_REVEAL_H_

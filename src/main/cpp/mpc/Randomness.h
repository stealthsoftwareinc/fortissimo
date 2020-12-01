/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_RANDOMNESS_H_
#define FF_MPC_RANDOMNESS_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/* 3rd Party Headers */
#include <openssl/err.h>
#include <openssl/rand.h>

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Promise.h>
#include <mpc/ModUtils.h>
#include <mpc/templates.h>

/* logging configuration */
#include <ff/logging.h>

/**
 * Randomness.h defines a RandomnessGenerator Fronctocol Interface
 * which requests an amount of some randomness instance type of
 * the dealer or of some online process. When the RandomnessGenerator
 * Fronctocol completes, the caller can take a RandomnessDispenser
 * from it. The Dispenser is a container for the generated randomness
 * instances and deals them in order, one at a time.
 *
 * Both RandomnessGenerator and RandomDispenser are templatized to
 * the randomness instance type (e.g. BeaverTriple), and to a metadata
 * type describing restrictions or classifications of the randomness
 * instance type (e.g. the prime modulus).
 */

namespace ff {
namespace mpc {

/**
 * pseudo random generator for use during testing.
 */
int pseudo_rand_bytes(uint8_t * buffer, size_t length);

/**
 * Randomly generate a number mod p on the uniform distribution.
 */
template<typename Number_T>
Number_T randomModP(Number_T const & p);
template<>
LargeNum randomModP(LargeNum const &);

/**
 * Randomly generate a random byte
 */
Boolean_t randomByte();

/**
 * Randomly generate a sequence of bytes.
 */
bool randomBytes(void * buffer, size_t const nbytes);

/**
 * Secret share a number arithmetically.
 */
template<typename Number_T>
void arithmeticSecretShare(
    size_t const n_parties,
    Number_T const & p,
    Number_T const & num,
    ::std::vector<Number_T> & shares);

/**
 * Secret share a number using XOR.
 */
template<typename Number_T>
void xorSecretShare(
    size_t const n_parties,
    Number_T const & num,
    ::std::vector<Number_T> & shares);

struct DoNotGenerateInfo {

  size_t instanceSize() const {
    return 0;
  }

  void generate(size_t, size_t, std::vector<uint32_t> &) {
    log_error("Do not use this");
    log_assert(1 == 0);
  }

  bool operator==(DoNotGenerateInfo const &) const {
    return true;
  }
  bool operator!=(DoNotGenerateInfo const & other) const {
    return !(*this == other);
  }
  DoNotGenerateInfo() = default;
};

/**
 * The RandomnessDispenser contains many instances of randomness and
 * deals them one at a time.
 */
template<typename Rand_T, typename Info_T>
class RandomnessDispenser {
public:
  using Randomness_T = Rand_T;
  using Information_T = Info_T;

  Information_T const info;

  RandomnessDispenser(Information_T const & i);

  /**
   * Insert a randomness instance into this dispenser. Intended for
   * use by the RandomnessGenerator fronctocol.
   */
  void insert(Randomness_T const & val);

  /**
   * Insert a randomness instance into this dispenser. Intended for
   * use by the RandomnessGenerator fronctocol.
   */
  void insert(Randomness_T && val);

  /**
   * Dispense one instance of randomness.
   */
  Randomness_T get();

  /**
   * How many instances of randomness remain available.
   */
  size_t size();

  /**
   * Shrink the container to the minimum possible size without removing
   * valid elements
   */
  void shrink();

  /**
   * Creates a smaller dispenser by removing randomness from this
   * dispenser.
   *
   * If there isn't enough randomness available, nullptr is returned.
   */
  std::unique_ptr<RandomnessDispenser<Randomness_T, Information_T>>
  littleDispenser(size_t const n);

  void clear() {
    this->values.clear();
  }

private:
  std::deque<Randomness_T> values;
};

/**
 * Fronctocol Abstraction for producing RandomnessDispensers.
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Rand_T,
    typename Info_T>
class RandomnessGenerator : public PromiseFronctocol<
                                Identity_T,
                                PeerSet_T,
                                IncomingMessage_T,
                                OutgoingMessage_T,
                                RandomnessDispenser<Rand_T, Info_T>> {
public:
  using Randomness_T = Rand_T;
  using Information_T = Info_T;

  size_t numDesired;
  Information_T const info;

  RandomnessGenerator(size_t num, Information_T const & i);
};

#include <mpc/Randomness.t.h>

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_RANDOMNESS_H_

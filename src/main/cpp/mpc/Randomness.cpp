/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <functional>
#include <random>
#include <stdexcept>

/* 3rd Party Headers */
#include <openssl/bn.h>

/* Fortissimo Headers */
#include <mpc/Randomness.h>
#include <mpc/templates.h>

/* logging config */
#include <ff/logging.h>

namespace ff {
namespace mpc {

const uint64_t seed = 21;

static auto rand_gen = std::bind(
    std::uniform_int_distribution<uint64_t>(0, 256),
    std::default_random_engine(seed));

int pseudo_rand_bytes(uint8_t * buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
    buffer[i] = static_cast<uint8_t>(rand_gen());
  }
  return 1;
}

constexpr size_t RANDOM_BUFFER_SIZE = 65536;
static thread_local std::array<unsigned char, RANDOM_BUFFER_SIZE>
    random_buffer;
static thread_local size_t random_buffer_place = RANDOM_BUFFER_SIZE;

bool randomBytes(void * buffer, size_t const nbytes) {
  int status = 1;

  if (nbytes > RANDOM_BUFFER_SIZE) {
    status = RAND_bytes((unsigned char *)buffer, (int)nbytes);
  } else {
    if (RANDOM_BUFFER_SIZE - random_buffer_place < nbytes) {
      status =
          RAND_bytes(random_buffer.data(), (int)random_buffer_place);
      random_buffer_place = 0;
    }

    memcpy(buffer, random_buffer.data() + random_buffer_place, nbytes);
    random_buffer_place = random_buffer_place + nbytes;
  }

  if (1 != status) {
    unsigned long err = ERR_get_error();
    log_error(
        "Error generating uniform random: %lu %s",
        err,
        ERR_error_string(err, nullptr));
    return false;
  }
  return true;
}

Boolean_t randomByte() {
  Boolean_t try_rand;
  if (!randomBytes(&try_rand, 1)) {
    throw std::runtime_error("randomness failed");
  }
  return try_rand;
}

static thread_local LargeNum prev_p(0);
static thread_local LargeNum fill_p;
static thread_local std::vector<unsigned char> largenum_rand;

template<>
LargeNum randomModP(LargeNum const & p) {
  LargeNum try_rand;

  if (prev_p != p) {
    size_t const p_bytes = (size_t)BN_num_bytes(p.peek());
    std::vector<unsigned char> empty(0, 0);
    largenum_rand.swap(empty); // clear, but no reallocations
    largenum_rand.resize(p_bytes, 0xff);
    BN_bin2bn(
        largenum_rand.data(), (int)largenum_rand.size(), fill_p.peek());
  }

  do {
    if (!randomBytes(largenum_rand.data(), largenum_rand.size())) {
      throw std::runtime_error("bad randomness");
    }
    BN_bin2bn(
        largenum_rand.data(),
        (int)largenum_rand.size(),
        try_rand.peek());
  } while (try_rand >= p * (fill_p / p));
  return try_rand % p;
}

} // namespace mpc
} // namespace ff

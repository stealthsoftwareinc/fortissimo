/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <ff/VectorPeerSet.h>
#include <ff/posixnet/posixnet.h>
#include <mpc/templates.h>

/* Logging configuration */
#include <ff/logging.h>

const uint64_t prime = 23;
const uint64_t base = 5;

template<FF_TYPENAMES>
class CheckField : public ff::Fronctocol<FF_TYPES> {
public:
  std::string name() override {
    return std::string("Check Field");
  }

  void init() override {
    this->getPeers().forEach([this](Identity_T const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(peer));
      omsg->template write<uint64_t>(prime);
      omsg->template write<uint64_t>(base);

      this->send(std::move(omsg));
    });
  }

  void handleReceive(IncomingMessage_T & imsg) override {
    uint64_t other_prime;
    imsg.template read<uint64_t>(other_prime);
    uint64_t other_base;
    imsg.template read<uint64_t>(other_base);

    if (prime != other_prime || base != other_base) {
      log_error("prime or base does not match");
      this->abort();
      return;
    }

    this->complete();
  }

  void handleComplete(ff::Fronctocol<FF_TYPES> &) override {
    log_error("check field shouldn't have a complete");
  }

  void handlePromise(ff::Fronctocol<FF_TYPES> &) override {
    log_error("check field shouldn't have a promise");
  }
};

static auto field_rand = std::bind(
    std::uniform_int_distribution<uint64_t>(0, prime / 2),
    std::default_random_engine(
        (uint64_t)::std::chrono::system_clock::now()
            .time_since_epoch()
            .count()));

template<FF_TYPENAMES>
class DiffieHellman : public ff::Fronctocol<FF_TYPES> {
public:
  std::string name() override {
    return std::string("Diffie Hellman");
  }

  uint64_t myrand = 0;
  uint64_t * returnSecret = nullptr;

  DiffieHellman(uint64_t * ret_sec) : returnSecret(ret_sec) {
  }

  void init() override {
    std::unique_ptr<ff::Fronctocol<FF_TYPES>> cf(
        new CheckField<FF_TYPES>());
    PeerSet_T ps(this->getPeers());

    this->invoke(std::move(cf), ps);

    this->myrand = field_rand();

    log_info("Using secret %lu", this->myrand);
  }

  void handleReceive(IncomingMessage_T & imsg) override {
    uint64_t recv_num;
    imsg.read(recv_num);

    uint64_t output = 1;
    for (size_t i = 0; i < this->myrand; i++) {
      output = output * recv_num % prime;
    }

    log_info("Shared secret is %lu", output);
    *this->returnSecret = output;
    this->complete();
  }

  void handleComplete(ff::Fronctocol<FF_TYPES> &) override {
    log_info("handled complete from check field");

    this->getPeers().forEach([this](Identity_T const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage_T> omsg(
          new OutgoingMessage_T(peer));

      uint64_t send_num = 1;
      for (size_t i = 0; i < this->myrand; i++) {
        send_num = send_num * base % prime;
      }

      omsg->template write<uint64_t>(send_num);

      this->send(std::move(omsg));
    });
  }

  void handlePromise(ff::Fronctocol<FF_TYPES> &) override {
    log_error("unused");
  }
};

TEST(DiffieHellman, string_identity_type) {
  std::map<std::string, std::unique_ptr<Fronctocol>> tests;
  uint64_t a_val = UINT64_MAX;
  uint64_t b_val = UINT64_MAX;
  tests["alice"] = std::unique_ptr<Fronctocol>(
      new DiffieHellman<TEST_TYPES>(&a_val));
  tests["bob"] = std::unique_ptr<Fronctocol>(
      new DiffieHellman<TEST_TYPES>(&b_val));

  EXPECT_TRUE(runTests(tests));

  EXPECT_NE(UINT64_MAX, a_val);
  EXPECT_EQ(a_val, b_val);
}

#define NUMERIC_TYPES \
  size_t, ff::VectorPeerSet<size_t>, \
      ff::posixnet::IncomingMessage<size_t>, \
      ff::posixnet::OutgoingMessage<size_t>

static std::function<
    std::unique_ptr<ff::posixnet::IncomingMessage<size_t>>(
        size_t const &, ff::posixnet::OutgoingMessage<size_t> &)>
    numeric_message_converter =
        ff::posixnet::outgoingToIncomingMessage<size_t>;

TEST(DiffieHellman, number_identity_type) {
  std::map<size_t, std::unique_ptr<ff::Fronctocol<NUMERIC_TYPES>>>
      tests;
  uint64_t a_val = UINT64_MAX;
  uint64_t b_val = UINT64_MAX;
  tests[0] = std::unique_ptr<ff::Fronctocol<NUMERIC_TYPES>>(
      new DiffieHellman<NUMERIC_TYPES>(&a_val));
  tests[1] = std::unique_ptr<ff::Fronctocol<NUMERIC_TYPES>>(
      new DiffieHellman<NUMERIC_TYPES>(&b_val));

  bool success = ff::tester::runTests<NUMERIC_TYPES>(
      tests, numeric_message_converter);
  EXPECT_TRUE(success);

  EXPECT_NE(UINT64_MAX, a_val);
  EXPECT_EQ(a_val, b_val);
}

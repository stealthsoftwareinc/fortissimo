/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <mpc/Batch.h>

/* Logging configuration */
#include <ff/logging.h>

using namespace ff::mpc;

class Nope : public Fronctocol {
public:
  std::string name() override {
    return std::string("Nope");
  }

  Nope() {
    log_debug("Nope constructor");
  }

  void init() override {
    log_debug("Nope init");
    this->getPeers().forEach([this](std::string const & peer) {
      if (peer != this->getSelf()) {
        std::unique_ptr<OutgoingMessage> omsg(
            new OutgoingMessage(peer));
        omsg->write<uint32_t>(4);
        this->send(std::move(omsg));
      }
    });
  }

  void handleReceive(IncomingMessage & im) override {
    log_debug("Nope handle recieve");
    uint32_t x;
    im.read<uint32_t>(x);
    log_assert(4 == x);

    this->complete();
  }

  void handleComplete(Fronctocol &) override {
    log_debug("Nope handle complete");
    this->abort();
  }

  void handlePromise(Fronctocol &) override {
    log_debug("Nope handle promise");
    this->abort();
  }
};

class ExchangeInt : public Fronctocol {
public:
  uint32_t * in;
  uint32_t * out;

  bool exchangeComplete = false;
  bool childComplete = false;

  std::string name() override {
    return std::string("Exchange Int");
  }

  void init() override;

  void handleReceive(IncomingMessage & im) override;

  void handleComplete(Fronctocol & f) override;

  void handlePromise(Fronctocol &) override {
    log_debug("Exchange int handle promise");
    this->abort();
  }

  ExchangeInt(uint32_t * i, uint32_t * o) : in(i), out(o) {
    log_debug("exchange int constructor");
  }
};

void ExchangeInt::init() {
  log_debug("Exchange int init");
  this->getPeers().forEach([this](std::string const & peer) {
    if (this->getSelf() == peer) {
      return;
    }

    std::unique_ptr<OutgoingMessage> om(new OutgoingMessage(peer));
    om->write(*this->in);
    *this->in = 0;

    this->send(std::move(om));
  });

  this->invoke(
      std::unique_ptr<Fronctocol>(new Nope()), this->getPeers());
}

void ExchangeInt::handleReceive(IncomingMessage & im) {
  log_debug("Exchange int handle receive");
  im.read(*this->out);
  this->exchangeComplete = true;

  if (this->exchangeComplete && this->childComplete) {
    this->complete();
  }
}

void ExchangeInt::handleComplete(Fronctocol &) {
  log_debug("Exchange int handle complete");
  this->childComplete = true;

  if (this->exchangeComplete && this->childComplete) {
    this->complete();
  }
}

TEST(Batch, Batch) {
  std::vector<uint32_t> l_ins;
  for (uint32_t i = 0; i < 10; i++) {
    l_ins.push_back(i);
  }

  std::vector<uint32_t> r_ins;
  for (uint32_t i = 20; i < 30; i++) {
    r_ins.push_back(i);
  }

  std::vector<uint32_t> l_outs(r_ins.size(), UINT32_MAX);
  std::vector<uint32_t> r_outs(l_ins.size(), UINT32_MAX);

  EXPECT_EQ(l_outs.size(), r_outs.size());
  EXPECT_EQ(10, r_outs.size());

  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  test["alice"] = std::unique_ptr<Fronctocol>(new Batch<TEST_TYPES>());
  for (size_t i = 0; i < 10; i++) {
    static_cast<Batch<TEST_TYPES> &>(*test["alice"])
        .children.emplace_back(new ExchangeInt(&l_ins[i], &l_outs[i]));
  }

  test["bob"] = std::unique_ptr<Fronctocol>(new Batch<TEST_TYPES>());
  for (size_t i = 0; i < 10; i++) {
    static_cast<Batch<TEST_TYPES> &>(*test["bob"])
        .children.emplace_back(new ExchangeInt(&r_ins[i], &r_outs[i]));
  }

  EXPECT_TRUE(runTests(test));

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(i, r_outs[i]);
  }

  for (uint32_t i = 0, j = 20; i < 10; i++, j++) {
    EXPECT_EQ(j, l_outs[i]);
  }

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(0, r_ins[i]);
  }

  for (uint32_t i = 0; i < 10; i++) {
    EXPECT_EQ(0, l_ins[i]);
  }
}

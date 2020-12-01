/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>

/* logging configuration */
#include <ff/logging.h>

class Aborter : public Fronctocol {
public:
  std::string name() override {
    return std::string("Aborter");
  }

  void init() override {
    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage> omsg(new OutgoingMessage(peer));
      omsg->write<uint64_t>(0);

      this->send(std::move(omsg));
    });
  }

  void handleReceive(IncomingMessage & imsg) override {
    uint64_t val;
    imsg.read<uint64_t>(val);

    if (this->getSelf() == "alice") {
      this->abort();
    } else {
      this->complete();
    }
  }

  void handleComplete(Fronctocol &) override {
    log_error("check field shouldn't have a complete");
  }

  void handlePromise(Fronctocol &) override {
    log_error("check field shouldn't have a promise");
  }
};

TEST(abort, abort) {
  std::map<std::string, std::unique_ptr<Fronctocol>> tests;
  tests["alice"] = std::unique_ptr<Fronctocol>(new Aborter());
  tests["bob"] = std::unique_ptr<Fronctocol>(new Aborter());

  EXPECT_FALSE(runTests(tests));
}

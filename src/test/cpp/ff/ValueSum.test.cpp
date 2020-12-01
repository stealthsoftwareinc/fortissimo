/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>

/* Logging config */
#include <ff/logging.h>

namespace dataowner {

class ExchangeIntFronctocol : public Fronctocol {
  uint32_t myVal;

public:
  std::string name() override {
    return std::string("ExchangeIntFronctocol");
  }

  uint32_t peerVal;
  ExchangeIntFronctocol(uint32_t myVal) : myVal(myVal) {
  }

  void init() override {
    log_debug("Dataowner Exchange Int Fronctocol init");

    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage> omsg(new OutgoingMessage(peer));
      omsg->write<uint32_t>(this->myVal);
      this->send(std::move(omsg));
    });
  }

  void handleReceive(IncomingMessage & imsg) override {
    log_debug("Dataowner Exchange Int Fronctocol handle receive");
    imsg.read<uint32_t>(this->peerVal);

    this->complete();
  }

  void handleComplete(Fronctocol &) override {
    log_error(
        "Dataowner Exchange Int Fronctocol shouldn't get complete");
  }
  void handlePromise(Fronctocol &) override {
    log_error(
        "Dataowner Exchange Int Fronctocol shouldn't get promise");
  }
};

class SendIntFronctocol : public Fronctocol {
  uint32_t sendVal;

public:
  std::string name() override {
    return std::string("SendIntFronctocol");
  }

  SendIntFronctocol(uint32_t send) : sendVal(send) {
  }

  void init() override {
    log_debug("Dataowner Send Int Fronctocol init");

    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage> omsg(new OutgoingMessage(peer));
      omsg->write<uint32_t>(this->sendVal);
      this->send(std::move(omsg));
    });

    this->complete();
  }
  void handleReceive(IncomingMessage &) override {
    log_error("Dataowner Send Int Fronctocol received an unexpected "
              "incoming message");
  }
  void handleComplete(Fronctocol &) override {
    log_error("Dataowner Send Int Fronctocol shouldn't get complete");
  }
  void handlePromise(Fronctocol &) override {
    log_error("Dataowner Send Int Fronctocol shouldn't get promise");
  }
};

class ValueSumFronctocol : public Fronctocol {
public:
  uint32_t myValue;
  uint32_t resultShare = 0;
  size_t expectedRandCount = 0;
  size_t expectedSendReturns = 0;

  std::string name() override {
    return std::string("ValueSumFronctocol");
  }

  ValueSumFronctocol(uint32_t val) : myValue(val) {
  }

  void init() override {
    log_info(
        "Dataowner Value Sum Fronctocol init. My Value is %u",
        this->myValue);

    this->resultShare += this->myValue;

    /* for each dataowner peer */
    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == "recipient" || peer == this->getSelf()) {
        return;
      }

      uint32_t sent_rand = (uint32_t)rand();
      this->resultShare += sent_rand;
      this->expectedRandCount++;

      std::unique_ptr<ExchangeIntFronctocol> eif(
          new ExchangeIntFronctocol(sent_rand));
      PeerSet ps;

      ps.add(peer);
      ps.add(this->getSelf());

      this->invoke(std::move(eif), ps);
      log_info("invoked EIF");
    });
  }

  void handleReceive(IncomingMessage &) override {
    /* Does not expect any incoming messages */
    log_error("Dataowner Value Sum Fronctocol did not expect any "
              "incoming messages");
  }

  void handleComplete(Fronctocol & f) override {
    log_debug("Dataowner Value Sum Fronctocol handle return");

    /* exchange int (with other dataowners gives a non-null return,
     * send int (to recipients) returns null.
     */
    if (this->expectedRandCount > 0) {
      this->resultShare -=
          static_cast<ExchangeIntFronctocol &>(f).peerVal;
      this->expectedRandCount--;

      if (this->expectedRandCount == 0) {
        this->getPeers().forEach([this](std::string const & peer) {
          if (peer != "recipient") {
            return;
          }

          std::unique_ptr<SendIntFronctocol> sif(
              new SendIntFronctocol(this->resultShare));
          PeerSet ps;
          ps.add(peer);
          ps.add(this->getSelf());

          log_debug("dataowner invoking send int fronctocol");
          this->invoke(std::move(sif), ps);
          this->expectedSendReturns++;
        });
      }
    } else {
      this->expectedSendReturns--;
      if (this->expectedSendReturns == 0) {
        log_info("Dataowner Value Sum Fronctocol completed success");
        this->complete();
      }
    }
  }
  void handlePromise(Fronctocol &) override {
    log_error("Dataowner Value Sum Fronctocol shouldn't get promise");
  }
};

} // namespace dataowner

namespace recipient {

class ReceiveIntFronctocol : public Fronctocol {
public:
  std::string name() override {
    return std::string("ReceiveIntFronctocol");
  }

  uint32_t recvVal;

  void init() override {
    log_debug("Recipient Receive Int Fronctocol init");
  }

  void handleReceive(IncomingMessage & msg) override {
    log_debug("Recipient Receive Int Fronctocol handle receive");
    msg.read<uint32_t>(this->recvVal);

    this->complete();
  }

  void handleComplete(Fronctocol &) override {
    log_error(
        "Recipient Recieve Int Fronctocol shouldn't get complete");
  }
  void handlePromise(Fronctocol &) override {
    log_error("Recipient Recieve Int Fronctocol shouldn't get promise");
  }
};

class ValueSumFronctocol : public Fronctocol {
  size_t numReceives = 0;

public:
  std::string name() override {
    return std::string("Recipient ValueSum");
  }

  uint32_t value = 0;
  void init() override {
    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<ReceiveIntFronctocol> rif(
          new ReceiveIntFronctocol());
      PeerSet ps;
      ps.add(this->getSelf());
      ps.add(peer);
      this->invoke(std::move(rif), ps);

      this->numReceives++;
    });

    log_info("Recipient Value Sum Fronctocol init");
  }

  void handleReceive(IncomingMessage &) override {
    log_error("Recipient Value Sum Fronctocol shouldn't get message");
  }
  void handleComplete(Fronctocol & f) override {
    log_debug("Recipient Value Sum Fronctocol handle return");

    ReceiveIntFronctocol & rif = static_cast<ReceiveIntFronctocol &>(f);

    this->numReceives--;
    this->value += rif.recvVal;

    if (this->numReceives == 0) {
      log_info(
          "Recipient Value Sum Froctocol received value: %u",
          this->value);
      this->complete();
    }
  }
  void handlePromise(Fronctocol &) override {
    log_error("Recipient Value Sum Fronctocol shouldn't get promise");
  }
};

} // namespace recipient

TEST(ValueSum, ValueSum) {
  std::map<std::string, std::unique_ptr<Fronctocol>> tests;
  tests["alice"] = std::unique_ptr<Fronctocol>(
      new dataowner::ValueSumFronctocol(1234));
  tests["bob"] = std::unique_ptr<Fronctocol>(
      new dataowner::ValueSumFronctocol(5678));
  tests["recipient"] =
      std::unique_ptr<Fronctocol>(new recipient::ValueSumFronctocol());

  EXPECT_TRUE(runTests(tests));
}

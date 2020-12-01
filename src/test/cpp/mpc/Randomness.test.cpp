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
#include <vector>

/* 3rd Party Headers */
#include <gtest/gtest.h>

/* Fortissimo Headers */
#include <mock.h>

#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <ff/Promise.h>
#include <mpc/Randomness.h>
#include <mpc/RandomnessDealer.h>

/* Logging Configuration */
#include <ff/logging.h>

using namespace ff::mpc;

TEST(Randomness, Dispenser) {
  RandomnessDispenser<uint32_t, uint32_t> rd(1234);
  EXPECT_EQ(1234, rd.info);
  rd.insert(1);
  EXPECT_EQ(1, rd.get());

  rd.insert(3);
  rd.insert(4);
  rd.insert(5);
  rd.insert(6);

  EXPECT_EQ(4, rd.size());
  ::std::unique_ptr<RandomnessDispenser<uint32_t, uint32_t>> ld =
      rd.littleDispenser(3);
  EXPECT_EQ(1, rd.size());

  EXPECT_EQ(3, ld->size());
  EXPECT_EQ(3, ld->get());
  EXPECT_EQ(2, ld->size());
  EXPECT_EQ(4, ld->get());
  EXPECT_EQ(1, ld->size());
  EXPECT_EQ(5, ld->get());
  EXPECT_EQ(0, ld->size());

  EXPECT_EQ(1, rd.size());
  EXPECT_EQ(6, rd.get());
  EXPECT_EQ(0, rd.size());

  EXPECT_EQ(nullptr, rd.littleDispenser(3));
}

TEST(Randomness, how_to_arithmetic_secret_share) {
  size_t n_parties = 4;
  uint32_t const p = 31;
  uint32_t const og = ff::mpc::randomModP<uint32_t>(p);

  std::vector<uint32_t> shares;
  shares.reserve(n_parties);
  for (size_t i = 1; i < n_parties; i++) {
    shares.push_back(ff::mpc::randomModP<uint32_t>(p));
  }
  EXPECT_EQ(n_parties - 1, shares.size());

  uint32_t last = og;
  for (uint32_t const s : shares) {
    last = (last + (p - s)) % p;
  }
  shares.push_back(last);

  EXPECT_EQ(n_parties, shares.size());

  uint32_t reconst = 0;
  for (uint32_t s : shares) {
    reconst = (reconst + s) % p;
  }

  EXPECT_EQ(og, reconst);
}

TEST(Randomness, arithmeticSecretShare) {
  uint32_t const p = 97;
  uint32_t og = ff::mpc::randomModP<uint32_t>(p);

  std::vector<uint32_t> shares;
  arithmeticSecretShare(4 /* parties */, p, og, shares);

  uint32_t check = 0;
  for (uint32_t share : shares) {
    check = (check + share) % p;
  }
  EXPECT_EQ(og, check);
}

TEST(Randomness, xorSecretShare) {

  for (uint8_t i = 0; i < UINT8_MAX; i++) {
    std::vector<uint8_t> shares;
    xorSecretShare(4 /* parties */, i, shares);

    uint32_t check = 0;
    for (uint32_t share : shares) {
      check = check ^ share;
    }
    EXPECT_EQ(i, check);
  }
}

struct Uint32Info;

struct Uint32Rand {
  uint32_t rand;

  Uint32Rand(uint32_t r) : rand(r) {
  }
  Uint32Rand(Uint32Rand const &) = default;
  Uint32Rand() = default;

  Uint32Rand(Uint32Info const &) : Uint32Rand() {
  }

  static std::string name() {
    return std::string("Uint32Rand");
  }
};

struct Uint32Info {
  uint32_t prime = 101;

  size_t instanceSize() const {
    return sizeof(uint32_t);
  }

  void generate(
      size_t n_parties, size_t, std::vector<Uint32Rand> & vals) const {
    uint32_t num = ff::mpc::randomModP<uint32_t>(this->prime);

    for (size_t i = 0; i < n_parties; i++) {
      vals.emplace_back(num);
    }
  }

  bool operator==(Uint32Info const & other) const {
    return this->prime == other.prime;
  }

  bool operator!=(Uint32Info const & other) const {
    return !(*this == other);
  }
};

namespace ff {

template<typename Identity_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg, Uint32Info & info) {
  return msg.template read<uint32_t>(info.prime);
}

template<typename Identity_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg, Uint32Info const & info) {
  return msg.template write<uint32_t>(info.prime);
}

template<typename Identity_T>
bool msg_read(
    ff::IncomingMessage<Identity_T> & msg, Uint32Rand & rand) {
  return msg.template read<uint32_t>(rand.rand);
}

template<typename Identity_T>
bool msg_write(
    ff::OutgoingMessage<Identity_T> & msg, Uint32Rand const & rand) {
  return msg.template write<uint32_t>(rand.rand);
}

} // namespace ff

TEST(Randomness, dealer_patron_immediate_await) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  Uint32Info info;
  size_t const num_desired = 20;

  std::unique_ptr<Promise<RandomnessDispenser<Uint32Rand, Uint32Info>>>
      p1;
  std::unique_ptr<Promise<RandomnessDispenser<Uint32Rand, Uint32Info>>>
      p2;

  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> r1;
  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> r2;

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [](Fronctocol * self) {
        log_debug("Starting dealer test");
        std::unique_ptr<Fronctocol> rd(
            new RandomnessHouse<TEST_TYPES, Uint32Rand, Uint32Info>());

        self->invoke(std::move(rd), self->getPeers());
      },
      finishTestOnComplete));

  test["alice"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting alice test");
        std::unique_ptr<PromiseFronctocol<
            RandomnessDispenser<Uint32Rand, Uint32Info>>>
            drg(new RandomnessPatron<
                TEST_TYPES,
                Uint32Rand,
                Uint32Info>("dealer", num_desired, info));

        p1 = self->promise(std::move(drg), self->getPeers());
        self->await(*p1);
      },
      failTestOnComplete,
      failTestOnReceive,
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing alice test");
        r1 = p1->getResult(f);
        EXPECT_EQ(true, r1 != nullptr);
        self->complete();
      }));

  test["bob"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting bob test");
        std::unique_ptr<PromiseFronctocol<
            RandomnessDispenser<Uint32Rand, Uint32Info>>>
            drg(new RandomnessPatron<
                TEST_TYPES,
                Uint32Rand,
                Uint32Info>("dealer", num_desired, info));

        p2 = self->promise(std::move(drg), self->getPeers());
        self->await(*p2);
      },
      failTestOnComplete,
      failTestOnReceive,
      [&](Fronctocol & f, Fronctocol * self) {
        log_debug("finishing bob test");
        r2 = p2->getResult(f);
        EXPECT_EQ(true, r2 != nullptr);
        self->complete();
      }));

  EXPECT_TRUE(runTests(test));

  EXPECT_EQ(true, r1 != nullptr);
  EXPECT_EQ(true, r2 != nullptr);
  EXPECT_EQ(true, r1->size() == num_desired);
  EXPECT_EQ(true, r1->size() == r2->size());
  EXPECT_EQ(true, r1->info == r2->info);

  // Utility of littleDispenser and clear
  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> lr1;
  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> lr2;

  lr1 = r1->littleDispenser(num_desired + 1);
  lr2 = r2->littleDispenser(num_desired + 1);

  EXPECT_EQ(nullptr, lr1);
  EXPECT_EQ(nullptr, lr2);

  lr1 = r1->littleDispenser(5);
  lr2 = r2->littleDispenser(5);

  EXPECT_NE(nullptr, lr1);
  EXPECT_NE(nullptr, lr2);

  EXPECT_EQ(5, lr1->size());
  EXPECT_EQ(5, lr2->size());
  lr1->clear();
  lr2->clear();
  EXPECT_EQ(0, lr1->size());
  EXPECT_EQ(0, lr2->size());

  // correctness of generation
  for (size_t i = 0; i < r1->size(); i++) {
    Uint32Rand l = r1->get();
    Uint32Rand r = r2->get();

    EXPECT_TRUE(l.rand == r.rand);
    EXPECT_TRUE(l.rand < info.prime);
  }
};

template<typename Result_T>
struct RoundTripsThenAwait : Fronctocol {
  std::unique_ptr<Promise<Result_T>> promise;
  std::unique_ptr<Result_T> * result;
  size_t numTrips;

  std::string name() override {
    return std::string("RoundTripsThenAwait");
  }

  RoundTripsThenAwait(
      std::unique_ptr<Promise<Result_T>> p,
      std::unique_ptr<Result_T> * r,
      size_t nt) :
      promise(std::move(p)), result(r), numTrips(nt) {
  }

  void init() override {
    log_debug("init RoundTripsThenAwait");

    this->getPeers().forEach([this](std::string const & peer) {
      if (peer != this->getSelf()) {
        log_debug("RoundTripsThenAwait sending first");
        std::unique_ptr<OutgoingMessage> omsg(
            new OutgoingMessage(peer));
        omsg->write<uint64_t>(this->numTrips);
        this->send(std::move(omsg));
      }
    });

    this->numTrips--;
  }

  void handleReceive(IncomingMessage & imsg) override {
    log_debug("handleRecieve RoundTripsThenAwait");

    uint64_t nope;
    imsg.read<uint64_t>(nope);
    (void)nope;

    this->numTrips--;
    if (this->numTrips > 0) {
      this->getPeers().forEach([this](std::string const & peer) {
        if (this->getSelf() == peer) {
          return;
        }
        std::unique_ptr<OutgoingMessage> omsg(
            new OutgoingMessage(peer));
        omsg->write<uint64_t>(this->numTrips);
        this->send(std::move(omsg));
      });
    } else {
      this->await(*this->promise);
    }
  }

  void handleComplete(Fronctocol &) override {
    log_error("unexpected handle complete in RoundTripsThenAwait");
  }

  void handlePromise(Fronctocol & pf) override {
    log_debug("result at %zu", (size_t)this->result);
    (*this->result) = this->promise->getResult(pf);
    if (*this->result == nullptr) {
      log_error("null result");
    }

    this->complete();
  }
};

TEST(Randomness, house_patron_child_await_after_complete) {
  std::map<std::string, std::unique_ptr<Fronctocol>> test;

  Uint32Info info;
  size_t const num_desired = 20;

  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> r1;
  std::unique_ptr<RandomnessDispenser<Uint32Rand, Uint32Info>> r2;

  log_debug("results at %zu and %zu", (size_t)&r1, (size_t)&r2);

  test["dealer"] = std::unique_ptr<Fronctocol>(new Tester(
      [](Fronctocol * self) {
        log_debug("Starting dealer test");
        std::unique_ptr<Fronctocol> rd(
            new RandomnessHouse<TEST_TYPES, Uint32Rand, Uint32Info>());
        self->invoke(std::move(rd), self->getPeers());
      },
      finishTestOnComplete));

  test["alice"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc test alice");
        std::unique_ptr<PromiseFronctocol<
            RandomnessDispenser<Uint32Rand, Uint32Info>>>
            drg(new RandomnessPatron<
                TEST_TYPES,
                Uint32Rand,
                Uint32Info>("dealer", num_desired, info));

        std::unique_ptr<Fronctocol> awaiter(
            new RoundTripsThenAwait<
                RandomnessDispenser<Uint32Rand, Uint32Info>>(
                self->promise(std::move(drg), self->getPeers()),
                &r1,
                25));

        PeerSet ps(self->getPeers());
        ps.remove("dealer");
        self->invoke(std::move(awaiter), ps);
      },
      finishTestOnComplete));

  test["bob"] = std::unique_ptr<Fronctocol>(new Tester(
      [&](Fronctocol * self) {
        log_debug("starting mpc test alice");
        std::unique_ptr<PromiseFronctocol<
            RandomnessDispenser<Uint32Rand, Uint32Info>>>
            drg(new RandomnessPatron<
                TEST_TYPES,
                Uint32Rand,
                Uint32Info>("dealer", num_desired, info));

        std::unique_ptr<Fronctocol> awaiter(
            new RoundTripsThenAwait<
                RandomnessDispenser<Uint32Rand, Uint32Info>>(
                self->promise(std::move(drg), self->getPeers()),
                &r2,
                25));

        PeerSet ps(self->getPeers());
        ps.remove("dealer");
        self->invoke(std::move(awaiter), ps);
      },
      finishTestOnComplete));

  EXPECT_TRUE(runTests(test));

  ASSERT_TRUE(r1 != nullptr);
  ASSERT_TRUE(r2 != nullptr);

  EXPECT_EQ(num_desired, r1->size());
  EXPECT_TRUE(r1->size() == r2->size());

  for (size_t i = 0; i < num_desired; i++) {
    uint32_t l = r1->get().rand;
    uint32_t r = r2->get().rand;

    EXPECT_TRUE(l == r);
    EXPECT_TRUE(l < info.prime);
  }
};

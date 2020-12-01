/**
 * Copyright Stealth Software Technologies, Inc.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <ff/Fronctocol.h>
#include <ff/PeerSet.h>
#include <ff/VectorPeerSet.h>
#include <ff/posixnet/posixnet.h>

#include <ff/logging.h>

using PeerSet = ff::VectorPeerSet<std::string>;

/** Convenience typedefs */
using IncomingMessage = ff::posixnet::IncomingMessage<std::string>;
using OutgoingMessage = ff::posixnet::OutgoingMessage<std::string>;
using Fronctocol = ff::
    Fronctocol<std::string, PeerSet, IncomingMessage, OutgoingMessage>;

const uint64_t PRIME = 23;
const uint64_t BASE = 5;

/****************************************
 * Diffie Hellman taken from Prior Demo *
 ****************************************/
class CheckField : public Fronctocol {
public:
  void init() override {
    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage> omsg(new OutgoingMessage(peer));
      omsg->write<uint64_t>(PRIME);
      omsg->write<uint64_t>(BASE);

      this->send(std::move(omsg));
    });
  }

  void handleReceive(IncomingMessage & imsg) override {
    uint64_t other_prime;
    imsg.read<uint64_t>(other_prime);
    uint64_t other_base;
    imsg.read<uint64_t>(other_base);

    if (PRIME != other_prime || BASE != other_base) {
      log_error("prime or base does not match");
      this->abort();
      return;
    }

    this->complete();
  }

  void handleComplete(Fronctocol &) override {
    log_error("check field shouldn't have a complete");
  }

  void handlePromise(Fronctocol &) override {
    log_error("check field shouldn't have a promise");
  }
};

static auto rand_gen = std::bind(
    std::uniform_int_distribution<uint64_t>(0, PRIME / 2),
    std::default_random_engine(
        (uint64_t)::std::chrono::system_clock::now()
            .time_since_epoch()
            .count()));

class DiffieHellman : public Fronctocol {
public:
  uint64_t myrand;

  uint64_t * shared_secret = nullptr;

  void init() override {
    std::unique_ptr<Fronctocol> cf(new CheckField());

    this->invoke(std::move(cf), this->getPeers());

    this->myrand = rand_gen();

    log_info("Using secret %lu", this->myrand);
  }

  void handleReceive(IncomingMessage & imsg) override {
    uint64_t recv_num;
    imsg.read(recv_num);

    uint64_t output = 1;
    for (size_t i = 0; i < this->myrand; i++) {
      output = output * recv_num % PRIME;
    }

    // log_info("Shared secret is %lu", output);
    log_assert(this->shared_secret != nullptr);
    *this->shared_secret = output;
    this->complete();
  }

  void handleComplete(Fronctocol &) override {
    log_info("handled complete from check field");

    this->getPeers().forEach([this](std::string const & peer) {
      if (peer == this->getSelf()) {
        return;
      }

      std::unique_ptr<OutgoingMessage> omsg(new OutgoingMessage(peer));

      uint64_t send_num = 1;
      for (size_t i = 0; i < this->myrand; i++) {
        send_num = send_num * BASE % PRIME;
      }

      omsg->write(send_num);

      this->send(std::move(omsg));
    });
  }

  void handlePromise(Fronctocol &) override {
    log_error("unused");
  }
};

/**
 * main reads the identities, and addresses from the command line,
 * then passes it to Fortissimo.
 */
int main(int argc, char * argv[]) {
  if (argc != 8) {
    printf("Usage dh_net_example [my identity] [identity 1] [address] "
           "[port] [identity 2] [address] [port]\n");
    exit(1);
  }
  std::vector<ff::posixnet::PeerInfo<std::string>> peers(2);
  std::string myIdentity(argv[1]);

  peers[0].identity = std::string(argv[2]);
  peers[0].address.ss_family = AF_INET;
  ((sockaddr_in *)&peers[0].address)->sin_port =
      htons((uint16_t)atoi(argv[4]));
  if (inet_pton(
          AF_INET,
          argv[3],
          &((sockaddr_in *)&peers[0].address)->sin_addr) != 1) {
    log_error("invlaid first address");
    exit(2);
  }

  peers[1].identity = std::string(argv[5]);
  peers[1].address.ss_family = AF_INET;
  ((sockaddr_in *)&peers[1].address)->sin_port =
      htons((uint16_t)atoi(argv[7]));
  if (inet_pton(
          AF_INET,
          argv[6],
          &((sockaddr_in *)&peers[1].address)->sin_addr) != 1) {
    log_error("invlaid first address");
    exit(2);
  }

  std::unique_ptr<Fronctocol> main(new DiffieHellman());

  uint64_t result = UINT64_MAX;
  static_cast<DiffieHellman *>(main.get())->shared_secret = &result;

  if (runFortissimoPosixNet(std::move(main), peers, myIdentity)) {
    log_info("protocol successful");
  } else {
    log_warn("protocol failed");
  }

  log_info("shared secret is %lu", result);
}

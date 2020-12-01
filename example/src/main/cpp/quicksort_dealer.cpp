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

#include <mpc/Quicksort.h>
#include <mpc/QuicksortDealer.h>

#include <ff/logging.h>

using namespace ff::mpc;

using PeerSet = ff::VectorPeerSet<std::string>;

/** Convenience typedefs */
using IncomingMessage = ff::posixnet::IncomingMessage<std::string>;
using OutgoingMessage = ff::posixnet::OutgoingMessage<std::string>;
using Fronctocol = ff::
    Fronctocol<std::string, PeerSet, IncomingMessage, OutgoingMessage>;
/**
 * Main reads the identities, and addresses from the command line,
 * then passes it to Fortissimo.
 */
int main(int argc, char * argv[]) {
  if (argc % 3 != 2) {
    printf("Usage dh_net_example [my identity] [identity 1] [address] "
           "[port] [identity 2] [address] [port] ... \n");
    exit(1);
  }
  size_t numPeers = (size_t)((argc - 2) / 3);
  std::vector<ff::posixnet::PeerInfo<std::string>> peers(numPeers);
  std::string myIdentity(argv[1]);

  for (size_t i = 0; i < numPeers; i++) {

    peers[i].identity = std::string(argv[2 + 3 * i]);
    peers[i].address.ss_family = AF_INET;
    ((sockaddr_in *)&peers[i].address)->sin_port =
        htons((uint16_t)atoi(argv[4 + 3 * i]));
    if (inet_pton(
            AF_INET,
            argv[3 * i + 3],
            &((sockaddr_in *)&peers[i].address)->sin_addr) != 1) {
      log_error("invalid %luth address", i);
      exit(2);
    }
  }

  const size_t NUM_KEYS = 4;
  const ArithmeticShare_t modulus = 299099; // a 19 bit prime
  //const ArithmeticShare_t modulus = 31; // a 5 bit prime
  const ArithmeticShare_t small_modulus = 47;
  const size_t bitsPerPrime = 19;
  //const size_t bitsPerPrime = 5;
  const size_t sqrt_ell = 5;
  //const size_t sqrt_ell = 3;
  const size_t MAX_LIST_SIZE = 25;

  std::vector<std::vector<ArithmeticShare_t>> lagrangePolynomialSet;
  size_t block_size = sqrt_ell;
  while (block_size < bitsPerPrime) {
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        small_modulus, static_cast<ArithmeticShare_t>(block_size)));
    //log_debug("block_size: %lu", block_size);
    block_size += sqrt_ell;
  }
  //log_debug("block_size: %lu", this->compareInfo.ell);
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      small_modulus, static_cast<ArithmeticShare_t>(bitsPerPrime)));

  block_size = 1;
  while ((block_size - 1) < sqrt_ell) {
    //log_debug("block_size: %lu", block_size);
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        small_modulus, static_cast<ArithmeticShare_t>(block_size)));
    block_size++;
  }

  std::string * starter = new std::string("");

  CompareInputInfo<std::string> compareInfo =
      CompareInputInfo<std::string>(
          modulus,
          small_modulus,
          bitsPerPrime,
          sqrt_ell,
          lagrangePolynomialSet,
          starter);

  ObservationList income_output = ObservationList();

  std::unique_ptr<Fronctocol> main(
      new QuicksortRandomnessHouse<
          std::string,
          PeerSet,
          IncomingMessage,
          OutgoingMessage>(compareInfo, MAX_LIST_SIZE, NUM_KEYS));

  if (runFortissimoPosixNet(std::move(main), peers, myIdentity)) {
    log_info("dealer protocol successful");
  } else {
    log_warn("protocol failed");
  }
}

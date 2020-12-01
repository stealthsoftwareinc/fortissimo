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

#include <mpc/SISOSort.h>

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

  std::string * dealerName = new std::string("dealer");

  const size_t NUM_ARITHMETIC_PAYLOADS = 1;
  const size_t NUM_XOR_PAYLOADS = 8;
  const size_t NUM_KEYS = 4;
  const ArithmeticShare_t modulus = 299099; // a 19 bit prime
  const size_t MAX_LIST_SIZE = 100;
  const size_t KEY_WITH_VAL = 1;
  std::vector<uint32_t> arithmeticPayloads;
  std::vector<uint32_t> keys;
  std::vector<Boolean_t> XORPayloads;

  std::vector<uint32_t> permutation =
      std::vector<uint32_t>(MAX_LIST_SIZE);
  for (size_t i = 0; i < MAX_LIST_SIZE; i++) {
    permutation[i] = static_cast<uint32_t>(i);
  }
  for (size_t i = 0; i < MAX_LIST_SIZE - 1; i++) {
    size_t j = i + randomModP<size_t>(MAX_LIST_SIZE - i);
    ArithmeticShare_t temp = permutation[j];
    permutation[j] = permutation[i];
    permutation[i] = temp;
  }

  ObservationList oList = ObservationList();
  std::vector<Observation> o_elements(0);
  for (size_t j = 0; j < MAX_LIST_SIZE; j++) {
    arithmeticPayloads =
        std::vector<uint32_t>(NUM_ARITHMETIC_PAYLOADS, 0);
    keys = std::vector<uint32_t>(NUM_KEYS, 0);
    XORPayloads = std::vector<Boolean_t>(NUM_XOR_PAYLOADS, 0x00);

    keys.at(KEY_WITH_VAL) =
        static_cast<ArithmeticShare_t>(permutation[j]);
    keys.at(KEY_WITH_VAL) %= modulus;

    Observation o;
    o.keyCols = keys;
    o.arithmeticPayloadCols = arithmeticPayloads;
    o.XORPayloadCols = XORPayloads;

    o_elements.push_back(o);
  }
  oList.elements = o_elements;
  oList.numKeyCols = NUM_KEYS;
  oList.numArithmeticPayloadCols = NUM_ARITHMETIC_PAYLOADS;
  oList.numXORPayloadCols = NUM_XOR_PAYLOADS;

  for (size_t i = 0; i < MAX_LIST_SIZE; i++) {
    log_debug(
        "starting share %u",
        oList.elements.at(i).keyCols.at(KEY_WITH_VAL));
  }

  std::string * starter = new std::string("income");

  std::unique_ptr<Fronctocol> main(
      new SISOSort<
          std::string,
          PeerSet,
          IncomingMessage,
          OutgoingMessage>(oList, modulus, starter, dealerName));

  //  ObservationList output =
  //          *static_cast<QuickSortFronctocol<TEST_TYPES> *>(main.get())->inputList;

  if (runFortissimoPosixNet(std::move(main), peers, myIdentity)) {
    log_info("protocol successful");
  } else {
    log_warn("protocol failed");
  }

  for (size_t i = 0; i < MAX_LIST_SIZE; i++) {
    log_info(
        "shared secret[%lu] is %u",
        i,
        oList.elements.at(i).keyCols.at(KEY_WITH_VAL));
  }
}

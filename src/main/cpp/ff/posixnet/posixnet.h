/**
 * Copyright Stealth Software Technologies, Inc.
 */

#if defined(WINDOWS) || defined(__WIN32__) || defined(__WIN64__) || \
    defined(_WIN32) || defined(_WIN64)
#include <ff/posixnet/posixnet_windows.h>
#else
#include <ff/posixnet/posixnet_posix.h>
#endif

/* C and POSIX Headers */
#include <fcntl.h>

/* C++ Headers */
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/FronctocolsManager.h>
#include <ff/Message.h>
#include <ff/Util.h>

/* logging configuration */
#include <ff/logging.h>

#ifndef FF_POSIX_NET_H_
#define FF_POSIX_NET_H_

namespace ff {
namespace posixnet {

/**
 * Describes a peer, both by its identity and connection information.
 */
template<typename Identity_T>
struct PeerInfo {
  /* Identity of this peer. */
  Identity_T identity;

  /**
   * A socket family struct which is sized large enough to be cast to
   * a sockaddr_in or sockaddr_in6 or any other address family supported
   * by your operating system. For Fortissimo to support it, this must be
   * a valid IPv4 or IPv6 address for your peer.
   */
  sockaddr_storage address;
};

/**
 * Converts a PeerInfos vector to a PeerSet. A default implementation exists
 * using the ``PeerSet::add()`` virtual method.
 */
template<typename Identity_T, typename PeerSet_T>
PeerSet_T
PeerInfos2PeerSet(std::vector<PeerInfo<Identity_T>> const & peer_infos);

/**
 * Implementation of an IncomingMessage which uses C buffers to hold
 * things.
 */
template<typename Identity_T>
class IncomingMessage : public ff::IncomingMessage<Identity_T> {
private:
  uint8_t * buffer = nullptr; // a buffer.
  size_t size = 0; // the total size of the buffer.
  size_t place = 0; // the place up to where the buffer has been read.

public:
  size_t remove(void * buf, size_t const len) override;

  size_t length() const override;

  void clear() override;

  IncomingMessage(
      Identity_T const & id,
      uint8_t * buf,
      size_t size,
      size_t place = 0);
  ~IncomingMessage();

  /**
   * The Cache sub-type must store the remaining message, and produce
   * a pointer to a valid IncomingMessage, when ``uncache()`` is called.
   *
   * The pointer must be "owned" (or at least destroyed) when the cache
   * is destroyed. This IncomingMessage type returns "this", but not all
   * IncomingMessage implementations need to return "this".
   */
  struct Cache {
    uint8_t const controlBlock;
    IncomingMessage<Identity_T> incomingMessage;

    Cache(
        uint8_t controlBlock,
        uint8_t * buffer,
        size_t size,
        size_t place,
        Identity_T const & sender);

    /**
     * Cache is responsible for freeing this message when the cache is
     * deleted.
     */
    IncomingMessage<Identity_T> * uncache();
  };

  ::std::unique_ptr<Cache> createCache(uint8_t const control_block);
};

/**
 * Implementation of an OutgoingMessage which uses C buffers to hold
 * things.
 */
template<typename Identity_T>
class OutgoingMessage : public ff::OutgoingMessage<Identity_T> {
private:
  uint8_t * buffer = nullptr;
  size_t capacity = 0;
  size_t place = 0;

  bool makeSpace(size_t const nchars);

public:
  size_t add(void const * buf, size_t const nchars) override;

  size_t prepend(void const * buf, size_t const nchars) override;

  size_t length() const override;

  void clear() override;

  OutgoingMessage(Identity_T const & id);
  ~OutgoingMessage();

  uint8_t * takeBuffer();
  uint8_t const * getBuffer() const;
};

/**
 * Helper function for ff::tester to convert a ff::posixnet::IncomingMessage
 * to an ff::posixnet::OutgoingMessage. This is largely a convenience to
 * enable the use of ff::tester based unit tests in a code base which uses
 * ff::posixnet.
 */
template<typename Identity_T>
::std::unique_ptr<IncomingMessage<Identity_T>>
outgoingToIncomingMessage(
    Identity_T const &, OutgoingMessage<Identity_T> &);

/**
 * Blocking function to run Fortissimo. After it returns the given main
 * fronctocol will have been ran with the given peers, or an error will
 * have occured.
 *
 * The main fronctocol should have references or pointers to its return
 * values, this function does not handle return values. This is expected
 * to be indicated by the main fronctocol using references or pointers.
 *
 * Fortissimo frees the main fronctocol, so its return value must be a
 * pointer to the caller's memory.
 *
 * The peers info must contain an entry for itself, this allows Fortissimo
 * to open its accept socket.
 *
 * The self parameter is the identity of itself.
 *
 * The function returns true on success, false on failure.
 */
template<typename Identity_T, typename PeerSet_T>
bool runFortissimoPosixNet(
    std::unique_ptr<ff::Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage<Identity_T>,
        OutgoingMessage<Identity_T>>> mainFronctocol,
    std::vector<PeerInfo<Identity_T>> const & peers_info,
    Identity_T const & self);

} // namespace posixnet
} // namespace ff

#include <ff/posixnet/posixnet.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_POSIX_NET_H_

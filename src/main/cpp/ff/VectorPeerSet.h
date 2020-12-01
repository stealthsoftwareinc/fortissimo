/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_VECTOR_PEERSET_H_
#define FF_VECTOR_PEERSET_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <functional>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Message.h>
#include <ff/PeerSet.h>
#include <ff/Util.h>

#include <ff/logging.h>

namespace ff {

template<typename Identity_T>
class VectorPeerSet : public PeerSet<Identity_T> {

private:
  struct Peer {
    Identity_T peer;
    fronctocolId_t id;
    bool completion;

    Peer(Identity_T const & p, fronctocolId_t i, bool c);
    Peer(Peer const & other);
    Peer & operator=(Peer const &) = default;

    bool operator==(Peer const & other) const;
    bool operator<(Peer const & other) const;
  };

  ::std::vector<Peer> peers;

  Peer * find(Identity_T const & peerId);

public:
  VectorPeerSet() = default;

  /** 
   * Copy constructor does a sort-on-copy operation
   * It also ignores fronctocolIds and completion status
   */
  VectorPeerSet(VectorPeerSet const & other);

  VectorPeerSet & operator=(VectorPeerSet const &) = default;

  /* Framework required for each */
  void forEach(
      ::std::function<void(
          Identity_T const &, fronctocolId_t &, bool &)> f) override;

  /* Convenience for each for fronctocol implementations. */
  void
  forEach(::std::function<void(Identity_T const &)> f) const override;

  /** Equality depends on sort, and checks only for Identity's equality */
  bool operator==(VectorPeerSet<Identity_T> const & other) const;

  void add(Identity_T const & peer) override;
  void remove(Identity_T const & peer) override;

  bool
  checkAndSetId(Identity_T const & p, fronctocolId_t const i) override;
  void setId(Identity_T const & peer, fronctocolId_t const id) override;
  void setCompleted(Identity_T const & peer) override;
  fronctocolId_t findPeerId(Identity_T const & peer) override;
  bool findCompletionStatus(Identity_T const & peer) override;
  bool hasPeer(Identity_T const & peer) override;

  size_t size() const override;

  template<typename Identity2_T>
  friend bool msg_write(
      OutgoingMessage<Identity2_T> &,
      VectorPeerSet<Identity2_T> const & ps);
  template<typename Identity2_T>
  friend bool msg_read(
      IncomingMessage<Identity2_T> &, VectorPeerSet<Identity2_T> & ps);
};

#include <ff/VectorPeerSet.t.h>

} // namespace ff

#endif // FF_VECTOR_PEERSET_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_PEERSET_H_
#define FF_PEERSET_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <functional>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Message.h>
#include <ff/Util.h>

namespace ff {

template<typename Identity_T>
struct PeerSet {
  /**
   * The copy constructor should copy the peer's Identities, but not
   * fronctocol IDs or completion statuses.
   */
  explicit PeerSet(PeerSet<Identity_T> const &) = default;

  /**
   * The default constructor should create a PeerSet with no peers whatsoever.
   */
  explicit PeerSet() = default;

  /**
   * Performs a given function on each peer in this set.
   *
   * This version of forEach is intended for Fortissimo itself to use.
   * Users should use the single argument version.
   *
   * The other parameters are
   *  - fronctocolid_t& (access/modify by reference) the ID of the peer's
   *    fronctocol. This should default to FRONCTOCOLID_INVALID.
   *  - bool& (access/modify by reference) The completion status of the
   *    peer's fronctocol. This should default to false.
   */
  virtual void
      forEach(::std::function<
              void(Identity_T const &, fronctocolId_t &, bool &)>) = 0;

  /**
   * Perform a given function on each peer in the set.
   *
   * This single argument (Identity of peer), is intended for users
   * of Fortissimo.
   */
  virtual void
      forEach(::std::function<void(Identity_T const &)>) const = 0;

  /**
   * Add or remove a peer to this PeerSet.
   * Only use these before a corresponding fronctocol has been invoked.
   */
  virtual void add(Identity_T const & peer) = 0;
  virtual void remove(Identity_T const & peer) = 0;

  virtual size_t size() const = 0;

  virtual ~PeerSet() = default;

  /**
   * Must have an equality operator. When testing equality, only Identities
   * should be considered, completion status and fronctocol Ids should be
   * ignored.
   *
   * Note: signature is commented, because the virtual method requires
   * comparison to *any* PeerSet, not PeerSets of the same type.
   */
  // virtual bool operator==(PeerSet<Identity_T> const& other) const = 0;

  /**
   * Check if the peer has a valid ID, and if not assign it.
   */
  virtual bool
  checkAndSetId(Identity_T const & peer, fronctocolId_t const id);

  /**
   * Set a peers ID with out checking if it is already valid.
   */
  virtual void setId(Identity_T const & peer, fronctocolId_t const id);

  /**
   * Check if the peer has all peer's IDs.
   */
  virtual bool hasAllPeerIds();

  /**
   * Set a peers completion status to true.
   */
  virtual void setCompleted(Identity_T const & peer);

  /**
   * Check if all of the peers have sent complete messages.
   */
  virtual bool checkAllComplete();

  /**
   * Finds the fronctocol ID from the given peer.
   */
  virtual fronctocolId_t findPeerId(Identity_T const & peer);

  /**
   * Finds the completion status of the given peer.
   */
  virtual bool findCompletionStatus(Identity_T const & peer);

  /**
   * Check if a peer has membership in this peerset.
   */
  virtual bool hasPeer(Identity_T const & peer);
};

#include <ff/PeerSet.t.h>

} // namespace ff

#endif // FF_PEERSET_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<typename Identity_T>
VectorPeerSet<Identity_T>::Peer ::Peer(
    Identity_T const & p, fronctocolId_t i, bool c) :
    peer(p), id(i), completion(c) {
}

template<typename Identity_T>
VectorPeerSet<Identity_T>::Peer ::Peer(Peer const & other) :
    peer(other.peer), id(FRONCTOCOLID_INVALID), completion(false) {
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::Peer ::operator==(
    Peer const & other) const {
  return this->peer == other.peer;
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::Peer ::operator<(
    Peer const & other) const {
  return this->peer < other.peer;
}

template<typename Identity_T>
VectorPeerSet<Identity_T>::VectorPeerSet(VectorPeerSet const & other) :
    PeerSet<Identity_T>(other), peers(other.peers) {
  std::sort(this->peers.begin(), this->peers.end());
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::forEach(
    ::std::function<void(Identity_T const &, fronctocolId_t &, bool &)>
        f) {
  for (size_t i = 0; i < this->peers.size(); i++) {
    fronctocolId_t prev = this->peers[i].id;
    f(this->peers[i].peer,
      this->peers[i].id,
      this->peers[i].completion);

    if_debug if (
        prev != FRONCTOCOLID_INVALID &&
        this->peers[i].id == FRONCTOCOLID_INVALID) {
      log_fatal("id was valid and is not anymore");
    }
  }
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::forEach(
    ::std::function<void(Identity_T const &)> f) const {
  for (size_t i = 0; i < this->peers.size(); i++) {
    f(this->peers[i].peer);
  }
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::operator==(
    VectorPeerSet<Identity_T> const & other) const {
  if (this->peers.size() != other.peers.size()) {
    return false;
  }

  for (size_t i = 0; i < this->peers.size(); i++) {
    if (this->peers[i].peer != other.peers[i].peer) {
      return false;
    }
  }

  return true;
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::add(Identity_T const & peer) {
  this->peers.emplace_back(peer, FRONCTOCOLID_INVALID, false);
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::remove(Identity_T const & peer) {
  // linear search, because this may not yet have been ordered as peers
  // are being added/removed.
  size_t idx = SIZE_MAX;
  size_t cnt = 0;
  this->forEach([&](Identity_T const & i_peer) {
    if (i_peer == peer) {
      idx = cnt;
    }
    cnt++;
  });
  this->peers.erase(this->peers.begin() + (ssize_t)idx);
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::checkAndSetId(
    Identity_T const & peer, fronctocolId_t const id) {
  Peer * p = this->find(peer);
  if (p == nullptr) {
    return false;
  } else if (p->peer == peer && p->id == FRONCTOCOLID_INVALID) {
    p->id = id;
    return true;
  } else {
    return false;
  }
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::setId(
    Identity_T const & peer, fronctocolId_t const id) {
  Peer * p = this->find(peer);
  if (p != nullptr) {
    p->id = id;
  }
}

template<typename Identity_T>
void VectorPeerSet<Identity_T>::setCompleted(Identity_T const & peer) {
  Peer * p = this->find(peer);
  if (p != nullptr) {
    p->completion = true;
  }
}

template<typename Identity_T>
fronctocolId_t
VectorPeerSet<Identity_T>::findPeerId(Identity_T const & peer) {
  Peer * p = this->find(peer);
  if (p == nullptr) {
    return FRONCTOCOLID_INVALID;
  } else {
    return p->id;
  }
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::findCompletionStatus(
    Identity_T const & peer) {
  Peer * p = this->find(peer);
  if (p == nullptr) {
    return false;
  } else {
    return p->completion;
  }
}

template<typename Identity_T>
bool VectorPeerSet<Identity_T>::hasPeer(Identity_T const & peer) {
  return this->find(peer) != nullptr;
}

template<typename Identity_T>
typename VectorPeerSet<Identity_T>::Peer *
VectorPeerSet<Identity_T>::find(Identity_T const & peerId) {
  size_t front = 0;
  size_t back = this->peers.size() - 1;

  while (front <= back && back != SIZE_MAX) {
    size_t mid = (front + back) / 2;
    if (this->peers[mid].peer == peerId) {
      return &this->peers[mid];
    } else if (this->peers[mid].peer < peerId) {
      front = mid + 1;
    } else /* if(this->peers[mid].peer > peerId) */ {
      back = mid - 1;
    }
  }

  return nullptr;
}

template<typename Identity_T>
size_t VectorPeerSet<Identity_T>::size() const {
  return this->peers.size();
}

template<typename Identity_T>
bool msg_write(
    OutgoingMessage<Identity_T> & omsg,
    VectorPeerSet<Identity_T> const & ps) {
  omsg.template write<uint32_t>((uint32_t)ps.size());

  for (size_t i = 0; i < ps.peers.size(); i++) {
    omsg.template write<Identity_T>(ps.peers[i].peer);
  }

  return true;
}

template<typename Identity_T>
bool msg_read(
    IncomingMessage<Identity_T> & imsg,
    VectorPeerSet<Identity_T> & ps) {
  bool ret = true;
  uint32_t len32 = 0;
  ret = ret && imsg.template read<uint32_t>(len32);

  for (size_t i = 0; i < (size_t)len32; i++) {
    Identity_T peer;
    ret = ret && imsg.template read<Identity_T>(peer);
    ps.add(peer);
  }

  std::sort(ps.peers.begin(), ps.peers.end());

  return ret;
}

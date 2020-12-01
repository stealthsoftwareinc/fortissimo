/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<typename Identity_T>
bool PeerSet<Identity_T>::checkAndSetId(
    Identity_T const & peer, fronctocolId_t const id) {
  bool ret = false;
  this->forEach([&](Identity_T const & p, fronctocolId_t & i, bool &) {
    if (p == peer && FRONCTOCOLID_INVALID == i) {
      i = id;
      ret = true;
    }
  });
  return ret;
}

template<typename Identity_T>
void PeerSet<Identity_T>::setId(
    Identity_T const & peer, fronctocolId_t const id) {
  this->forEach([&](Identity_T const & p, fronctocolId_t & i, bool &) {
    if (p == peer) {
      i = id;
    }
  });
}

template<typename Identity_T>
bool PeerSet<Identity_T>::hasAllPeerIds() {
  bool ret = true;
  this->forEach([&](Identity_T const &, fronctocolId_t & i, bool &) {
    ret = ret && i != FRONCTOCOLID_INVALID;
  });
  return ret;
}

template<typename Identity_T>
void PeerSet<Identity_T>::setCompleted(Identity_T const & peer) {
  this->forEach([&](Identity_T const & p, fronctocolId_t &, bool & cs) {
    if (p == peer) {
      cs = true;
    }
  });
}

template<typename Identity_T>
bool PeerSet<Identity_T>::checkAllComplete() {
  bool ret = true;
  this->forEach([&](Identity_T const &, fronctocolId_t &, bool & cs) {
    ret = ret && cs;
  });
  return ret;
}

template<typename Identity_T>
fronctocolId_t
PeerSet<Identity_T>::findPeerId(Identity_T const & peer) {
  fronctocolId_t ret = FRONCTOCOLID_INVALID;
  this->forEach([&](Identity_T const & p, fronctocolId_t & i, bool &) {
    if (peer == p) {
      ret = i;
    }
  });
  return ret;
}

template<typename Identity_T>
bool PeerSet<Identity_T>::findCompletionStatus(
    Identity_T const & peer) {
  bool ret = false;
  this->forEach([&](Identity_T const & p, fronctocolId_t &, bool & cs) {
    if (peer == p) {
      ret = cs;
    }
  });
  return ret;
}

template<typename Identity_T>
bool PeerSet<Identity_T>::hasPeer(Identity_T const & peer) {
  bool ret = false;
  this->forEach([&](Identity_T const & p, fronctocolId_t &, bool &) {
    if (peer == p) {
      ret = true;
    }
  });
  return ret;
}

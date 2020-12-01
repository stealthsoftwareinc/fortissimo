/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolHandler<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    init(::std::vector<::std::unique_ptr<Action<
             Identity_T,
             PeerSet_T,
             IncomingMessage_T,
             OutgoingMessage_T>>> * actions) {
  this->implementation->setActions(actions);
  this->implementation->init();
  this->implementation->setActions(nullptr);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolHandler<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleReceive(
        IncomingMessage_T & msg,
        ::std::vector<::std::unique_ptr<Action<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>> * actions) {
  this->implementation->setActions(actions);
  this->implementation->handleReceive(msg);
  this->implementation->setActions(nullptr);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolHandler<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleComplete(
        Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & fronctocol,
        ::std::vector<::std::unique_ptr<Action<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>> * actions) {
  this->implementation->setActions(actions);
  this->implementation->handleComplete(fronctocol);
  this->implementation->setActions(nullptr);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolHandler<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handlePromise(
        Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & fronctocol,
        ::std::vector<::std::unique_ptr<Action<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>> * actions) {
  this->implementation->setActions(actions);
  this->implementation->handlePromise(fronctocol);
  this->implementation->setActions(nullptr);
}

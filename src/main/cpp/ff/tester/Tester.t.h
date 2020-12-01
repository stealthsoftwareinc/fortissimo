/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::function<void(
    IncomingMessage_T &,
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> *)>
failTestOnReceive() {
  return [](IncomingMessage_T &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> * self) {
    log_error("Test did not expect an incoming message. Aborting.");

#ifdef FAIL
    FAIL();
#endif

    self->abort();
  };
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::function<void(
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> &,
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> *)>
failTestOnComplete() {
  return [](Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> * self) {
    log_error("Test did not expect a completed fronctocol. Aborting.");

#ifdef FAIL
    FAIL();
#endif

    self->abort();
  };
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::function<void(
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> &,
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> *)>
finishTestOnComplete() {
  return [](Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> * self) { self->complete(); };
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::function<void(
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> &,
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> *)>
failTestOnPromise() {
  return [](Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> * self) {
    log_error("Test did not expect a promised fronctocol. Aborting.");

#ifdef FAIL
    FAIL();
#endif

    self->abort();
  };
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::function<void(
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> &,
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> *)>
finishTestOnPromise() {
  return [](Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> * self) { self->complete(); };
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
std::string
Tester<Identity_T, PeerSet_T, IncomingMessage_T, OutgoingMessage_T>::
    name() {
  return std::string("Tester");
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
Tester<Identity_T, PeerSet_T, IncomingMessage_T, OutgoingMessage_T>::
    Tester(
        ::std::function<void(Fronctocol<
                             Identity_T,
                             PeerSet_T,
                             IncomingMessage_T,
                             OutgoingMessage_T> *)> init,
        ::std::function<void(
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> *)> complete,
        ::std::function<void(
            IncomingMessage_T &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> *)> receive,
        ::std::function<void(
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> &,
            Fronctocol<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T> *)> promise) :
    initHandler(::std::move(init)),
    completeHandler(::std::move(complete)),
    receiveHandler(::std::move(receive)),
    promiseHandler(::std::move(promise)) {
}
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Tester<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::init() {
  this->initHandler(this);
}
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Tester<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::handleReceive(IncomingMessage_T & imsg) {
  this->receiveHandler(imsg, this);
}
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Tester<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleComplete(Fronctocol<
                   Identity_T,
                   PeerSet_T,
                   IncomingMessage_T,
                   OutgoingMessage_T> & f) {
  this->completeHandler(f, this);
}
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Tester<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handlePromise(Fronctocol<
                  Identity_T,
                  PeerSet_T,
                  IncomingMessage_T,
                  OutgoingMessage_T> & f) {
  this->promiseHandler(f, this);
}

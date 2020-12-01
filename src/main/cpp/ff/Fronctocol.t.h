/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::send(::std::unique_ptr<OutgoingMessage_T>
                                 omsg) {
  log_assert(nullptr != this->actions);
  this->actions->emplace_back(new SendAction<
                              Identity_T,
                              PeerSet_T,
                              IncomingMessage_T,
                              OutgoingMessage_T>(::std::move(omsg)));
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::complete() {
  log_assert(nullptr != this->actions);
  this->actions->emplace_back(new CompleteAction<
                              Identity_T,
                              PeerSet_T,
                              IncomingMessage_T,
                              OutgoingMessage_T>());
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    invoke(
        ::std::unique_ptr<Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>> f,
        PeerSet_T const & p) {
  log_assert(nullptr != this->actions);
  this->actions->emplace_back(new InvokeAction<
                              Identity_T,
                              PeerSet_T,
                              IncomingMessage_T,
                              OutgoingMessage_T>(::std::move(f), p));
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
template<typename Result_T>
::std::unique_ptr<Promise<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T,
    Result_T>>
Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    promise(
        ::std::unique_ptr<PromiseFronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T,
            Result_T>> pf,
        PeerSet_T const & p) {
  log_assert(nullptr != this->actions);
  ::std::unique_ptr<Promise<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T,
      Result_T>>
      promise(new Promise<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T,
              Result_T>(pf.get()));
  this->actions->emplace_back(
      new InvokeAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(::std::move(pf), p, true));
  return promise;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
template<typename Result_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    await(Promise<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T,
          Result_T> & pf) {
  this->actions->emplace_back(
      new AwaitAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(pf.promisedFronctocol));
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::abort() {
  log_assert(nullptr != this->actions);
  this->actions->emplace_back(new AbortAction<
                              Identity_T,
                              PeerSet_T,
                              IncomingMessage_T,
                              OutgoingMessage_T>());
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    setActions(::std::vector<::std::unique_ptr<Action<
                   Identity_T,
                   PeerSet_T,
                   IncomingMessage_T,
                   OutgoingMessage_T>>> * actions) {
  this->actions = actions;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    setHandler(FronctocolHandler<
               Identity_T,
               PeerSet_T,
               IncomingMessage_T,
               OutgoingMessage_T> const * handler) {
  this->handler = handler;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
::std::string Fronctocol<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::name() {
  return ::std::string("unnamed fronctocol");
}

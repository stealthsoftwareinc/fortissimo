/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_ACTIONS_H_
#define FF_ACTIONS_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>

/* 3rd Party Headers */

/* Fortissimo Headers */

namespace ff {

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
class Fronctocol;

enum class ActionType { send, invoke, complete, await, abortion };

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct Action {
  ActionType const type;

  virtual ~Action() = default;

  /* Abstract */

protected:
  Action(ActionType type) : type(type) {
  }
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct SendAction : public Action<
                        Identity_T,
                        PeerSet_T,
                        IncomingMessage_T,
                        OutgoingMessage_T> {
  ::std::unique_ptr<OutgoingMessage_T> msg;

  SendAction(::std::unique_ptr<OutgoingMessage_T> msg) :
      Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(ActionType::send),
      msg(::std::move(msg)) {
  }
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct InvokeAction : public Action<
                          Identity_T,
                          PeerSet_T,
                          IncomingMessage_T,
                          OutgoingMessage_T> {
  ::std::unique_ptr<Fronctocol<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>
      fronctocol;
  PeerSet_T peers;
  bool promised;

  InvokeAction(
      ::std::unique_ptr<Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>> fronctocol,
      PeerSet_T const & peers,
      bool promised = false) :
      Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(ActionType::invoke),
      fronctocol(::std::move(fronctocol)),
      peers(peers),
      promised(promised) {
  }
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct CompleteAction : public Action<
                            Identity_T,
                            PeerSet_T,
                            IncomingMessage_T,
                            OutgoingMessage_T> {
  CompleteAction() :
      Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(ActionType::complete) {
  }
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct AwaitAction : public Action<
                         Identity_T,
                         PeerSet_T,
                         IncomingMessage_T,
                         OutgoingMessage_T> {
  Fronctocol<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> const * const awaitedFronctocol;

  AwaitAction(Fronctocol<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> const * const af) :
      Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(ActionType::await),
      awaitedFronctocol(af) {
  }
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct AbortAction : public Action<
                         Identity_T,
                         PeerSet_T,
                         IncomingMessage_T,
                         OutgoingMessage_T> {
  AbortAction() :
      Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(ActionType::abortion) {
  }
};

} // namespace ff

#endif // FF_ACTIONS_H_

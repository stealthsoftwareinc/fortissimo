/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_FRONCTOCOL_HANDLER_H_
#define FF_FRONCTOCOL_HANDLER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Actions.h>
#include <ff/Fronctocol.h>
#include <ff/Util.h>

/* Logging Config */
#include <ff/logging.h>

namespace ff {

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
class Fronctocol;

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
struct FronctocolHandler {
  fronctocolId_t id = FRONCTOCOLID_INVALID;
  Identity_T const & self;
  FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> * parent = nullptr;
  ::std::vector<FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> *>
      cradle;
  ::std::vector<::std::unique_ptr<FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>>
      womb;
  PeerSet_T peers;
  ::std::unique_ptr<Fronctocol<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>
      implementation = nullptr;

  ::std::deque<::std::unique_ptr<typename IncomingMessage_T::Cache>>
      incomingMessageCaches;

  bool promised = false;
  bool completed = false;
  bool collected = false;

  explicit FronctocolHandler(
      Identity_T const & s, PeerSet_T const & p) :
      self(s), peers(p) {
  }

  explicit FronctocolHandler(Identity_T const & s) : self(s) {
  }

  void init(::std::vector<::std::unique_ptr<Action<
                Identity_T,
                PeerSet_T,
                IncomingMessage_T,
                OutgoingMessage_T>>> * actions);
  void handleReceive(
      IncomingMessage_T & msg,
      ::std::vector<::std::unique_ptr<Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>> * actions);
  void handleComplete(
      Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & fronctocol,
      ::std::vector<::std::unique_ptr<Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>> * actions);
  void handlePromise(
      Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & fronctocol,
      ::std::vector<::std::unique_ptr<Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>> * actions);

  LogTimer timer;
};

#include <ff/FronctocolHandler.t.h>

} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_FRONCTOCOL_HANDLER_H_

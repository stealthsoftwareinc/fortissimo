/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_FRONCTOCOLS_MANAGER_H_
#define FF_FRONCTOCOLS_MANAGER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Actions.h>
#include <ff/Fronctocol.h>
#include <ff/FronctocolHandler.h>
#include <ff/Util.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
class FronctocolsManager {
private:
  /**
   * The identity of this participant in the protocol.
   */
  Identity_T const & self;

  /**
   * Generates the next ID by incrementing.
   */
  fronctocolId_t fronctocolIdGenerator = 1;

  /**
   * Map (with ownership) of existing fronctocol IDs to fronctcols.
   */
  ::std::unordered_map<
      fronctocolId_t,
      ::std::unique_ptr<FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>>
      fronctocols;

  /**
   * Flags used to indicate completion status of the protocol.
   */
  bool initialized = false;
  bool finished = false;
  bool aborted = false;

public:
  FronctocolsManager(Identity_T const & self);
  /**
   * Performs first time initialization tasks.
   *
   * Note that if messages are received before ``init`` is called, they
   * can be cached for later. (TODO: implement this feature).
   */
  void init(
      ::std::unique_ptr<Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>> main,
      PeerSet_T peers,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);

  /**
   * Performs update tasks when a message arrives.
   */
  void handleReceive(
      IncomingMessage_T & imsg,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);

  /**
   * Finished indicates that this server has finished its main fronctocol,
   * but other servers may still be processing.
   *
   * Closed indicates that all servers have successfully completed all
   * fronctocols.
   *
   * Aborted indicates that connections should be forcibly closed because
   * something went wrong.
   */
  bool isFinished() const;
  bool isClosed() const;
  bool isAborted() const;

  size_t getNumFronctocols() const;

private:
  /**
   * Handles actions created by fronctocols during their updates.
   */
  void handleActions(
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      ::std::vector<::std::unique_ptr<Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>> & actions,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);

  /**
   * Helper functions for handleReceive
   */
  void distributeMessage(
      uint8_t const ctrl_blk,
      IncomingMessage_T & imsg,
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleSyncMsg(
      IncomingMessage_T & imsg,
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handlePayloadMsg(
      IncomingMessage_T & imsg,
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleCompleteMsg(
      IncomingMessage_T & imsg,
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);

  /**
   * helper functions for handleActions
   */
  void handleInvokeAction(
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      InvokeAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & action,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleSendAction(
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      SendAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & action,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleCompleteAction(
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      CompleteAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & action,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleAwaitAction(
      FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & handler,
      AwaitAction<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> & action,
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
  void handleAbort(
      ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs);
};

#include <ff/FronctocolsManager.t.h>

} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_FRONCTOCOLS_MANAGER_H_

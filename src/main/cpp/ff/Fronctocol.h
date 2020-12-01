/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_FRONCTOCOL_H_
#define FF_FRONCTOCOL_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Actions.h>
#include <ff/FronctocolHandler.h>
#include <ff/Promise.h>
#include <ff/Util.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
class Fronctocol {
private:
  FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> const * handler = nullptr;
  ::std::vector<::std::unique_ptr<Action<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>> * actions = nullptr;

public:
  virtual ~Fronctocol() = default;

  /**
   * This handler is called once after a Fronctocol is invoked.
   */
  virtual void init() = 0;

  /**
   * This handler is called when a messages is received for this
   * fronctocol.
   */
  virtual void handleReceive(IncomingMessage_T & msg) = 0;

  /**
   * This handler is called after a child fronctocol completes.
   */
  virtual void handleComplete(Fronctocol & f) = 0;

  /**
   * This handler is called after an awaited PromiseFronctocol completes.
   */
  virtual void handlePromise(Fronctocol & pf) = 0;

  /**
   * Getters for cursory information about the fronctocol itself.
   *
   * getId is not intended for users, but it is probably also harmless.
   */
  Identity_T const & getSelf() const {
    return this->handler->self;
  }
  PeerSet_T const & getPeers() const {
    return this->handler->peers;
  }
  fronctocolId_t getId() const {
    return this->handler->id;
  }

  /**
   * Sends a message to a peer fronctocol on another server.
   */
  void send(::std::unique_ptr<OutgoingMessage_T> omsg);

  /**
   * Indicate that this fronctocol is finished.
   */
  void complete();

  /**
   * Indicates that this fronctocol is calling another fronctocol.
   */
  void invoke(
      ::std::unique_ptr<Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>> f,
      PeerSet_T const & p);

  /**
   * Indicates that this fronctocol is beginning a fronctocol as a
   * promise fronctocol.
   */
  template<typename Result_T>
  ::std::unique_ptr<Promise<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T,
      Result_T>>
  promise(
      ::std::unique_ptr<PromiseFronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T,
          Result_T>> pf,
      PeerSet_T const & p);

  /**
   * Indicates that this fronctocol is awaiting a promised fronctocol.
   */
  template<typename Result_T>
  void await(Promise<
             Identity_T,
             PeerSet_T,
             IncomingMessage_T,
             OutgoingMessage_T,
             Result_T> & pf);

  /**
   * Indicates to the framework that something went terribly wrong and
   * the protocol should be aborted.
   */
  void abort();

  /**
   * Sets the actions vector. Not intended for consumption.
   */
  void setActions(::std::vector<::std::unique_ptr<Action<
                      Identity_T,
                      PeerSet_T,
                      IncomingMessage_T,
                      OutgoingMessage_T>>> * actions);

  /**
   * Set the handler pointer. Not intended for consumption.
   */
  void setHandler(FronctocolHandler<
                  Identity_T,
                  PeerSet_T,
                  IncomingMessage_T,
                  OutgoingMessage_T> const * handler);

  /**
   * Return a name for the fronctocol, used for gathering metrics.
   */
  virtual ::std::string name();
};

#include <ff/Fronctocol.t.h>

} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_FRONCTOCOL_H_

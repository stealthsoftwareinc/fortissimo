/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_TESTER_TESTER_H_
#define FF_TESTER_TESTER_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <functional>
#include <utility>

/* Fortissimo Headers */
#include <ff/Fronctocol.h>

/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace tester {

/**
 * These are default test implementations which expect not to be
 * invoked.
 *
 * The Fronctocol* argument to each of these is the ``this`` fronctocol.
 * Use it for ``invoke``, ``send``, ``complete``, etc. methods.
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
failTestOnReceive();

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
failTestOnComplete();

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
finishTestOnComplete();

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
failTestOnPromise();

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
finishTestOnPromise();

/**
 * A Fronctocol designed for ease of use during testing. Its
 * implementations of ``init`` and ``handle[Action]`` invoke
 * lambdas which are given to the constructor.
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
class Tester : public Fronctocol<
                   Identity_T,
                   PeerSet_T,
                   IncomingMessage_T,
                   OutgoingMessage_T> {
private:
  ::std::function<void(Fronctocol<
                       Identity_T,
                       PeerSet_T,
                       IncomingMessage_T,
                       OutgoingMessage_T> *)>
      initHandler;

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
      completeHandler;

  ::std::function<void(
      IncomingMessage_T &,
      Fronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T> *)>
      receiveHandler;

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
      promiseHandler;

public:
  std::string name() override;

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
              OutgoingMessage_T> *)> complete =
          failTestOnComplete<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T>(),
      ::std::function<void(
          IncomingMessage_T &,
          Fronctocol<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> *)> receive =
          failTestOnReceive<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T>(),
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
              OutgoingMessage_T> *)> promise =
          failTestOnPromise<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T>());

  void init() override;
  void handleReceive(IncomingMessage_T & imsg) override;
  void handleComplete(Fronctocol<
                      Identity_T,
                      PeerSet_T,
                      IncomingMessage_T,
                      OutgoingMessage_T> & f) override;
  void handlePromise(Fronctocol<
                     Identity_T,
                     PeerSet_T,
                     IncomingMessage_T,
                     OutgoingMessage_T> & f) override;
};

#include <ff/tester/Tester.t.h>

} // namespace tester
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_TESTER_TESTER_H_

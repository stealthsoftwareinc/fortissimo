/**
 * Copyright Stealth Software Technologies, Inc.
 */

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <ff/Message.h>
#include <ff/PeerSet.h>

#include <mock.h>

#include <ff/logging.h>

static std::function<std::unique_ptr<IncomingMessage>(
    std::string const &, OutgoingMessage &)>
    message_converter =
        ff::posixnet::outgoingToIncomingMessage<::std::string>;

bool runTests(
    std::map<std::string, std::unique_ptr<Fronctocol>> & tests,
    uint64_t seed) {
  return ff::tester::
      runTests<std::string, PeerSet, IncomingMessage, OutgoingMessage>(
          tests, message_converter, seed);
}

bool runTests(
    std::map<std::string, std::unique_ptr<Fronctocol>> & tests) {
  return ff::tester::
      runTests<std::string, PeerSet, IncomingMessage, OutgoingMessage>(
          tests, message_converter);
}

const ::std::function<void(IncomingMessage &, Fronctocol *)>
    failTestOnReceive = ff::tester::failTestOnReceive<
        ::std::string,
        PeerSet,
        IncomingMessage,
        OutgoingMessage>();

const ::std::function<void(Fronctocol &, Fronctocol *)>
    failTestOnComplete = ff::tester::failTestOnComplete<
        ::std::string,
        PeerSet,
        IncomingMessage,
        OutgoingMessage>();

const ::std::function<void(Fronctocol &, Fronctocol *)>
    finishTestOnComplete = ff::tester::finishTestOnComplete<
        ::std::string,
        PeerSet,
        IncomingMessage,
        OutgoingMessage>();

const ::std::function<void(Fronctocol &, Fronctocol *)>
    failTestOnPromise = ff::tester::failTestOnPromise<
        ::std::string,
        PeerSet,
        IncomingMessage,
        OutgoingMessage>();

const ::std::function<void(Fronctocol &, Fronctocol *)>
    finishTestOnPromise = ff::tester::finishTestOnPromise<
        ::std::string,
        PeerSet,
        IncomingMessage,
        OutgoingMessage>();

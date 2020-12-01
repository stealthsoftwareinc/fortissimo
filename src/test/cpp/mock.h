/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_TESTS_MOCK_H_
#define FF_TESTS_MOCK_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Message.h>
#include <ff/PeerSet.h>
#include <ff/Promise.h>
#include <ff/VectorPeerSet.h>
#include <ff/posixnet/posixnet.h>
#include <ff/tester/Tester.h>
#include <ff/tester/runTests.h>

using IncomingMessage = ff::posixnet::IncomingMessage<::std::string>;
using OutgoingMessage = ff::posixnet::OutgoingMessage<::std::string>;

using PeerSet = ff::VectorPeerSet<::std::string>;

using Fronctocol = ff::Fronctocol<
    ::std::string,
    PeerSet,
    IncomingMessage,
    OutgoingMessage>;

template<typename Result_T>
using Promise = ff::Promise<
    ::std::string,
    PeerSet,
    IncomingMessage,
    OutgoingMessage,
    Result_T>;

template<typename Result_T>
using PromiseFronctocol = ff::PromiseFronctocol<
    ::std::string,
    PeerSet,
    IncomingMessage,
    OutgoingMessage,
    Result_T>;

/* Wrappers for the runTests function. */
bool runTests(
    std::map<std::string, std::unique_ptr<Fronctocol>> & tests,
    uint64_t seed);
bool runTests(
    std::map<std::string, std::unique_ptr<Fronctocol>> & tests);

using Tester = ff::tester::
    Tester<::std::string, PeerSet, IncomingMessage, OutgoingMessage>;

extern const ::std::function<void(IncomingMessage &, Fronctocol *)>
    failTestOnReceive;

extern const ::std::function<void(Fronctocol &, Fronctocol *)>
    failTestOnComplete;

extern const ::std::function<void(Fronctocol &, Fronctocol *)>
    finishTestOnComplete;

extern const ::std::function<void(Fronctocol &, Fronctocol *)>
    failTestOnPromise;

extern const ::std::function<void(Fronctocol &, Fronctocol *)>
    finishTestOnPromise;

#endif // FF_TESTS_MOCK_H_

/* Macro to shorten the correct list of fronctocol template parameters */
#ifndef TEST_TYPES
#define TEST_TYPES \
  std::string, PeerSet, IncomingMessage, OutgoingMessage
#endif

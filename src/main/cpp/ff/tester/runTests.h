/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */

/* C++ Headers */
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/FronctocolsManager.h>

/* logging configuration */
#include <ff/logging.h>

#ifndef FF_TEST_RUNNER_H_
#define FF_TEST_RUNNER_H_

namespace ff {
namespace tester {

/**
 * Returns true if all FronctocolsManagers report no errors (isAborted is
 * false).
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
bool runTests(
    ::std::map<
        Identity_T,
        ::std::unique_ptr<Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>> & tests,
    ::std::function<::std::unique_ptr<IncomingMessage_T>(
        Identity_T const & sender, OutgoingMessage_T & omsg)> &
        converter,
    uint64_t seed);

/**
 * Returns true if all FronctocolsManagers report no errors (isAborted is
 * false).
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
bool runTests(
    ::std::map<
        Identity_T,
        ::std::unique_ptr<Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>> & tests,
    ::std::function<::std::unique_ptr<IncomingMessage_T>(
        Identity_T const & sender, OutgoingMessage_T & omsg)> &
        converter);

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
PeerSet_T makeTestPeerSet(::std::map<
                          Identity_T,
                          ::std::unique_ptr<Fronctocol<
                              Identity_T,
                              PeerSet_T,
                              IncomingMessage_T,
                              OutgoingMessage_T>>> & tests);

#include <ff/tester/runTests.t.h>

} // namespace tester
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_TEST_RUNNER_H_

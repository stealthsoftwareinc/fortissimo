/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_BATCH_H_
#define FF_MPC_BATCH_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <deque>
#include <memory>
#include <utility>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Actions.h>
#include <ff/Fronctocol.h>
#include <ff/Message.h>

#include <mpc/templates.h>

/* Logging config */
#include <ff/logging.h>

namespace ff {
namespace mpc {

/**
 * A fronctocol which batches many sub-fronctocols to minimize overhead.
 * A condition of this batching is that all sub-fronctocols have the same
 * "pattern" of sends, recieves, invokes, and completes.
 *
 * A batched fronctocol should not expect, in handleReceive, that msg.length()
 * decreases all the way to zero. The same message is given to each child,
 * thus it decreases until on the last child it should reach zero.
 */
template<FF_TYPENAMES>
struct Batch : public Fronctocol<FF_TYPES> {
private:
  bool checkFirstLastActionTypes(
      ::std::deque<::std::vector<::std::unique_ptr<Action<FF_TYPES>>>> &
          actions);

  void handleActions(
      ::std::deque<::std::vector<::std::unique_ptr<Action<FF_TYPES>>>> &
          actions);

  ::std::unique_ptr<FronctocolHandler<FF_TYPES>> batchHandler = nullptr;

public:
  virtual ::std::string name() override;

  ::std::vector<::std::unique_ptr<Fronctocol<FF_TYPES>>> children;

  Batch();
  Batch(::std::vector<::std::unique_ptr<Fronctocol<FF_TYPES>>> && cs);

  void init() override;

  void handleReceive(IncomingMessage_T & imsg) override;

  void handleComplete(Fronctocol<FF_TYPES> & f) override;

  void handlePromise(Fronctocol<FF_TYPES> & f) override;

  /**
   * These virtual methods will be called by batch before the batch starts
   * and after it completes. This allows a subclass to do some setup
   * and teardown actions before and after the batch children are run.
   */
  virtual void onInit() {
  }
  virtual void onComplete() {
  }
};

} // namespace mpc
} // namespace ff

#include <mpc/Batch.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_BATCH_H_

/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_PROMISE_H_
#define FF_PROMISE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>
#include <utility>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <ff/Util.h>

/**
 * A Promise is a special way of invoking a fronctocol, which indicates
 * to the framework that its complete doesn't notify the fronctocol
 * which invoked it.
 *
 * Instead, The caller is given a special "Promise" object which it can
 * use or give away. The "Promise" can be redeemed exactly once to
 * be notified when the promised Fronctocol completes.
 */

namespace ff {

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Result_T>
class Promise;

/**
 * A special abstract class to describe all Promise Fronctocols.
 * It produces a result of the given ``Result_T`` type.
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Result_T>
struct PromiseFronctocol : public Fronctocol<
                               Identity_T,
                               PeerSet_T,
                               IncomingMessage_T,
                               OutgoingMessage_T> {
public:
  ::std::unique_ptr<Result_T> result;
};

/**
 * This is the special object that is given to the caller.
 * Calling ``Fronctocol::await()`` on an object of this type will
 * ask the framework to notify the ``await()`` caller when the
 * promised fronctocol completes.
 *
 * When the promised fronctocol completes the ``handlePromise``
 * method of the ``await()``er will be invoked.
 */
template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Result_T>
class Promise {
public:
  /**
   * getResult is paired with ``handlePromise`` to test if this
   * promise is the given fronctocol is paired to this promise.
   *
   * It returns conditionally either the result of its own
   * fronctocol, otherwise it returns nullptr.
   */
  ::std::unique_ptr<Result_T> getResult(Fronctocol<
                                        Identity_T,
                                        PeerSet_T,
                                        IncomingMessage_T,
                                        OutgoingMessage_T> & f);

  /**
   * The private constructor is friended with the ``promise()`` method
   * of ``Fronctocol`` to ensure that an empty promise is not created.
   */
  Promise(PromiseFronctocol<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T,
          Result_T> * pf) :
      promisedFronctocol(pf) {
  }

  Promise(Promise const & p) = delete;
  Promise & operator=(Promise const & p) = delete;

private:
  PromiseFronctocol<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T,
      Result_T> const * const promisedFronctocol;

  friend Fronctocol<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>;
};

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T,
    typename Result_T>
::std::unique_ptr<Result_T> Promise<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T,
    Result_T>::
    getResult(Fronctocol<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> & f) {
  if (f.getId() == this->promisedFronctocol->getId()) {
    return ::std::move(static_cast<PromiseFronctocol<
                           Identity_T,
                           PeerSet_T,
                           IncomingMessage_T,
                           OutgoingMessage_T,
                           Result_T> &>(f)
                           .result);
  } else {
    return ::std::unique_ptr<Result_T>(nullptr);
  }
}

} // namespace ff

#endif // FF_PROMISE_H_

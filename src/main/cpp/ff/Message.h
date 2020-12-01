/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MESSAGE_H_
#define FF_MESSAGE_H_

#include <array>
#include <climits>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include <sst/catalog/bignum.hpp>

/* Logging Configuration */
#include <ff/logging.h>

namespace ff {

template<typename Identity_T>
class IncomingMessage {
public:
  /**
   * The identity of this message's sender.
   */
  Identity_T const sender;

  IncomingMessage(Identity_T const & sender) : sender(sender) {
  }

  IncomingMessage(IncomingMessage const &) = delete;
  IncomingMessage(IncomingMessage &&) = delete;
  IncomingMessage & operator=(IncomingMessage const &) = delete;
  IncomingMessage & operator=(IncomingMessage &&) = delete;

  virtual ~IncomingMessage() = default;

  /**
   * Remove len many bytes from the message's buffer and place said bytes
   * into buf. Buf must not be null, and must have at least len many bytes
   * available to write into.
   *
   * Returns the actual number of bytes read.
   */
  virtual size_t remove(void * buf, size_t const len) = 0;

  /**
   * Returns the number of bytes available to be removed from the incoming
   * message.
   */
  virtual size_t length() const = 0;

  /**
   * Empty the remaining contents of the message, without reading them.
   */
  virtual void clear() = 0;

  /**
   * Reads a value of given type from this message.
   *
   * return false on failure;
   */
  template<typename Value_T>
  bool read(Value_T & result);
};

template<typename Identity_T>
class OutgoingMessage {
public:
  /**
   * The identity of this messages intended recipient.
   */
  Identity_T const recipient;

  OutgoingMessage(Identity_T const & recipient) : recipient(recipient) {
  }

  OutgoingMessage(OutgoingMessage const &) = delete;
  OutgoingMessage(OutgoingMessage &&) = delete;
  OutgoingMessage & operator=(OutgoingMessage const &) = delete;
  OutgoingMessage & operator=(OutgoingMessage &&) = delete;

  virtual ~OutgoingMessage() = default;

  /**
   * Copies the content of buf onto the end of the outgoing message.
   *
   * returns the number of bytes written.
   */
  virtual size_t add(void const * buf, size_t const nchars) = 0;

  /**
   * Copies the content of buf onto the beginning of the outgoing message.
   *
   * returns the number of bytes written.
   */
  virtual size_t prepend(void const * buf, size_t const nchars) = 0;

  /**
   * Returns the number of bytes already written to the outgoing message.
   */
  virtual size_t length() const = 0;

  /**
   * Empty the contents of the message, before they are sent.
   */
  virtual void clear() = 0;

  /**
   * Writes a value of the given type to this message.
   *
   * return false on failure.
   */
  template<typename Value_T>
  bool write(Value_T const & result);
};

/**
 * To allow read/write to/from an arbitrary type, implement the
 * following function overloads.
 *
 *   template<typename Identity_T>
 *   bool ff::msg_read(IncomingMessage_T<Identity_T>&, YourType&)
 *   template<typename Identity_T>
 *   bool ff::msg_write(OutgoingMessage_T<Identity_T>&, YourType const&)
 *
 * A number of them are prewritten for integer types and strings.
 */
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, uint8_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, uint16_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, uint32_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, uint64_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, int8_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, int16_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, int32_t &);
template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, int64_t &);

template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, uint8_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, uint16_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, uint32_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, uint64_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, int8_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, int16_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, int32_t const);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, int64_t const);

template<typename Identity_T>
bool msg_read(IncomingMessage<Identity_T> &, ::std::string &);
template<typename Identity_T>
bool msg_write(OutgoingMessage<Identity_T> &, ::std::string const &);

} // namespace ff

#include <ff/Message.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MESSAGE_H_

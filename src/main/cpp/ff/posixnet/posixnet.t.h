/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace posixnet {

constexpr static time_t CLOSE_TIMEOUT = 60; // seconds;
constexpr static size_t OUTGOING_MESSAGE_SIZE_FLOOR = 16; // bytes

template<typename Identity_T, typename PeerSet_T>
PeerSet_T PeerInfos2PeerSet(
    std::vector<PeerInfo<Identity_T>> const & peer_infos) {
  PeerSet_T builder;
  for (size_t i = 0; i < peer_infos.size(); i++) {
    builder.add(peer_infos[i].identity);
  }

  // intentional copy to allow optional sorting.
  PeerSet_T ret(builder);
  return ret;
}

template<typename Identity_T>
size_t
IncomingMessage<Identity_T>::remove(void * buf, size_t const len) {
  log_assert(buf != nullptr);

  size_t real_len;
  if (len > this->size - this->place) {
    real_len = this->size - this->place;
  } else {
    real_len = len;
  }

  memcpy(buf, this->buffer + this->place, real_len);
  this->place += real_len;

  return real_len;
}

template<typename Identity_T>
size_t IncomingMessage<Identity_T>::length() const {
  return this->size - this->place;
}

template<typename Identity_T>
void IncomingMessage<Identity_T>::clear() {
  this->place = this->size;
}

template<typename Identity_T>
IncomingMessage<Identity_T>::IncomingMessage(
    Identity_T const & id, uint8_t * buf, size_t size, size_t place) :
    ff::IncomingMessage<Identity_T>(id),
    buffer(buf),
    size(size),
    place(place) {
}

template<typename Identity_T>
IncomingMessage<Identity_T>::~IncomingMessage() {
  if (this->buffer != nullptr) {
    free(this->buffer);
  }
}

template<typename Identity_T>
IncomingMessage<Identity_T>::Cache::Cache(
    uint8_t controlBlock,
    uint8_t * buffer,
    size_t size,
    size_t place,
    Identity_T const & sender) :
    controlBlock(controlBlock),
    incomingMessage(sender, buffer, size, place) {
}

template<typename Identity_T>
::std::unique_ptr<typename IncomingMessage<Identity_T>::Cache>
IncomingMessage<Identity_T>::createCache(uint8_t const control_block) {
  log_assert(this->buffer != nullptr);

  ::std::unique_ptr<Cache> cache(new Cache(
      control_block,
      this->buffer,
      this->size,
      this->place,
      this->sender));

  this->buffer = nullptr;

  return cache;
}

template<typename Identity_T>
IncomingMessage<Identity_T> *
IncomingMessage<Identity_T>::Cache::uncache() {
  return &this->incomingMessage;
}

template<typename Identity_T>
bool OutgoingMessage<Identity_T>::makeSpace(size_t const nchars) {
  if (this->buffer == nullptr) {
    // nchars * 1.5
    this->capacity = nchars + (nchars >> 1);
    // capacity overflowed, so don't try to upsize it.
    if (this->capacity < nchars) {
      this->capacity = nchars;
    }
    // floor starting space, along with solving the int(1 * 1.5) == 1 problem.
    if (this->capacity < OUTGOING_MESSAGE_SIZE_FLOOR) {
      this->capacity = OUTGOING_MESSAGE_SIZE_FLOOR;
    }
    this->buffer = (uint8_t *)malloc(this->capacity);
  } else {
    if (this->place + nchars < this->place) {
      log_error("Message overflowed");
      return false;
    } else {
      while (nchars > this->capacity - this->place) {
        size_t prev_cap = this->capacity;
        this->capacity = this->capacity + (this->capacity >> 1);
        // capacity * 1.5 adjustment overflowed
        if (this->capacity < prev_cap) {
          this->capacity = this->place + nchars;
        }
      }
    }
    this->buffer =
        (uint8_t *)realloc((void *)this->buffer, this->capacity);
  }
  if (this->buffer == nullptr) {
    log_perror();
    return false;
  } else {
    return true;
  }
}

template<typename Identity_T>
size_t OutgoingMessage<Identity_T>::add(
    void const * buf, size_t const nchars) {
  if (!this->makeSpace(nchars)) {
    return 0;
  }

  memcpy(this->buffer + this->place, buf, nchars);
  this->place += nchars;

  return nchars;
}

template<typename Identity_T>
size_t OutgoingMessage<Identity_T>::prepend(
    void const * buf, size_t const nchars) {
  this->makeSpace(nchars);

  memmove(this->buffer + nchars, this->buffer, this->place);
  memcpy(this->buffer, buf, nchars);
  this->place += nchars;

  return nchars;
}

template<typename Identity_T>
size_t OutgoingMessage<Identity_T>::length() const {
  return this->place;
}

template<typename Identity_T>
void OutgoingMessage<Identity_T>::clear() {
  free(this->takeBuffer());
}

template<typename Identity_T>
OutgoingMessage<Identity_T>::OutgoingMessage(Identity_T const & id) :
    ff::OutgoingMessage<Identity_T>(id) {
}

template<typename Identity_T>
OutgoingMessage<Identity_T>::~OutgoingMessage() {
  if (this->buffer != nullptr) {
    log_warn(
        "Outgoing Message to %s deleted before message sent.",
        identity_to_string(this->recipient).c_str());
    free(this->buffer);
  }
}

template<typename Identity_T>
uint8_t * OutgoingMessage<Identity_T>::takeBuffer() {
  uint8_t * ret = this->buffer;
  this->buffer = nullptr;
  this->capacity = 0;
  this->place = 0;
  return ret;
}

template<typename Identity_T>
uint8_t const * OutgoingMessage<Identity_T>::getBuffer() const {
  return this->buffer;
}

template<typename Identity_T>
::std::unique_ptr<IncomingMessage<Identity_T>>
outgoingToIncomingMessage(
    Identity_T const & idty, OutgoingMessage<Identity_T> & omsg) {
  size_t const length = omsg.length();
  return ::std::unique_ptr<IncomingMessage<Identity_T>>(
      new IncomingMessage<Identity_T>(idty, omsg.takeBuffer(), length));
}

} // namespace posixnet

template<typename Identity_T>
bool msg_write(
    ::ff::OutgoingMessage<Identity_T> & omsg,
    ::ff::posixnet::OutgoingMessage<Identity_T> const & data) {
  return omsg.add(data.getBuffer(), data.length()) == data.length();
}

namespace posixnet {

/**
 * The connection handler is a helper class to encapsulate some of the
 * socket level minutia.
 */
template<typename Identity_T>
class ConnectionHandler {
  /**
   * buffers to be sent.
   */
  ::std::deque<::std::pair<size_t, uint8_t *>> outgoingBuffers;

  size_t outgoingBufferPlace = 0;

  /**
   * Buffer for reading in messages.
   */
  uint8_t * incomingBuffer = nullptr;
  size_t incomingBufferLen = 0;
  uint8_t incomingLenBuffer[sizeof(uint64_t)];
  size_t incomingLenBufferLen = 0;

  size_t incomingLen = 0;
  bool hasIncomingLen = false;

public:
  /**
   * Pointer (non-owned) to this connection's pollfd
   */
  ff_pollfd * const pfd;

  /**
   * Indicates that a reconnect attempt is necessary.
   */
  bool needReconnect = false;

  /**
   * Indicates that the first connection has been made successfully.
   */
  bool connected = false;

  /**
   * Identity of the peer this connection goes to.
   */
  Identity_T const & peer;

  ConnectionHandler(ConnectionHandler &) = delete;
  ConnectionHandler & operator=(ConnectionHandler &) = delete;
  ConnectionHandler && operator=(ConnectionHandler &&) = delete;

  ConnectionHandler(Identity_T const & peer, ff_pollfd * const pfd) :
      pfd(pfd), peer(peer) {
    log_debug("ConnectionHandler constructor");
  }

  ConnectionHandler(ConnectionHandler && other) :
      outgoingBuffers(::std::move(other.outgoingBuffers)),
      pfd(other.pfd),
      needReconnect(other.needReconnect),
      peer(other.peer) {
    log_debug("ConnectionHandler move constructor");
  }

  ~ConnectionHandler() {
    log_debug(
        "ConnectionHandler destructor, %zu buffers remaining",
        this->outgoingBuffers.size());
    for (size_t i = 0; i < this->outgoingBuffers.size(); i++) {
      free(this->outgoingBuffers[i].second);
    }
  }

  /**
   * Indicates that the outgoing queue is empty.
   */
  bool isOutgoingQueueEmpty() {
    return this->outgoingBuffers.size() == 0;
  }

  /**
   * When a new connection is made, accept the connection and setup
   * an IncomingConnectionInitializer to read the identity.
   */
  void acceptNewConn(
      ff_pollfd * pollfds,
      size_t num_pollfds,
      ::std::vector<ConnectionHandler<Identity_T>> & conns) {
    log_trace("calling accept");
    int newfd = accept(this->pfd->fd, nullptr, nullptr);
    log_trace("called accept");
    if (newfd < 0) {
      log_error("accepted file descriptor is bad");
      log_perror();
    } else {
      for (size_t i = conns.size(); i < num_pollfds; i++) {
        if (pollfds[i].fd == -1) {
          log_assert(i < num_pollfds);
          log_assert(i >= conns.size());
          log_debug("accepting with i=%zu, newfd=%i", i, newfd);

          pollfds[i].fd = newfd;

          log_trace("calling (re) set_non_blocking_socket");
          SET_NON_BLOCKING_SOCKET(pollfds[i].fd);
          log_trace("called (re) set_non_blocking_socket");

          int const nodelay = 1;
          log_trace("calling set_no_delay");
          if (FF_SET_NO_DELAY(pollfds[i].fd, &nodelay) != 0) {
            log_perror();
            close(pollfds[i].fd);
            pollfds[i].fd = -1;
            break;
          }
          log_trace("called set_no_delay");

          pollfds[i].events = FF_POLLIN;
          break;
        }
      }
    }
  }

  /**
   * read incoming messages. This is a two (or more) step process.
   * 
   *  1. Read the length of the message, then return nullptr.
   *  2. Read the message then return an IncomingMessage object.
   *
   * This is split into two steps because the length is needed to allocate
   * a message buffer, and multiple calls to recv could block.
   */
  ::std::unique_ptr<IncomingMessage<Identity_T>> handleInput() {
    ::std::unique_ptr<IncomingMessage<Identity_T>> ret = nullptr;
    ssize_t inlen; // declared ahead of time for common error handling.
    if (!this->hasIncomingLen) {
      // Read the message header, upto one uint8_t, remembering that
      // recv might not return the entire buffer requested.
      log_trace(
          "calling recv for message length, fd: %i", this->pfd->fd);
      inlen = FF_RECV(
          this->pfd->fd,
          this->incomingLenBuffer + this->incomingLenBufferLen,
          sizeof(uint64_t) - this->incomingLenBufferLen,
          0);
      log_trace("called recv for message length, got %zi", inlen);
      if (inlen > 0) {
        this->incomingLenBufferLen += (size_t)inlen;
        log_assert(this->incomingLenBufferLen <= sizeof(uint64_t));
        if (this->incomingLenBufferLen == sizeof(uint64_t)) {
          this->incomingLen =
              (size_t)ff::buffer_to_uint64(this->incomingLenBuffer);
          log_debug("incoming length %zu", this->incomingLen);
          this->hasIncomingLen = true;
          this->incomingLenBufferLen = 0;
        } else {
          log_debug("waiting for more message length");
        }
      }
    } else {
      if (this->incomingBuffer == nullptr) {
        log_trace(
            "Mallocing incoming message buffer, fd: %i", this->pfd->fd);
        this->incomingBuffer = (uint8_t *)malloc(this->incomingLen);
        if (this->incomingBuffer == nullptr) {
          log_perror();
          ret = nullptr;
          return ret;
        }
      }
      // Read the entire message, remembering it may be delivered in parts.
      log_trace(
          "calling recv for incoming message, fd: %i, have %zu of %zu",
          this->pfd->fd,
          this->incomingBufferLen,
          this->incomingLen);
      inlen = FF_RECV(
          this->pfd->fd,
          this->incomingBuffer + this->incomingBufferLen,
          this->incomingLen - this->incomingBufferLen,
          0);
      log_trace("called recv for incoming message, got: %zd", inlen);
      if (inlen > 0) {
        this->incomingBufferLen += (size_t)inlen;
        log_debug(
            "read %zu of message length %zu",
            this->incomingBufferLen,
            this->incomingLen);
        log_assert(this->incomingBufferLen <= this->incomingLen);
        if (this->incomingBufferLen == this->incomingLen) {
          // Construct the message, and reset for the next message.
          log_trace("returning a newly read message");
          ret = ::std::unique_ptr<IncomingMessage<Identity_T>>(
              new IncomingMessage<Identity_T>(
                  this->peer, this->incomingBuffer, this->incomingLen));
          this->hasIncomingLen = false;
          this->incomingBuffer = nullptr;
          this->incomingLen = 0;
          this->incomingBufferLen = 0;
        }
      }
    }

    if (inlen < 0 && errno != EAGAIN) {
      if (errno == ECONNRESET) {
        log_error("peer connection closed forcibly");
      } else {
        log_error("peer connection error");
        log_perror();
      }
      this->needReconnect = true;
    } else if (inlen == 0 && errno != EAGAIN) {
      log_error("peer closed.");
      this->needReconnect = true;
    } else {
      log_debug("trying recv again, because EAGAIN");
    }

    return ret;
  }

  /**
   * the first message sent is always this party's identity.
   *
   * Sending is performed in one step, unless the OS breaks it up by
   * returning that fewer than requested bytes were written with write.
   */
  void handleOutput(Identity_T const & self) {
    if (!this->connected) {
      /* Prepend the identity message to the outgoing message queue */
      OutgoingMessage<Identity_T> om(this->peer);
      om.write(self);
      uint8_t lenBuf[sizeof(uint64_t)];
      uint64_to_buffer((uint64_t)om.length(), lenBuf);
      om.prepend(lenBuf, sizeof(uint64_t));
      this->outgoingBufferPlace = 0;
      size_t const length = om.length();
      this->outgoingBuffers.push_front(
          ::std::pair<size_t, uint8_t *>(length, om.takeBuffer()));
      this->connected = true;
      log_debug("added identity message to front of queue");
      log_info(
          "Outgoing connection to %s established",
          identity_to_string(this->peer).c_str());
    }

    if (this->outgoingBuffers.size() == 0) {
      /* Check if there's nothing to send, and disable POLLOUT, otherwise
       * POLLOUT will be given repeatedly, sort of just spamming.
       *
       * Enqueueing a new message will reenable POLLOUT
       */
      log_debug("no outgoing messages to send, fd: %i", this->pfd->fd);
      this->pfd->events = this->pfd->events & ~FF_POLLOUT;
      return;
    }

    /* Write the first buffer out. There are a few helper attributes
     * to track partially sent messages
     */
    log_trace(
        "calling write for a message, fd: %i, sent %zu of %zu",
        this->pfd->fd,
        this->outgoingBufferPlace,
        this->outgoingBuffers.front().first);
    ssize_t outlen = write(
        this->pfd->fd,
        this->outgoingBuffers.front().second +
            this->outgoingBufferPlace,
        this->outgoingBuffers.front().first -
            this->outgoingBufferPlace);
    log_trace("called write for a message, sent %zd more", outlen);
    if (outlen >= 0) {
      /* Check how much of the buffer got sent */
      this->outgoingBufferPlace += (size_t)outlen;
      log_debug("more to send");
      log_assert(
          this->outgoingBufferPlace <=
          this->outgoingBuffers.front().first);
      if (this->outgoingBufferPlace ==
          this->outgoingBuffers.front().first) {
        log_debug("finished sending buffer");
        /* Delete the buffer if it is complete */
        this->outgoingBufferPlace = 0;
        free(this->outgoingBuffers.front().second);
        this->outgoingBuffers.pop_front();
        log_debug("finished writing message to peer");
      }
    } else {
      if (errno != EAGAIN) {
        log_error("error while writing.");
        log_perror();
      } else {
        log_debug("try write again, because EAGAIN");
      }
    }
  }

  void enqueOutgoingMessage(
      ::std::unique_ptr<OutgoingMessage<Identity_T>> out_msg) {
    /* (re) enable POLLOUT event so that the OS can advise when to send
     * without blocking.
     */
    this->pfd->events = this->pfd->events | FF_POLLOUT;

    /* Prepend the message length */
    log_debug(
        "enqueing outgoing message length %zu", out_msg->length());
    uint8_t lenBuf[sizeof(uint64_t)];
    uint64_to_buffer((uint64_t)out_msg->length(), lenBuf);
    out_msg->prepend(lenBuf, sizeof(uint64_t));

    /* Add the message to the queue */
    size_t const length = out_msg->length();
    this->outgoingBuffers.push_back(
        ::std::pair<size_t, uint8_t *>(length, out_msg->takeBuffer()));
  }

  /**
   * During setup, one server might start first, and the other gets a
   * "could not connect". This will attempt to reconnect after a little bit.
   */
  void attemptReconnect(
      Identity_T const & self, PeerInfo<Identity_T> const & peer_info) {
    /* Close the old socket */
    if (this->pfd->fd >= 0) {
      shutdown(this->pfd->fd, SHUT_RDWR);
      close(this->pfd->fd);
    }

    if (this->peer > self) {
      /* peer will attempt to re connect(3P) */
      log_debug("waiting for peer to reconnect");
      this->pfd->fd = -1;
      this->needReconnect = false;
      this->connected = false;
    } else {
      /* I must attempt to reconnect */
      log_debug("attempting to reconnect");
      log_trace("calling (re) socket");
      this->pfd->fd = socket(AF_INET, SOCK_STREAM, 0);
      if (this->pfd->fd < 0) {
        log_perror();
        this->needReconnect = true;
        this->connected = false;
        this->pfd->fd = -1;
        this->pfd->events = FF_POLLIN;
        return;
      }
      log_trace("called (re) socket");

      log_trace("calling (re) connect");
      log_info(
          "re connecting to %s",
          identity_to_string(this->peer).c_str());
      if (connect(
              this->pfd->fd,
              (sockaddr *)&peer_info.address,
              sizeof(sockaddr_storage)) != 0 &&
          errno != EINPROGRESS) {
        log_perror();
        this->needReconnect = true;
        this->connected = false;
        close(this->pfd->fd);
        this->pfd->fd = -1;
        return;
      }
      log_trace("called (re) connect");

      log_trace("calling (re) set_non_blocking_socket");
      SET_NON_BLOCKING_SOCKET(this->pfd->fd);
      log_trace("called (re) set_non_blocking_socket");

      int const nodelay = 1;
      log_trace("calling (re) set_no_delay");
      if (FF_SET_NO_DELAY(this->pfd->fd, &nodelay) != 0) {
        log_perror();
        this->needReconnect = true;
        this->connected = false;
        close(this->pfd->fd);
        this->pfd->fd = -1;
        return;
      }
      log_trace("called (re) set_no_delay");

      this->needReconnect = false;
      this->connected = false;
      this->pfd->events = FF_POLLIN | FF_POLLOUT;
    }
  }
};

/**
 * When a connection comes in the first thing to read is the identity.
 * This class reads the identity, using a higher order pollfd. 
 * 
 * It indicates to control code that it can move the file descriptor
 * to a lower order pollfd by giving an identity to move it too.
 */
template<typename Identity_T>
class IncomingConnectionInitializer {
  uint8_t lenBuf[sizeof(uint64_t)];
  size_t lenBufLen = 0;
  size_t length = 0;
  uint8_t * buf = nullptr;
  size_t bufLen = 0;

public:
  bool identityReady;
  Identity_T peer;

  /**
   * Read the length of the identity message, then read the
   * identity message. Like in ConnectionHandler, length and message
   * are read separately.
   */
  void readInput(ff_pollfd * pfd, Identity_T const & self) {
    ssize_t inlen =
        0; // declared ahead of time for common error handling
    if (this->lenBufLen < sizeof(uint64_t)) {
      /* Read the length first */
      log_trace(
          "calling recv for peer identity length, fd: %i", pfd->fd);
      inlen = FF_RECV(
          pfd->fd,
          this->lenBuf + this->lenBufLen,
          sizeof(uint64_t) - this->lenBufLen,
          0);
      log_trace(
          "called recv for peer identity length, got: %zd", inlen);
      if (inlen > 0) {
        this->lenBufLen += (size_t)inlen;
        log_assert(this->lenBufLen <= sizeof(uint64_t));
        if (this->lenBufLen == sizeof(uint64_t)) {
          log_debug("read the whole peer identity length");
          /* When the length is ready, create a buffer of sufficient size */
          this->length = buffer_to_uint64(this->lenBuf);
          this->buf = (uint8_t *)malloc(this->length);
        } else {
          log_debug("waiting for more peer identity length");
        }
        return;
      }
    } else {
      /* Read the message and read the identity, then indicate the
       * identity to control code.
       */
      log_trace(
          "calling recv for peer identity, fd: %i, have %zu of %zu",
          pfd->fd,
          this->bufLen,
          this->length);
      inlen = FF_RECV(
          pfd->fd,
          this->buf + this->bufLen,
          this->length - this->bufLen,
          0);
      log_trace("called recv for peer identity, got: %zd", inlen);
      if (inlen > 0) {
        this->bufLen += (size_t)inlen;
        log_assert(this->bufLen <= this->length);
        if (this->bufLen == this->length) {
          log_debug("read full peer identity");
          IncomingMessage<Identity_T> im(self, this->buf, this->length);
          this->buf = nullptr;
          im.read(this->peer);
          this->identityReady = true;
        } else {
          log_debug("waiting for more peer identity");
        }
        return;
      }
    }

    bool close_sck = false;
    if (inlen < 0 && errno != EAGAIN) {
      if (errno == ECONNRESET) {
        log_error("incoming connection closed forcibly");
      } else {
        log_error("incoming connection error");
        log_perror();
      }

      close_sck = true;
    } else if (inlen == 0 && errno != EAGAIN) {
      log_error("incoming connection closed.");

      close_sck = true;
    } else if (errno == EAGAIN) {
      log_debug("try recv again, because EAGAIN");
    } else {
      log_perror();
    }

    if (close_sck) {
      this->reset();
      shutdown(pfd->fd, SHUT_RDWR);
      close(pfd->fd);
      pfd->fd = -1;
    }

    return;
  }

  /**
   * Reset this IncomingConnectionInitializer so it can be reused
   */
  void reset() {
    log_debug("resetting incoming connection initializer");
    this->lenBufLen = 0;
    this->length = 0;
    if (this->buf != nullptr) {
      free(this->buf);
    }
    this->buf = nullptr;
    this->bufLen = 0;
    this->identityReady = false;
  }
};

struct PollfdWrap {
  ff_pollfd * pollfds = nullptr;
  size_t num_pollfds = 0;

  PollfdWrap() = default;

  PollfdWrap(PollfdWrap const &) = delete;
  PollfdWrap(PollfdWrap &&) = delete;
  PollfdWrap & operator=(PollfdWrap const &) = delete;
  PollfdWrap & operator=(PollfdWrap &&) = delete;

  ~PollfdWrap() {
    if (this->pollfds == nullptr) {
      return;
    }
    for (size_t i = 0; i < this->num_pollfds; i++) {
      if (this->pollfds[i].fd >= 0) {
        close(this->pollfds[i].fd);
      }
    }
    free(this->pollfds);
  }
};

template<typename Identity_T, typename PeerSet_T>
bool runFortissimoPosixNet(
    std::unique_ptr<ff::Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage<Identity_T>,
        OutgoingMessage<Identity_T>>> mainFronctocol,
    std::vector<PeerInfo<Identity_T>> const & peers_info,
    Identity_T const & self) {
  /* Step 1. make an array of pollfds. There should be one element for each
   * peer_info, and an additional pollfd for each peer_info with identity
   * greater than self.
   *
   * The pollfd array should be coindexed by the peer_info index, upto the
   * last peer_info. subsequent peer_infos are for accepted connections
   * before the identity of the connection is confirmed.
   *
   * Also make a vector of ConnectionHandlers, one per peer_info.
   *
   * Also make a vector of IncomingConnectionInitializers, one for each pollfd
   * extra than ConnectionHandlers.
   *
   * Also make a map of Identity_T to peers_info or conns index.
   */
  size_t num_accepts = 0;
  for (PeerInfo<Identity_T> const & peer : peers_info) {
    if (peer.identity > self) {
      num_accepts++;
    }
  }

  PollfdWrap pfdw;
  pfdw.num_pollfds = peers_info.size() + num_accepts;
  pfdw.pollfds =
      (ff_pollfd *)malloc(sizeof(ff_pollfd) * pfdw.num_pollfds);
  if (pfdw.pollfds == nullptr) {
    log_perror();
    return false;
  }
  memset(pfdw.pollfds, 0, pfdw.num_pollfds * sizeof(ff_pollfd));
  for (size_t i = 0; i < pfdw.num_pollfds; i++) {
    pfdw.pollfds[i].fd = -1;
    pfdw.pollfds[i].events = FF_POLLIN | FF_POLLOUT;
  }

  ::std::vector<ConnectionHandler<Identity_T>> conns;
  conns.reserve(peers_info.size());
  for (size_t i = 0; i < peers_info.size(); i++) {
    conns.emplace_back(peers_info[i].identity, &pfdw.pollfds[i]);
  }

  ::std::vector<IncomingConnectionInitializer<Identity_T>> initers;
  initers.reserve(pfdw.num_pollfds - conns.size());
  for (size_t i = conns.size(); i < pfdw.num_pollfds; i++) {
    initers.emplace_back();
  }

  /* A lambda for distributing outgoing messages to connections */
  auto distribute =
      [&conns](::std::vector<::std::unique_ptr<
                   OutgoingMessage<Identity_T>>> & out_msgs) -> void {
    for (size_t i = 0; i < out_msgs.size(); i++) {
      for (size_t j = 0; j < conns.size(); j++) {
        log_assert(out_msgs[i] != nullptr);
        if (out_msgs[i]->recipient == conns[j].peer) {
          conns[j].enqueOutgoingMessage(::std::move(out_msgs[i]));
          break;
        }
      }
    }
  };

  /* Step 2. open a listen socket for self */
  {
    bool self_made = false;
    for (size_t i = 0; i < peers_info.size(); i++) {
      if (peers_info[i].identity == self) {
        log_trace("calling socket for listen socket");
        pfdw.pollfds[i].fd =
            socket(peers_info[i].address.ss_family, SOCK_STREAM, 0);
        if (pfdw.pollfds[i].fd < 0) {
          log_perror();
          return false;
        }
        log_trace("called socket for listen socket");

        int enable = 1;
        log_trace("calling setsocketopt for listen socket");
        if (FF_SETSOCKOPT(
                pfdw.pollfds[i].fd,
                SOL_SOCKET,
                SO_REUSEADDR,
                (void *)&enable,
                sizeof(enable)) < 0) {
          log_perror();
          return false;
        }
        log_trace("called setsocketopt for listen socket");

        log_trace("calling set_non_blocking_socket for listen socket");
        SET_NON_BLOCKING_SOCKET(pfdw.pollfds[i].fd);
        log_trace("called set_non_blocking_socket for listen socket");

        if (AF_INET == peers_info[i].address.ss_family) {
          log_debug(
              "binding to port %hu",
              ntohs(((sockaddr_in *)&peers_info[i].address)->sin_port));
        }
        log_trace("calling bind for listen socket");
        if (bind(
                pfdw.pollfds[i].fd,
                (sockaddr *)&peers_info[i].address,
                sizeof(sockaddr_storage)) != 0) {
          log_perror();
          return false;
        }
        log_trace("called bind for listen socket");

        log_trace("calling listen for listen socket");
        if (listen(pfdw.pollfds[i].fd, SOMAXCONN) != 0) {
          log_perror();
          return false;
        }
        log_trace("called listen for listen socket");
        pfdw.pollfds[i].events = FF_POLLIN;

        self_made = true;
        conns[i].connected = true;
      }
    }

    if (self_made == false) {
      log_error("No accept socket for incoming connections");
      return false;
    }
  }

  /* Step 3. open a connect socket for each peer_info with an identity less
   * than self.
   */
  log_info("has %zu peers", peers_info.size());
  for (size_t i = 0; i < peers_info.size(); i++) {
    if (peers_info[i].identity < self) {
      log_trace("calling socket for peer connection");
      pfdw.pollfds[i].fd = socket(AF_INET, SOCK_STREAM, 0);
      if (pfdw.pollfds[i].fd < 0) {
        log_perror();
        conns[i].needReconnect = true;
      }
      log_trace("called socket for peer connection");

      log_trace("calling connect for peer connection");
      log_info(
          "connecting to %s",
          identity_to_string(peers_info[i].identity).c_str());
      if (connect(
              pfdw.pollfds[i].fd,
              (sockaddr *)&peers_info[i].address,
              sizeof(sockaddr_storage)) != 0 &&
          errno != EINPROGRESS) {
        log_perror();
        conns[i].needReconnect = true;
        close(pfdw.pollfds[i].fd);
        pfdw.pollfds[i].fd = -1;
      }
      log_trace("called connect for peer connection");

      log_trace("calling set_non_blocking_socket for peer connection");
      SET_NON_BLOCKING_SOCKET(pfdw.pollfds[i].fd);
      log_trace("called set_non_blocking_socket for peer connection");

      int const nodelay = 1;
      log_trace("calling set_no_delay for peer connection");
      if (pfdw.pollfds[i].fd > 0 &&
          FF_SET_NO_DELAY(pfdw.pollfds[i].fd, &nodelay) != 0) {
        log_perror();
        conns[i].needReconnect = true;
        close(pfdw.pollfds[i].fd);
        pfdw.pollfds[i].fd = 1;
      }
      log_trace("called set_no_delay for peer connection");

      pfdw.pollfds[i].events = FF_POLLIN | FF_POLLOUT;
    } else {
      log_info(
          "peer %s will connect to me",
          identity_to_string(peers_info[i].identity).c_str());
    }
  }

  /* Step 4. create the fronctocols manager, but do not init it. */
  FronctocolsManager<
      Identity_T,
      PeerSet_T,
      IncomingMessage<Identity_T>,
      OutgoingMessage<Identity_T>>
      fmanager(self);

  /* Step 5. begin the poll loop. */
  bool connected = false; // all connections established.

  log_debug("num_pollfds=%zu", pfdw.num_pollfds);

  while (!fmanager.isClosed()) {
    log_trace("calling poll");
    int num_ready = FF_POLL(pfdw.pollfds, pfdw.num_pollfds, 2000);
    log_trace("called poll");

    if (num_ready < 0) {
      log_perror();
    } else if (num_ready == 0) {
      if (fmanager.isFinished()) {
        log_info("time out while waiting for peers to finish");
      } else if (connected) {
        log_info("time out while waiting for new messages");
      } else {
        log_info("time out while waiting for peers to connect");
      }
    }

    for (size_t i = 0; i < pfdw.num_pollfds; i++) {
      log_debug(
          "i=%zu, fd=%i, POLLIN=%i POLLOUT=%i",
          i,
          pfdw.pollfds[i].fd,
          pfdw.pollfds[i].events & FF_POLLIN,
          pfdw.pollfds[i].events & FF_POLLOUT);
      log_debug("Revents: %i", pfdw.pollfds[i].revents);

      if (i >= conns.size()) {
        /* This is an incoming connection, ready to read the identity */
        bool fail = false;
        if (pfdw.pollfds[i].revents & FF_POLLIN) {
          log_debug("reading identity from new connection");
          initers[i - conns.size()].readInput(&pfdw.pollfds[i], self);
          if (initers[i - conns.size()].identityReady) {
            Identity_T const & identity =
                initers[i - conns.size()].peer;

            size_t idx = 0;
            for (size_t j = 0; j < conns.size(); j++) {
              if (conns[j].peer == identity) {
                idx = j;
                log_debug(
                    "idx=%lu, %s, %s",
                    idx,
                    identity_to_string(identity).c_str(),
                    identity_to_string(conns[j].peer).c_str());
                break;
              }
            }

            pfdw.pollfds[idx].fd = pfdw.pollfds[i].fd;
            conns[idx].connected = true;
            conns[idx].needReconnect = false;
            pfdw.pollfds[i].fd = -1;
            pfdw.pollfds[i].events = FF_POLLIN | FF_POLLOUT;
            initers[i - conns.size()].reset();

            log_info(
                "Incoming connection from %s established",
                identity_to_string(conns[idx].peer).c_str());
          }
        }
        // POLLOUT should be disabled
        if (pfdw.pollfds[i].revents & FF_POLLERR) {
          log_error(
              "error while reading identity from new connection.");
          fail = true;
        }
        if (pfdw.pollfds[i].revents & FF_POLLHUP) {
          log_error(
              "hang up while reading identity from new connection.");
          fail = true;
        }

        if (fail) {
          close(pfdw.pollfds[i].fd);
          pfdw.pollfds[i].fd = -1;
        }
      } else if (conns[i].peer == self) {
        /* This is a new incoming connection, ready to be accepted */
        if (pfdw.pollfds[i].revents & FF_POLLIN) {
          conns[i].acceptNewConn(pfdw.pollfds, pfdw.num_pollfds, conns);
        }
        // POLLOUT should be disabled
        if (pfdw.pollfds[i].revents & FF_POLLERR) {
          log_error(
              "error while reading identity from new connection.");
        }
        if (pfdw.pollfds[i].revents & FF_POLLHUP) {
          log_error(
              "hang up while reading identity from new connection.");
        }
      } else // this is an outgoing connection awaiting to be established,
      // or an already established connection.
      {
        if (pfdw.pollfds[i].revents & FF_POLLIN) {
          log_debug("handling input");
          std::unique_ptr<IncomingMessage<Identity_T>> in_msg =
              conns[i].handleInput();

          if (in_msg != nullptr) {
            log_debug("got a message");
            std::vector<std::unique_ptr<OutgoingMessage<Identity_T>>>
                out_msgs;
            fmanager.handleReceive(*in_msg, &out_msgs);
            distribute(out_msgs);
          }
        }
        if (pfdw.pollfds[i].revents & FF_POLLOUT) {
          log_debug(
              "handling output connected=%i, i=%zu",
              conns[i].connected,
              i);
          conns[i].handleOutput(self);
        }

        bool failure = false;
        if (pfdw.pollfds[i].revents & FF_POLLERR) {
          log_error(
              "error while reading identity from new connection.");
          failure = true;
        }
        if (pfdw.pollfds[i].revents & FF_POLLHUP) {
          log_error(
              "hang up while reading identity from new connection.");
          failure = true;
        }

        if (failure) {
          close(pfdw.pollfds[i].fd);
          pfdw.pollfds[i].fd = -1;
          conns[i].needReconnect = true;
        }
      }
    }

    /* check if all connections have been established for the first time. */
    if (!connected) {
      bool all_connected = true;
      for (size_t i = 0; i < conns.size(); i++) {
        log_debug("i=%zu, connected=%i", i, conns[i].connected);
        if (!conns[i].connected) {
          all_connected = false;
          break;
        }
      }

      if (all_connected) {
        log_info(
            "All connections established, starting secure computation");

        for (size_t i = 0; i < conns.size(); i++) {
          pfdw.pollfds[i].events = FF_POLLIN;
        }

        PeerSet_T ps =
            PeerInfos2PeerSet<Identity_T, PeerSet_T>(peers_info);

        ::std::vector<::std::unique_ptr<OutgoingMessage<Identity_T>>>
            out_msgs;
        fmanager.init(::std::move(mainFronctocol), ps, &out_msgs);
        distribute(out_msgs);
        connected = true;
      }
    }

    /* Check for connections which need reconnections */
    for (size_t i = 0; i < conns.size(); i++) {
      if (conns[i].needReconnect) {
        conns[i].attemptReconnect(self, peers_info[i]);
      }
    }

    /* If the protocol was aborted, finish sending all messages, then
     * close. */
    if (fmanager.isAborted()) {
      bool all_finished = true;
      for (size_t i = 0; i < conns.size(); i++) {
        all_finished = all_finished && conns[i].isOutgoingQueueEmpty();
      }

      if (all_finished) {
        break;
      }
    }
  }

  if (fmanager.isAborted()) {
    log_error("Secure computation finished unsuccessfully");
  } else {
    log_info("Secure computation completed successfully");
  }

  /* Let each peer know that I'm done. */
  for (size_t i = 0; i < pfdw.num_pollfds; i++) {
    shutdown(pfdw.pollfds[i].fd, SHUT_WR);
  }

  /* Wait for each peer to finish */
  for (size_t i = 0; i < pfdw.num_pollfds; i++) {
    if (pfdw.pollfds[i].fd >= 0) {
      ssize_t ret;
      uint64_t data;
      time_t start_time = time(nullptr);
      do {
        log_trace("calling recv for close");
        ret = FF_RECV(
            pfdw.pollfds[i].fd, (void *)&data, sizeof(uint64_t), 0);
        log_trace("called recv for close");

        if (start_time + CLOSE_TIMEOUT < time(nullptr)) {
          log_debug("timed out closing");
          break;
        }
      } while (ret > 0 ||
               (ret < 0 && errno != EBADF && errno != ECONNRESET &&
                errno != ENOTCONN && errno != ENOTSOCK));
    }
  }

  return fmanager.isClosed() && !fmanager.isAborted();
}

} // namespace posixnet
} // namespace ff

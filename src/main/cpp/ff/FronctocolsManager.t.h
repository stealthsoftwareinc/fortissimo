/**
 * Copyright Stealth Software Technologies, Inc.
 */

static fronctocolId_t constexpr MAIN_ID = 0;

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::FronctocolsManager(Identity_T const & self) :
    self(self) {
  this->fronctocols[MAIN_ID] = ::std::unique_ptr<FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>(
      new FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>(this->self));
  FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> & main_handler = *this->fronctocols[MAIN_ID];
  main_handler.id = MAIN_ID;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    init(
        ::std::unique_ptr<Fronctocol<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>> main,
        PeerSet_T peers,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  log_assert(!this->initialized, "Init invoked multiple times");

  /* Step 1. Insert an empty FronctocolHandler for main with peers and ID
   * 0 into the fronctocols map. */
  FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> & main_handler = *this->fronctocols[MAIN_ID];

  main_handler.implementation = ::std::move(main);
  main_handler.implementation->setHandler(&main_handler);
  main_handler.peers = peers;
  main_handler.peers.forEach([](Identity_T const &,
                                fronctocolId_t & id,
                                bool &) { id = MAIN_ID; });
  main_handler.timer = log_time_start_ctx(
      (::std::string("FF: ") + main_handler.implementation->name() +
       "; ID: " + ::std::to_string(main_handler.id))
          .c_str(),
      "parent: none");

  /* Step 2. Set this's initialized flag. */
  this->initialized = true;

  /* Step 3. call main's init method */
  ::std::vector<::std::unique_ptr<Action<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>>
      actions;
  log_time_update(main_handler.timer, "main init start");
  main_handler.init(&actions);
  log_time_update(main_handler.timer, "main init end");
  this->handleActions(main_handler, actions, omsgs);

  actions.clear();

  /* Step 4. Handle each cached message. */
  for (::std::unique_ptr<typename IncomingMessage_T::Cache> &
           imsg_cache : main_handler.incomingMessageCaches) {
    this->distributeMessage(
        imsg_cache->controlBlock,
        *imsg_cache->uncache(),
        main_handler,
        omsgs);
  }
  main_handler.incomingMessageCaches.clear();
}

constexpr uint8_t CTRLBLK_SYNC = 0x00;
constexpr uint8_t CTRLBLK_PAYLOAD = 0x01;
constexpr uint8_t CTRLBLK_COMPLETE = 0x02;
constexpr uint8_t CTRLBLK_ABORT = 0x04;

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleReceive(
        IncomingMessage_T & imsg,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. Read the common fields of the message header, control block
   * and fronctocol id. */
  uint8_t ctrl_blk;
  imsg.template read<uint8_t>(ctrl_blk);
  uint64_t fronctocol_id;
  imsg.template read<uint64_t>(fronctocol_id);

  log_debug(
      "Message from %s, ctrlblk=0x%hhx, fronctocol=%lu",
      identity_to_string(imsg.sender).c_str(),
      ctrl_blk,
      fronctocol_id);

  /* Step 2. If this is aborted, disregard the message. */
  if (this->isAborted()) {
    imsg.clear();
    return;
  }

  /* Step 3. If this is an abort message call the handleAbortMsg(...)
   * method. */
  if (ctrl_blk == CTRLBLK_ABORT) {
    log_error(
        "Abort recieved from peer %s",
        identity_to_string(imsg.sender).c_str());
    imsg.clear();
    this->handleAbort(omsgs);
    return;
  }

  /* Step 4. look up the fronctocol in the map of fronctocols. */
  typename std::unordered_map<
      fronctocolId_t,
      std::unique_ptr<FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>>::iterator finder =
      this->fronctocols.find(fronctocol_id);
  if (finder == this->fronctocols.end()) {
    log_warn(
        "Cannot handle message from %s for non-existant fronctocol %lu",
        identity_to_string(imsg.sender).c_str(),
        fronctocol_id);
    log_debug("FRONCTOCOLID_INVALID is %lu", FRONCTOCOLID_INVALID);
    return;
  }

  /* Step 4. Check if this message needs to be cached. */
  if (!this->initialized || !finder->second->peers.hasAllPeerIds()) {
    finder->second->incomingMessageCaches.push_back(
        imsg.createCache(ctrl_blk));
    return;
  }

  if_debug if (!finder->second->peers.hasPeer(imsg.sender)) {
    log_warn("message from non participant in fronctocol");
  }
  log_assert(finder->second->incomingMessageCaches.size() == 0);

  /* Step 5. delegate to the appropriate sub-handler. */
  this->distributeMessage(ctrl_blk, imsg, *finder->second, omsgs);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    distributeMessage(
        uint8_t const ctrl_blk,
        IncomingMessage_T & imsg,
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & handler,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  if (ctrl_blk == CTRLBLK_SYNC) {
    this->handleSyncMsg(imsg, handler, omsgs);
  } else if (ctrl_blk == CTRLBLK_PAYLOAD) {
    this->handlePayloadMsg(imsg, handler, omsgs);
  } else if (ctrl_blk == CTRLBLK_COMPLETE) {
    this->handleCompleteMsg(imsg, handler, omsgs);
  }
  /* TODO: handle bad ctrl_blk */
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleActions(
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
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  if_debug if (!handler.peers.hasAllPeerIds()) {
    log_warn(
        "Handling actions for fronctocol %lu with missing peer ids",
        handler.id);
  }
  for (size_t i = 0; i < actions.size(); i++) {
    if (this->isAborted()) {
      return;
    }

    if (actions[i]->type == ActionType::invoke) {
      this->handleInvokeAction(
          handler,
          static_cast<InvokeAction<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> &>(*actions[i]),
          omsgs);
    } else if (actions[i]->type == ActionType::send) {
      this->handleSendAction(
          handler,
          static_cast<SendAction<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> &>(*actions[i]),
          omsgs);
    } else if (actions[i]->type == ActionType::complete) {
      this->handleCompleteAction(
          handler,
          static_cast<CompleteAction<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> &>(*actions[i]),
          omsgs);
    } else if (actions[i]->type == ActionType::await) {
      this->handleAwaitAction(
          handler,
          static_cast<AwaitAction<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T> &>(*actions[i]),
          omsgs);
    } else if (actions[i]->type == ActionType::abortion) {
      log_error("Abort action occured");
      this->handleAbort(omsgs);
    } else {
      log_fatal("Invalid action");
    }
  }
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleSyncMsg(
        IncomingMessage_T & imsg,
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & parent,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. Read the peer set and peer's child ID out of the message */
  PeerSet_T peerset;
  imsg.template read<PeerSet_T>(peerset);
  fronctocolId_t peer_id;
  imsg.template read<fronctocolId_t>(peer_id);

  log_debug(
      "handling sync from %s, peerchildid=%lu",
      identity_to_string(imsg.sender).c_str(),
      peer_id);

  /* Step 2. Search the parent's cradle for a child with the same peerset,
   * and without an ID for the sender. */
  for (size_t i = 0; i < parent.cradle.size(); i++) {
    if (parent.cradle[i]->peers == peerset &&
        parent.cradle[i]->peers.checkAndSetId(imsg.sender, peer_id)) {
      /* If found and the child has received all peer IDs, start it. */
      if (parent.cradle[i]->peers.hasAllPeerIds()) {
        log_debug(
            "init fronctocol %lu from sync message",
            parent.cradle[i]->id);
        ::std::vector<::std::unique_ptr<Action<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>>
            actions;
        log_time_update(
            parent.cradle[i]->timer, "init after sync start");
        parent.cradle[i]->init(&actions);
        log_time_update(parent.cradle[i]->timer, "init after sync end");
        this->handleActions(*parent.cradle[i], actions, omsgs);

        for (::std::unique_ptr<typename IncomingMessage_T::Cache> &
                 imsg_cache : parent.cradle[i]->incomingMessageCaches) {
          this->distributeMessage(
              imsg_cache->controlBlock,
              *imsg_cache->uncache(),
              *parent.cradle[i],
              omsgs);
        }
        parent.cradle[i]->incomingMessageCaches.clear();

        parent.cradle.erase(parent.cradle.begin() + (ssize_t)i);
      }

      return;
    }
  }

  /* Step 3. Check in the parent's womb for a child with the same peerset
   * but without an ID for the sender. */
  for (size_t i = 0; i < parent.womb.size(); i++) {
    if (parent.womb[i]->peers == peerset &&
        parent.womb[i]->peers.checkAndSetId(imsg.sender, peer_id)) {
      return;
    }
  }

  /* Step 4. If none of the above succeeded, add a new fronctocol to the
   * womb. */
  parent.womb.emplace_back(new FronctocolHandler<
                           Identity_T,
                           PeerSet_T,
                           IncomingMessage_T,
                           OutgoingMessage_T>(this->self, peerset));
  parent.womb[parent.womb.size() - 1]->peers.setId(
      imsg.sender, peer_id);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handlePayloadMsg(
        IncomingMessage_T & imsg,
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & handler,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  log_assert(handler.peers.hasAllPeerIds());
  if_debug if (handler.completed) {
    log_warn("Delivering message to completed fronctocol");
  }

  ::std::vector<::std::unique_ptr<Action<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>>
      actions;
  log_time_update(handler.timer, "handle receive start");
  handler.handleReceive(imsg, &actions);
  log_time_update(handler.timer, "handle receive end");
  this->handleActions(handler, actions, omsgs);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleCompleteMsg(
        IncomingMessage_T & imsg,
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & handler,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> *) {
  log_debug(
      "fronctocol %lu collected=%i", handler.id, handler.collected);
  /* Step 1. Mark that the sender has completed this fronctocol. */
  handler.peers.setCompleted(imsg.sender);

  /* Step 2. Remove the fronctocol, if it was collected, and all peers
   * completed. */
  if (handler.collected && handler.peers.checkAllComplete()) {
    if (handler.id == 0) {
      this->finished = true;
    }
    log_debug("erasing fronctocol %lu", handler.id);
    log_time_update(handler.timer, "freed after complete message");
    this->fronctocols.erase(handler.id);
  }
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleInvokeAction(
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & parent,
        InvokeAction<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & action,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. In debug mode, warn if the child was invoked with an empty
   * peer set, or if it is missing itself from the peerset. */
  if_debug {
    size_t n_peers = 0;
    bool has_self = false;
    action.peers.forEach(
        [&](Identity_T const & peer, fronctocolId_t &, bool &) {
          n_peers++;
          if (peer == this->self) {
            has_self = true;
          }
        });

    if (n_peers < 2) {
      log_warn("Invoking fronctocol with fewer than 2 participants");
    }
    if (!has_self) {
      log_warn("Invoking fronctocol with missing self participant");
    }
  }

  fronctocolId_t child_id = this->fronctocolIdGenerator++;
  ::std::unique_ptr<FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T>>
      child_handler = nullptr;

  /* Step 2. Attempt to find a FronctocolHandler in the parents womb which
   * matches the peerset.
   *
   * (Portions of Step 4 are performed in this block also)*/
  {
    size_t i = 0;
    for (i = 0; i < parent.womb.size(); i++) {
      if (parent.womb[i]->peers == action.peers &&
          parent.womb[i]->peers.checkAndSetId(this->self, child_id)) {
        break;
      }
    }

    if (i < parent.womb.size()) {
      child_handler = ::std::move(parent.womb[i]);
      parent.womb.erase(parent.womb.begin() + (ssize_t)i);
    } else // Step 3. If none found in womb, make a new one.
    {
      child_handler = ::std::unique_ptr<FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>(
          new FronctocolHandler<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T>(this->self, action.peers));
      child_handler->peers.setId(this->self, child_id);
    }
  }

  /* Step 4. Fill out missing attributes of the child's handler. */
  child_handler->id = child_id;
  child_handler->parent = (action.promised) ? nullptr : &parent;
  child_handler->implementation = ::std::move(action.fronctocol);
  child_handler->promised = action.promised;
  child_handler->implementation->setHandler(child_handler.get());
  child_handler->timer = log_time_start_ctx(
      (::std::string("FF: ") + child_handler->implementation->name() +
       "; ID: " + ::std::to_string(child_handler->id))
          .c_str(),
      (std::string("parent: ") + ::std::to_string(parent.id)).c_str());
  log_time_update(
      parent.timer,
      (::std::string("child: ") + std::to_string(child_handler->id) +
       ((action.promised) ? ", promise" : ", invoke"))
          .c_str());

  /* Step 5. Send a sync message to each of the child's peers. */
  child_handler->peers.forEach(
      [&](Identity_T const & peer, fronctocolId_t &, bool &) {
        if (peer == this->self) {
          return;
        }

        fronctocolId_t peer_parent = parent.peers.findPeerId(peer);
        ::std::unique_ptr<OutgoingMessage_T> omsg(
            new OutgoingMessage_T(peer));

        log_debug(
            "Sending SYNC message to peer %s, parentid=%lu, "
            "childid=%lu, peerparentid=%lu",
            identity_to_string(peer).c_str(),
            parent.id,
            child_handler->id,
            peer_parent);

        omsg->template write<uint8_t>(CTRLBLK_SYNC);
        omsg->template write<fronctocolId_t>(peer_parent);
        omsg->template write<PeerSet_T>(child_handler->peers);
        omsg->template write<fronctocolId_t>(child_handler->id);

        omsgs->push_back(::std::move(omsg));
      });

  /* Step 6. Either invoke the child, or insert it into the parent's cradle. */
  if (child_handler->peers.hasAllPeerIds()) {
    log_debug(
        "init fronctocol %lu from invoke action", child_handler->id);
    ::std::vector<::std::unique_ptr<Action<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T>>>
        actions;
    log_time_update(child_handler->timer, "init immediately start");
    child_handler->init(&actions);
    log_time_update(child_handler->timer, "init immediately end");
    this->handleActions(*child_handler, actions, omsgs);

    for (::std::unique_ptr<typename IncomingMessage_T::Cache> &
             imsg_cache : child_handler->incomingMessageCaches) {
      this->distributeMessage(
          imsg_cache->controlBlock,
          *imsg_cache->uncache(),
          *child_handler,
          omsgs);
    }
    child_handler->incomingMessageCaches.clear();
  } else {
    parent.cradle.push_back(child_handler.get());
  }

  /* Step 7. Enter the child's ID and handler into the FronctocolsManager. */
  this->fronctocols[child_handler->id] = ::std::move(child_handler);
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleSendAction(
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
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. In debug mode, warn if the intended recipient has already
   * completed. */
  if_debug {
    if (!handler.peers.hasPeer(action.msg->recipient)) {
      log_warn("Sending message to a non-peer");
    }
    if (handler.peers.findCompletionStatus(action.msg->recipient)) {
      log_warn("Sending message to a peer which has already completed");
    }
  }

  /* Step 2. Prepend a payload header.*/
  ::std::unique_ptr<OutgoingMessage_T> omsg = ::std::move(action.msg);

  log_time_update(
      handler.timer,
      (::std::string("send message recipient: \"") +
       identity_to_string(omsg->recipient) +
       "\" size: " + std::to_string(omsg->length()))
          .c_str());

  fronctocolId_t const peer_id =
      handler.peers.findPeerId(omsg->recipient);
  log_debug(
      "Sending message to %s, myid=%lu, peerid=%lu",
      identity_to_string(omsg->recipient).c_str(),
      handler.id,
      peer_id);

  uint8_t buf[sizeof(uint8_t) + sizeof(fronctocolId_t)];
  static_assert(sizeof(uint8_t) == 1, "sizeof(uint8_t) isn't 1");
  buf[0] = CTRLBLK_PAYLOAD;
  fronctocolId_to_buffer(peer_id, buf + 1);

  omsg->prepend((void *)buf, sizeof(uint8_t) + sizeof(fronctocolId_t));

  /* Step 3. Add it to the outgoing messages list. */
  omsgs->push_back(::std::move(omsg));
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleCompleteAction(
        FronctocolHandler<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> & handler,
        CompleteAction<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T> &,
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. Mark f as being completed, but not yet collected. */
  handler.completed = true;
  log_time_update(handler.timer, "completed");

  /* Step 2. Set f's own completion flag in its peer set. */
  handler.peers.setCompleted(this->self);

  /* Step 3. Send a complete message to each of f's peers. */
  handler.peers.forEach(
      [&](Identity_T const & peer, fronctocolId_t & id, bool &) {
        if (peer == this->self) {
          return;
        }

        log_debug(
            "Sending complete message to %s for myid=%lu, peerid=%lu",
            identity_to_string(peer).c_str(),
            handler.id,
            id);

        ::std::unique_ptr<OutgoingMessage_T> omsg(
            new OutgoingMessage_T(peer));
        omsg->template write<uint8_t>(CTRLBLK_COMPLETE);
        omsg->template write<fronctocolId_t>(id);

        omsgs->push_back(::std::move(omsg));
      });

  /* Step 4. If this is not the main fronctocol, attempt to call its
   * parents handler. */
  if (handler.id != 0) {
    if (handler.parent != nullptr) {
      ::std::vector<::std::unique_ptr<Action<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>>
          actions;
      log_assert(!handler.parent->completed);
      if (handler.promised) {
        log_time_update(
            handler.parent->timer, "handle promise on complete start");
        handler.parent->handlePromise(
            *handler.implementation, &actions);
        log_time_update(
            handler.parent->timer, "handle promise on complete end");
      } else if (!handler.promised) {
        log_time_update(handler.parent->timer, "handle complete start");
        handler.parent->handleComplete(
            *handler.implementation, &actions);
        log_time_update(handler.parent->timer, "handle complete end");
      }
      this->handleActions(*handler.parent, actions, omsgs);
      handler.collected = true;
    }
  } else /* Step 5. If this is the main fronctocol mark it as finished. */
  {
    handler.collected = true;
    this->finished = true;
  }

  /* if it was collected, and all peers are completed, remove it */
  if (handler.collected && handler.peers.checkAllComplete()) {
    log_time_update(handler.timer, "freed immediately");
    this->fronctocols.erase(handler.id);
  }
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleAwaitAction(
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
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  /* Step 1. Check for erroneous calls to await */
  typename ::std::unordered_map<
      fronctocolId_t,
      ::std::unique_ptr<FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>>::iterator finder =
      this->fronctocols.find(action.awaitedFronctocol->getId());
  log_assert(
      finder != this->fronctocols.end(),
      "Cannot await a nonexistant fronctocol");

  FronctocolHandler<
      Identity_T,
      PeerSet_T,
      IncomingMessage_T,
      OutgoingMessage_T> & awaited = *finder->second;
  log_assert(awaited.promised, "Awaited fronctocol is not a promise.");

  log_debug(
      "Called await on %lu, parent: %zu",
      awaited.id,
      (size_t)awaited.parent);

  log_assert(
      awaited.parent == nullptr,
      "Promise Fronctocols cannot be awaited twice.");

  /* Step 2. Assign the awaited fronctocol the awaiter as parent. */
  awaited.parent = &handler;

  /* Step 3. Check if the awaited fronctocol is completed. */
  if (awaited.completed) {
    ::std::vector<::std::unique_ptr<Action<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T>>>
        actions;
    log_time_update(handler.timer, "handle promise on await start");
    handler.handlePromise(*awaited.implementation, &actions);
    log_time_update(handler.timer, "handle promise on await end");
    this->handleActions(handler, actions, omsgs);

    awaited.collected = true;

    if (awaited.peers.checkAllComplete()) {
      log_time_update(awaited.timer, "freed after awaited");
      this->fronctocols.erase(awaited.id);
    }
  }
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
void FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::
    handleAbort(
        ::std::vector<::std::unique_ptr<OutgoingMessage_T>> * omsgs) {
  typename ::std::unordered_map<
      fronctocolId_t,
      ::std::unique_ptr<FronctocolHandler<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>>::iterator finder =
      this->fronctocols.find(0);
  if (finder != this->fronctocols.end()) {
    finder->second->peers.forEach(
        [&](Identity_T const & peer, fronctocolId_t &, bool &) {
          if (peer == this->self) {
            return;
          }

          ::std::unique_ptr<OutgoingMessage_T> omsg(
              new OutgoingMessage_T(peer));
          omsg->template write<uint8_t>(CTRLBLK_ABORT);
          omsg->template write<fronctocolId_t>(FRONCTOCOLID_INVALID);

          omsgs->push_back(::std::move(omsg));
        });
  }
  this->aborted = true;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
bool FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::isFinished() const {
  return this->finished;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
bool FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::isClosed() const {
  return this->finished && this->fronctocols.size() == 0;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
bool FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::isAborted() const {
  return this->aborted;
}

template<
    typename Identity_T,
    typename PeerSet_T,
    typename IncomingMessage_T,
    typename OutgoingMessage_T>
size_t FronctocolsManager<
    Identity_T,
    PeerSet_T,
    IncomingMessage_T,
    OutgoingMessage_T>::getNumFronctocols() const {
  return this->fronctocolIdGenerator;
}

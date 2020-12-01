/**
 * Copyright Stealth Software Technologies, Inc.
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
    uint64_t seed) {
  log_info("Running Tests with seed %lu", seed);

  // One FronctocolsManager per participant
  ::std::map<
      Identity_T,
      FronctocolsManager<
          Identity_T,
          PeerSet_T,
          IncomingMessage_T,
          OutgoingMessage_T>>
      managers;
  // One queue of messages between each pair of peers (sender, recipient)
  ::std::map<
      ::std::pair<Identity_T, Identity_T>,
      ::std::deque<::std::unique_ptr<OutgoingMessage_T>>>
      msgs;
  // List of all pairs of peers (sender, recipient),
  // (easy to choose one at random).
  ::std::vector<::std::pair<Identity_T, Identity_T>> identity_pairs;

  size_t n_msgs = 0;

  // helper to distribute messages into the msgs map.
  auto findPeer = [&](Identity_T const & sender,
                      Identity_T const & recipient)
      -> std::pair<Identity_T, Identity_T> const & {
    for (::std::pair<Identity_T, Identity_T> const & pair :
         identity_pairs) {
      if (pair.first == sender && pair.second == recipient) {
        return pair;
      }
    }
    log_fatal(
        "Identity pair from %s to %s not found",
        identity_to_string(sender).c_str(),
        identity_to_string(recipient).c_str());
  };
  auto distributor =
      [&](::std::vector<::std::unique_ptr<OutgoingMessage_T>> & omsgs,
          Identity_T const & sender) -> void {
    for (::std::unique_ptr<OutgoingMessage_T> & msg : omsgs) {
      Identity_T const & recipient = msg->recipient;
      ::std::pair<Identity_T, Identity_T> const & pair =
          findPeer(sender, recipient);
      log_debug(
          "Distributing message from %s to %s",
          identity_to_string(sender).c_str(),
          identity_to_string(recipient).c_str());
      msgs.find(pair)->second.push_back(::std::move(msg));
      n_msgs++;

      log_assert(msgs.size() == identity_pairs.size());
    }
  };

  for (::std::pair<
           Identity_T const,
           ::std::unique_ptr<Fronctocol<
               Identity_T,
               PeerSet_T,
               IncomingMessage_T,
               OutgoingMessage_T>>> & sender_pair : tests) {
    // Start by listing all the pairs of (sender, receiver) and making
    // a message queue for each pair.
    Identity_T const & sender_identity = sender_pair.first;
    for (::std::pair<
             Identity_T const,
             ::std::unique_ptr<Fronctocol<
                 Identity_T,
                 PeerSet_T,
                 IncomingMessage_T,
                 OutgoingMessage_T>>> & receiver_pair : tests) {
      Identity_T const & receiver_identity = receiver_pair.first;
      identity_pairs.emplace_back(sender_identity, receiver_identity);
      msgs[identity_pairs.back()] =
          ::std::deque<::std::unique_ptr<OutgoingMessage_T>>();
      log_debug(
          "identity pair from %s to %s",
          identity_to_string(identity_pairs.back().first).c_str(),
          identity_to_string(identity_pairs.back().second).c_str());
    }
  }

  // Create a FronctocolsManager for each identity/fronctocol pair.
  for (::std::pair<
           Identity_T const,
           ::std::unique_ptr<Fronctocol<
               Identity_T,
               PeerSet_T,
               IncomingMessage_T,
               OutgoingMessage_T>>> & test_pair : tests) {
    Identity_T const & test_identity = test_pair.first;
    LOG_ORGANIZATION = identity_to_string(test_identity);

    // Create the manager and insert it into the map
    managers.insert(::std::pair<
                    Identity_T,
                    FronctocolsManager<
                        Identity_T,
                        PeerSet_T,
                        IncomingMessage_T,
                        OutgoingMessage_T>>(
        test_identity,
        FronctocolsManager<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>(test_identity)));

    // Retrieve the manager from the map, and invoke init()
    ::std::vector<::std::unique_ptr<OutgoingMessage_T>> omsgs;
    typename ::std::map<
        Identity_T,
        FronctocolsManager<
            Identity_T,
            PeerSet_T,
            IncomingMessage_T,
            OutgoingMessage_T>>::iterator finder =
        managers.find(test_identity);
    log_assert(finder != managers.end());
    finder->second.init(
        ::std::move(test_pair.second),
        makeTestPeerSet<Identity_T, PeerSet_T>(tests),
        &omsgs);

    size_t prevnmsgs = n_msgs;
    distributor(omsgs, test_identity);
    log_assert(n_msgs == prevnmsgs + omsgs.size());
    (void)prevnmsgs;
  }

  auto generator = ::std::bind(
      ::std::uniform_int_distribution<size_t>(
          0, identity_pairs.size() - 1),
      ::std::mt19937_64(seed));

  while (n_msgs > 0) {
    log_assert(msgs.size() == identity_pairs.size());

    // choose a (sender, receiver) pair
    size_t const pair_id = generator();
    log_assert(
        identity_pairs.size() > pair_id,
        "pairid=%lu, size=%lu",
        pair_id,
        identity_pairs.size());
    ::std::pair<Identity_T, Identity_T> const & pair =
        identity_pairs[pair_id];
    typename ::std::map<
        ::std::pair<Identity_T, Identity_T>,
        ::std::deque<::std::unique_ptr<OutgoingMessage_T>>>::iterator
        pair_finder = msgs.find(pair);
    log_assert(
        pair_finder != msgs.end(),
        "identity pair sending from %s to %s",
        identity_to_string(pair.first).c_str(),
        identity_to_string(pair.second).c_str());

    if (pair_finder->second.size() > 0) {
      log_debug(
          "identity pair sending from %s to %s",
          identity_to_string(pair.first).c_str(),
          identity_to_string(pair.second).c_str());
      // find the message, manager, and other setups.
      ::std::unique_ptr<OutgoingMessage_T> sent_msg =
          ::std::move(pair_finder->second.front());
      pair_finder->second.pop_front();
      n_msgs--;
      typename ::std::map<
          Identity_T,
          FronctocolsManager<
              Identity_T,
              PeerSet_T,
              IncomingMessage_T,
              OutgoingMessage_T>>::iterator finder =
          managers.find(pair.second);
      log_assert(finder != managers.end());

      ::std::unique_ptr<IncomingMessage_T> read_msg =
          ::std::move(converter(pair.first, *sent_msg));

      LOG_ORGANIZATION = identity_to_string(finder->first);
      ::std::vector<std::unique_ptr<OutgoingMessage_T>> omsgs;
      finder->second.handleReceive(*read_msg, &omsgs);
      distributor(omsgs, pair.second);
    }
  }

  bool ret = true;
  for (::std::pair<
           Identity_T const,
           FronctocolsManager<
               Identity_T,
               PeerSet_T,
               IncomingMessage_T,
               OutgoingMessage_T>> const & manager : managers) {
    if (manager.second.isAborted()) {
      return false;
    }

    ret = ret && manager.second.isFinished();
  }

  if (!ret) {
    log_error("Test Runner has no more messages to send, but "
              "FronctocolsManagers does not report as finished. This "
              "probably "
              "indicates a deadlock condition");

#ifdef ADD_FAILURE
    ADD_FAILURE();
#endif
  }

  return ret;
}

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
        converter) {
  return runTests(
      tests,
      converter,
      (uint64_t)::std::chrono::system_clock::now()
          .time_since_epoch()
          .count());
}

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
                              OutgoingMessage_T>>> & tests) {
  PeerSet_T builder;
  for (::std::pair<
           Identity_T const,
           ::std::unique_ptr<Fronctocol<
               Identity_T,
               PeerSet_T,
               IncomingMessage_T,
               OutgoingMessage_T>>> const & test : tests) {
    builder.add(test.first);
  }

  // copy to allow for optional set sorting.
  PeerSet_T ret(builder);
  return ret;
}

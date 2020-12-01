/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
std::string RandomnessHouse<FF_TYPES, Randomness_T, Info_T>::name() {
  return std::string("Randomness House ") + Randomness_T::name();
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessHouse<FF_TYPES, Randomness_T, Info_T>::init() {
  log_debug("Randomness House initializing");
  this->numParties = 0;
  this->getPeers().forEach([this](Identity_T const & peer) {
    if (peer != this->getSelf()) {
      this->numParties++;
    }
  });
}

/**
 * Maximum size (in bytes) of a batch of randomness instances.
 * TODO: choose a sane value here.
 */
size_t constexpr DEFAULT_BATCH_SIZE = 250000;

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessHouse<FF_TYPES, Randomness_T, Info_T>::handleReceive(
    IncomingMessage_T & msg) {
  if (this->numReceived == 0) {
    log_debug("randomness house first sync received");
    uint64_t numDesired64;
    msg.template read<uint64_t>(numDesired64);
    this->numDesired = (size_t)numDesired64;
    msg.template read<Info_T>(this->info);
  } else {
    log_debug("randomness house continuing to receive syncs");
    uint64_t numDesired64;
    msg.template read<uint64_t>(numDesired64);
    if ((size_t)numDesired64 > this->numDesired) {
      this->numDesired = (size_t)numDesired64;
      if_debug log_warn(
          "Increasing number of randomness instancess due to "
          "disagreement amongst mpcs");
    }
    Info_T curr_info;
    msg.template read<Info_T>(curr_info);
    if (this->info != curr_info) {
      log_error("Differing randomness metadata information received");
      this->abort();
      return;
    }
  }

  this->numReceived++;

  if (this->numReceived == this->numParties) {
    log_debug(
        "randomness house beginning to send randomness instances");

    size_t const single_instance_size = this->info.instanceSize();
    size_t const batch_size =
        (single_instance_size > DEFAULT_BATCH_SIZE) ?
        single_instance_size :
        DEFAULT_BATCH_SIZE;
    size_t const num_per_batch = batch_size / single_instance_size;
    size_t const num_batches = (this->numDesired % num_per_batch == 0) ?
        this->numDesired / num_per_batch :
        1 + (this->numDesired / num_per_batch);
    size_t total_sent = 0;
    size_t i = 0;
    size_t j = 0;

    log_debug(
        "rands per batch: %zu, num batches: %zu",
        num_per_batch,
        num_batches);

    for (i = 0; i < num_batches; i++) {
      log_debug("i: %lu", i);

      std::vector<std::unique_ptr<OutgoingMessage_T>> omsgs;
      this->getPeers().forEach([&](Identity_T const & peer) {
        if (this->getSelf() == peer) {
          return;
        }

        omsgs.emplace_back(new OutgoingMessage_T(peer));

        if (i == 0) {
          omsgs[omsgs.size() - 1]->template write<uint64_t>(
              (uint64_t)num_batches);
          omsgs[omsgs.size() - 1]->template write<uint64_t>(
              (uint64_t)num_per_batch);
        }
      });

      for (j = 0; j < num_per_batch && total_sent < this->numDesired;
           j++) {
        ::std::vector<Randomness_T> vals;
        log_debug("Calling generate");
        this->info.generate(omsgs.size(), total_sent, vals);
        log_assert(vals.size() == omsgs.size());
        for (size_t k = 0; k < omsgs.size(); k++) {
          omsgs[k]->template write<Randomness_T>(vals[k]);
        }
        total_sent++;
      }
      log_assert(j == num_per_batch || total_sent == this->numDesired);

      for (std::unique_ptr<OutgoingMessage_T> & omsg : omsgs) {
        log_debug("sending randomness");
        this->send(std::move(omsg));
      }
    }
    log_assert(i == num_batches);
    this->complete();
    log_debug("done sending randomness instances");
  }
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessHouse<FF_TYPES, Randomness_T, Info_T>::handleComplete(
    Fronctocol<FF_TYPES> &) {
  log_fatal("Randomness House can't handle complete.");
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessHouse<FF_TYPES, Randomness_T, Info_T>::handlePromise(
    Fronctocol<FF_TYPES> &) {
  log_fatal("Randomness House can't handle Promise.");
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
std::string RandomnessPatron<FF_TYPES, Randomness_T, Info_T>::name() {
  return std::string("Randomness Patron ") + Randomness_T::name();
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessPatron<FF_TYPES, Randomness_T, Info_T>::init() {

  std::unique_ptr<OutgoingMessage_T> om(
      new OutgoingMessage_T(this->dealer));

  om->template write<uint64_t>((uint64_t)this->numDesired);
  om->template write<Info_T>(this->info);

  this->send(std::move(om));

  this->result =
      ::std::unique_ptr<RandomnessDispenser<Randomness_T, Info_T>>(
          new RandomnessDispenser<Randomness_T, Info_T>(this->info));

  if (this->numDesired == 0) {
    log_warn("Requesting 0 randomness.");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessPatron<FF_TYPES, Randomness_T, Info_T>::handleReceive(
    IncomingMessage_T & im) {
  log_debug(
      "Dealer Randomness Generator receiving batch %zu",
      this->batchesReceived + 1);
  if (this->batchesReceived ==
      0) // first message has a bit of metadata.
  {
    uint64_t batchesTotal64;
    uint64_t batchSize64;
    im.template read<uint64_t>(batchesTotal64);
    im.template read<uint64_t>(batchSize64);
    this->batchesTotal = (size_t)batchesTotal64;
    this->batchSize = (size_t)batchSize64;
  }

  for (size_t i = 0; i < batchSize && im.length() > 0; i++) {
    Randomness_T val(this->info);
    im.template read<Randomness_T>(val);
    this->result->insert(val);
  }

  log_assert(im.length() == 0);

  this->batchesReceived++;
  if (this->batchesReceived == this->batchesTotal) {
    this->complete();
    log_debug("Randomness Patron finished");
  };
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessPatron<FF_TYPES, Randomness_T, Info_T>::handleComplete(
    Fronctocol<FF_TYPES> &) {
  log_fatal("Unexpected handle complete on Randomness Patron");
}

template<FF_TYPENAMES, typename Randomness_T, typename Info_T>
void RandomnessPatron<FF_TYPES, Randomness_T, Info_T>::handlePromise(
    Fronctocol<
        Identity_T,
        PeerSet_T,
        IncomingMessage_T,
        OutgoingMessage_T> &) {
  log_fatal("Unexpected handle complete on Randomness Patron");
}

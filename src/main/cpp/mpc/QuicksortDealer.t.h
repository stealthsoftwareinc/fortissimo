/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("QuickSort House large mod: ") +
      dec(this->compareInfo.p) +
      " small mod: " + dec(this->compareInfo.s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    QuicksortRandomnessHouse(
        const CompareInfo<Identity_T, Large_T, Small_T> & compareInfo,
        size_t numElements,
        size_t numKeyCols) :
    compareInfo(compareInfo),
    numElements(numElements),
    numKeyCols(numKeyCols) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;

  size_t block_size = this->compareInfo.lambda;
  while (block_size < this->compareInfo.ell) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    log_debug("block_size: %zu", block_size);
    block_size += this->compareInfo.lambda;
  }
  log_debug("block_size: %zu", this->compareInfo.ell);
  UnboundedFaninOrRandomnessNeeds.emplace_back(this->compareInfo.ell);

  block_size = 1;
  while ((block_size - 1) < this->compareInfo.lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  this->numDealersRemaining =
      UnboundedFaninOrRandomnessNeeds.size() + 4;

  for (size_t i = 0; i < UnboundedFaninOrRandomnessNeeds.size(); i++) {
    std::unique_ptr<Fronctocol<FF_TYPES>> rd(
        new RandomnessHouse<
            FF_TYPES,
            ExponentSeries<Small_T>,
            ExponentSeriesInfo<Small_T>>());
    this->invoke(std::move(rd), this->getPeers());
  }
  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Small_T>,
                                                BeaverInfo<Small_T>>());
  this->invoke(std::move(rd2), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Boolean_t>,
                                                BooleanBeaverInfo>());
  this->invoke(std::move(rd3), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd4(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<Small_T>,
          TypeCastInfo<Small_T>>());
  this->invoke(std::move(rd4), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd5(
      new RandomnessHouse<
          FF_TYPES,
          DecomposedBitSet<Large_T, Small_T>,
          DecomposedBitSetInfo<Large_T, Small_T>>());
  this->invoke(std::move(rd5), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleReceive(IncomingMessage_T &) {
  log_error(
      "Quicksort RandomnessHouse received unexpected handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> &) {
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("Quicksort Fronctocol received unexpected "
            "handle promise");
  this->abort();
}

/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("SISO Sort House modulus") + dec(this->modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    SISOSortRandomnessHouse(
        const size_t listSize,
        const Large_T modulus,
        const Identity_T * revealer,
        const Identity_T * dealerIdentity) :
    listSize(listSize),
    modulus(modulus),
    keyModulus(modulus),
    revealer(revealer),
    dealerIdentity(dealerIdentity) {

  this->d = static_cast<size_t>(approxLog2(
      this->listSize)); // still assuming this is a power of 2

  this->small_modulus =
      nextPrime(static_cast<Small_T>(approxLog2(keyModulus)) + 3);
  this->bitsPerPrime = static_cast<size_t>(approxLog2(keyModulus)) + 1;
  this->sqrt_ell = static_cast<size_t>(std::sqrt(bitsPerPrime)) + 1;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    SISOSortRandomnessHouse(
        const size_t listSize,
        const Large_T modulus,
        const Large_T keyModulus,
        const Identity_T * revealer,
        const Identity_T * dealerIdentity) :
    listSize(listSize),
    modulus(modulus),
    keyModulus(keyModulus),
    revealer(revealer),
    dealerIdentity(dealerIdentity) {

  this->d = static_cast<size_t>(approxLog2(
      this->listSize)); // still assuming this is a power of 2

  this->small_modulus =
      nextPrime(static_cast<Small_T>(approxLog2(keyModulus)) + 3);
  this->bitsPerPrime = static_cast<size_t>(approxLog2(keyModulus)) + 1;
  this->sqrt_ell = static_cast<size_t>(std::sqrt(bitsPerPrime)) + 1;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("SISOSortRandomnessHouse init");

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Large_T>,
                                                BeaverInfo<Large_T>>());
  this->invoke(std::move(rd3), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3b(
      new RandomnessHouse<
          FF_TYPES,
          BeaverTriple<Large_T>,
          BeaverInfo<Large_T>>());
  this->invoke(std::move(rd3b), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd4(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Boolean_t>,
                                                BooleanBeaverInfo>());

  this->invoke(std::move(rd4), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(
      new RandomnessHouse<
          FF_TYPES,
          WaksmanBits<Large_T>,
          WaksmanInfo<Large_T>>());

  this->invoke(std::move(rd2), this->getPeers());

  std::unique_ptr<Fronctocol<FF_TYPES>> rd(
      new QuicksortRandomnessHouse<FF_TYPES, Large_T, Small_T>(
          CompareInfo<Identity_T, Large_T, Small_T>(
              this->modulus,
              this->small_modulus,
              this->bitsPerPrime,
              this->sqrt_ell,
              this->emptyLagrangePolynomialSet,
              this->revealer),
          this->listSize,
          this->NULL_KEY_COUNT));
  this->invoke(std::move(rd), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("SISO House received unexpected handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> &) {
  log_debug("SISOSortRandomnessHouse handleComplete");
  this->numSubDealers--;
  if (this->numSubDealers == 0) {
    log_debug("Dealer complete");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSortRandomnessHouse<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("SISO House received unexpected "
            "handle promise");
  this->abort();
}

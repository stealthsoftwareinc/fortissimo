/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Divide Randomness House large mod: ") +
      dec(this->info->modulus) +
      " small mod: " + dec(this->info->modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    DivideRandomnessHouse(
        DivideInfo<Identity_T, Large_T, Small_T> const * const info) :
    info(info) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("DivideRandomnessHouse init");

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd(
      new PrefixOrRandomnessHouse<FF_TYPES, Small_T>(
          new PrefixOrInfo<Identity_T, Small_T>(
              this->info->smallModulus,
              this->info->ell,
              this->info->revealer)));
  this->invoke(std::move(rd), this->getPeers());

  // low-level
  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Large_T>,
                                                BeaverInfo<Large_T>>());
  this->invoke(std::move(rd2), this->getPeers());

  // high-level
  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3(
      new PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>(
          new CompareInfo<Identity_T, Large_T, Small_T>(
              this->info->modulus, this->info->revealer)));
  this->invoke(std::move(rd3), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd4(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<Small_T>,
          TypeCastFromBitInfo<Small_T>>());
  this->invoke(std::move(rd4), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd5(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Small_T>,
                                                BeaverInfo<Small_T>>());
  this->invoke(std::move(rd5), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd6(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<Small_T>,
          TypeCastInfo<Small_T>>());
  this->invoke(std::move(rd6), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd7(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<Large_T>,
          TypeCastFromBitInfo<Large_T>>());
  this->invoke(std::move(rd7), this->getPeers());

  this->numDealersRemaining = 7;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T & imsg) {
  log_error("DivideRandomnessHouse received unexpected handle receive");
  (void)imsg;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_debug("DivideRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessHouse<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("DivideRandomnessHouse received unexpected "
            "handle promise");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Divide Randomness Patron large mod: ") +
      dec(this->info->modulus) +
      " small mod: " + dec(this->info->modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    DivideRandomnessPatron(
        DivideInfo<Identity_T, Large_T, Small_T> const * const info,
        const Identity_T * dealerIdentity,
        const size_t dispenserSize) :
    divideDispenser(new RandomnessDispenser<
                    DivideRandomness<Large_T, Small_T>,
                    DoNotGenerateInfo>(DoNotGenerateInfo())),
    info(info),
    dealerIdentity(dealerIdentity),
    dispenserSize(
        dispenserSize) { // dispenserSize = num divides we're going to need

  this->numComparesNeeded = 2 * this->info->ell;
  this->numBeaverTriplesNeeded = 2 * this->info->ell;
  this->numPrefixOrsNeeded = 1;
  this->numStartTCTriplesFromBitNeeded = this->info->ell;
  this->numStartTCTriplesNeeded = this->info->ell;
  this->numEndTCTriplesNeeded = 2 * this->info->ell;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on DividePatron");

  std::unique_ptr<Fronctocol<FF_TYPES>> patron(
      new PrefixOrRandomnessPatron<FF_TYPES, Small_T>(
          new PrefixOrInfo<Identity_T, Small_T>(
              this->info->smallModulus,
              this->info->ell,
              this->info->revealer),
          dealerIdentity,
          this->numPrefixOrsNeeded * this->dispenserSize));
  this->invoke(std::move(patron), this->getPeers());
  this->state = awaitingPrefixOr;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T & imsg) {
  log_error("DividePatron Fronctocol received unexpected "
            "handle receive");
  (void)imsg;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("DividePatron received handle complete");
  switch (this->state) {
    case awaitingPrefixOr: {
      this->prefixOrDispenser = std::move(
          static_cast<PrefixOrRandomnessPatron<FF_TYPES, Small_T> &>(f)
              .prefixOrDispenser);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              BeaverTriple<Large_T>,
              BeaverInfo<Large_T>>(
              *dealerIdentity,
              this->numBeaverTriplesNeeded * this->dispenserSize,
              BeaverInfo<Large_T>(this->info->modulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingMultiply;

    } break;
    case awaitingMultiply: {
      this->multiplyDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<Large_T>,
                            BeaverInfo<Large_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>(
              new CompareInfo<Identity_T, Large_T, Small_T>(
                  this->info->modulus, this->info->revealer),
              dealerIdentity,
              this->numComparesNeeded * this->dispenserSize));

      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingCompare;
    } break;
    case awaitingCompare: {
      this->compareDispenser =
          std::move(static_cast<PosIntCompareRandomnessPatron<
                        FF_TYPES,
                        Large_T,
                        Small_T> &>(f)
                        .posIntCompareDispenser);
      log_debug("Launching tcts");
      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<Small_T>,
              TypeCastFromBitInfo<Small_T>>(
              *dealerIdentity,
              this->numStartTCTriplesFromBitNeeded *
                  this->dispenserSize,
              TypeCastFromBitInfo<Small_T>(this->info->smallModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingTct;
    } break;
    case awaitingTct: {
      log_debug("Case awaitingTct");
      this->smallPrimeTCTripleFromBitDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<Small_T>,
                            TypeCastFromBitInfo<Small_T>>> &>(f)
                        .result);
      log_debug("Launching tcts");

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              BeaverTriple<Small_T>,
              BeaverInfo<Small_T>>(
              *dealerIdentity,
              this->numStartTCTriplesNeeded * this->dispenserSize,
              BeaverInfo<Small_T>(this->info->smallModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingMultiply2;
    } break;
    case awaitingMultiply2: {
      log_debug("Case awaitingMultiply2");
      this->smallPrimeBeaverDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<Small_T>,
                            BeaverInfo<Small_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<Small_T>,
              TypeCastInfo<Small_T>>(
              *dealerIdentity,
              this->numStartTCTriplesNeeded * this->dispenserSize,
              TypeCastInfo<Small_T>(this->info->smallModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingTct2;
    } break;
    case awaitingTct2: {
      log_debug("Case awaitingTct");
      this->smallPrimeTCTripleDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<Small_T>,
                            TypeCastInfo<Small_T>>> &>(f)
                        .result);

      log_debug("Launching tcts");
      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<Large_T>,
              TypeCastFromBitInfo<Large_T>>(
              *dealerIdentity,
              this->numEndTCTriplesNeeded * this->dispenserSize,
              TypeCastFromBitInfo<Large_T>(this->info->modulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingTct3;
    } break;
    case awaitingTct3: {
      log_debug("Case awaitingTct");
      this->endPrimeTCTripleDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<Large_T>,
                            TypeCastFromBitInfo<Large_T>>> &>(f)
                        .result);

      this->generateOutputDispenser();
    } break;
    default:
      log_error("State machine in unexpected state");
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("DividePatron Fronctocol received unexpected"
            "handle promise");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void DivideRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    generateOutputDispenser() {
  for (size_t i = 0; i < this->dispenserSize; i++) {

    this->divideDispenser->insert(DivideRandomness<Large_T, Small_T>(
        std::move(
            prefixOrDispenser->littleDispenser(numPrefixOrsNeeded)),
        std::move(
            multiplyDispenser->littleDispenser(numBeaverTriplesNeeded)),
        std::move(compareDispenser->littleDispenser(numComparesNeeded)),
        std::move(smallPrimeTCTripleFromBitDispenser->littleDispenser(
            numStartTCTriplesFromBitNeeded)),
        std::move(smallPrimeBeaverDispenser->littleDispenser(
            numStartTCTriplesNeeded)),
        std::move(smallPrimeTCTripleDispenser->littleDispenser(
            numStartTCTriplesNeeded)),
        std::move(endPrimeTCTripleDispenser->littleDispenser(
            numEndTCTriplesNeeded))));
  }
  log_debug("calling this->complete");

  this->complete();
}

} // namespace mpc
} // namespace ff

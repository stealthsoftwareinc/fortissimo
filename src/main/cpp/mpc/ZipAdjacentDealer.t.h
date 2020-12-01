/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Zip Adjacent Randomness House modulus: ") +
      dec(this->info->multiplyInfo.info.modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    ZipAdjacentRandomnessHouse(
        ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const
            info) :
    info(info) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("ZipAdjacentRandomnessHouse init");

  // high-level
  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd(
      new CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>(
          &this->info->compareInfo));
  this->invoke(std::move(rd), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<Large_T>,
          TypeCastFromBitInfo<Large_T>>());
  this->invoke(std::move(rd2), this->getPeers());

  // low-level
  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Large_T>,
                                                BeaverInfo<Large_T>>());
  this->invoke(std::move(rd3), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd4(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Boolean_t>,
                                                BooleanBeaverInfo>());
  this->invoke(std::move(rd4), this->getPeers());

  this->numDealersRemaining = 4;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleReceive(IncomingMessage_T &) {
  log_error("ZipAdjacentRandomnessHouse received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> &) {
  log_debug("ZipAdjacentRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_info("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("ZipAdjacentRandomnessHouse received unexpected "
            "handle promise");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Zip Adjacent Randomness Patron modulus: ") +
      dec(this->info->multiplyInfo.info.modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    ZipAdjacentRandomnessPatron(
        ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const
            info,
        const Identity_T * dealerIdentity,
        const size_t dispenserSize) :
    zipAdjacentDispenser(new RandomnessDispenser<
                         ZipAdjacentRandomness<Large_T, Small_T>,
                         DoNotGenerateInfo>(DoNotGenerateInfo())),
    info(info),
    dealerIdentity(dealerIdentity),
    dispenserSize(
        dispenserSize) { // dispenserSize = num ZipAdjacents we're going to need

  this->numComparesNeeded = this->info->batchSize - 1;
  this->numTCTsNeeded = this->info->batchSize - 1;
  this->numArithmeticBeaverTriplesNeeded = 2 *
      (this->info->batchSize - 1) *
      (this->info->numArithmeticPayloadCols);
  this->numXORBeaverTriplesNeeded =
      2 * (this->info->batchSize - 1) * (this->info->numXORPayloadCols);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on ZipAdjacentPatron");

  std::unique_ptr<Fronctocol<FF_TYPES>> patron(
      new CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>(
          &this->info->compareInfo,
          dealerIdentity,
          this->numComparesNeeded * this->dispenserSize));
  this->invoke(std::move(patron), this->getPeers());
  this->state = awaitingCompare;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handleReceive(IncomingMessage_T &) {
  log_error("ZipAdjacentPatron Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("ZipAdjacentPatron received handle complete");
  switch (this->state) {
    case awaitingCompare: {
      this->compareDispenser = std::move(
          static_cast<
              CompareRandomnessPatron<FF_TYPES, Large_T, Small_T> &>(f)
              .compareDispenser);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<Large_T>,
              TypeCastFromBitInfo<Large_T>>(
              *dealerIdentity,
              this->numTCTsNeeded * this->dispenserSize,
              TypeCastFromBitInfo<Large_T>(
                  this->info->multiplyInfo.info.modulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingTct;
    } break;
    case awaitingTct: {
      log_debug("Case awaitingTct");
      this->endPrimeTCTripleDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<Large_T>,
                            TypeCastFromBitInfo<Large_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              BeaverTriple<Large_T>,
              BeaverInfo<Large_T>>(
              *dealerIdentity,
              this->numArithmeticBeaverTriplesNeeded *
                  this->dispenserSize,
              BeaverInfo<Large_T>(
                  this->info->multiplyInfo.info.modulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingArithmeticMultiply;

    } break;
    case awaitingArithmeticMultiply: {
      this->arithmeticMultiplyDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<Large_T>,
                            BeaverInfo<Large_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>(
              *dealerIdentity,
              this->numXORBeaverTriplesNeeded * this->dispenserSize,
              BooleanBeaverInfo()));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingXORMultiply;

    } break;
    case awaitingXORMultiply: {
      this->XORMultiplyDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<Boolean_t>,
                            BooleanBeaverInfo>> &>(f)
                        .result);

      this->generateOutputDispenser();
    } break;
    default:
      log_error("State machine in unexpected state");
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("ZipAdjacentPatron Fronctocol received unexpected"
            "handle promise");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacentRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    generateOutputDispenser() {
  for (size_t i = 0; i < this->dispenserSize; i++) {
    this->zipAdjacentDispenser->insert(
        ZipAdjacentRandomness<Large_T, Small_T>(
            std::move(this->compareDispenser->littleDispenser(
                this->numComparesNeeded)),
            std::move(this->endPrimeTCTripleDispenser->littleDispenser(
                this->numTCTsNeeded)),
            std::move(
                this->arithmeticMultiplyDispenser->littleDispenser(
                    this->numArithmeticBeaverTriplesNeeded)),
            std::move(this->XORMultiplyDispenser->littleDispenser(
                this->numXORBeaverTriplesNeeded))));
  }
  log_debug("calling this->complete");

  this->complete();
}

} // namespace mpc
} // namespace ff

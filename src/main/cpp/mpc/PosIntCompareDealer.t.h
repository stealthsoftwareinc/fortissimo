/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("PosInt Compare Randomness House large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    PosIntCompareRandomnessHouse(
        CompareInfo<Identity_T, Large_T, Small_T> const * const
            compareInfo) :
    compareInfo(compareInfo) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("PosIntCompareRandomnessHouse init");

  this->numDealersRemaining = 2;

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd(
      new CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>(
          this->compareInfo));
  this->invoke(std::move(rd), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Boolean_t>,
                                                BooleanBeaverInfo>());
  this->invoke(std::move(rd2), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleReceive(IncomingMessage_T &) {
  log_error("SISO House received unexpected handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> &) {
  log_debug("PosIntCompareRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("SISO House received unexpected "
            "handle promise");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("PosInt Compare Randomness Patron large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    PosIntCompareRandomnessPatron(
        CompareInfo<Identity_T, Large_T, Small_T> const * const
            compareInfo,
        const Identity_T * dealerIdentity,
        const size_t dispenserSize) :
    posIntCompareDispenser(new RandomnessDispenser<
                           PosIntCompareRandomness<Large_T, Small_T>,
                           DoNotGenerateInfo>(DoNotGenerateInfo())),
    compareInfo(compareInfo),
    dealerIdentity(dealerIdentity),
    dispenserSize(dispenserSize) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on PosIntComparePatron");

  std::unique_ptr<Fronctocol<FF_TYPES>> comparePromiseDispenserGadget(
      new CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>(
          this->compareInfo,
          dealerIdentity,
          this->dispenserSize * this->numCompares));
  this->invoke(
      std::move(comparePromiseDispenserGadget), this->getPeers());
  this->promiseState = awaitingCompare;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handleReceive(IncomingMessage_T &) {
  log_error("PosIntComparePatron Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("PosIntComparePatron received  handle complete");
  this->fullCompareDispenser = std::move(
      static_cast<
          CompareRandomnessPatron<FF_TYPES, Large_T, Small_T> &>(f)
          .compareDispenser);
  log_assert(this->fullCompareDispenser != nullptr);

  std::unique_ptr<PromiseFronctocol<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>>
      multiplyPromiseDispenserGadget(new RandomnessPatron<
                                     FF_TYPES,
                                     BeaverTriple<Boolean_t>,
                                     BooleanBeaverInfo>(
          *dealerIdentity,
          this->dispenserSize * this->numMultiplies,
          BooleanBeaverInfo()));
  this->fullMultiplyPromiseDispenser = this->promise(
      std::move(multiplyPromiseDispenserGadget), this->getPeers());
  this->await(*this->fullMultiplyPromiseDispenser);
  this->promiseState = awaitingMultiply;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("PosIntComparePatron Fronctocol received "
            "handle promise");

  switch (this->promiseState) {
    case (awaitingCompare): {

    } break;
    case (awaitingMultiply): {
      this->fullMultiplyDispenser =
          this->fullMultiplyPromiseDispenser->getResult(f);
      log_assert(this->fullMultiplyDispenser != nullptr);
      this->generateOutputDispenser();
    } break;
    default:
      log_error(
          "PosIntCompare promise state machine in unexpected state");
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    generateOutputDispenser() {
  for (size_t i = 0; i < this->dispenserSize; i++) {

    this->posIntCompareDispenser->insert(
        PosIntCompareRandomness<Large_T, Small_T>(
            std::move(this->fullCompareDispenser->littleDispenser(
                this->numCompares)),
            std::move(this->fullMultiplyDispenser->littleDispenser(
                this->numMultiplies))));
  }

  this->complete();
}

} // namespace mpc
} // namespace ff

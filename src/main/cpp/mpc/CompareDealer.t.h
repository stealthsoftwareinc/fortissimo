/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Compare House large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::
    CompareRandomnessHouse(
        CompareInfo<Identity_T, Large_T, Small_T> const * const
            compareInfo) :
    compareInfo(compareInfo) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("CompareRandomnessHouse init");
  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;

  size_t block_size = this->compareInfo->lambda;
  while (block_size < this->compareInfo->ell) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    log_debug("block_size: %lu", block_size);
    block_size += this->compareInfo->lambda;
  }
  log_debug("block_size: %lu", this->compareInfo->ell);
  UnboundedFaninOrRandomnessNeeds.emplace_back(this->compareInfo->ell);

  block_size = 1;
  while ((block_size - 1) < this->compareInfo->lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  this->numDealersRemaining =
      UnboundedFaninOrRandomnessNeeds.size() + 3;

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
void CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("SISO House received unexpected handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_debug("CompareRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessHouse<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("SISO House received unexpected "
            "handle promise");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string
CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Compare Patron large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    CompareRandomnessPatron(
        CompareInfo<Identity_T, Large_T, Small_T> const * const
            compareInfo,
        const Identity_T * dealerIdentity,
        const size_t dispenserSize) :
    compareDispenser(new RandomnessDispenser<
                     CompareRandomness<Large_T, Small_T>,
                     DoNotGenerateInfo>(DoNotGenerateInfo())),
    compareInfo(compareInfo),
    dealerIdentity(dealerIdentity),
    dispenserSize(dispenserSize) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on ComparePatron");

  size_t block_size = this->compareInfo->lambda;
  while (block_size < this->compareInfo->ell) {
    this->UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size += this->compareInfo->lambda;
  }
  this->UnboundedFaninOrRandomnessNeeds.emplace_back(
      this->compareInfo->ell);

  block_size = 1;
  while ((block_size - 1) < this->compareInfo->lambda) {
    this->UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }

  this->BeaverTriplesNeeded = 2 * this->compareInfo->ell +
      UnboundedFaninOrRandomnessNeeds.size() + 2;

  this->beaverInfo = BeaverInfo<Small_T>(this->compareInfo->s);

  this->dbsInfo = DecomposedBitSetInfo<Large_T, Small_T>(
      this->compareInfo->p,
      this->compareInfo->s,
      this->compareInfo->ell);

  std::unique_ptr<PromiseFronctocol<
      FF_TYPES,
      RandomnessDispenser<
          ExponentSeries<Small_T>,
          ExponentSeriesInfo<Small_T>>>>
      exponentPromiseDispenserGadget(new RandomnessPatron<
                                     FF_TYPES,
                                     ExponentSeries<Small_T>,
                                     ExponentSeriesInfo<Small_T>>(
          *this->dealerIdentity,
          this->dispenserSize,
          ExponentSeriesInfo<Small_T>(
              this->compareInfo->s,
              UnboundedFaninOrRandomnessNeeds[0])));
  this->exponentPromiseDispenser = this->promise(
      std::move(exponentPromiseDispenserGadget), this->getPeers());
  this->await(*this->exponentPromiseDispenser);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("ComparePatron Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> &) {
  log_error("ComparePatron received unexpected handle complete");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("ComparePatron Fronctocol received "
            "handle promise");

  switch (this->promiseState) {
    case (awaitingExponentSeries): {
      this->fullExponentDispensers.emplace_back(
          std::move(this->exponentPromiseDispenser->getResult(f)));
      log_assert(
          this->fullExponentDispensers
              [this->fullExponentDispensers.size() - 1] != nullptr);
      if (this->fullExponentDispensers.size() ==
          this->UnboundedFaninOrRandomnessNeeds.size()) {
        std::unique_ptr<PromiseFronctocol<
            FF_TYPES,
            RandomnessDispenser<
                BeaverTriple<Small_T>,
                BeaverInfo<Small_T>>>>
            multiplyPromiseDispenserGadget(new RandomnessPatron<
                                           FF_TYPES,
                                           BeaverTriple<Small_T>,
                                           BeaverInfo<Small_T>>(
                *dealerIdentity,
                this->dispenserSize * this->BeaverTriplesNeeded,
                this->beaverInfo));
        this->fullMultiplyPromiseDispenser = this->promise(
            std::move(multiplyPromiseDispenserGadget),
            this->getPeers());
        this->await(*this->fullMultiplyPromiseDispenser);
        this->promiseState = awaitingMultiply;
      } else {
        std::unique_ptr<PromiseFronctocol<
            FF_TYPES,
            RandomnessDispenser<
                ExponentSeries<Small_T>,
                ExponentSeriesInfo<Small_T>>>>
            exponentPromiseDispenserGadget(new RandomnessPatron<
                                           FF_TYPES,
                                           ExponentSeries<Small_T>,
                                           ExponentSeriesInfo<Small_T>>(
                *dealerIdentity,
                this->dispenserSize,
                ExponentSeriesInfo<Small_T>(
                    this->compareInfo->s,
                    UnboundedFaninOrRandomnessNeeds
                        [this->fullExponentDispensers.size()])));
        this->exponentPromiseDispenser = this->promise(
            std::move(exponentPromiseDispenserGadget),
            this->getPeers());
        this->await(*this->exponentPromiseDispenser);
      }

    } break;
    case (awaitingMultiply): {
      this->fullMultiplyDispenser =
          this->fullMultiplyPromiseDispenser->getResult(f);
      log_assert(this->fullMultiplyDispenser != nullptr);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              TypeCastTriple<Small_T>,
              TypeCastInfo<Small_T>>>>
          tctPromiseDispenserGadget(new RandomnessPatron<
                                    FF_TYPES,
                                    TypeCastTriple<Small_T>,
                                    TypeCastInfo<Small_T>>(
              *dealerIdentity,
              2 * this->dispenserSize,
              TypeCastInfo<Small_T>(this->compareInfo->s)));
      this->fullTctPromiseDispenser = this->promise(
          std::move(tctPromiseDispenserGadget), this->getPeers());
      this->await(*this->fullTctPromiseDispenser);
      this->promiseState = awaitingTct;
    } break;
    case (awaitingTct): {
      this->fullTctDispenser =
          this->fullTctPromiseDispenser->getResult(f);
      log_assert(this->fullTctDispenser != nullptr);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              DecomposedBitSet<Large_T, Small_T>,
              DecomposedBitSetInfo<Large_T, Small_T>>>>
          dbsPromiseDispenserGadget(
              new RandomnessPatron<
                  FF_TYPES,
                  DecomposedBitSet<Large_T, Small_T>,
                  DecomposedBitSetInfo<Large_T, Small_T>>(
                  *dealerIdentity, this->dispenserSize, this->dbsInfo));
      this->fullDbsPromiseDispenser = this->promise(
          std::move(dbsPromiseDispenserGadget), this->getPeers());
      this->await(*this->fullDbsPromiseDispenser);
      this->promiseState = awaitingDbs;
    } break;
    case (awaitingDbs): {
      this->fullDbsDispenser =
          this->fullDbsPromiseDispenser->getResult(f);
      log_assert(this->fullDbsDispenser != nullptr);
      this->generateOutputDispenser();
    } break;
    default:
      log_error("Compare promise state machine in unexpected state");
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void CompareRandomnessPatron<FF_TYPES, Large_T, Small_T>::
    generateOutputDispenser() {
  for (size_t i = 0; i < this->dispenserSize; i++) {
    std::vector<ExponentSeries<Small_T>> localLittleExponentSeries =
        std::vector<ExponentSeries<Small_T>>();
    for (size_t k = 0; k < UnboundedFaninOrRandomnessNeeds.size();
         k++) {
      localLittleExponentSeries.emplace_back(
          this->fullExponentDispensers[k]->get());
    }

    this->littleMultiplyDispenser =
        this->fullMultiplyDispenser->littleDispenser(
            this->BeaverTriplesNeeded);

    this->compareDispenser->insert(CompareRandomness<Large_T, Small_T>(
        std::move(localLittleExponentSeries),
        std::move(this->littleMultiplyDispenser),
        std::move(this->fullTctDispenser->get()),
        std::move(this->fullTctDispenser->get()),
        std::move(this->fullDbsDispenser->get())));
  }

  this->complete();
}

} // namespace mpc
} // namespace ff

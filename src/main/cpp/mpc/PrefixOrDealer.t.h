/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
std::string PrefixOrRandomnessHouse<FF_TYPES, Number_T>::name() {
  return std::string("Prefix Or Randomness House small mod: ") +
      dec(this->info->s);
}

template<FF_TYPENAMES, typename Number_T>
PrefixOrRandomnessHouse<FF_TYPES, Number_T>::PrefixOrRandomnessHouse(
    PrefixOrInfo<Identity_T, Number_T> const * const info) :
    info(info) {
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessHouse<FF_TYPES, Number_T>::init() {
  log_debug("PrefixOrRandomnessHouse init");

  std::vector<size_t> UnboundedFaninOrRandomnessNeeds;

  size_t block_size = this->info->lambda;
  size_t numElements = this->info->ell;
  while (block_size < numElements) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    log_debug("block_size: %lu", block_size);
    block_size += this->info->lambda;
  }
  log_debug("block_size: %lu", numElements);
  UnboundedFaninOrRandomnessNeeds.emplace_back(numElements);

  block_size = 1;
  while ((block_size - 1) < this->info->lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  this->numDealersRemaining =
      1 + UnboundedFaninOrRandomnessNeeds.size();

  for (size_t i = 0; i < UnboundedFaninOrRandomnessNeeds.size(); i++) {

    std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd(
        new RandomnessHouse<
            FF_TYPES,
            ExponentSeries<Number_T>,
            ExponentSeriesInfo<Number_T>>());
    this->invoke(std::move(rd), this->getPeers());
  }

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(
      new RandomnessHouse<
          FF_TYPES,
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>());
  this->invoke(std::move(rd2), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessHouse<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("SISO House received unexpected handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessHouse<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_debug("PrefixOrRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessHouse<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("SISO House received unexpected "
            "handle promise");
}

template<FF_TYPENAMES, typename Number_T>
std::string PrefixOrRandomnessPatron<FF_TYPES, Number_T>::name() {
  return std::string("Prefix Or Randomness Patron small mod: ") +
      dec(this->info->s);
}

template<FF_TYPENAMES, typename Number_T>
PrefixOrRandomnessPatron<FF_TYPES, Number_T>::PrefixOrRandomnessPatron(
    PrefixOrInfo<Identity_T, Number_T> const * const info,
    const Identity_T * dealerIdentity,
    const size_t dispenserSize) :
    prefixOrDispenser(new RandomnessDispenser<
                      PrefixOrRandomness<Number_T>,
                      DoNotGenerateInfo>(DoNotGenerateInfo())),
    info(info),
    dealerIdentity(dealerIdentity),
    dispenserSize(dispenserSize) {
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessPatron<FF_TYPES, Number_T>::init() {
  log_debug("PrefixOrPatron init");

  size_t block_size = this->info->lambda;
  size_t numElements = this->info->ell;
  while (block_size < numElements) {
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    log_debug("block_size: %lu", block_size);
    block_size += this->info->lambda;
  }
  log_debug("block_size: %lu", numElements);
  UnboundedFaninOrRandomnessNeeds.emplace_back(numElements);

  block_size = 1;
  while ((block_size - 1) < this->info->lambda) {
    log_debug("block_size: %lu", block_size);
    UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }
  log_debug("Total size: %lu", UnboundedFaninOrRandomnessNeeds.size());

  this->BeaverTriplesNeeded =
      2 * numElements + UnboundedFaninOrRandomnessNeeds.size();
  log_debug("This->BeaverTriplesNeeded %zu", this->BeaverTriplesNeeded);
  this->numPromisesRemaining = UnboundedFaninOrRandomnessNeeds.size();

  for (size_t i = 0; i < UnboundedFaninOrRandomnessNeeds.size(); i++) {
    log_debug("Adding ExponentSeries randomness generator");
    std::unique_ptr<PromiseFronctocol<
        FF_TYPES,
        RandomnessDispenser<
            ExponentSeries<Number_T>,
            ExponentSeriesInfo<Number_T>>>>
        es_drg(new RandomnessPatron<
               FF_TYPES,
               ExponentSeries<Number_T>,
               ExponentSeriesInfo<Number_T>>(
            *dealerIdentity,
            this->dispenserSize,
            ExponentSeriesInfo<Number_T>(
                this->info->s,
                this->UnboundedFaninOrRandomnessNeeds[i])));
    esFullPromiseDispensers.emplace_back(
        this->promise(std::move(es_drg), this->getPeers()));
  }
  log_debug(
      "this->info->s, %u, # needed, %zu",
      this->info->s,
      this->BeaverTriplesNeeded * this->dispenserSize);
  std::unique_ptr<PromiseFronctocol<
      FF_TYPES,
      RandomnessDispenser<
          BeaverTriple<Number_T>,
          BeaverInfo<Number_T>>>>
      beaver_drg(new RandomnessPatron<
                 FF_TYPES,
                 BeaverTriple<Number_T>,
                 BeaverInfo<Number_T>>(
          *dealerIdentity,
          this->BeaverTriplesNeeded * this->dispenserSize,
          BeaverInfo<Number_T>(this->info->s)));
  fullMultiplyPromiseDispenser =
      this->promise(std::move(beaver_drg), this->getPeers());
  this->await(*esFullPromiseDispensers[0]);
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessPatron<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("PrefixOrPatron Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessPatron<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("PrefixOrPatron received unexpected handle complete");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessPatron<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("PrefixOrPatron Fronctocol received "
            "handle promise");

  log_debug(
      "this->numPromisesRemaining %zu", this->numPromisesRemaining);

  if (this->numPromisesRemaining > 1) {
    esFullDispensers.emplace_back(
        esFullPromiseDispensers
            [UnboundedFaninOrRandomnessNeeds.size() -
             this->numPromisesRemaining--]
                ->getResult(f));
    this->await(*esFullPromiseDispensers
                    [UnboundedFaninOrRandomnessNeeds.size() -
                     this->numPromisesRemaining]);
  } else if (this->numPromisesRemaining == 1) {
    esFullDispensers.emplace_back(
        esFullPromiseDispensers
            [UnboundedFaninOrRandomnessNeeds.size() - 1]
                ->getResult(f));
    this->await(*fullMultiplyPromiseDispenser);
    this->numPromisesRemaining--;
  } else {
    fullMultiplyDispenser = fullMultiplyPromiseDispenser->getResult(f);
    this->generateOutputDispenser();
  }
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOrRandomnessPatron<FF_TYPES, Number_T>::
    generateOutputDispenser() {
  for (size_t i = 0; i < this->dispenserSize; i++) {
    std::vector<ExponentSeries<Number_T>> localLittleExponentSeries =
        std::vector<ExponentSeries<Number_T>>();
    for (size_t k = 0; k < UnboundedFaninOrRandomnessNeeds.size();
         k++) {
      localLittleExponentSeries.emplace_back(
          this->esFullDispensers[k]->get());
    }

    this->prefixOrDispenser->insert(PrefixOrRandomness<Number_T>(
        std::move(localLittleExponentSeries),
        std::move(this->fullMultiplyDispenser->littleDispenser(
            this->BeaverTriplesNeeded))));
  }

  this->complete();
}

} // namespace mpc
} // namespace ff

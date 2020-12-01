/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("QuickSort");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::QuickSortFronctocol(
    ObservationList<Large_T> * list,
    const CompareInfo<Identity_T, Large_T, Small_T> & compareInfo,
    const Identity_T * revealId,
    const Identity_T * dealerId) :
    inputList(list),
    multiplyInfo(revealId, BooleanBeaverInfo()),
    compareInfo(compareInfo),
    compareDispenser(new RandomnessDispenser<
                     CompareRandomness<Large_T, Small_T>,
                     DoNotGenerateInfo>(DoNotGenerateInfo())),
    revealIdentity(revealId),
    dealerIdentity(dealerId) {

  this->pivots = std::vector<size_t>(this->inputList->elements.size());
  this->comparisons =
      std::vector<Boolean_t>(this->inputList->elements.size());
  this->fullComparisonShares = std::vector<Boolean_t>(
      this->inputList->elements.size() * this->inputList->numKeyCols);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on quicksort");

  this->blockInfo = std::list<LoHiPair>();
  this->blockInfo.push_front(
      LoHiPair(0, this->inputList->elements.size() - 1));

  for (size_t i = 0; i < inputList->elements.size(); i++) {
    this->pivots[i] = (inputList->elements.size() - 1) / 2;
  }

  size_t block_size = this->compareInfo.lambda;
  while (block_size < this->compareInfo.ell) {
    this->UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size += this->compareInfo.lambda;
  }
  this->UnboundedFaninOrRandomnessNeeds.emplace_back(
      this->compareInfo.ell);

  block_size = 1;
  while ((block_size - 1) < this->compareInfo.lambda) {
    this->UnboundedFaninOrRandomnessNeeds.emplace_back(block_size);
    block_size++;
  }

  this->maxNumberCompares = this->inputList->elements.size() *
      this->inputList->numKeyCols *
      static_cast<size_t>(3 *
                              std::log2(static_cast<double>(
                                  this->inputList->elements.size())) +
                          1);
  this->BeaverTriplesNeeded = 2 * this->compareInfo.ell +
      UnboundedFaninOrRandomnessNeeds.size() + 2;

  this->XORBeaverTriplesNeeded =
      (this->maxNumberCompares * (this->inputList->numKeyCols - 1)) /
      this->inputList->numKeyCols;

  this->beaverInfo = BeaverInfo<Small_T>(this->compareInfo.s);

  this->dbsInfo = DecomposedBitSetInfo<Large_T, Small_T>(
      this->compareInfo.p, this->compareInfo.s, this->compareInfo.ell);

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
          this->maxNumberCompares,
          ExponentSeriesInfo<Small_T>(
              this->compareInfo.s,
              UnboundedFaninOrRandomnessNeeds[0])));
  this->exponentPromiseDispenser = this->promise(
      std::move(exponentPromiseDispenserGadget), this->getPeers());
  this->await(*this->exponentPromiseDispenser);

  this->littleExponentDispensers =
      std::vector<std::unique_ptr<RandomnessDispenser<
          ExponentSeries<Small_T>,
          ExponentSeriesInfo<Small_T>>>>();

  for (size_t k = 0; k < this->UnboundedFaninOrRandomnessNeeds.size();
       k++) {
    this->littleExponentDispensers.emplace_back(nullptr);
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::
    buildCompareDispenser() {

  for (size_t i = 0; i < this->maxNumberCompares; i++) {
    std::vector<ExponentSeries<Small_T>> localLittleExponentSeries =
        std::vector<ExponentSeries<Small_T>>();
    localLittleExponentSeries.reserve(
        this->UnboundedFaninOrRandomnessNeeds.size());
    for (size_t k = 0; k < this->UnboundedFaninOrRandomnessNeeds.size();
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

  for (size_t i = 0; i < this->UnboundedFaninOrRandomnessNeeds.size();
       i++) {
    this->fullExponentDispensers[i]->shrink();
  }
  this->fullMultiplyDispenser->shrink();
  this->fullTctDispenser->shrink();
  this->fullDbsDispenser->shrink();

  this->runComparisons();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::runComparisons() {

  std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
  batch->children.reserve(
      this->inputList->elements.size() * this->inputList->numKeyCols);

  for (size_t i = 0; i < this->inputList->elements.size(); i++) {
    if (pivots[i] != DO_NOT_COMPARE && pivots[i] != i) {
      for (size_t j = 0; j < this->inputList->numKeyCols; j++) {
        batch->children.emplace_back(
            new Compare<FF_TYPES, Large_T, Small_T>(
                this->inputList->elements[i].keyCols[j],
                this->inputList->elements[pivots[i]].keyCols[j],
                &this->compareInfo,
                std::move(this->compareDispenser->get())));
      }
    } else {
      log_debug("DO NOT COMPARE");
    }
  }

  PeerSet_T ps(this->getPeers());
  ps.remove(*dealerIdentity);
  this->invoke(std::move(batch), ps);
  this->state = awaitingBatchedCompare;
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Quicksort Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete");
  // case logic
  switch (this->state) {
    case awaitingBatchedCompare: {
      log_debug("Case awaitingBatchedCompare");

      /* retrieve results from batched compare */ {
        Batch<FF_TYPES> * batch = static_cast<Batch<FF_TYPES> *>(&f);
        this->fullComparisonShares.clear();
        this->fullComparisonShares.reserve(batch->children.size());
        for (size_t i = 0; i < batch->children.size(); i++) {
          Compare<FF_TYPES, Large_T, Small_T> * cmp =
              static_cast<Compare<FF_TYPES, Large_T, Small_T> *>(
                  batch->children[i].get());
          this->fullComparisonShares.push_back(cmp->outputShare);
        }
      }

      if (this->inputList->numKeyCols == 1) {
        this->partialComparisonsOutput = this->fullComparisonShares;
        std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
        batch->children.reserve(this->fullComparisonShares.size());

        for (size_t i = 0; i < this->fullComparisonShares.size(); i++) {
          batch->children.emplace_back(new Reveal<FF_TYPES, Boolean_t>(
              this->partialComparisonsOutput[i], this->revealIdentity));
        }

        PeerSet_T ps(this->getPeers());
        ps.remove(*dealerIdentity);
        log_debug("Invoking batched reveal");
        this->invoke(std::move(batch), ps);
        this->state = awaitingBatchedReveal;
      } else {
        numMultiplies = this->inputList->numKeyCols - 1;

        ::std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
        this->partialComparisonsOutput.resize(
            this->fullComparisonShares.size() /
            this->inputList->numKeyCols);
        batch->children.reserve(
            this->fullComparisonShares.size() /
            this->inputList->numKeyCols);

        size_t place = 0;
        for (size_t i = this->inputList->numKeyCols - 1;
             i < this->fullComparisonShares.size();
             i += this->inputList->numKeyCols) {
          batch->children.emplace_back(
              new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                  (this->fullComparisonShares[i - 1] / 2),
                  this->fullComparisonShares[i],
                  &this->partialComparisonsOutput[place++],
                  this->XORMultiplyDispenser->get(),
                  &this->multiplyInfo));
        }

        log_assert(place == batch->children.size());
        log_assert(place == this->partialComparisonsOutput.size());

        PeerSet_T ps(this->getPeers());
        ps.remove(*dealerIdentity);
        log_debug("Invoking batched multiply");
        this->invoke(std::move(batch), ps);
        this->state = awaitingBatchedMultiply;
      }
    } break;
    case awaitingBatchedMultiply: {
      if (this->numMultiplies > 1) {

        this->numMultiplies--;

        ::std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
        this->partialComparisonsOutput.resize(
            this->fullComparisonShares.size() /
            this->inputList->numKeyCols);

        size_t place = 0;
        for (size_t i = this->numMultiplies;
             i < this->fullComparisonShares.size();
             i += this->inputList->numKeyCols) {
          batch->children.emplace_back(
              new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                  (this->fullComparisonShares[i - 1] / 2),
                  this->partialComparisonsOutput
                          [i / this->inputList->numKeyCols] ^
                      (this->fullComparisonShares[i] % 2),
                  &this->partialComparisonsOutput[place++],
                  this->XORMultiplyDispenser->get(),
                  &this->multiplyInfo));
        }

        log_assert(place == batch->children.size());
        log_assert(place == this->partialComparisonsOutput.size());

        PeerSet_T ps(this->getPeers());
        ps.remove(*dealerIdentity);
        log_debug("Invoking batched multiply");
        this->invoke(std::move(batch), ps);
      } else {

        std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
        batch->children.reserve(
            this->fullComparisonShares.size() /
            this->inputList->numKeyCols);
        for (size_t i = 0; i < this->fullComparisonShares.size();
             i += this->inputList->numKeyCols) {
          batch->children.emplace_back(new Reveal<FF_TYPES, Boolean_t>(
              this->partialComparisonsOutput
                      [i / this->inputList->numKeyCols] ^
                  this->fullComparisonShares[i] % 2,
              this->revealIdentity));
        }
        PeerSet_T ps(this->getPeers());
        ps.remove(*dealerIdentity);
        log_debug("Invoking batched reveal");
        this->invoke(std::move(batch), ps);
        this->state = awaitingBatchedReveal;
      }
    } break;
    case awaitingBatchedReveal: {
      log_debug("Case awaitingBatchedReveal");

      /* retrieve results from batched reveal */ {
        Batch<FF_TYPES> * batch = static_cast<Batch<FF_TYPES> *>(&f);
        log_assert(
            batch->children.size() ==
            this->partialComparisonsOutput.size());
        for (size_t i = 0; i < batch->children.size(); i++) {
          Reveal<FF_TYPES, Boolean_t> * rev =
              static_cast<Reveal<FF_TYPES, Boolean_t> *>(
                  batch->children[i].get());
          this->partialComparisonsOutput[i] = rev->openedValue;
        }
      }

      uint8_t recursion_is_complete = 1;
      std::list<LoHiPair>::iterator it;
      size_t results_counter = 0;
      for (size_t i = 0; i < this->inputList->elements.size(); i++) {
        this->comparisons[i] = 2;
        if (pivots[i] != DO_NOT_COMPARE && pivots[i] != i) {
          this->comparisons[i] =
              this->partialComparisonsOutput[results_counter];
          results_counter++;
        }
      }
      log_debug("In quicksort logic proper");
      for (size_t i = 0; i < this->comparisons.size(); i++) {
        log_debug("Comparison[%lu] = %u", i, this->comparisons[i]);
      }
      for (it = this->blockInfo.begin(); it != this->blockInfo.end();) {
        size_t i = (*it).lo;
        size_t j = (*it).hi;
        const size_t old_lo = i;
        const size_t old_hi = j;
        if (i < j) {
          while (true) {
            while (this->comparisons[i] == 0) {
              i++;
            }
            while (this->comparisons[j] == 1) {
              j--;
            }
            if (i >= j) {
              break;
            }
            inputList->swap(i, j);
            Boolean_t temp_comparison = comparisons[i];
            comparisons[i] = comparisons[j];
            comparisons[j] = temp_comparison;

            i++;
            j--;
          }
          (*it).hi = j;
          it++;
          this->blockInfo.insert(it, LoHiPair(j + 1, old_hi));

          if (old_lo < j) {
            recursion_is_complete = 0;
            for (size_t k = old_lo; k <= j; k++) {
              this->pivots[k] = (old_lo + j) / 2;
            }
          } else {
            this->pivots[j] = DO_NOT_COMPARE;
          }

          if (j + 1 < old_hi) {
            recursion_is_complete = 0;
            for (size_t k = j + 1; k <= old_hi; k++) {
              this->pivots[k] = (j + 1 + old_hi) / 2;
            }
          } else {
            this->pivots[j + 1] = DO_NOT_COMPARE;
          }
        } else {
          it++;
        }
      }

      if (recursion_is_complete == 1) {
        this->complete();
      } else {
        this->runComparisons();
      }
    } break;
    default:
      log_error("Compare state machine in unexpected state");
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void QuickSortFronctocol<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Quicksort Fronctocol received "
            "handle promise");

  switch (this->promiseState) {
    case (awaitingExponentSeries): {
      log_debug("quicksort promise awaiting exponent series");
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
                this->maxNumberCompares * this->BeaverTriplesNeeded,
                this->beaverInfo));
        this->fullMultiplyPromiseDispenser = this->promise(
            std::move(multiplyPromiseDispenserGadget),
            this->getPeers());
        this->await(*this->fullMultiplyPromiseDispenser);
        this->promiseState = awaitingMultiply;
        log_debug(
            "changing to multiply, to get %zu beaver triples",
            this->maxNumberCompares * this->BeaverTriplesNeeded);
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
                this->maxNumberCompares,
                ExponentSeriesInfo<Small_T>(
                    this->compareInfo.s,
                    UnboundedFaninOrRandomnessNeeds
                        [this->fullExponentDispensers.size()])));
        this->exponentPromiseDispenser = this->promise(
            std::move(exponentPromiseDispenserGadget),
            this->getPeers());
        this->await(*this->exponentPromiseDispenser);
      }

    } break;
    case (awaitingMultiply): {
      log_debug("quicksort promise awaiting multiply");
      this->fullMultiplyDispenser =
          this->fullMultiplyPromiseDispenser->getResult(f);
      log_assert(this->fullMultiplyDispenser != nullptr);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>>>
          XORMultiplyPromiseDispenserGadget(new RandomnessPatron<
                                            FF_TYPES,
                                            BeaverTriple<Boolean_t>,
                                            BooleanBeaverInfo>(
              *dealerIdentity,
              this->XORBeaverTriplesNeeded,
              BooleanBeaverInfo()));
      this->XORMultiplyPromiseDispenser = this->promise(
          std::move(XORMultiplyPromiseDispenserGadget),
          this->getPeers());
      this->await(*this->XORMultiplyPromiseDispenser);
      this->promiseState = awaitingXORMultiply;
      log_debug(
          "changing to get XOR multiplies. %zu beaver triples",
          this->XORBeaverTriplesNeeded);
    } break;
    case (awaitingXORMultiply): {
      log_debug("quicksort promise awaiting XOR multiply");
      this->XORMultiplyDispenser =
          this->XORMultiplyPromiseDispenser->getResult(f);
      log_assert(this->XORMultiplyDispenser != nullptr);

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
              2 * this->maxNumberCompares,
              TypeCastInfo<Small_T>(this->compareInfo.s)));
      this->fullTctPromiseDispenser = this->promise(
          std::move(tctPromiseDispenserGadget), this->getPeers());
      this->await(*this->fullTctPromiseDispenser);
      this->promiseState = awaitingTct;
    } break;
    case (awaitingTct): {
      log_debug("quicksort promise awaiting TCT");
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
                  *dealerIdentity,
                  this->maxNumberCompares,
                  this->dbsInfo));
      this->fullDbsPromiseDispenser = this->promise(
          std::move(dbsPromiseDispenserGadget), this->getPeers());
      this->await(*this->fullDbsPromiseDispenser);
      this->promiseState = awaitingDbs;
    } break;
    case (awaitingDbs): {
      log_debug("quicksort promise awaiting DBS");
      this->fullDbsDispenser =
          this->fullDbsPromiseDispenser->getResult(f);
      log_assert(this->fullDbsDispenser != nullptr);
      this->buildCompareDispenser();
    } break;
    default:
      log_error("Compare promise state machine in unexpected state");
      this->abort();
  }
}

} // namespace mpc
} // namespace ff

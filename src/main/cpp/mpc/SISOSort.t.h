/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string SISOSort<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("SISO Sort modulus: ") + dec(this->modulus) +
      " size: " + std::to_string(this->sharedList.elements.size()) +
      " key columns: " + std::to_string(this->sharedList.numKeyCols) +
      " arithmetic columns: " +
      std::to_string(this->sharedList.numArithmeticPayloadCols);
  +" boolean columns: " +
      std::to_string(this->sharedList.numXORPayloadCols);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
SISOSort<FF_TYPES, Large_T, Small_T>::SISOSort(
    ObservationList<Large_T> & sharedList,
    Large_T modulus,
    const Identity_T * revealer,
    const Identity_T * dealerIdentity) :
    sharedList(sharedList),
    modulus(modulus),
    keyModulus(modulus),
    revealer(revealer),
    beaverInfo(modulus),
    keyBeaverInfo(modulus),
    dealerIdentity(dealerIdentity) {
  this->setup();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
SISOSort<FF_TYPES, Large_T, Small_T>::SISOSort(
    ObservationList<Large_T> & sharedList,
    Large_T modulus,
    Large_T keyModulus,
    const Identity_T * revealer,
    const Identity_T * dealerIdentity) :
    sharedList(sharedList),
    modulus(modulus),
    keyModulus(keyModulus),
    revealer(revealer),
    beaverInfo(modulus),
    keyBeaverInfo(keyModulus),
    dealerIdentity(dealerIdentity) {
  this->setup();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSort<FF_TYPES, Large_T, Small_T>::setup() {
  size_t list_size = this->sharedList.elements.size();
  this->d = static_cast<size_t>(std::ceil(
      std::log2(list_size))); // still assuming this is a power of 2

  this->expandedListSize = static_cast<size_t>(1 << this->d);
  log_debug("this->expandedListSize = %zu", this->expandedListSize);

  this->waksmanInfo = WaksmanInfo<Large_T>(
      this->modulus,
      this->keyModulus,
      this->expandedListSize,
      this->d,
      this->expandedListSize * (this->d - 1) + 1);

  this->smallModulus =
      nextPrime(static_cast<Small_T>(approxLog2(this->keyModulus)) + 3);
  this->bitsPerPrime =
      static_cast<size_t>(approxLog2(this->keyModulus)) + 1;
  this->sqrtEll =
      static_cast<size_t>(std::sqrt(this->bitsPerPrime)) + 1;

  this->lagrangePolynomialSet = std::vector<std::vector<Small_T>>();
  this->lagrangePolynomialSet.reserve(
      this->sqrtEll + this->bitsPerPrime / this->sqrtEll);

  size_t block_size = this->sqrtEll;
  while (block_size < this->bitsPerPrime) {
    this->lagrangePolynomialSet.emplace_back(
        computeLagrangeCoeffsForPrefixOr(
            this->smallModulus, static_cast<Small_T>(block_size)));
    //log_debug("block_size: %lu", block_size);
    block_size += this->sqrtEll;
  }
  //log_debug("block_size: %lu", this->compareInfo.ell);
  this->lagrangePolynomialSet.emplace_back(
      computeLagrangeCoeffsForPrefixOr(
          this->smallModulus,
          static_cast<Small_T>(this->bitsPerPrime)));

  block_size = 1;
  while ((block_size - 1) < this->sqrtEll) {
    //log_debug("block_size: %lu", block_size);
    this->lagrangePolynomialSet.emplace_back(
        computeLagrangeCoeffsForPrefixOr(
            this->smallModulus, static_cast<Small_T>(block_size)));
    block_size++;
  }
  log_debug("SISOSort Constructor successful");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSort<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on SISOSort");

  if_debug {
    log_assert(this->sharedList.numKeyCols > 0);
    for (size_t i = 0; i < this->sharedList.elements.size(); i++) {
      log_assert(
          this->sharedList.elements[i].keyCols.size() ==
          this->sharedList.numKeyCols);
      log_assert(
          this->sharedList.elements[i].arithmeticPayloadCols.size() ==
          this->sharedList.numArithmeticPayloadCols);
      log_assert(
          this->sharedList.elements[i].XORPayloadCols.size() ==
          this->sharedList.numXORPayloadCols);
    }
  }

  size_t BeaverTriplesNeeded =
      ((this->d - 1) * this->expandedListSize + 1) *
      (this->sharedList.numArithmeticPayloadCols); // for Waksman
  log_debug("BeaverTriplesNeeded: %zu", BeaverTriplesNeeded);

  log_debug("HI");
  std::unique_ptr<PromiseFronctocol<
      FF_TYPES,
      RandomnessDispenser<BeaverTriple<Large_T>, BeaverInfo<Large_T>>>>
      multiplyPromiseDispenserGadget(new RandomnessPatron<
                                     FF_TYPES,
                                     BeaverTriple<Large_T>,
                                     BeaverInfo<Large_T>>(
          *dealerIdentity, BeaverTriplesNeeded, this->beaverInfo));
  log_debug("HI");
  this->multiplyPromiseDispenser = this->promise(
      std::move(multiplyPromiseDispenserGadget), this->getPeers());
  this->await(*this->multiplyPromiseDispenser);
  log_debug("HI");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSort<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("SISOSort handlePromise");
  this->numPromisesRemaining--;
  log_debug(
      "numPromisesRemaining in SISOSort: %lu",
      this->numPromisesRemaining);
  switch (this->numPromisesRemaining) {
    case (3): {
      this->mrd = this->multiplyPromiseDispenser->getResult(f);

      log_assert(this->mrd != nullptr);

      size_t BeaverTriplesKeyNeeded =
          ((this->d - 1) * this->expandedListSize + 1) *
          (this->sharedList.numKeyCols); // for Waksman
      log_debug("BeaverTriplesKeyNeeded: %zu", BeaverTriplesKeyNeeded);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              BeaverTriple<Large_T>,
              BeaverInfo<Large_T>>>>
          multiplyPromiseDispenserGadget(new RandomnessPatron<
                                         FF_TYPES,
                                         BeaverTriple<Large_T>,
                                         BeaverInfo<Large_T>>(
              *dealerIdentity,
              BeaverTriplesKeyNeeded,
              this->keyBeaverInfo));

      this->multiplyKeyPromiseDispenser = this->promise(
          std::move(multiplyPromiseDispenserGadget), this->getPeers());
      this->await(*this->multiplyKeyPromiseDispenser);

    } break;
    case (2): {

      this->mrd_key = this->multiplyKeyPromiseDispenser->getResult(f);

      log_assert(this->mrd_key != nullptr);

      size_t XORBeaverTriplesNeeded =
          ((this->d - 1) * this->expandedListSize + 1) *
          (this->sharedList.numXORPayloadCols + 1);
      log_debug("XOR BeaverTriplesNeeded: %lu", XORBeaverTriplesNeeded);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              BeaverTriple<Boolean_t>,
              BooleanBeaverInfo>>>
          multiplyPromiseDispenserGadget(new RandomnessPatron<
                                         FF_TYPES,
                                         BeaverTriple<Boolean_t>,
                                         BooleanBeaverInfo>(
              *dealerIdentity,
              XORBeaverTriplesNeeded,
              BooleanBeaverInfo()));

      this->XORMultiplyPromiseDispenser = this->promise(
          std::move(multiplyPromiseDispenserGadget), this->getPeers());
      this->await(*XORMultiplyPromiseDispenser);
    } break;
    case (1): {
      this->XORmrd = this->XORMultiplyPromiseDispenser->getResult(f);
      log_assert(this->XORmrd != nullptr);

      std::unique_ptr<PromiseFronctocol<
          FF_TYPES,
          RandomnessDispenser<
              WaksmanBits<Large_T>,
              WaksmanInfo<Large_T>>>>
          waksman_drg(new RandomnessPatron<
                      FF_TYPES,
                      WaksmanBits<Large_T>,
                      WaksmanInfo<Large_T>>(
              *dealerIdentity, 1, waksmanInfo));
      this->waksmanPromiseDispenser =
          this->promise(std::move(waksman_drg), this->getPeers());
      this->await(*waksmanPromiseDispenser);
    } break;
    case (0): {
      auto a = this->waksmanPromiseDispenser->getResult(f);
      log_assert(a != nullptr);
      auto b = a->get();

      std::unique_ptr<WaksmanShuffle<FF_TYPES, Large_T>> waksman(
          new WaksmanShuffle<FF_TYPES, Large_T>(
              this->sharedList,
              this->modulus,
              this->keyModulus,
              b,
              this->d,
              std::move(this->mrd),
              std::move(this->mrd_key),
              std::move(this->XORmrd),
              this->revealer));
      PeerSet_T ps(this->getPeers());
      ps.remove(*dealerIdentity);
      this->invoke(std::move(waksman), ps);
    } break;
    default: {
      log_error("unexpected num promises remaining");
    }
  }
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSort<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in SISOSort");
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void SISOSort<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_debug("Calling handleComplete");
  switch (this->state) {
    case (awaitingWaksman): {
      log_debug("awaitingWaksman");
      compareInfo = std::move(
          std::unique_ptr<CompareInfo<Identity_T, Large_T, Small_T>>(
              new CompareInfo<Identity_T, Large_T, Small_T>(
                  this->keyModulus,
                  this->smallModulus,
                  this->bitsPerPrime,
                  this->sqrtEll,
                  this->lagrangePolynomialSet,
                  this->revealer)));
      log_debug("created compareInfo object");
      std::unique_ptr<QuickSortFronctocol<FF_TYPES, Large_T, Small_T>>
          quicksort(new QuickSortFronctocol<FF_TYPES, Large_T, Small_T>(
              &sharedList,
              *compareInfo.get(),
              revealer,
              dealerIdentity));
      log_debug("invoking");
      this->invoke(std::move(quicksort), this->getPeers());
      this->state = awaitingQuicksort;
    } break;
    case (awaitingQuicksort): {
      log_debug("quicksort finished. siso sort completing now.");
      this->complete();
    } break;
    default:
      log_error("SISOSort state machine in unexpected state");
  }
}

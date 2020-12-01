/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<typename Identity_T, typename Large_T, typename Small_T>
ZipAdjacentInfo<Identity_T, Large_T, Small_T>::ZipAdjacentInfo(
    const size_t batchSize,
    const size_t numArithmeticPayloadCols,
    const size_t numXORPayloadCols,
    const Large_T modulus,
    Identity_T const * const revealer) :
    batchSize(batchSize),
    numArithmeticPayloadCols(numArithmeticPayloadCols),
    numXORPayloadCols(numXORPayloadCols),
    compareInfo(modulus, revealer),
    multiplyInfo(revealer, BeaverInfo<Large_T>(modulus)),
    booleanMultiplyInfo(revealer, BooleanBeaverInfo()) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string ZipAdjacent<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("Zip Adjacent modulus: ") +
      dec(this->info->multiplyInfo.info.modulus);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
ZipAdjacent<FF_TYPES, Large_T, Small_T>::ZipAdjacent(
    const ObservationList<Large_T> & inputList,
    ZipAdjacentInfo<Identity_T, Large_T, Small_T> const * const info,
    ZipAdjacentRandomness<Large_T, Small_T> && randomness) :
    inputList(inputList),
    info(info),
    randomness(std::move(randomness)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacent<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on ZipAdjacent");

  std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

  this->compareResults.resize(this->inputList.elements.size() - 1);
  for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
    /* 
     *  For now, assume there are only two keys in keyCols
     * And we'll enforce that in general, once we have BIG_NUM
     * Second entry of keyCols is verticalIndex, so we don't want this
     * to be part of the comparison
     */

    /*
     * Also, note that, we're actually doing equality testing here, so 
     * this is overkill. With some effort, we can (and should) replace this
     * with a modified version of BitwiseCompare that calls
     * UnboundedFaninOr once.
     */
    log_debug(
        "Comparing %u, %u",
        inputList.elements[i].keyCols[0],
        inputList.elements[i + 1].keyCols[0]);
    batch->children.emplace_back(
        new Compare<FF_TYPES, Large_T, Small_T>(
            inputList.elements[i].keyCols[0],
            inputList.elements[i + 1].keyCols[0],
            &this->info->compareInfo,
            std::move(this->randomness.compareDispenser->get())));
  }

  this->invoke(std::move(batch), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacent<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in ZipAdjacent");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacent<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in ZipAdjacent");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void ZipAdjacent<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete");

  // case logic
  switch (this->state) {
    case awaitingCompare: {
      log_debug("Case awaitingCompare");
      Batch<FF_TYPES> & old_batch = static_cast<Batch<FF_TYPES> &>(f);
      for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
        this->compareResults[i] =
            (static_cast<Compare<FF_TYPES, Large_T, Small_T> &>(
                 *old_batch.children[i])
                 .outputShare) /
            0x02;
        log_debug("Compare share %hhu", this->compareResults[i]);
        log_debug(
            "Compare share not truncated %hhu",
            static_cast<Compare<FF_TYPES, Large_T, Small_T> &>(
                *old_batch.children[i])
                .outputShare);
      }

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
      this->typeCastFromBitResults.resize(
          this->inputList.elements.size() - 1);
      for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
        batch->children.emplace_back(
            new TypeCastFromBit<FF_TYPES, Large_T>(
                this->compareResults[i],
                this->info->multiplyInfo.info.modulus,
                this->info->compareInfo.revealer,
                this->randomness.tctDispenser->get()));
      }

      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingTypeCastFromBit;
    } break;
    case awaitingTypeCastFromBit: {
      log_debug("Case awaitingTypeCastFromBit");

      Batch<FF_TYPES> & old_batch = static_cast<Batch<FF_TYPES> &>(f);
      for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
        this->typeCastFromBitResults[i] =
            static_cast<TypeCastFromBit<FF_TYPES, Large_T> &>(
                *old_batch.children[i])
                .outputBitShare;
        log_debug("typeCast share %u", this->typeCastFromBitResults[i]);
      }

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

      log_debug(
          "beavertriples is %zu x %zu",
          2 * this->inputList.elements.size() - 2,
          this->inputList.numArithmeticPayloadCols);

      this->arithmeticMultiplyResults.resize(
          2 * this->inputList.elements.size() - 2);
      for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
        this->arithmeticMultiplyResults[2 * i].resize(
            this->inputList.numArithmeticPayloadCols);
        for (size_t j = 0; j < this->inputList.numArithmeticPayloadCols;
             j++) {
          batch->children.emplace_back(
              new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
                  this->typeCastFromBitResults[i],
                  this->inputList.elements[i].arithmeticPayloadCols[j],
                  &this->arithmeticMultiplyResults[2 * i][j],
                  this->randomness.arithmeticBeaverDispenser->get(),
                  &this->info->multiplyInfo));
        }
        this->arithmeticMultiplyResults[2 * i + 1].resize(
            this->inputList.numArithmeticPayloadCols);
        for (size_t j = 0; j < this->inputList.numArithmeticPayloadCols;
             j++) {
          batch->children.emplace_back(
              new Multiply<FF_TYPES, Large_T, BeaverInfo<Large_T>>(
                  this->typeCastFromBitResults[i],
                  this->inputList.elements[i + 1]
                      .arithmeticPayloadCols[j],
                  &this->arithmeticMultiplyResults[2 * i + 1][j],
                  this->randomness.arithmeticBeaverDispenser->get(),
                  &this->info->multiplyInfo));
        }
      }

      if (batch->children.size() > 0) {
        this->invoke(std::move(batch), this->getPeers());
      } else {
        log_debug("no arithmetic multiplies");
        this->numMultipliesRemaining--;
      }

      log_debug("XOR batching");

      std::unique_ptr<Batch<FF_TYPES>> batch2(new Batch<FF_TYPES>());

      this->XORMultiplyResults.resize(
          2 * this->inputList.elements.size() - 2);

      for (size_t i = 0; i < this->inputList.elements.size() - 1; i++) {
        this->XORMultiplyResults[2 * i].resize(
            this->inputList.numXORPayloadCols);
        for (size_t j = 0; j < this->inputList.numXORPayloadCols; j++) {
          batch2->children.emplace_back(
              new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                  this->compareResults[i],
                  this->inputList.elements[i].XORPayloadCols[j],
                  &this->XORMultiplyResults[2 * i][j],
                  this->randomness.XORBeaverDispenser->get(),
                  &this->info->booleanMultiplyInfo));
        }
        this->XORMultiplyResults[2 * i + 1].resize(
            this->inputList.numXORPayloadCols);
        for (size_t j = 0; j < this->inputList.numXORPayloadCols; j++) {
          batch2->children.emplace_back(
              new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                  this->compareResults[i],
                  this->inputList.elements[i + 1].XORPayloadCols[j],
                  &this->XORMultiplyResults[2 * i + 1][j],
                  this->randomness.XORBeaverDispenser->get(),
                  &this->info->booleanMultiplyInfo));
        }
      }
      if (batch2->children.size() > 0) {
        this->invoke(std::move(batch2), this->getPeers());
      } else {
        log_debug("no XOR multiplies");
        this->numMultipliesRemaining--;
      }

      this->state = awaitingMultiply;
    } break;
    case awaitingMultiply: {
      log_debug("Case awaitingMultiply");
      this->numMultipliesRemaining--;
      if (this->numMultipliesRemaining == 0) {

        //std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
        zippedAdjacentPairs.elements.reserve(
            this->inputList.elements.size() * 2);

        for (size_t i = 0; i < this->inputList.elements.size() - 1;
             i++) {

          Observation<Large_T> o1;
          Observation<Large_T> o2;

          o1.arithmeticPayloadCols =
              std::move(this->arithmeticMultiplyResults[2 * i]);
          o2.arithmeticPayloadCols =
              std::move(this->arithmeticMultiplyResults[2 * i + 1]);
          o1.XORPayloadCols =
              std::move(this->XORMultiplyResults[2 * i]);
          o2.XORPayloadCols =
              std::move(this->XORMultiplyResults[2 * i + 1]);
          log_debug(
              "o1.arithmeticPayloadCols.size() %zu, "
              "o2.arithmeticPayloadCols.size() %zu",
              o1.arithmeticPayloadCols.size(),
              o2.arithmeticPayloadCols.size());

          zippedAdjacentPairs.elements.push_back(o1);
          zippedAdjacentPairs.elements.push_back(o2);
        }

        this->complete();
      }
    } break;
    default:
      log_error(
          "Condtional Evaluate state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

/**
 * Copyright Stealth Software Technologies, Inc.
 */

/*
 WaksmanBits
*/

namespace ff {

template<typename Identity_T, typename Number_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::WaksmanInfo<Number_T> & info) {
  bool success = msg.template read<Number_T>(info.p);
  success = success && msg.template read<Number_T>(info.keyModulus);
  uint64_t n64 = 0UL;
  success = success && msg.template read<uint64_t>(n64);
  info.n = (size_t)n64;
  uint64_t d64 = 0UL;
  success = success && msg.template read<uint64_t>(d64);
  info.d = (size_t)d64;
  uint64_t w_of_n64 = 0UL;
  success = success && msg.template read<uint64_t>(w_of_n64);
  info.w_of_n = (size_t)w_of_n64;
  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::WaksmanInfo<Number_T> const & info) {
  bool success = msg.template write<Number_T>(info.p);
  success = success && msg.template write<Number_T>(info.keyModulus);
  success = success && msg.template write<uint64_t>((uint64_t)info.n);
  success = success && msg.template write<uint64_t>((uint64_t)info.d);
  success =
      success && msg.template write<uint64_t>((uint64_t)info.w_of_n);
  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::WaksmanBits<Number_T> & bits) {

  log_debug("reading waksman instance");

  uint64_t w_of_n64 = 0UL;
  bool success = msg.template read<uint64_t>(w_of_n64);
  size_t w_of_n = (size_t)w_of_n64;

  Number_T wi_input_arithmetic = 0UL;
  Boolean_t wi_input_boolean = 0x00;
  bits.arithmeticBitShares.reserve(w_of_n);
  bits.keyBitShares.reserve(w_of_n);
  bits.XORBitShares.reserve(w_of_n);

  for (size_t i = 0UL; i < w_of_n; i++) {
    success =
        success && msg.template read<Number_T>(wi_input_arithmetic);
    bits.arithmeticBitShares.emplace_back(wi_input_arithmetic);
  }
  for (size_t i = 0UL; i < w_of_n; i++) {
    success =
        success && msg.template read<Number_T>(wi_input_arithmetic);
    bits.keyBitShares.emplace_back(wi_input_arithmetic);
  }
  for (size_t i = 0UL; i < w_of_n; i++) {
    success = success && msg.template read<Boolean_t>(wi_input_boolean);
    bits.XORBitShares.emplace_back(wi_input_boolean);
  }

  return success;
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::WaksmanBits<Number_T> const & bits) {
  bool success = msg.template write<uint64_t>(
      (uint64_t)bits.arithmeticBitShares.size());

  for (size_t i = 0UL; i < bits.arithmeticBitShares.size(); i++) {
    success = success &&
        msg.template write<Number_T>(bits.arithmeticBitShares[i]);
  }
  for (size_t i = 0UL; i < bits.keyBitShares.size(); i++) {
    success =
        success && msg.template write<Number_T>(bits.keyBitShares[i]);
  }
  for (size_t i = 0UL; i < bits.arithmeticBitShares.size(); i++) {
    success =
        success && msg.template write<Boolean_t>(bits.XORBitShares[i]);
  }

  return success;
}

namespace mpc {

template<typename Number_T>
WaksmanBits<Number_T>::WaksmanBits(
    std::vector<Number_T> && arithmeticBitShares,
    std::vector<Number_T> && keyBitShares,
    std::vector<Boolean_t> && XORBitShares) :
    arithmeticBitShares(arithmeticBitShares),
    keyBitShares(keyBitShares),
    XORBitShares(XORBitShares) {
}

template<typename Number_T>
bool WaksmanInfo<Number_T>::operator==(
    WaksmanInfo<Number_T> const & other) const {
  if_debug {
    if (this->p == other.p && this->d == other.d) {
      // n, w_of_n are determined by d, so this is just a double-check
      log_assert(this->n == other.n && this->w_of_n == other.w_of_n);
      return true;
    } else {
      return false;
    }
  }
  else {
    return (
        this->p == other.p && this->keyModulus == other.keyModulus &&
        this->d == other.d);
  }
}

template<typename Number_T>
WaksmanInfo<Number_T>::WaksmanInfo(
    const Number_T p,
    const Number_T keyModulus,
    const size_t n,
    const size_t d,
    const size_t w_of_n) :
    p(p), keyModulus(keyModulus), n(n), d(d), w_of_n(w_of_n) {
}

template<typename Number_T>
void swap(std::vector<Number_T> & bits, size_t i, size_t j) {
  Number_T temp = bits[i];
  bits[i] = bits[j];
  bits[j] = temp;
}

template<typename Number_T>
void WaksmanInfo<Number_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<WaksmanBits<Number_T>> & vals) const {
  log_debug(
      "Generating random WaksmanBits object for d = %lu", this->d);
  /** Step 1. Randomly create the "original" randomness instance. */

  std::vector<size_t> permutation;
  std::vector<size_t> inverse_permutation(this->n, 0UL);
  permutation.reserve(this->n);
  for (size_t i = 0UL; i < this->n; i++) {
    permutation.emplace_back(static_cast<Number_T>(i));
  }

  for (size_t i = 0UL; i < this->n - 1; i++) {
    size_t j = i + randomModP<size_t>(this->n - i);
    swap(permutation, i, j);
  }
  for (size_t i = 0UL; i < this->n; i++) {
    inverse_permutation[static_cast<size_t>(permutation[i])] =
        static_cast<size_t>(i);
  }

  /*
   * For now, this block of code is written under the assumption that
   * n = 2^d. TODO: Make compatible with arbitrary n
   */
  std::vector<size_t> copy_permutation(this->n, 0UL);
  if_debug {
    log_assert(this->n == static_cast<size_t>(1UL << this->d));
    for (size_t i = 0UL; i < this->n; i++) {
      log_debug("sigma[%lu]=%lu", i, permutation[i]);
      copy_permutation[i] = inverse_permutation[i];
    }
    for (size_t i = 0UL; i < this->n; i++) {
      log_debug("sigma_inv[%lu]=%lu", i, inverse_permutation[i]);
    }
  }
  std::vector<Boolean_t> bits_XOR =
      std::vector<Boolean_t>(this->w_of_n);
  std::vector<Number_T> bits_arithmetic =
      std::vector<Number_T>(this->w_of_n);
  std::vector<Number_T> bits_arithmetic_key =
      std::vector<Number_T>(this->w_of_n);
  size_t front_iterator = 0UL;
  size_t back_iterator = this->w_of_n - 1UL;
  std::vector<Boolean_t> coloring(this->n, UNCOLORED);
  for (size_t i = 0UL; i < this->d; i++) {
    for (size_t j = 0UL; j < this->n; j++) {
      log_debug("sigma[%lu]=%lu", j, permutation[j]);
    }
    for (size_t j = 0UL; j < this->n; j++) {
      log_debug("sigma_inv[%lu]=%lu", j, inverse_permutation[j]);
    }
    for (size_t j = 0UL; j < this->n; j++) {
      coloring[j] = UNCOLORED;
    }
    for (size_t j = 0UL; j < (1UL << i); j++) {
      size_t currentPos = static_cast<size_t>(inverse_permutation[j]);

      while (coloring[currentPos] == UNCOLORED) {
        log_debug("currentPos is %lu", currentPos);
        coloring[currentPos] = RED;
        currentPos = toggle(i, currentPos);
        coloring[currentPos] = BLUE;
        log_debug(
            "After toggle currentPos is %lu and sigma(%lu)=%lu",
            currentPos,
            currentPos,
            permutation[currentPos]);
        currentPos =
            inverse_permutation[toggle(i, permutation[currentPos])];
      }
      for (size_t k = j; k < this->n; k += (1UL << i)) {
        size_t currentPos = k;
        log_debug(
            "in Follow-up loop outside while loop, currentPos = %lu",
            k);
        while (coloring[currentPos] == UNCOLORED) {
          log_debug("Follow-up loop: currentPos is %lu", currentPos);
          coloring[currentPos] = RED;
          currentPos = toggle(i, currentPos);
          coloring[currentPos] = BLUE;

          currentPos =
              inverse_permutation[toggle(i, permutation[currentPos])];
        }
      }
    }
    log_debug("Hi");

    if_debug {
      for (size_t something_else = 0UL;
           something_else < coloring.size();
           something_else++) {
        log_debug(
            "Coloring at %lu is %u",
            something_else,
            coloring[something_else]);
      }
    }
    log_debug("hi again");
    for (size_t j = 0UL; j < (1UL << i); j++) {
      for (size_t k = j; k < (this->n); k += (2UL << i)) {
        if (coloring[k] == RED) {
          bits_arithmetic[front_iterator] = 0;
          bits_arithmetic_key[front_iterator] = 0;
          bits_XOR[front_iterator] = 0x00;
        } else {
          bits_arithmetic[front_iterator] = 1;
          bits_arithmetic_key[front_iterator] = 1;
          bits_XOR[front_iterator] = 0xff;
        }
        front_iterator++;
      }
    }
    log_debug("After again");
    for (size_t j = 0UL; j < (1UL << i); j++) {
      inverse_permutation[j] = static_cast<size_t>(
          ((inverse_permutation[j] >> (i + 1UL)) << (i + 1UL)) + j);
      inverse_permutation[j + (1UL << i)] = static_cast<size_t>(
          ((inverse_permutation[j + (1UL << i)] >> (i + 1UL))
           << (i + 1UL)) +
          j + (1UL << i));
      log_debug("j: %lu", j);
      for (size_t k = j + (2UL << i); k < this->n; k += (2UL << i)) {
        log_debug("k: %lu", k);
        if (coloring[inverse_permutation[k]] == RED) {
          bits_arithmetic[back_iterator] = 0UL;
          bits_arithmetic_key[back_iterator] = 0;
          bits_XOR[back_iterator] = 0x00;
        } else {
          log_debug("Swapping %lu and %lu", k, k + (1UL << i));
          swap(inverse_permutation, k, k + (1UL << i));
          bits_arithmetic[back_iterator] = 1;
          bits_arithmetic_key[back_iterator] = 1;
          bits_XOR[back_iterator] = 0xff;
        }
        inverse_permutation[k] = static_cast<size_t>(
            ((inverse_permutation[k] >> (i + 1UL)) << (i + 1UL)) + j);
        inverse_permutation[k + (1UL << i)] = static_cast<size_t>(
            ((inverse_permutation[k + (1UL << i)] >> (i + 1UL))
             << (i + 1UL)) +
            j + (1UL << i));
        back_iterator--;
      }
    }
    for (size_t j = 0UL; j < this->n; j++) {
      permutation[inverse_permutation[j]] = static_cast<size_t>(j);
    }
  }
  if_debug {
    log_assert(front_iterator == back_iterator + 1UL);
    for (size_t i = 0UL; i < bits_arithmetic.size(); i++) {
      log_debug(
          "Waksman val at %lu is %s",
          i,
          dec(bits_arithmetic[i]).c_str());
    }
  }
  log_debug("out of bit assignment loops");

  /** Step 2. randomly secret share the original. */

  vals.resize(n_parties);
  // = std::vector<WaksmanBits>(
  //   n_parties, WaksmanBits(
  //       std::vector<ArithmeticShare_t>(w_of_n,0),
  //       std::vector<Boolean_t>(w_of_n,0)));

  for (size_t i = 1UL; i < n_parties; i++) {
    vals[i].arithmeticBitShares.resize(w_of_n);
    vals[i].keyBitShares.resize(w_of_n);
    vals[i].XORBitShares.resize(w_of_n);
    for (size_t j = 0UL; j < this->w_of_n; j++) {
      vals[i].arithmeticBitShares[j] = randomModP<Number_T>(this->p);
      vals[i].keyBitShares[j] = randomModP<Number_T>(this->keyModulus);
      vals[i].XORBitShares[j] = randomByte();
    }
  }
  log_debug("Done with step 2");

  /** Step 3. The last secret share is computed from the priors. */

  vals[0UL].arithmeticBitShares.resize(w_of_n);
  vals[0UL].keyBitShares.resize(w_of_n);
  vals[0UL].XORBitShares.resize(w_of_n);

  for (size_t i = 0UL; i < this->w_of_n; i++) {
    vals[0UL].arithmeticBitShares[i] = bits_arithmetic[i];
    vals[0UL].keyBitShares[i] = bits_arithmetic_key[i];
    vals[0UL].XORBitShares[i] = bits_XOR[i];
    for (size_t j = 1UL; j < n_parties; j++) {
      vals[0UL].arithmeticBitShares[i] = modSub(
          vals[0UL].arithmeticBitShares[i],
          vals[j].arithmeticBitShares[i],
          this->p);
      vals[0UL].keyBitShares[i] = modSub(
          vals[0UL].keyBitShares[i],
          vals[j].keyBitShares[i],
          this->keyModulus);
      vals[0UL].XORBitShares[i] =
          vals[0UL].XORBitShares[i] ^ vals[j].XORBitShares[i];
    }
  }

  log_debug("Done with step 3");

  if_debug {
    std::vector<size_t> test_permutation;
    test_permutation.reserve(this->n);
    for (size_t i = 0; i < this->n; i++) {
      test_permutation.emplace_back(static_cast<size_t>(i));
    }

    size_t WaksmanBitsCounter = 0;
    for (size_t i = 0; i < this->d; i++) {
      for (size_t j = 0; j < (1UL << i); j++) {
        for (size_t k = j; k < this->n; k += (2UL << i)) {
          Number_T currentBit = 0;
          for (size_t ell = 0; ell < n_parties; ell++) {
            currentBit = modAdd(
                currentBit,
                vals[ell].arithmeticBitShares[WaksmanBitsCounter],
                this->p);
          }
          log_assert(currentBit == bits_arithmetic[WaksmanBitsCounter]);
          if (currentBit == 1) {
            log_debug("Going to swap %lu and %lu", k, k + (1UL << i));
            swap(test_permutation, k, k + (1UL << i));
          }
          WaksmanBitsCounter++;
        }
      }
    }
    log_debug("back half");
    for (size_t i = this->d - 1; i != 0; i--) {
      for (size_t j = 0; j < (1UL << (i - 1UL)); j++) {
        size_t j_prime = (1UL << (i - 1UL)) - 1UL - j;
        for (size_t k = j_prime + this->n - (2UL << (i - 1UL));
             k != j_prime;
             k -= (2UL << (i - 1UL))) {
          log_debug("j_prime and k are %lu, %lu", j_prime, k);
          Number_T currentBit = 0;
          for (size_t ell = 0; ell < n_parties; ell++) {
            currentBit = modAdd(
                currentBit,
                vals[ell].arithmeticBitShares[WaksmanBitsCounter],
                this->p);
          }
          if (currentBit == 1UL) {
            swap(test_permutation, k, k + (1UL << (i - 1UL)));
          }
          WaksmanBitsCounter++;
        }
      }
    }
    for (size_t i = 0; i < this->n; i++) {
      log_debug(
          "vals at %zu: %lu, %lu",
          i,
          test_permutation[i],
          copy_permutation[i]);
    }
    for (size_t i = 0; i < this->n; i++) {
      log_assert(test_permutation[i] == copy_permutation[i]);
    }
  }
}

template<FF_TYPENAMES, typename Number_T>
std::string WaksmanShuffle<FF_TYPES, Number_T>::name() {
  return std::string("Waksman Shuffle");
}

template<FF_TYPENAMES, typename Number_T>
WaksmanShuffle<FF_TYPES, Number_T>::WaksmanShuffle(
    ObservationList<Number_T> & sharedList,
    Number_T modulus,
    Number_T keyModulus,
    WaksmanBits<Number_T> & swapBitShares,
    size_t d, // == log_2(sharedList.elements.size())
    std::unique_ptr<RandomnessDispenser<
        BeaverTriple<Number_T>,
        BeaverInfo<Number_T>>> mrd,
    std::unique_ptr<RandomnessDispenser<
        BeaverTriple<Number_T>,
        BeaverInfo<Number_T>>> mrd_key,
    std::unique_ptr<
        RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
        XORmrd,
    const Identity_T * revealer) :
    sharedList(sharedList),
    modulus(modulus),
    keyModulus(keyModulus),
    swapBitShares(swapBitShares),
    d(d),
    mrd(std::move(mrd)),
    mrd_key(std::move(mrd_key)),
    multiplyInfo(revealer, BeaverInfo<Number_T>(modulus)),
    multiplyKeyInfo(revealer, BeaverInfo<Number_T>(keyModulus)),
    XORmrd(std::move(XORmrd)),
    XORmultiplyInfo(revealer, BooleanBeaverInfo()),
    revealer(revealer) {
  this->info = BeaverInfo<Number_T>(this->modulus);
  this->info_key = BeaverInfo<Number_T>(this->keyModulus);
  this->batchedMultiplyResults = std::vector<Number_T>(
      (static_cast<size_t>(1UL << (this->d - 1UL))) *
          (this->sharedList.numArithmeticPayloadCols),
      0UL);
  this->batchedKeyMultiplyResults = std::vector<Number_T>(
      (static_cast<size_t>(1UL << (this->d - 1UL))) *
          (this->sharedList.numKeyCols),
      0UL);
  this->batchedXORMultiplyResults = std::vector<Boolean_t>(
      (static_cast<size_t>(1UL << (this->d - 1UL))) *
          (this->sharedList.numXORPayloadCols + 1UL),
      0UL);
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::init() {
  log_debug("Calling init on WaksmanShuffle");

  if (*revealer == this->getSelf()) {
    for (size_t i = 0; i < sharedList.elements.size(); i++) {
      sharedList.elements[i].XORPayloadCols.emplace_back(0x01);
    }
  } else {
    for (size_t i = 0; i < sharedList.elements.size(); i++) {
      sharedList.elements[i].XORPayloadCols.emplace_back(0x00);
    }
  }

  for (size_t i = sharedList.elements.size();
       i < static_cast<size_t>(1UL << this->d);
       i++) {
    Observation<Number_T> Ob;
    Ob.keyCols =
        std::vector<Number_T>(this->sharedList.numKeyCols, 0UL);
    Ob.arithmeticPayloadCols = std::vector<Number_T>(
        this->sharedList.numArithmeticPayloadCols, 0UL);
    Ob.XORPayloadCols = std::vector<Boolean_t>(
        this->sharedList.numXORPayloadCols + 1UL, 0x00);
    sharedList.elements.emplace_back(Ob);
  }

  this->batchMultiplyForSwaps();
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in WaksmanShuffle");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::launchFinalReveal() {
  log_debug("WaksmanShuffle calling launchFinalReveal");

  std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
  for (size_t j = 0UL; j < static_cast<size_t>(1UL << d); j++) {
    batch->children.emplace_back(new Reveal<FF_TYPES, Boolean_t>(
        this->sharedList.elements[j]
            .XORPayloadCols[this->sharedList.numXORPayloadCols],
        revealer));
  }

  this->invoke(std::move(batch), this->getPeers());
  this->state = awaitingFinalReveal;
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::batchMultiplyForSwaps() {
  log_debug("WaksmanShuffle calling batchMultiplyForSwaps");

  std::unique_ptr<Batch<FF_TYPES>> multiply_batch(
      new Batch<FF_TYPES>());
  this->XORmultiplyBatch =
      ::std::unique_ptr<Batch<FF_TYPES>>(new Batch<FF_TYPES>());
  size_t arith_place = 0UL;
  size_t arith_key_place = 0UL;
  size_t xor_place = 0lU;

  switch (this->state) {
    case (leftHalf): {
      log_debug(
          "this->sharedList.elements.size() = %lu",
          this->sharedList.elements.size());

      this->batchedMultiplyResults.clear();
      this->batchedKeyMultiplyResults.clear();
      this->batchedMultiplyResults.resize(
          (1UL << this->depth) *
          (this->sharedList.elements.size() / (2UL << this->depth)) *
          (this->sharedList.numArithmeticPayloadCols));
      this->batchedKeyMultiplyResults.resize(
          (1UL << this->depth) *
          (this->sharedList.elements.size() / (2UL << this->depth)) *
          (this->sharedList.numKeyCols));
      this->batchedXORMultiplyResults.clear();
      this->batchedXORMultiplyResults.resize(
          (1UL << this->depth) *
          (this->sharedList.elements.size() / (2UL << this->depth)) *
          (this->sharedList.numXORPayloadCols + 1UL));

      for (size_t j = 0UL; j < (1UL << this->depth); j++) {
        for (size_t k = j; k < this->sharedList.elements.size();
             k += (2UL << this->depth)) {
          for (size_t ell = 0UL; ell < this->sharedList.numKeyCols;
               ell++) {
            Number_T difference = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .keyCols[ell],
                this->sharedList.elements[k].keyCols[ell],
                this->keyModulus);

            multiply_batch->children.emplace_back(
                new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                    this->swapBitShares
                        .keyBitShares[this->waksmanVectorCounter],
                    difference,
                    &this->batchedKeyMultiplyResults[arith_key_place++],
                    this->mrd_key->get(),
                    &this->multiplyKeyInfo));
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numArithmeticPayloadCols;
               ell++) {
            Number_T difference = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .arithmeticPayloadCols[ell],
                this->sharedList.elements[k].arithmeticPayloadCols[ell],
                this->modulus);

            multiply_batch->children.emplace_back(
                new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                    this->swapBitShares.arithmeticBitShares
                        [this->waksmanVectorCounter],
                    difference,
                    &this->batchedMultiplyResults[arith_place++],
                    this->mrd->get(),
                    &this->multiplyInfo));
          }

          for (size_t ell = 0UL;
               ell < this->sharedList.numXORPayloadCols + 1UL;
               ell++) {
            Boolean_t difference =
                this->sharedList.elements[k + (1UL << this->depth)]
                    .XORPayloadCols[ell] ^
                this->sharedList.elements[k].XORPayloadCols[ell];

            this->XORmultiplyBatch->children.emplace_back(
                new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                    this->swapBitShares
                        .XORBitShares[this->waksmanVectorCounter],
                    difference,
                    &this->batchedXORMultiplyResults[xor_place++],
                    this->XORmrd->get(),
                    &this->XORmultiplyInfo));
          }
          this->waksmanVectorCounter++;
        }
      }

      log_debug(
          "About to invoke and multiplications has length %lu and "
          "results length %lu",
          multiply_batch->children.size(),
          this->batchedMultiplyResults.size());
    } break;
    case (rightHalf): {

      this->batchedMultiplyResults.clear();
      this->batchedKeyMultiplyResults.clear();
      this->batchedMultiplyResults.resize(
          ((this->sharedList.elements.size() / 2UL) -
           (1UL << this->depth)) *
          (this->sharedList.numArithmeticPayloadCols));
      this->batchedKeyMultiplyResults.resize(
          ((this->sharedList.elements.size() / 2UL) -
           (1UL << this->depth)) *
          (this->sharedList.numKeyCols));
      this->batchedXORMultiplyResults.clear();
      this->batchedXORMultiplyResults.resize(
          ((this->sharedList.elements.size() / 2UL) -
           (1UL << this->depth)) *
          (this->sharedList.numXORPayloadCols + 1UL));

      for (size_t j = 0UL; j < (1UL << this->depth); j++) {
        size_t j_prime = (1UL << this->depth) - 1 - j;
        for (size_t k = j_prime + this->sharedList.elements.size() -
                 (2UL << this->depth);
             k != j_prime;
             k -= (2UL << this->depth)) {
          for (size_t ell = 0UL; ell < this->sharedList.numKeyCols;
               ell++) {
            Number_T difference = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .keyCols[ell],
                this->sharedList.elements[k].keyCols[ell],
                this->keyModulus);

            multiply_batch->children.emplace_back(
                new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                    this->swapBitShares
                        .keyBitShares[this->waksmanVectorCounter],
                    difference,
                    &this->batchedKeyMultiplyResults[arith_key_place++],
                    this->mrd_key->get(),
                    &this->multiplyKeyInfo));
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numArithmeticPayloadCols;
               ell++) {
            Number_T difference = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .arithmeticPayloadCols[ell],
                this->sharedList.elements[k].arithmeticPayloadCols[ell],
                this->modulus);

            multiply_batch->children.emplace_back(
                new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                    this->swapBitShares.arithmeticBitShares
                        [this->waksmanVectorCounter],
                    difference,
                    &this->batchedMultiplyResults[arith_place++],
                    this->mrd->get(),
                    &this->multiplyInfo));
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numXORPayloadCols + 1UL;
               ell++) {
            Boolean_t difference =
                this->sharedList.elements[k + (1UL << this->depth)]
                    .XORPayloadCols[ell] ^
                this->sharedList.elements[k].XORPayloadCols[ell];

            this->XORmultiplyBatch->children.emplace_back(
                new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
                    this->swapBitShares
                        .XORBitShares[this->waksmanVectorCounter],
                    difference,
                    &this->batchedXORMultiplyResults[xor_place++],
                    this->XORmrd->get(),
                    &this->XORmultiplyInfo));
          }
          this->waksmanVectorCounter++;
        }
      }

      log_debug(
          "About to invoke and multiplications has length %lu and "
          "results length %lu",
          multiply_batch->children.size(),
          this->batchedMultiplyResults.size() +
              this->batchedKeyMultiplyResults.size());
    } break;
    default:
      log_error("Waksman state machine in unexpected state");
  }

  log_assert(
      multiply_batch->children.size() ==
      this->batchedMultiplyResults.size() +
          this->batchedKeyMultiplyResults.size());
  log_assert(
      multiply_batch->children.size() == arith_place + arith_key_place);
  log_assert(
      this->XORmultiplyBatch->children.size() ==
      this->batchedXORMultiplyResults.size());
  log_assert(this->XORmultiplyBatch->children.size() == xor_place);

  this->invoke(::std::move(multiply_batch), this->getPeers());

  awaitingArithmeticMultiplications = 1;
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in CompareWrapper");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void WaksmanShuffle<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete");
  if (awaitingArithmeticMultiplications) {
    log_debug(
        "About to invoke boolean and multiplications has length %lu "
        "and "
        "results length %lu",
        this->XORmultiplyBatch->children.size(),
        this->batchedXORMultiplyResults.size());
    this->invoke(std::move(this->XORmultiplyBatch), this->getPeers());
    awaitingArithmeticMultiplications = false;
    return;
  }

  switch (this->state) {
    case (leftHalf): {
      log_debug("Case left half, depth: %lu", this->depth);
      size_t multiplication_counter = 0UL;
      size_t key_multiplication_counter = 0UL;
      size_t XOR_multiplication_counter = 0UL;
      for (size_t j = 0LU; j < (1UL << this->depth); j++) {
        for (size_t k = j; k < this->sharedList.elements.size();
             k += (2UL << this->depth)) {
          log_debug("k: %lu", k);
          for (size_t ell = 0; ell < this->sharedList.numKeyCols;
               ell++) {
            this->sharedList.elements[k].keyCols[ell] = modAdd(
                this->sharedList.elements[k].keyCols[ell],
                this->batchedKeyMultiplyResults
                    [key_multiplication_counter],
                this->keyModulus);
            this->sharedList.elements[k + (1UL << this->depth)]
                .keyCols[ell] = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .keyCols[ell],
                this->batchedKeyMultiplyResults
                    [key_multiplication_counter],
                this->keyModulus);
            key_multiplication_counter++;
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numArithmeticPayloadCols;
               ell++) {
            this->sharedList.elements[k]
                .arithmeticPayloadCols[ell] = modAdd(
                this->sharedList.elements[k].arithmeticPayloadCols[ell],
                this->batchedMultiplyResults[multiplication_counter],
                this->modulus);
            this->sharedList.elements[k + (1UL << this->depth)]
                .arithmeticPayloadCols[ell] = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .arithmeticPayloadCols[ell],
                this->batchedMultiplyResults[multiplication_counter],
                this->modulus);

            multiplication_counter++;
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numXORPayloadCols + 1UL;
               ell++) {
            log_debug("ell %lu", ell);
            Boolean_t difference = this->batchedXORMultiplyResults
                                       [XOR_multiplication_counter];
            this->sharedList.elements[k].XORPayloadCols[ell] =
                this->sharedList.elements[k].XORPayloadCols[ell] ^
                difference;
            this->sharedList.elements[k + (1UL << this->depth)]
                .XORPayloadCols[ell] =
                this->sharedList.elements[k + (1lu << this->depth)]
                    .XORPayloadCols[ell] ^
                difference;
            XOR_multiplication_counter++;
          }
        }
      }
      if (this->depth == this->d - 1UL) {
        this->state = rightHalf;
        this->depth--;
        this->batchMultiplyForSwaps();
      } else {
        this->depth++;
        this->batchMultiplyForSwaps();
      }
    } break;
    case (rightHalf): {
      log_debug("Case right half, depth: %lu", this->depth);
      size_t multiplication_counter = 0UL;
      size_t key_multiplication_counter = 0UL;
      size_t XOR_multiplication_counter = 0UL;
      for (size_t j = 0; j < (1UL << this->depth); j++) {
        size_t j_prime = (1UL << this->depth) - 1UL - j;
        for (size_t k = j_prime + this->sharedList.elements.size() -
                 (2UL << this->depth);
             k != j_prime;
             k -= (2UL << this->depth)) {
          for (size_t ell = 0UL; ell < this->sharedList.numKeyCols;
               ell++) {
            this->sharedList.elements[k].keyCols[ell] = modAdd(
                this->sharedList.elements[k].keyCols[ell],
                this->batchedKeyMultiplyResults
                    [key_multiplication_counter],
                this->keyModulus);
            this->sharedList.elements[k + (1UL << this->depth)]
                .keyCols[ell] = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .keyCols[ell],
                this->batchedKeyMultiplyResults
                    [key_multiplication_counter],
                this->keyModulus);
            key_multiplication_counter++;
          }
          for (size_t ell = 0lu;
               ell < this->sharedList.numArithmeticPayloadCols;
               ell++) {
            this->sharedList.elements[k]
                .arithmeticPayloadCols[ell] = modAdd(
                this->sharedList.elements[k].arithmeticPayloadCols[ell],
                this->batchedMultiplyResults[multiplication_counter],
                this->modulus);
            this->sharedList.elements[k + (1UL << this->depth)]
                .arithmeticPayloadCols[ell] = modSub(
                this->sharedList.elements[k + (1UL << this->depth)]
                    .arithmeticPayloadCols[ell],
                this->batchedMultiplyResults[multiplication_counter],
                this->modulus);

            multiplication_counter++;
          }
          for (size_t ell = 0UL;
               ell < this->sharedList.numXORPayloadCols + 1UL;
               ell++) {
            Boolean_t difference = this->batchedXORMultiplyResults
                                       [XOR_multiplication_counter];
            this->sharedList.elements[k].XORPayloadCols[ell] =
                this->sharedList.elements[k].XORPayloadCols[ell] ^
                difference;
            this->sharedList.elements[k + (1UL << this->depth)]
                .XORPayloadCols[ell] =
                this->sharedList.elements[k + (1UL << this->depth)]
                    .XORPayloadCols[ell] ^
                difference;
            XOR_multiplication_counter++;
          }
        }
      }
      if (this->depth == 0UL) {
        this->launchFinalReveal();
      } else {
        this->depth--;
        this->batchMultiplyForSwaps();
      }
    } break;
    case (awaitingFinalReveal): {
      this->batchedRevealResults =
          std::vector<Boolean_t>(static_cast<size_t>(1UL << d));

      Batch<FF_TYPES> * batch = static_cast<Batch<FF_TYPES> *>(&f);
      log_assert(
          batch->children.size() == static_cast<size_t>(1UL << d));
      for (size_t i = 0UL; i < batch->children.size(); i++) {
        Reveal<FF_TYPES, Boolean_t> * rev =
            static_cast<Reveal<FF_TYPES, Boolean_t> *>(
                batch->children[i].get());
        this->batchedRevealResults[i] = rev->openedValue;
      }

      log_debug("deleting elements");
      for (size_t i = static_cast<size_t>(1UL << this->d); i > 0UL;
           i--) {
        if (this->batchedRevealResults[i - 1UL] == 0x00) {
          this->sharedList.elements.erase(
              this->sharedList.elements.begin() + (ssize_t)(i - 1UL));
        } else {
          this->sharedList.elements[i - 1UL].XORPayloadCols.erase(
              this->sharedList.elements[i - 1UL]
                  .XORPayloadCols.begin() +
              (ssize_t)this->sharedList.numXORPayloadCols);
        }
      }
      this->complete();
    } break;
    default:
      log_error("Waksman state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

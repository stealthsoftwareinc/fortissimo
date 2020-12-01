/**
 * Copyright Stealth Software Technologies, Inc.
 */

template<typename Identity_T, typename Number_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::TypeCastFromBitInfo<Number_T> & info) {
  return msg.template read<Number_T>(info.modulus);
}

template<typename Identity_T, typename Number_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::TypeCastFromBitInfo<Number_T> const & info) {
  return msg.template write<Number_T>(info.modulus);
}

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::
        ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T> &
            info) {
  bool success = true;
  success =
      success && msg.template read<LargeNumber_T>(info.endModulus);
  success =
      success && msg.template read<MediumNumber_T>(info.startModulus);
  success =
      success && msg.template read<SmallNumber_T>(info.smallModulus);
  std::size_t bitLength = 0;
  success = success && msg.template read<std::size_t>(bitLength);
  info.x_bitLength = static_cast<std::size_t>(bitLength);
  return success;
}

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::ModConvUpAuxInfo<
        SmallNumber_T,
        MediumNumber_T,
        LargeNumber_T> const & info) {
  bool success = true;
  success =
      success && msg.template write<LargeNumber_T>(info.endModulus);
  success =
      success && msg.template write<MediumNumber_T>(info.startModulus);
  success =
      success && msg.template write<SmallNumber_T>(info.smallModulus);
  success = success &&
      msg.template write<std::size_t>(
          static_cast<std::size_t>(info.x_bitLength));
  return success;
}

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
bool msg_read(
    IncomingMessage<Identity_T> & msg,
    mpc::ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T> &
        aux) {
  bool success = true;
  success = success && msg.template read<LargeNumber_T>(aux.r);
  success = success && msg.template read<LargeNumber_T>(aux.x);

  uint64_t ell_val = 0;
  success = success && msg.template read<uint64_t>(ell_val);
  size_t ell = static_cast<size_t>(ell_val);
  aux.bits_of_x.reserve(ell);

  SmallNumber_T aux_input;
  for (size_t i = 0; i < ell; i++) {
    success = success && msg.template read<SmallNumber_T>(aux_input);
    aux.bits_of_x.emplace_back(aux_input);
  }

  success = success && msg.template read<Boolean_t>(aux.LSB_of_r);
  return success;
}

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
bool msg_write(
    OutgoingMessage<Identity_T> & msg,
    mpc::ModConvUpAux<
        SmallNumber_T,
        MediumNumber_T,
        LargeNumber_T> const & aux) {
  bool success = true;
  success = success && msg.template write<LargeNumber_T>(aux.r);
  success = success && msg.template write<LargeNumber_T>(aux.x);

  success = success &&
      msg.template write<uint64_t>(
          static_cast<uint64_t>(aux.bits_of_x.size()));

  for (const auto & bit_x : aux.bits_of_x) {
    success = success && msg.template write<SmallNumber_T>(bit_x);
  }

  success = success && msg.template write<Boolean_t>(aux.LSB_of_r);
  return success;
}

namespace mpc {

template<typename Number_T>
TypeCastFromBitInfo<Number_T>::TypeCastFromBitInfo(
    const Number_T modulus) :
    modulus(modulus) {
}

template<typename Number_T>
bool TypeCastFromBitInfo<Number_T>::operator==(
    TypeCastFromBitInfo<Number_T> const & other) const {
  return this->modulus == other.modulus;
}

template<typename Number_T>
void TypeCastFromBitInfo<Number_T>::generate(
    size_t n_parties,
    size_t,
    std::vector<TypeCastTriple<Number_T>> & vals) const {

  log_debug("Generating type cast from bit triple");

  /** Step 1. Randomly create the "original" randomness instance. */
  auto const r_2 = randomModP<Boolean_t>(2);

  // using << as exponent for powers of 2.
  Number_T const r_0 = (r_2 == 0) ? 0 : 1;
  Number_T const r_1 = (r_2 == 0) ? 1 : 0;

  TypeCastTriple<Number_T> const og(r_0, r_1, r_2);

  /** Step 2. randomly secret share the original. */
  vals.reserve(n_parties);
  for (size_t i = 1; i < n_parties; i++) {
    auto sr_2 = randomModP<Boolean_t>(2);

    auto sr_0 = randomModP<Number_T>(this->modulus);
    auto sr_1 = randomModP<Number_T>(this->modulus);

    vals.emplace_back(sr_0, sr_1, sr_2);
  }

  /** Step 3. The last secret share is computed from the priors. */
  auto lr_2 = og.r_2;
  auto lr_0 = og.r_0;
  auto lr_1 = og.r_1;
  for (TypeCastTriple<Number_T> & prev : vals) {
    lr_2 = lr_2 ^ prev.r_2;
    lr_0 = (lr_0 + (this->modulus - prev.r_0)) % this->modulus;
    lr_1 = (lr_1 + (this->modulus - prev.r_1)) % this->modulus;
  }
  vals.emplace_back(lr_0, lr_1, lr_2);

  if_debug {
    auto tr_2 = vals.front().r_2;
    auto tr_0 = vals.front().r_0;
    auto tr_1 = vals.front().r_1;
    for (size_t i = 1; i < vals.size(); i++) {
      tr_2 = tr_2 ^ vals[i].r_2;
      tr_0 = (tr_0 + vals[i].r_0) % this->modulus;
      tr_1 = (tr_1 + vals[i].r_1) % this->modulus;
    }

    log_assert(tr_0 == og.r_0);
    log_assert(tr_1 == og.r_1);
    log_assert(tr_2 == og.r_2);
  }
}

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    ModConvUpAux(
        LargeNumber_T const & r,
        LargeNumber_T const & x,
        std::vector<SmallNumber_T> && bits_of_x,
        Boolean_t LSB_of_r) :
    r(r), x(x), bits_of_x(std::move(bits_of_x)), LSB_of_r(LSB_of_r) {
}

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    ModConvUpAuxInfo(
        LargeNumber_T const & endModulus,
        MediumNumber_T const & startModulus) :
    endModulus(endModulus), startModulus(startModulus) {
  this->x_bitLength = static_cast<size_t>(1 + approxLog2(startModulus));
  this->smallModulus =
      nextPrime(static_cast<SmallNumber_T>(x_bitLength + 1));
}

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
bool ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T>::
operator==(ModConvUpAuxInfo<
           SmallNumber_T,
           MediumNumber_T,
           LargeNumber_T> const & other) const {
  return this->startModulus == other.startModulus &&
      this->endModulus == other.endModulus;
}

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpAuxInfo<SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    generate(
        size_t n_parties,
        size_t,
        std::vector<ModConvUpAux<
            SmallNumber_T,
            MediumNumber_T,
            LargeNumber_T>> & vals) const {

  log_debug("Generating ModConvUp auxiliary randomness");
  log_debug("This->x_bitLength == %zu", this->x_bitLength);

  /** Step 1. Randomly create the "original" randomness instance. */
  LargeNumber_T const r = randomModP<LargeNumber_T>(this->endModulus);
  LargeNumber_T const x = r % this->startModulus;

  std::vector<SmallNumber_T> bits_of_x(this->x_bitLength, 0);

  LargeNumber_T temp_x = x;

  for (size_t i = 0; i < this->x_bitLength; i++) {
    bits_of_x[this->x_bitLength - 1 - i] =
        static_cast<SmallNumber_T>(temp_x % 2);
    temp_x /= 2;
  }

  auto const LSB_of_r = static_cast<Boolean_t>(r % 2);

  /** Step 2. randomly secret share the original. */
  vals.reserve(n_parties);
  for (size_t i = 1; i < n_parties; i++) {
    auto sr = randomModP<LargeNumber_T>(this->endModulus);
    auto sx = randomModP<LargeNumber_T>(this->endModulus);

    std::vector<SmallNumber_T> s_bits_of_x;
    for (size_t j = 0; j < this->x_bitLength; j++) {
      s_bits_of_x.emplace_back(
          randomModP<SmallNumber_T>(this->smallModulus));
    }

    auto s_LSB_of_r = randomModP<Boolean_t>(2);

    vals.emplace_back(sr, sx, std::move(s_bits_of_x), s_LSB_of_r);
  }

  /** Step 3. The last secret share is computed from the priors. */
  auto lr = r;
  auto lx = x;

  std::vector<SmallNumber_T> l_bits_of_x;
  for (size_t i = 0; i < this->x_bitLength; i++) {
    l_bits_of_x.emplace_back(bits_of_x[i]);
  }

  auto l_LSB_of_r = LSB_of_r;

  for (ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T> &
           prev : vals) {
    lr = (lr + (this->endModulus - prev.r)) % this->endModulus;
    lx = (lx + (this->endModulus - prev.x)) % this->endModulus;
    for (size_t i = 0; i < this->x_bitLength; i++) {
      l_bits_of_x[i] =
          (l_bits_of_x[i] + (this->smallModulus - prev.bits_of_x[i])) %
          this->smallModulus;
    }
    l_LSB_of_r = l_LSB_of_r ^ prev.LSB_of_r;
  }
  vals.emplace_back(lr, lx, std::move(l_bits_of_x), l_LSB_of_r);

  if_debug {
    LargeNumber_T tr = vals.front().r;
    LargeNumber_T tx = vals.front().x;
    std::vector<SmallNumber_T> t_bits_of_x;
    for (size_t i = 0; i < this->x_bitLength; i++) {
      t_bits_of_x.emplace_back(vals.front().bits_of_x[i]);
    }
    auto t_LSB_of_r = vals.front().LSB_of_r;

    for (size_t i = 1; i < vals.size(); i++) {
      tr = (tr + vals[i].r) % this->endModulus;
      tx = (tx + vals[i].x) % this->endModulus;
      for (size_t j = 0; j < this->x_bitLength; j++) {
        t_bits_of_x[j] = (t_bits_of_x[j] + vals[i].bits_of_x[j]) %
            this->smallModulus;
      }
      t_LSB_of_r = t_LSB_of_r ^ vals[i].LSB_of_r;
    }
    log_assert(tr == r);
    log_assert(tx == x);
    LargeNumber_T t_val = 0;
    for (size_t i = 0; i < this->x_bitLength; i++) {
      t_val *= 2;
      t_val += t_bits_of_x[i];
      log_assert(t_bits_of_x[i] == bits_of_x[i]);
    }
    log_assert(t_val == tx);
    log_assert(t_LSB_of_r == LSB_of_r);

    log_debug("HI");
    for (size_t i = 0; i < vals.size(); i++) {
      log_debug(
          "vals[%zu].bits_of_x.size() = %zu",
          i,
          vals[i].bits_of_x.size());
    }
  }
}

template<FF_TYPENAMES, typename Number_T>
std::string TypeCastFromBit<FF_TYPES, Number_T>::name() {
  return std::string("Typecast From Bit modulus: ") +
      dec(this->modulus);
}

template<FF_TYPENAMES, typename Number_T>
TypeCastFromBit<FF_TYPES, Number_T>::TypeCastFromBit(
    const Boolean_t XORShareOfBit,
    const Number_T modulus,
    const Identity_T * rev,
    const TypeCastTriple<Number_T> & tcTriple) :
    XORShareOfBit(XORShareOfBit),
    modulus(modulus),
    revealer(rev),
    tcTriple(tcTriple) {
}

template<FF_TYPENAMES, typename Number_T>
void TypeCastFromBit<FF_TYPES, Number_T>::init() {
  log_debug(
      "Calling init on TypeCastFromBit with "
      "tcTriple %s %s %hhu",
      dec(this->tcTriple.r_0).c_str(),
      dec(this->tcTriple.r_1).c_str(),
      this->tcTriple.r_2);

  std::unique_ptr<Reveal<FF_TYPES, Boolean_t>> reveal(
      new Reveal<FF_TYPES, Boolean_t>(
          this->XORShareOfBit ^ this->tcTriple.r_2, this->revealer));
  this->invoke(std::move(reveal), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void TypeCastFromBit<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in TypeCastFromBit");
}

template<FF_TYPENAMES, typename Number_T>
void TypeCastFromBit<FF_TYPES, Number_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in TypeCastFromBit");
}

template<FF_TYPENAMES, typename Number_T>
void TypeCastFromBit<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete on TypeCastFromBit");

  Boolean_t ov =
      static_cast<Reveal<FF_TYPES, Boolean_t> &>(f).openedValue;
  log_debug("opened value is %u", ov);
  log_assert(ov == 0x00 || ov == 0x01);
  if (ov == 0x00) {
    outputBitShare = this->tcTriple.r_0;
  } else {
    outputBitShare = this->tcTriple.r_1;
  }
  this->complete();
}

template<
    typename Identity_T,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpInfo<
    Identity_T,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::
    ModConvUpInfo(
        const LargeNumber_T endModulus,
        const MediumNumber_T startModulus,
        Identity_T const * const revealer) :
    mcuaInfo(endModulus, startModulus),
    prefInfo(
        this->mcuaInfo.smallModulus,
        this->mcuaInfo.x_bitLength,
        revealer),
    multiplyInfo(revealer, BooleanBeaverInfo()) {
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
std::string
ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    name() {
  return std::string("Mod Conv Up large mod: ") +
      dec(this->info->mcuaInfo.endModulus) +
      " medium mod: " + dec(this->info->mcuaInfo.startModulus) +
      " small mod: " + dec(this->info->prefInfo.s);
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    ModConvUp(
        const MediumNumber_T inputShare,
        ModConvUpInfo<
            Identity_T,
            SmallNumber_T,
            MediumNumber_T,
            LargeNumber_T> const * const info,
        ModConvUpRandomness<
            SmallNumber_T,
            MediumNumber_T,
            LargeNumber_T> && randomness) :
    inputShare(inputShare),
    info(info),
    randomness(std::move(randomness)) {
}

template<
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpRandomness<SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    ModConvUpRandomness(
        std::unique_ptr<RandomnessDispenser<
            BitwiseCompareRandomness<SmallNumber_T>,
            DoNotGenerateInfo>> bitwiseDispenser,
        std::unique_ptr<RandomnessDispenser<
            TypeCastTriple<LargeNumber_T>,
            TypeCastFromBitInfo<LargeNumber_T>>>
            endPrimeTCTripleDispenser,
        std::unique_ptr<RandomnessDispenser<
            BeaverTriple<Boolean_t>,
            BooleanBeaverInfo>> XORBeaverDispenser,
        const ModConvUpAux<
            SmallNumber_T,
            MediumNumber_T,
            LargeNumber_T> & mcua) :
    bitwiseDispenser(std::move(bitwiseDispenser)),
    endPrimeTCTripleDispenser(std::move(endPrimeTCTripleDispenser)),
    XORBeaverDispenser(std::move(XORBeaverDispenser)),
    mcua(mcua) {
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    init() {
  log_debug("Calling init on ModConvUp");

  std::unique_ptr<Reveal<FF_TYPES, LargeNumber_T>> reveal(
      new Reveal<FF_TYPES, LargeNumber_T>(
          modAdd(
              static_cast<LargeNumber_T>(this->inputShare),
              this->randomness.mcua.r,
              this->info->mcuaInfo.endModulus),
          this->info->mcuaInfo.endModulus,
          this->info->prefInfo.rev));
  this->invoke(std::move(reveal), this->getPeers());
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in ModConvUp");
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    handleReceive(IncomingMessage_T &) {
  log_error("Unexpected handleReceive in ModConvUp");
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUp<FF_TYPES, SmallNumber_T, MediumNumber_T, LargeNumber_T>::
    handleComplete(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete on ModConvUp");
  // case logic
  switch (this->state) {
    case awaitingReveal: {
      log_debug("Case awaitingReveal in ModConvUp");
      auto & reveal = static_cast<Reveal<FF_TYPES, LargeNumber_T> &>(f);
      LargeNumber_T c = reveal.openedValue;
      log_debug("Revealed value %s", dec(c).c_str());

      this->t = c % this->info->mcuaInfo.startModulus;

      this->LSB_sum_shares_plus_r = 0;
      if (*this->info->prefInfo.rev == this->getSelf()) {
        this->LSB_sum_shares_plus_r = static_cast<Boolean_t>(c % 2);
      }
      this->q_tilde =
          (this->info->mcuaInfo.endModulus %
           this->info->mcuaInfo.startModulus);

      /** By adding 1, we fix the edge case where t = x tilde*/
      LargeNumber_T t_temp = t + 1;

      this->tBitsMod_s = std::vector<SmallNumber_T>(
          this->info->mcuaInfo.x_bitLength, 0);

      for (size_t i = 0; i < this->info->mcuaInfo.x_bitLength; i++) {
        this->tBitsMod_s[this->info->mcuaInfo.x_bitLength - 1 - i] =
            static_cast<SmallNumber_T>(t_temp % 2);
        t_temp /= 2;
      }

      log_debug(
          "first length %zu, second length %zu",
          this->tBitsMod_s.size(),
          this->randomness.mcua.bits_of_x.size());

      std::unique_ptr<BitwiseCompare<FF_TYPES, SmallNumber_T>>
          bitwiseCompare(new BitwiseCompare<FF_TYPES, SmallNumber_T>(
              this->randomness.mcua.bits_of_x,
              this->tBitsMod_s,
              &this->info->prefInfo,
              this->randomness.bitwiseDispenser->get()));
      this->invoke(std::move(bitwiseCompare), this->getPeers());
      this->state = awaitingFirstBitwiseCompare;

    } break;
    case awaitingFirstBitwiseCompare: {
      log_debug("Case awaitingFirstBitwiseCompare");
      this->firstStartPrimeCarryBit =
          static_cast<BitwiseCompare<FF_TYPES, SmallNumber_T> &>(f)
              .outputShare %
          2;
      if (*this->info->prefInfo.rev == this->getSelf()) {
        this->firstStartPrimeCarryBit =
            this->firstStartPrimeCarryBit ^ 0x01;
      }

      log_debug(
          "firstStartPrimeCarryBit %hhu",
          this->firstStartPrimeCarryBit);
      /** i.e. bitwisecompare returns a > b, where b is public, 
          we want t+q >= x, which is equivalent to t+q + 1 > x. 
          Note that we add 1 after reducing mod startModulus for 
          the case where t + q_tilde = x = startModulus - 1
           */
      LargeNumber_T compare_value_2 =
          ((this->t + this->q_tilde) %
           this->info->mcuaInfo.startModulus) +
          1;

      log_debug(
          "compare_value_2 %s and t: %s",
          dec(compare_value_2).c_str(),
          dec(this->t).c_str());

      std::vector<SmallNumber_T> compare_bits_2(
          this->info->mcuaInfo.x_bitLength, 0);
      LargeNumber_T t_temp = compare_value_2;

      for (size_t i = 0; i < this->info->mcuaInfo.x_bitLength; i++) {
        compare_bits_2[this->info->mcuaInfo.x_bitLength - 1 - i] =
            static_cast<SmallNumber_T>(t_temp % 2);
        t_temp /= 2;
      }

      std::unique_ptr<BitwiseCompare<FF_TYPES, SmallNumber_T>>
          bitwiseCompare2(new BitwiseCompare<FF_TYPES, SmallNumber_T>(
              this->randomness.mcua.bits_of_x,
              compare_bits_2,
              &this->info->prefInfo,
              this->randomness.bitwiseDispenser->get()));
      this->invoke(std::move(bitwiseCompare2), this->getPeers());
      this->state = awaitingSecondBitwiseCompare;
    } break;
    case awaitingSecondBitwiseCompare: {
      log_debug("Case awaitingSecondBitwiseCompare");
      this->secondStartPrimeCarryBit =
          static_cast<BitwiseCompare<FF_TYPES, SmallNumber_T> &>(f)
              .outputShare %
          2;
      this->endPrimeCarryBit =
          static_cast<Boolean_t>(this->inputShare % 2) ^
          this->randomness.mcua.LSB_of_r;
      if (*this->info->prefInfo.rev == this->getSelf()) {
        this->endPrimeCarryBit = this->endPrimeCarryBit ^
            static_cast<Boolean_t>(this->LSB_sum_shares_plus_r);
      }

      log_debug(
          "secondStartPrimeCarryBit %hhu, endPrimeCarryBit %hhu",
          this->secondStartPrimeCarryBit,
          this->endPrimeCarryBit);

      log_debug("b");
      if (this->t + (this->q_tilde) >=
          this->info->mcuaInfo.startModulus) {
        this->state = preparingFakeXORMultiply;
        this->secondStartPrimeCarryBit =
            this->secondStartPrimeCarryBit ^
            this->firstStartPrimeCarryBit;
        log_debug("c");
      } else {
        this->state = awaitingFirstXORMultiply;
        log_debug("c'");
      }
      this->andResult = 0;
      std::unique_ptr<Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>>
          multiply(new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
              this->endPrimeCarryBit,
              this->secondStartPrimeCarryBit,
              &this->andResult,
              std::move(this->randomness.XORBeaverDispenser->get()),
              &this->info->multiplyInfo));
      this->invoke(std::move(multiply), this->getPeers());
    } break;
    case preparingFakeXORMultiply: {
      std::unique_ptr<Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>>
          multiply(new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
              0,
              0,
              &this->fakeAndResult,
              std::move(this->randomness.XORBeaverDispenser->get()),
              &this->info->multiplyInfo));
      this->invoke(std::move(multiply), this->getPeers());

      this->state = awaitingLastXORMultiply;

    } break;
    case awaitingFirstXORMultiply: {
      Boolean_t copy_val = this->andResult;
      log_debug("Case awaitingFirstXORMultiply");
      std::unique_ptr<Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>>
          multiply(new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
              copy_val,
              this->firstStartPrimeCarryBit,
              &this->andResult,
              std::move(this->randomness.XORBeaverDispenser->get()),
              &this->info->multiplyInfo));
      this->invoke(std::move(multiply), this->getPeers());

      this->state = awaitingLastXORMultiply;
    } break;
    case awaitingLastXORMultiply: {
      log_debug("Case awaitingLastXORMultiply");
      std::unique_ptr<TypeCastFromBit<FF_TYPES, LargeNumber_T>>
          typeCast(new TypeCastFromBit<FF_TYPES, LargeNumber_T>(
              this->firstStartPrimeCarryBit,
              this->info->mcuaInfo.endModulus,
              this->info->prefInfo.rev,
              this->randomness.endPrimeTCTripleDispenser->get()));
      this->invoke(std::move(typeCast), this->getPeers());

      this->state = awaitingFirstTypeCast;
    } break;
    case awaitingFirstTypeCast: {
      log_debug("Case awaitingFirstTypeCast");
      this->firstStartPrimeCarryBitArithmetic =
          static_cast<TypeCastFromBit<FF_TYPES, LargeNumber_T> &>(f)
              .outputBitShare;
      std::unique_ptr<TypeCastFromBit<FF_TYPES, LargeNumber_T>>
          typeCast(new TypeCastFromBit<FF_TYPES, LargeNumber_T>(
              this->endPrimeCarryBit,
              this->info->mcuaInfo.endModulus,
              this->info->prefInfo.rev,
              this->randomness.endPrimeTCTripleDispenser->get()));
      this->invoke(std::move(typeCast), this->getPeers());

      this->state = awaitingSecondTypeCast;
    } break;
    case awaitingSecondTypeCast: {
      log_debug("Case awaitingSecondTypeCast");
      this->endPrimeCarryBitArithmetic =
          static_cast<TypeCastFromBit<FF_TYPES, LargeNumber_T> &>(f)
              .outputBitShare;
      std::unique_ptr<TypeCastFromBit<FF_TYPES, LargeNumber_T>>
          typeCast(new TypeCastFromBit<FF_TYPES, LargeNumber_T>(
              this->andResult,
              this->info->mcuaInfo.endModulus,
              this->info->prefInfo.rev,
              this->randomness.endPrimeTCTripleDispenser->get()));
      this->invoke(std::move(typeCast), this->getPeers());

      this->state = awaitingThirdTypeCast;
    } break;
    case awaitingThirdTypeCast: {
      log_debug("Case awaitingThirdTypeCast");
      this->andResultArithmetic =
          static_cast<TypeCastFromBit<FF_TYPES, LargeNumber_T> &>(f)
              .outputBitShare;

      log_debug(
          "bits: this->firstStartPrimeCarryBitArithmetic, "
          "this->endPrimeCarryBitArithmetic, this->andResultArithmetic "
          "%s, %s, %s",
          dec(this->firstStartPrimeCarryBitArithmetic).c_str(),
          dec(this->endPrimeCarryBitArithmetic).c_str(),
          dec(this->andResultArithmetic).c_str());

      log_debug(
          "t, -x, p, q_tilde, -p  %s, %s, %s, %s, %s",
          dec(t).c_str(),
          dec(this->info->mcuaInfo.endModulus - this->randomness.mcua.x)
              .c_str(),
          dec(this->info->mcuaInfo.startModulus).c_str(),
          dec(this->q_tilde).c_str(),
          dec(this->info->mcuaInfo.endModulus -
              this->info->mcuaInfo.startModulus)
              .c_str());

      if (*this->info->prefInfo.rev == this->getSelf()) {
        LargeNumber_T tmp1 = modAdd(
            this->t,
            this->info->mcuaInfo.endModulus - this->randomness.mcua.x,
            this->info->mcuaInfo.endModulus);
        LargeNumber_T tmp2 = modMul(
            static_cast<LargeNumber_T>(
                this->info->mcuaInfo.startModulus),
            this->firstStartPrimeCarryBitArithmetic,
            this->info->mcuaInfo.endModulus);
        LargeNumber_T tmp3 = modMul(
            this->q_tilde,
            this->endPrimeCarryBitArithmetic,
            this->info->mcuaInfo.endModulus);
        LargeNumber_T tmp4 = modMul(
            this->info->mcuaInfo.endModulus -
                static_cast<LargeNumber_T>(
                    this->info->mcuaInfo.startModulus),
            this->andResultArithmetic,
            this->info->mcuaInfo.endModulus);

        this->outputShare = modAdd(
            tmp1,
            modAdd(
                tmp2,
                modAdd(tmp3, tmp4, this->info->mcuaInfo.endModulus),
                this->info->mcuaInfo.endModulus),
            this->info->mcuaInfo.endModulus);
      } else {
        LargeNumber_T tmp1 = modMul(
            static_cast<LargeNumber_T>(
                this->info->mcuaInfo.startModulus),
            this->firstStartPrimeCarryBitArithmetic,
            this->info->mcuaInfo.endModulus);
        LargeNumber_T tmp2 = modMul(
            this->q_tilde,
            this->endPrimeCarryBitArithmetic,
            this->info->mcuaInfo.endModulus);
        LargeNumber_T tmp3 = modMul(
            this->info->mcuaInfo.endModulus -
                static_cast<LargeNumber_T>(
                    this->info->mcuaInfo.startModulus),
            this->andResultArithmetic,
            this->info->mcuaInfo.endModulus);
        this->outputShare = modAdd(
            tmp1,
            modAdd(
                tmp2,
                modAdd(
                    this->info->mcuaInfo.endModulus -
                        this->randomness.mcua.x,
                    tmp3,
                    this->info->mcuaInfo.endModulus),
                this->info->mcuaInfo.endModulus),
            this->info->mcuaInfo.endModulus);
      }
      this->complete();
    } break;
    default:
      log_error("ModConvUp state machine in unexpected state");
  }
}

} // namespace mpc

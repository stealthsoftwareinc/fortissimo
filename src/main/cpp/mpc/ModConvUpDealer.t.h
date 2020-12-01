/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
std::string ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::name() {
  return std::string("Mod Conv Up Randomness House large mod: ") +
      dec(this->info->mcuaInfo.endModulus) +
      " medium mod: " + dec(this->info->mcuaInfo.startModulus) +
      " small mod: " + dec(this->info->prefInfo.s);
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::
    ModConvUpRandomnessHouse(ModConvUpInfo<
                             Identity_T,
                             SmallNumber_T,
                             MediumNumber_T,
                             LargeNumber_T> const * const info) :
    info(info) {
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::init() {

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd(
      new PrefixOrRandomnessHouse<FF_TYPES, SmallNumber_T>(
          &info->prefInfo));
  this->invoke(std::move(rd), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd2(
      new RandomnessHouse<
          FF_TYPES,
          BeaverTriple<SmallNumber_T>,
          BeaverInfo<SmallNumber_T>>());
  this->invoke(std::move(rd2), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd3(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<SmallNumber_T>,
          TypeCastInfo<SmallNumber_T>>());
  this->invoke(std::move(rd3), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd5(
      new RandomnessHouse<
          FF_TYPES,
          TypeCastTriple<LargeNumber_T>,
          TypeCastFromBitInfo<LargeNumber_T>>());
  this->invoke(std::move(rd5), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd6(new RandomnessHouse<
                                                FF_TYPES,
                                                BeaverTriple<Boolean_t>,
                                                BooleanBeaverInfo>());
  this->invoke(std::move(rd6), this->getPeers());

  std::unique_ptr<ff::Fronctocol<FF_TYPES>> rd7(
      new RandomnessHouse<
          FF_TYPES,
          ModConvUpAux<SmallNumber_T, MediumNumber_T, LargeNumber_T>,
          ModConvUpAuxInfo<
              SmallNumber_T,
              MediumNumber_T,
              LargeNumber_T>>());
  this->invoke(std::move(rd7), this->getPeers());
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handleReceive(IncomingMessage_T &) {
  log_error("SISO House received unexpected handle receive");
  this->abort();
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handleComplete(ff::Fronctocol<FF_TYPES> &) {
  log_debug("ModConvUpRandomnessHouse handleComplete");
  this->numDealersRemaining--;
  if (this->numDealersRemaining == 0) {
    log_debug("Dealer done");
    this->complete();
  }
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessHouse<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("SISO House received unexpected handle promise");
  this->abort();
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
std::string ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::name() {
  return std::string("Mod Conv Up Randomness Patron large mod: ") +
      dec(this->info->mcuaInfo.endModulus) +
      " medium mod: " + dec(this->info->mcuaInfo.startModulus) +
      " small mod: " + dec(this->info->prefInfo.s);
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::
    ModConvUpRandomnessPatron(
        ModConvUpInfo<
            Identity_T,
            SmallNumber_T,
            MediumNumber_T,
            LargeNumber_T> const * const info,
        const Identity_T * dealerIdentity,
        const size_t dispenserSize) :
    modConvUpDispenser(new RandomnessDispenser<
                       ModConvUpRandomness<
                           SmallNumber_T,
                           MediumNumber_T,
                           LargeNumber_T>,
                       DoNotGenerateInfo>(DoNotGenerateInfo())),
    bitwiseDispenser(new RandomnessDispenser<
                     BitwiseCompareRandomness<SmallNumber_T>,
                     DoNotGenerateInfo>(DoNotGenerateInfo())),
    info(info),
    dealerIdentity(dealerIdentity),
    dispenserSize(dispenserSize) {
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::init() {
  log_debug("ModConvUpPatron init");

  std::unique_ptr<Fronctocol<FF_TYPES>> patron(
      new PrefixOrRandomnessPatron<FF_TYPES, SmallNumber_T>(
          &info->prefInfo,
          dealerIdentity,
          this->numBitwiseComparesNeeded * this->dispenserSize));
  this->invoke(std::move(patron), this->getPeers());
  this->state = awaitingPrefixOr;
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handleReceive(IncomingMessage_T &) {
  log_error("ModConvUpPatron Fronctocol received unexpected "
            "handle receive");
  this->abort();
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handleComplete(ff::Fronctocol<FF_TYPES> & f) {
  log_debug("ModConvUpPatron received handle complete");

  switch (this->state) {
    case awaitingPrefixOr: {
      this->fullPrefixOrDispenser = std::move(
          static_cast<
              PrefixOrRandomnessPatron<FF_TYPES, SmallNumber_T> &>(f)
              .prefixOrDispenser);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              BeaverTriple<SmallNumber_T>,
              BeaverInfo<SmallNumber_T>>(
              *dealerIdentity,
              this->numSmallPrimeBeaverTriplesNeeded *
                  this->dispenserSize,
              BeaverInfo<SmallNumber_T>(
                  this->info->mcuaInfo.smallModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingMultiply;

    } break;
    case awaitingMultiply: {
      this->fullStartPrimeMultiplyDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<SmallNumber_T>,
                            BeaverInfo<SmallNumber_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<SmallNumber_T>,
              TypeCastInfo<SmallNumber_T>>(
              *dealerIdentity,
              this->numSmallPrimeTCTsNeeded * this->dispenserSize,
              TypeCastInfo<SmallNumber_T>(
                  this->info->mcuaInfo.smallModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingStartTCT;
    } break;
    case awaitingStartTCT: {
      this->fullStartTCTDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<SmallNumber_T>,
                            TypeCastInfo<SmallNumber_T>>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(
          new RandomnessPatron<
              FF_TYPES,
              TypeCastTriple<LargeNumber_T>,
              TypeCastFromBitInfo<LargeNumber_T>>(
              *dealerIdentity,
              this->numEndPrimeTCTsNeeded * this->dispenserSize,
              TypeCastFromBitInfo<LargeNumber_T>(
                  this->info->mcuaInfo.endModulus)));
      this->invoke(std::move(patron), this->getPeers());
      this->state = awaitingEndTCT;
    } break;
    case awaitingEndTCT: {
      this->fullEndPrimeTCTDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            TypeCastTriple<LargeNumber_T>,
                            TypeCastFromBitInfo<LargeNumber_T>>> &>(f)
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
      this->fullXORMultiplyDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            BeaverTriple<Boolean_t>,
                            BooleanBeaverInfo>> &>(f)
                        .result);

      std::unique_ptr<Fronctocol<FF_TYPES>> patron(new RandomnessPatron<
                                                   FF_TYPES,
                                                   ModConvUpAux<
                                                       SmallNumber_T,
                                                       MediumNumber_T,
                                                       LargeNumber_T>,
                                                   ModConvUpAuxInfo<
                                                       SmallNumber_T,
                                                       MediumNumber_T,
                                                       LargeNumber_T>>(
          *dealerIdentity,
          this->numModConvUpAuxNeeded * this->dispenserSize,
          this->info->mcuaInfo));
      this->invoke(std::move(patron), this->getPeers());

      this->state = awaitingModConvUpAux;
    } break;
    case awaitingModConvUpAux: {

      this->fullModConvUpAuxDispenser =
          std::move(static_cast<PromiseFronctocol<
                        FF_TYPES,
                        RandomnessDispenser<
                            ModConvUpAux<
                                SmallNumber_T,
                                MediumNumber_T,
                                LargeNumber_T>,
                            ModConvUpAuxInfo<
                                SmallNumber_T,
                                MediumNumber_T,
                                LargeNumber_T>>> &>(f)
                        .result);

      this->generateBitwiseOutputDispenser();
    } break;
    default:
      log_error("State machine in unexpected state");
  }
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::handlePromise(ff::Fronctocol<FF_TYPES> &) {
  log_error("ModConvUpPatron Fronctocol received unexpected "
            "handle promise");
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::generateBitwiseOutputDispenser() {
  for (size_t i = 0;
       i < this->numBitwiseComparesNeeded * this->dispenserSize;
       i++) {

    this->bitwiseDispenser->insert(
        BitwiseCompareRandomness<SmallNumber_T>(
            this->fullPrefixOrDispenser->get(),
            this->fullStartPrimeMultiplyDispenser->get(),
            this->fullStartTCTDispenser->get(),
            this->fullStartPrimeMultiplyDispenser->get(),
            this->fullStartTCTDispenser->get()));
  }

  this->generateOutputDispenser();
}

template<
    FF_TYPENAMES,
    typename SmallNumber_T,
    typename MediumNumber_T,
    typename LargeNumber_T>
void ModConvUpRandomnessPatron<
    FF_TYPES,
    SmallNumber_T,
    MediumNumber_T,
    LargeNumber_T>::generateOutputDispenser() {

  for (size_t i = 0; i < this->dispenserSize; i++) {

    this->modConvUpDispenser->insert(ModConvUpRandomness<
                                     SmallNumber_T,
                                     MediumNumber_T,
                                     LargeNumber_T>(
        std::move(this->bitwiseDispenser->littleDispenser(
            this->numBitwiseComparesNeeded)),
        std::move(this->fullEndPrimeTCTDispenser->littleDispenser(
            this->numEndPrimeTCTsNeeded)),
        std::move(this->fullXORMultiplyDispenser->littleDispenser(
            this->numXORBeaverTriplesNeeded)),
        this->fullModConvUpAuxDispenser->get()));
  }

  this->complete();
}

} // namespace mpc
} // namespace ff

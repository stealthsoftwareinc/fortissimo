/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<typename Number_T>
std::vector<std::vector<Number_T>> generateLagrangePolynomialSet(
    size_t bitsPerPrime,
    size_t sqrtEll,
    Number_T const & smallModulus) {
  std::vector<std::vector<Number_T>> lagrangePolynomialSet =
      std::vector<std::vector<Number_T>>();
  lagrangePolynomialSet.reserve(bitsPerPrime / sqrtEll + sqrtEll + 1);

  size_t block_size = sqrtEll;
  while (block_size < bitsPerPrime) {
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        smallModulus, static_cast<Number_T>(block_size)));
    block_size += sqrtEll;
  }
  lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
      smallModulus, static_cast<Number_T>(bitsPerPrime)));

  block_size = 1;
  while ((block_size - 1) < sqrtEll) {
    lagrangePolynomialSet.emplace_back(computeLagrangeCoeffsForPrefixOr(
        smallModulus, static_cast<Number_T>(block_size)));
    block_size++;
  }
  return lagrangePolynomialSet;
}

template<typename Identity_T, typename Number_T>
PrefixOrInfo<Identity_T, Number_T>::PrefixOrInfo(
    const Number_T s,
    const size_t lambda, // such that lambda^2 > inputVals.size()
    const size_t ell,
    const std::vector<std::vector<Number_T>> & lagrangePolynomialSet,
    const Identity_T * rev) :
    s(s),
    lambda(lambda),
    ell(ell),
    lagrangePolynomialSet(lagrangePolynomialSet),
    rev(rev) {
}

template<typename Identity_T, typename Number_T>
PrefixOrInfo<Identity_T, Number_T>::PrefixOrInfo(
    const Number_T s, const size_t ell, const Identity_T * revealer) :
    s(s),
    lambda(static_cast<size_t>(
        std::ceil(std::sqrt(static_cast<double>(ell + 1))))),
    ell(ell),
    lagrangePolynomialSet(generateLagrangePolynomialSet(
        this->ell, this->lambda, this->s)),
    rev(revealer) {
}

template<FF_TYPENAMES, typename Number_T>
std::string PrefixOr<FF_TYPES, Number_T>::name() {
  return std::string("Prefix Or small mod: ") + dec(this->info->s);
}

template<FF_TYPENAMES, typename Number_T>
PrefixOr<FF_TYPES, Number_T>::PrefixOr(
    const std::vector<Number_T> & inputVals,
    PrefixOrInfo<Identity_T, Number_T> const * const info,
    PrefixOrRandomness<Number_T> && randomness) :
    inputVals(inputVals),
    info(info),
    randomness(std::move(randomness)),
    multiplyInfo(this->info->rev, BeaverInfo<Number_T>(this->info->s)),
    unboundedFaninOrInfo(this->info->s, this->info->rev) {
  log_debug(
      "size of exponentSeries: %zu",
      this->randomness.exponentSeries.size());
  log_assert(
      this->info->lambda * this->info->lambda > this->inputVals.size());
  log_debug("this->info->lambda %zu", this->info->lambda);
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOr<FF_TYPES, Number_T>::init() {
  log_debug("Calling init on prefixor");
  this->fronctocolResults = std::vector<Number_T>(
      ((this->inputVals.size() - 1) / this->info->lambda) + 1);

  this->initAfterRandomness();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOr<FF_TYPES, Number_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in PrefixOr");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOr<FF_TYPES, Number_T>::initAfterRandomness() {
  log_debug(
      "Init after randomness this->inputVals.size(): %zu, lambda: %zu",
      this->inputVals.size(),
      this->info->lambda);

  ::std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
  Number_T current_A = 0;

  size_t place = 0;
  for (size_t i = 0; i < this->inputVals.size(); i++) {
    current_A = modAdd(current_A, this->inputVals[i], this->info->s);
    if ((i + 1) % this->info->lambda == 0) {
      batch->children.emplace_back(
          new UnboundedFaninOr<FF_TYPES, Number_T>(
              current_A,
              i + 1,
              &this->fronctocolResults[place++],
              std::move(
                  this->randomness.exponentSeries[exponentSeriesIndex]),
              this->randomness.multiplyDispenser->get(),
              this->info->lagrangePolynomialSet[exponentSeriesIndex],
              &this->unboundedFaninOrInfo));

      exponentSeriesIndex++;
      log_debug("exponentSeriesIndex: %zu", this->exponentSeriesIndex);
    }
  }

  if ((this->inputVals.size()) % this->info->lambda != 0) {
    batch->children.emplace_back(
        new UnboundedFaninOr<FF_TYPES, Number_T>(
            current_A,
            this->inputVals.size(),
            &this->fronctocolResults[place++],
            std::move(
                this->randomness.exponentSeries[exponentSeriesIndex]),
            this->randomness.multiplyDispenser->get(),
            this->info->lagrangePolynomialSet[exponentSeriesIndex],
            &this->unboundedFaninOrInfo));
    exponentSeriesIndex++;
  }

  log_assert(this->fronctocolResults.size() == batch->children.size());
  log_assert(this->fronctocolResults.size() == place);
  log_debug("exponentSeriesIndex: %zu", this->exponentSeriesIndex);
  log_debug(
      "input length: %zu Output length: %zu",
      batch->children.size(),
      this->fronctocolResults.size());
  this->invoke(std::move(batch), this->getPeers());
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOr<FF_TYPES, Number_T>::handleReceive(IncomingMessage_T &) {
  log_error("Unexpected handleReceive in PrefixOr");
  this->abort();
}

template<FF_TYPENAMES, typename Number_T>
void PrefixOr<FF_TYPES, Number_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> &) {
  log_debug("Calling handleComplete");
  // case logic
  switch (this->state) {
    case awaitingFirstBatchedUnboundedFaninOr: {
      log_debug("Case awaitingFirstBatchedUnboundedFaninOr");

      this->y_values = this->fronctocolResults;

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

      this->fronctocolResults2.resize(this->inputVals.size());
      for (size_t i = 0; i < this->info->lambda; i++) {
        batch->children.emplace_back(
            new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                this->inputVals[i],
                this->y_values[i / this->info->lambda],
                &this->fronctocolResults2[i],
                this->randomness.multiplyDispenser->get(),
                &this->multiplyInfo));
      }

      for (size_t i = this->info->lambda; i < this->inputVals.size();
           i++) {
        batch->children.emplace_back(
            new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                this->inputVals[i],
                (this->info->s +
                 this->y_values[i / this->info->lambda] -
                 this->y_values[(i / this->info->lambda) - 1]) %
                    this->info->s,
                &this->fronctocolResults2[i],
                this->randomness.multiplyDispenser->get(),
                &this->multiplyInfo));
      }

      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingFirstBatchedMultiply;
    } break;
    case awaitingFirstBatchedMultiply: {
      log_debug("Case awaitingFirstBatchedMultiply");
      for (size_t i = 0; i < this->inputVals.size(); i++) {
        log_debug(
            "result %zu = %s",
            i,
            dec(this->fronctocolResults2[i]).c_str());
      }

      for (size_t j = 0; j < this->info->lambda; j++) {
        Number_T w_value_partial = 0;
        for (size_t i = 0;
             i * this->info->lambda + j < this->inputVals.size();
             i++) {
          w_value_partial = modAdd(
              w_value_partial,
              this->fronctocolResults2[i * this->info->lambda + j],
              this->info->s);
        }
        this->w_values.emplace_back(w_value_partial);
      }
      this->fronctocolResults.resize(this->w_values.size());

      log_debug(
          "w_values.size(): %zu and exponentSeries.size(): %zu",
          this->w_values.size(),
          this->randomness.exponentSeries.size());

      ::std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());
      Number_T current_A = 0;

      for (size_t i = 0; i < this->w_values.size(); i++) {
        current_A = modAdd(current_A, this->w_values[i], this->info->s);

        batch->children.emplace_back(
            new UnboundedFaninOr<FF_TYPES, Number_T>(
                current_A,
                i + 1,
                &this->fronctocolResults[i],
                std::move(this->randomness
                              .exponentSeries[exponentSeriesIndex]),
                this->randomness.multiplyDispenser->get(),
                this->info->lagrangePolynomialSet[exponentSeriesIndex],
                &this->unboundedFaninOrInfo));
        this->exponentSeriesIndex++;
        log_debug(
            "exponentSeriesIndex is %zu", this->exponentSeriesIndex);
      }

      log_debug("About to invoke UFIOs");
      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingSecondBatchedUnboundedFaninOr;

    } break;
    case awaitingSecondBatchedUnboundedFaninOr: {
      log_debug("Case awaitingSecondBatchedUnboundedFaninOr");
      for (size_t i = 0; i < this->fronctocolResults.size(); i++) {
        log_debug(
            "v[%zu]=%s", i, dec(this->fronctocolResults[i]).c_str());
      }

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

      for (size_t i = 0; i < this->info->lambda; i++) {
        batch->children.emplace_back(
            new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                this->y_values[i / this->info->lambda],
                this->fronctocolResults[i % this->info->lambda],
                &this->fronctocolResults2[i],
                this->randomness.multiplyDispenser->get(),
                &this->multiplyInfo));
      }

      for (size_t i = this->info->lambda; i < this->inputVals.size();
           i++) {
        Number_T tmp = modSub(
            this->y_values[i / this->info->lambda],
            this->y_values[(i / this->info->lambda) - 1],
            this->info->s);
        batch->children.emplace_back(
            new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                tmp,
                this->fronctocolResults[i % this->info->lambda],
                &this->fronctocolResults2[i],
                this->randomness.multiplyDispenser->get(),
                &this->multiplyInfo));
      }

      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingSecondBatchedMultiply;
    } break;
    case awaitingSecondBatchedMultiply: {
      log_debug("Case awaitingSecondBatchedMultiply");
      this->orResults.reserve(this->inputVals.size());
      for (size_t i = 0; i < this->info->lambda; i++) {
        this->orResults.emplace_back(this->fronctocolResults2[i]);
      }
      for (size_t i = this->info->lambda; i < this->inputVals.size();
           i++) {
        this->orResults.emplace_back(modAdd(
            this->fronctocolResults2[i],
            this->y_values[(i / this->info->lambda) - 1],
            this->info->s));
      }
      this->complete();
    } break;
    default:
      log_error("PrefixOr state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

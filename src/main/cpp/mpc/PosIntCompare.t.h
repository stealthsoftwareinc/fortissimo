/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<typename Large_T, typename Small_T>
PosIntCompareRandomness<Large_T, Small_T>::PosIntCompareRandomness(
    std::unique_ptr<RandomnessDispenser<
        CompareRandomness<Large_T, Small_T>,
        DoNotGenerateInfo>> compareDispenser, // size 3
    std::unique_ptr<
        RandomnessDispenser<BeaverTriple<Boolean_t>, BooleanBeaverInfo>>
        beaverDispenser) :
    compareDispenser(std::move(compareDispenser)),
    beaverDispenser(std::move(beaverDispenser)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
std::string PosIntCompare<FF_TYPES, Large_T, Small_T>::name() {
  return std::string("PosInt Compare large mod: ") +
      dec(this->compareInfo->p) +
      " small mod: " + dec(this->compareInfo->s);
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
PosIntCompare<FF_TYPES, Large_T, Small_T>::PosIntCompare(
    Large_T const & share_of_x, // mod p
    Large_T const & share_of_y, // mod p
    CompareInfo<Identity_T, Large_T, Small_T> const * const compareInfo,
    PosIntCompareRandomness<Large_T, Small_T> && randomness) :
    share_of_x(share_of_x),
    share_of_y(share_of_y),
    compareInfo(compareInfo),
    randomness(std::move(randomness)) {
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompare<FF_TYPES, Large_T, Small_T>::init() {
  log_debug("Calling init on PosIntCompare");

  std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

  batch->children.emplace_back(new Compare<FF_TYPES, Large_T, Small_T>(
      this->share_of_x,
      this->share_of_y,
      compareInfo,
      std::move(randomness.compareDispenser->get())));

  batch->children.emplace_back(new Compare<FF_TYPES, Large_T, Small_T>(
      this->share_of_x,
      0,
      compareInfo,
      std::move(randomness.compareDispenser->get())));

  batch->children.emplace_back(new Compare<FF_TYPES, Large_T, Small_T>(
      this->share_of_y,
      0,
      compareInfo,
      std::move(randomness.compareDispenser->get())));

  this->invoke(std::move(batch), this->getPeers());
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompare<FF_TYPES, Large_T, Small_T>::handlePromise(
    ff::Fronctocol<FF_TYPES> &) {
  log_error("Unexpected handlePromise in PosIntCompare");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompare<FF_TYPES, Large_T, Small_T>::handleReceive(
    IncomingMessage_T &) {
  log_error("Unexpected handleReceive in PosIntCompare");
  this->abort();
}

template<FF_TYPENAMES, typename Large_T, typename Small_T>
void PosIntCompare<FF_TYPES, Large_T, Small_T>::handleComplete(
    ff::Fronctocol<FF_TYPES> & f) {
  log_debug("Calling handleComplete in PosIntCompare");
  // case logic
  switch (this->state) {
    case awaitingBatchedCompare: {
      log_debug("Case awaitingBatchedCompare in PosIntCompare");

      Batch<FF_TYPES> & old_batch = static_cast<Batch<FF_TYPES> &>(f);

      Boolean_t x_compare_y =
          static_cast<Compare<FF_TYPES, Large_T, Small_T> &>(
              *old_batch.children[0])
              .outputShare;
      Boolean_t x_compare_zero =
          static_cast<Compare<FF_TYPES, Large_T, Small_T> &>(
              *old_batch.children[1])
              .outputShare;
      Boolean_t y_compare_zero =
          static_cast<Compare<FF_TYPES, Large_T, Small_T> &>(
              *old_batch.children[2])
              .outputShare;

      std::unique_ptr<Batch<FF_TYPES>> batch(new Batch<FF_TYPES>());

      Boolean_t first_mult_second_input =
          x_compare_zero ^ y_compare_zero;

      if (this->getSelf() == *this->compareInfo->revealer) {
        first_mult_second_input = first_mult_second_input ^ 1;
      }
      first_mult_second_input %= 2;
      first_mult_second_input =
          static_cast<Boolean_t>(first_mult_second_input * 3);

      batch->children.emplace_back(
          new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
              x_compare_y,
              first_mult_second_input,
              &this->firstMultOutput,
              std::move(randomness.beaverDispenser->get()),
              new MultiplyInfo<Identity_T, BooleanBeaverInfo>(
                  this->compareInfo->revealer, BooleanBeaverInfo())));

      batch->children.emplace_back(
          new Multiply<FF_TYPES, Boolean_t, BooleanBeaverInfo>(
              y_compare_zero,
              x_compare_zero ^ y_compare_zero,
              &this->secondMultOutput,
              std::move(randomness.beaverDispenser->get()),
              new MultiplyInfo<Identity_T, BooleanBeaverInfo>(
                  this->compareInfo->revealer, BooleanBeaverInfo())));

      this->invoke(std::move(batch), this->getPeers());
      this->state = awaitingBatchedXORMultiply;

    } break;
    case awaitingBatchedXORMultiply: {
      log_debug("Case awaitingBatchedXORMultiply");
      this->outputShare =
          this->firstMultOutput ^ this->secondMultOutput;
      this->complete();
    } break;
    default:
      log_error("Compare state machine in unexpected state");
  }
}

} // namespace mpc
} // namespace ff

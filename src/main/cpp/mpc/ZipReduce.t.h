/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
std::string ZipReduce<FF_TYPES, Number_T>::name() {
  return std::string("Zip Reduce");
}

template<FF_TYPENAMES, typename Number_T>
ZipReduce<FF_TYPES, Number_T>::ZipReduce(
    ObservationList<Number_T> && zippedAdjacentPairs,
    ZipReduceFactory<FF_TYPES, Number_T> & fronctocolFactory) :
    zippedAdjacentPairs(zippedAdjacentPairs),
    fronctocolFactory(fronctocolFactory) {
}

template<FF_TYPENAMES, typename Number_T>
void ZipReduce<FF_TYPES, Number_T>::onInit() {
  log_debug("Calling init on ZipReduce");

  this->children.reserve(this->zippedAdjacentPairs.elements.size() / 2);
  log_debug(
      "children.size %zu and zippedAdjacentPairs.elements.size()/2 %zu",
      this->children.size(),
      this->zippedAdjacentPairs.elements.size() / 2);

  for (size_t i = 0; i < this->zippedAdjacentPairs.elements.size() / 2;
       i++) {
    log_debug("i:%zu", i);
    log_debug(
        "val %u",
        this->zippedAdjacentPairs.elements[2 * i]
            .arithmeticPayloadCols[0]);
    std::unique_ptr<Observation<Number_T>> o3(
        new Observation<Number_T>());
    std::unique_ptr<Observation<Number_T>> o4(
        new Observation<Number_T>());
    o3->arithmeticPayloadCols =
        std::move(this->zippedAdjacentPairs.elements[2 * i]
                      .arithmeticPayloadCols);
    o4->arithmeticPayloadCols =
        std::move(this->zippedAdjacentPairs.elements[2 * i + 1]
                      .arithmeticPayloadCols);
    std::unique_ptr<ZipReduceFronctocol<FF_TYPES, Number_T>> fronctocol(
        fronctocolFactory.generate(std::move(o3), std::move(o4)));
    log_debug("HI");
    this->children.emplace_back(std::move(fronctocol));
  }
}

template<FF_TYPENAMES, typename Number_T>
void ZipReduce<FF_TYPES, Number_T>::onComplete() {
  log_debug("Calling handleComplete");

  for (size_t i = 0; i < this->zippedAdjacentPairs.elements.size() / 2;
       i++) {
    outputList.elements.emplace_back(std::move(
        static_cast<ZipReduceFronctocol<FF_TYPES, Number_T> &>(
            *this->children[i])
            .output));
  }
  outputList.numKeyCols = outputList.elements[0].keyCols.size();
  outputList.numArithmeticPayloadCols =
      outputList.elements[0].arithmeticPayloadCols.size();
  outputList.numXORPayloadCols =
      outputList.elements[0].XORPayloadCols.size();

  log_debug("here");
}

} // namespace mpc
} // namespace ff

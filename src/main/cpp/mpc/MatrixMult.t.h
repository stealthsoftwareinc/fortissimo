/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
::std::string MatrixMult<FF_TYPES, Number_T>::name() {
  return std::string("MatrixMult A: ") +
      std::to_string(this->A->getNumRows()) + "x" +
      std::to_string(this->A->getNumColumns()) +
      " B: " + std::to_string(this->B->getNumRows()) + "x" +
      std::to_string(this->B->getNumColumns()) +
      " mod: " + dec(this->multInfo.info.modulus);
}

template<FF_TYPENAMES, typename Number_T>
void MatrixMult<FF_TYPES, Number_T>::onInit() {
  log_assert(this->A->getNumColumns() > 0);
  log_assert(this->B->getNumColumns() > 0);
  log_assert(this->A->getNumRows() > 0);
  log_assert(this->B->getNumRows() > 0);

  log_assert(this->A->getNumColumns() == this->B->getNumRows());
  log_assert(this->C->getNumRows() == this->A->getNumRows());
  log_assert(this->C->getNumColumns() == this->B->getNumColumns());
  log_assert(
      this->multInfo.info.modulus == this->beaverTriples->info.modulus);

  size_t num_prods = this->A->getNumRows() * this->B->getNumColumns() *
      this->A->getNumColumns();
  log_assert(
      this->beaverTriples->size() >= num_prods,
      "num beavers %lu",
      this->beaverTriples->size());

  this->products.resize(num_prods);
  this->children.reserve(num_prods);

  size_t prod_place = 0;
  for (size_t i = 0; i < this->A->getNumRows(); i++) {
    for (size_t j = 0; j < this->B->getNumColumns(); j++) {
      for (size_t k = 0; k < this->A->getNumColumns(); k++) {
        this->children.emplace_back(
            new Multiply<FF_TYPES, Number_T, BeaverInfo<Number_T>>(
                this->A->at(i, k),
                this->B->at(k, j),
                &this->products[prod_place++],
                this->beaverTriples->get(),
                &this->multInfo));
      }
    }
  }
}

template<FF_TYPENAMES, typename Number_T>
void MatrixMult<FF_TYPES, Number_T>::onComplete() {
  size_t prod_place = 0;
  for (size_t i = 0; i < this->A->getNumRows(); i++) {
    for (size_t j = 0; j < this->B->getNumColumns(); j++) {
      Number_T sum = 0;
      for (size_t k = 0; k < this->A->getNumColumns(); k++) {
        sum = modAdd(
            sum,
            this->products[prod_place++],
            this->multInfo.info.modulus);
      }
      this->C->at(i, j) = sum;
    }
  }
}

} // namespace mpc
} // namespace ff

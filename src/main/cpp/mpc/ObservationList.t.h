/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

// TODO(KIM): see if we can use std::move here.
template<typename Number_T>
void ObservationList<Number_T>::swap(size_t i, size_t j) {
  Observation<Number_T> temp = this->elements[j];
  this->elements[j] = this->elements[i];
  this->elements[i] = temp;
}

} // namespace mpc
} // namespace ff

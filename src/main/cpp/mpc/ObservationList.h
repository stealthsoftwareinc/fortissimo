

#ifndef FF_MPC_OBSERVATION_LIST_H_
#define FF_MPC_OBSERVATION_LIST_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cmath>
#include <cstdint>
#include <list>
#include <vector>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <mpc/templates.h>

namespace ff {
namespace mpc {

/*
 * i.e. a single element is made up of a list of keys
 * and payloads. We sort by keys, lexicographically.
 */
template<typename Number_T>
struct Observation {
  std::vector<Number_T> keyCols;
  std::vector<Number_T> arithmeticPayloadCols;
  std::vector<Boolean_t> XORPayloadCols;
};

template<typename Number_T>
struct ObservationList {
  std::vector<Observation<Number_T>> elements;
  size_t numKeyCols;
  size_t numArithmeticPayloadCols;
  size_t numXORPayloadCols;

  void swap(size_t i, size_t j);
};

} // namespace mpc
} // namespace ff

#include <mpc/ObservationList.t.h>

#endif //FF_MPC_OBSERVATION_LIST_H_

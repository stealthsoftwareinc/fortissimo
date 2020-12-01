/**
 * Copyright Stealth Software Technologies, Inc.
 */

namespace ff {
namespace mpc {

template<typename Number_T>
bool isPrime(Number_T value) {
  if (value % Number_T(2) == Number_T(0) && value != Number_T(2)) {
    return false;
  } else if (
      value % Number_T(3) == Number_T(0) && value != Number_T(3)) {
    return false;
  }
  for (Number_T i = Number_T(6);
       (i - Number_T(1)) * (i - Number_T(1)) <= value;
       i += Number_T(6)) {
    if (value % (i + Number_T(1)) == Number_T(0)) {
      return false;
    }
    if (value % (i - Number_T(1)) == Number_T(0)) {
      return false;
    }
  }
  return true;
}

template<typename Number_T>
Number_T nextPrime(Number_T value) {
  if (value % Number_T(2) == Number_T(0)) {
    return (nextPrime<Number_T>(value + Number_T(1)));
  }
  while (!isPrime<Number_T>(value)) {
    log_debug("Testing value %s", dec(value).c_str());
    value += Number_T(2);
  }
  return value;
}

} // namespace mpc
} // namespace ff

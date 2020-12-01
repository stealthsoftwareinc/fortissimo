/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_ABSTRACT_FACTORY_H_
#define FF_MPC_ABSTRACT_FACTORY_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <memory>

/* 3rd Party Headers */

/* Fortissimo Headers */
#include <ff/Fronctocol.h>
#include <mpc/ObservationList.h>
#include <mpc/templates.h>
/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
class ZipReduceFronctocol : public Fronctocol<FF_TYPES> {
public:
  Observation<Number_T> output;
};

template<FF_TYPENAMES, typename Number_T>
class ZipReduceFactory {
public:
  ZipReduceFactory() {
  }
  /*
   * Generates fronctocols of type ZipReduceFronctocol that act on adjacent observations from an
   * observtion list
   */

  virtual std::unique_ptr<ZipReduceFronctocol<FF_TYPES, Number_T>>
  generate(
      std::unique_ptr<Observation<Number_T>> o1,
      std::unique_ptr<Observation<Number_T>> o2) = 0;
};

} // namespace mpc
} // namespace ff

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_ABSTRACT_FACTORY_H_

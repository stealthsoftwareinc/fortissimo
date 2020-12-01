/**
 * Copyright Stealth Software Technologies, Inc.
 */

#ifndef FF_MPC_ZIP_REDUCE_H_
#define FF_MPC_ZIP_REDUCE_H_

/* C and POSIX Headers */

/* C++ Headers */
#include <cstdint>
#include <memory>
#include <utility>
/* 3rd Party Headers */

/* Fortissimo Headers */
#include <mpc/AbstractZipReduceFactory.h>
#include <mpc/Batch.h>
#include <mpc/ObservationList.h>
#include <mpc/templates.h>
/* logging configuration */
#include <ff/logging.h>

namespace ff {
namespace mpc {

template<FF_TYPENAMES, typename Number_T>
class ZipReduce : public Batch<FF_TYPES> {
public:
  std::string name() override;
  ObservationList<Number_T> outputList;

  /*
   * Takes in a sorted ObservationList and processes it
   * Assume for now that numKeyCols = 1
   * TO-DO: re-write SISOSort to force numKeyCols = 1.
   */

  ZipReduce(
      ObservationList<Number_T> && zippedAdjacentPairs,
      ZipReduceFactory<FF_TYPES, Number_T> & fronctocolFactory);

  void onInit() override;
  void onComplete() override;

private:
  ObservationList<Number_T>
      zippedAdjacentPairs; // ready to be fed into ZipReduce factory

  ZipReduceFactory<FF_TYPES, Number_T> & fronctocolFactory;
};

} // namespace mpc
} // namespace ff

#include <mpc/ZipReduce.t.h>

#define LOG_UNCLUDE
#include <ff/logging.h>

#endif // FF_MPC_ZIP_REDUCE_H_

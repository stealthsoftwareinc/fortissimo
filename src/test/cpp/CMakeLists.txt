project(shared_test)

include_directories(${GTEST_INCLUDE_DIRS})

include_directories(
  ../../main/cpp
  ../../../lib/include
  ./
)

link_directories(
  ../../../lib/lib
)

add_executable(fortissimo_test
  mock.cpp
  mock.h

  ff/logging.test.cpp
  ff/Message.test.cpp
  ff/ValueSum.test.cpp
  ff/DiffieHellman.test.cpp
  ff/Util.test.cpp
  ff/abort.test.cpp
  ff/VectorPeerSet.test.cpp

  mpc/Waksman.test.cpp

  mpc/Randomness.test.cpp
  mpc/Batch.test.cpp
  mpc/Multiply.test.cpp
  mpc/UnboundedFaninOr.test.cpp

  mpc/PrefixOr.test.cpp
  mpc/TypeCastBit.test.cpp
  mpc/lagrange.test.cpp
  mpc/BitwiseCompare.test.cpp
  mpc/Compare.test.cpp
  mpc/PosIntCompare.test.cpp

  # mpc/BatchCompare.test.cpp

  mpc/Quicksort.test.cpp
  mpc/SISOSort.test.cpp
  mpc/Divide.test.cpp
  mpc/ModConvUp.test.cpp

  mpc/MatrixMult.test.cpp
  mpc/MatrixInverse.test.cpp
  # mpc/PrefixOr2parties.test.cpp
  # mpc/Compare2parties.test.cpp
  # mpc/BitwiseCompare2parties.test.cpp

  mpc/ZipAdjacent.test.cpp
  mpc/ZipReduce.test.cpp
)

target_link_libraries(fortissimo_test
  gtest
  gtest_main
  fortissimo
)

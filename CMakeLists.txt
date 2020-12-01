cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(fortissimo)

set(CMAKE_CXX_FLAGS_RELEASE " -DNDEBUG -O2 -Werror ")
set(CMAKE_CXX_FLAGS_CI " -Werror ")
set(CMAKE_CXX_FLAGS_METRICS " -DLOG_ENABLE_TIMER -DLOG_NO_COLOR -DNDEBUG -O2 ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
  -std=c++11 \
  -Wall \
  -Wextra \
  -Wconversion \
  -Wcast-align \
  ")

if(NOT TARGET gtest)
  add_subdirectory(lib/google-test-dir)
endif()

cmake_policy(SET CMP0015 NEW)

add_subdirectory(src/main/cpp)
add_subdirectory(src/test/cpp)
#add_subdirectory(example/src/main/cpp)

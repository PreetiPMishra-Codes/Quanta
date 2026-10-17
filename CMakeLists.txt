cmake_minimum_required(VERSION 3.16)
project(quanta VERSION 0.1 LANGUAGES CXX)
cmake_policy(SET CMP0135 NEW)

option(QUANTA_BUILD_TESTS "Build the unit tests (downloads GoogleTest)" ON)

# --- Language settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Default to Release so benchmarks aren't accidentally slow ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Core library ---
add_library(quanta_core STATIC
  src/orders/order.cpp
  src/traders/trader.cpp
  src/exchange/order_book.cpp
  src/exchange/matching_engine.cpp
  src/exchange/exchange.cpp
  src/risk/risk_manager.cpp
  src/strategies/strategies.cpp
  src/simulation/simulator.cpp
  src/benchmark/latency_recorder.cpp
)
target_include_directories(quanta_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(quanta_core PUBLIC -Wall -Wextra -Wpedantic)

# --- Programs ---
add_executable(quanta src/main.cpp)
target_link_libraries(quanta PRIVATE quanta_core)

# Benchmarks: build these from a Release configuration
add_executable(bench_baseline benchmarks/bench_baseline.cpp)
target_link_libraries(bench_baseline PRIVATE quanta_core)

add_executable(bench_profile benchmarks/bench_profile.cpp)
target_link_libraries(bench_profile PRIVATE quanta_core)

add_executable(bench_book benchmarks/bench_book.cpp)
target_link_libraries(bench_book PRIVATE quanta_core)

# --- Tests ---
if(QUANTA_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()
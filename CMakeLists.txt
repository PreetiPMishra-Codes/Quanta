cmake_minimum_required(VERSION 3.16)
project(quanta VERSION 0.1 LANGUAGES CXX)

# --- Language settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Default to Release so benchmarks aren't accidentally slow ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Core library (header-only for now) ---
add_library(quanta_core INTERFACE)
target_include_directories(quanta_core INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(quanta_core INTERFACE -Wall -Wextra -Wpedantic)

# --- Download GoogleTest automatically ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
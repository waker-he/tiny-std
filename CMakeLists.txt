cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# Set experimental flag to enable `import std` support from CMake.
# This must be enabled before C++ language support.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
  # This specific value changes as experimental support evolves. See
  # `Help/dev/experimental.rst` in the CMake source corresponding to
  # your CMake build for the exact value to use.
  "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

# Set compiler to clang++ and Use libc++ as the standard library
# must be put before project()
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fexperimental-library")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
add_compile_options(-Wall -Wextra)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ needs to be enabled.
project(tiny-std LANGUAGES CXX)

# Tell CMake that we explicitly want `import std`. This will initialize the
# property on all targets declared after this to 1
set(CMAKE_CXX_MODULE_STD 1)


include(cmake/clang_format.cmake)
include(cmake/CPM.cmake)

# Dependencies Management through CPM
CPMAddPackage("gh:boost-ext/ut@2.1.0")
CPMAddPackage("gh:martinus/nanobench@4.3.11")

# Boost is not managed by CPM, might change in future
find_package(Boost 1.81 REQUIRED)

add_subdirectory(module)
add_subdirectory(test)

# Conditionally include subdirectories based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode. Including benchmark directory.")
    add_subdirectory(benchmark)
endif()

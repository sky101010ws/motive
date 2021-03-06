# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
if(fpl_ios)
  cmake_minimum_required(VERSION 3.3.1)
else()
  cmake_minimum_required(VERSION 2.8.12)
endif()

project(motive)

# Compile the game with the debug flag
#set(motive_DEBUG ON)

set(MOTIVE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MOTIVE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MOTIVE_BINARY_DIR})

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_CURRENT_BINARY_DIR}/obj)

# Directory which contains the source for 3rd party libraries.
if(NOT DEFINED third_party_root)
  get_filename_component(
    third_party_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../../external"
    ABSOLUTE)
endif()

# Directory which contains source for FPL libraries.
if(NOT DEFINED fpl_root)
  get_filename_component(
      fpl_root "${CMAKE_CURRENT_SOURCE_DIR}/../../libs" ABSOLUTE)
endif()

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
if(NOT DEFINED dependencies_root)
  set(dependencies_root "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
  if(EXISTS "${dependencies_root}")
    set(third_party_root "${dependencies_root}")
    set(fpl_root "${dependencies_root}")
  endif()
endif()

# Configurable locations of dependencies of this project.
set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")

# Option to enable / disable the test build.
option(motive_build_tests "Build tests for this project." ON)
option(motive_build_samples "Build samples for this project." ON)

# Option to only build flatc
option(motive_only_flatc "Only build FlatBuffers compiler." OFF)

# Option to only build debug functions. Handy, but take up code space.
option(motive_debug_functions "Only build FlatBuffers compiler." ON)

# Option to instrument the code with timers. Useful for benchmarking.
option(motive_enable_benchmarks "Measure performance of key subsystems." OFF)

# Include MathFu in this project with test and benchmark builds disabled.
set(mathfu_build_benchmarks OFF CACHE BOOL "")
set(mathfu_build_tests OFF CACHE BOOL "")
add_subdirectory(${dependencies_mathfu_dir} ${tmp_dir}/mathfu)

# Include FlatBuffers in this project.
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
if(NOT TARGET flatc)
  add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)
endif()

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include)
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.fbs)

# Generate rules to build the set of output files from the set of input
# schema files.
build_flatbuffers("${FLATBUFFERS_SCHEMAS}"
                  ""
                  motive_generated_includes
                  ""
                  ${MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR}
                  ""
                  "")

# Motive source files.
set(motive_SRCS
    include/motive/common.h
    include/motive/engine.h
    include/motive/motivator.h
    include/motive/init.h
    include/motive/io/flatbuffers.h
    include/motive/math/angle.h
    include/motive/math/bulk_spline_evaluator.h
    include/motive/math/compact_spline.h
    include/motive/math/curve.h
    include/motive/math/dual_cubic.h
    include/motive/math/range.h
    include/motive/processor.h
    include/motive/target.h
    include/motive/util.h
    include/motive/version.h
    src/motive/anim.cpp
    src/motive/anim_table.cpp
    src/motive/engine.cpp
    src/motive/motivator.cpp
    src/motive/init.cpp
    src/motive/io/flatbuffers.cpp
    src/motive/math/bulk_spline_evaluator.cpp
    src/motive/math/compact_spline.cpp
    src/motive/math/curve.cpp
    src/motive/math/dual_cubic.cpp
    src/motive/processor.cpp
    src/motive/processor/matrix_processor.cpp
    src/motive/processor/overshoot_processor.cpp
    src/motive/processor/rig_processor.cpp
    src/motive/processor/spline_processor.cpp
    src/motive/util/benchmark.cpp
    src/motive/util/optimizations.cpp
    src/motive/version.cpp)

# Includes for this project.
include_directories(src include)
if(WIN32)
  include_directories(external/include/windows)
endif()
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${dependencies_fplutil_dir}/libfplutil/include)
include_directories(${dependencies_mathfu_dir}/benchmarks)
include_directories(${dependencies_mathfu_dir}/include)

# Detect clang
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR
   CMAKE_CXX_COMPILER_ID MATCHES ".*clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4065 /wd4355")
  add_definitions(-DNOMINMAX)
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra -Wno-unused-function")
  # -Wno-unused-function is because of webp encode.h
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

if(motive_DEBUG)
  # if we want to define this, it needs to be only in debug builds
  add_definitions(-D_DEBUG)
  add_definitions(-DMOTIVE_VERIFY_INTERNAL_STATE)
endif()

if(motive_debug_functions)
  add_definitions(-DFPL_CURVE_GRAPH_FUNCTIONS)
endif()

if(motive_enable_benchmarks)
  add_definitions(-DBENCHMARK_MOTIVE)
endif()

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

# Executable target.
add_library(motive ${motive_SRCS})

# Set iOS specific attributes
mathfu_set_ios_attributes(motive)

# Dependencies for the executable target.
add_dependencies(motive motive_generated_includes)

# Additional flags for the target.
mathfu_configure_flags(motive)

# Tests.
if(motive_build_tests)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/tests)
endif()

# Samples.
if(motive_build_samples)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/samples)
endif()

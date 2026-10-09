# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# The native PC port (README.md). Configure from this directory:
#
#   cmake --preset release && cmake --build --preset release && ctest --preset release
#
# The game programs (openrac-rac1, ...) are built from a decompilation's C by
# tools/hostgen; they need Clang (to read that C) and Python 3. The programs
# that open a window need SDL3, found on the system or fetched by CMake.
cmake_minimum_required(VERSION 3.24)
project(openrac_port LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # the guest contract uses GNU statement expressions and cleanup
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "The port is 64-bit only: game memory is a 4 GB reservation.")
endif()
if(MSVC AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Build the port with Clang (clang-cl on Windows): the guest contract "
                      "needs __int128 and GNU C extensions that MSVC's C compiler lacks.")
endif()

option(OPENRAC_PORT_WINDOW "Build the programs that open a window (needs SDL3)" ON)
option(OPENRAC_PORT_GAMES "Build the games from their decompilations (needs Clang and Python 3)" ON)
set(OPENRAC_RAC1_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../games/rac1/pal" CACHE PATH
    "The Ratchet & Clank (PAL) decompilation to build: OpenRAC's, or your own checkout of it")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings)

enable_testing()

# ---- common: the log and small helpers, no dependencies ----
add_library(openrac_common STATIC
  common/log.cpp
)
target_include_directories(openrac_common PUBLIC .)
openrac_warnings(openrac_common)

# ---- runtime: game memory, the guest contract, the executable loader ----
add_library(openrac_runtime STATIC
  runtime/crash.cpp
  runtime/elf.cpp
  runtime/guest.cpp
  runtime/memory.cpp
)
target_include_directories(openrac_runtime PUBLIC runtime/include)
target_link_libraries(openrac_runtime PUBLIC openrac_common)
openrac_warnings(openrac_runtime)

add_executable(runtime_test tests/runtime_test.cpp tests/guest_c.c)
target_link_libraries(runtime_test PRIVATE openrac_runtime)
openrac_warnings(runtime_test)
add_test(NAME runtime COMMAND runtime_test)

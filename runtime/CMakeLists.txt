# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
cmake_minimum_required(VERSION 3.24)
project(openrac_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(OPENRAC_RUNTIME_WINDOW "Build the programs that open a window (needs SDL3)" ON)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)

# The hardware contract the games draw through: no dependencies.
add_library(openrac_ps2 STATIC
  src/ps2/dma.cpp
  src/ps2/gif.cpp
  src/ps2/gs.cpp
  src/ps2/gs_memory.cpp
  src/ps2/vif.cpp
)
target_include_directories(openrac_ps2 PUBLIC src/ps2 src)

if(OPENRAC_RUNTIME_WINDOW)
  find_package(SDL3 REQUIRED CONFIG)
  add_library(openrac_host STATIC src/host/window.cpp)
  target_include_directories(openrac_host PUBLIC src)
  target_link_libraries(openrac_host PUBLIC openrac_ps2 SDL3::SDL3)

  add_executable(openrac-gsdemo src/app/gsdemo.cpp)
  target_link_libraries(openrac-gsdemo PRIVATE openrac_host)
else()
  add_executable(openrac-gsdemo src/app/gsdemo.cpp)
  target_compile_definitions(openrac-gsdemo PRIVATE OPENRAC_NO_WINDOW)
  target_link_libraries(openrac-gsdemo PRIVATE openrac_ps2)
endif()

enable_testing()
add_executable(openrac-ps2-tests tests/test_ps2.cpp)
target_link_libraries(openrac-ps2-tests PRIVATE openrac_ps2)
add_test(NAME ps2 COMMAND openrac-ps2-tests)

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
# The console multiplies and then adds; a fused multiply-add rounds differently.
add_compile_options(-ffp-contract=off)

# The hardware contract the games draw through: no dependencies.
add_library(openrac_ps2 STATIC
  src/ps2/dma.cpp
  src/ps2/ee.cpp
  src/ps2/ee_mmi.cpp
  src/ps2/gif.cpp
  src/ps2/gs.cpp
  src/ps2/gs_memory.cpp
  src/ps2/vif.cpp
  src/ps2/vu.cpp
)
target_include_directories(openrac_ps2 PUBLIC src/ps2 src)

# The machine around the hardware model: kernel services, the disc, the
# library functions answered by the runtime.
add_library(openrac_sys STATIC
  src/sys/disc.cpp
  src/sys/machine.cpp
  src/sys/services.cpp
)
target_include_directories(openrac_sys PUBLIC src)
target_link_libraries(openrac_sys PUBLIC openrac_ps2)

if(OPENRAC_RUNTIME_WINDOW)
  find_package(SDL3 REQUIRED CONFIG)
  add_library(openrac_host STATIC src/host/window.cpp)
  target_include_directories(openrac_host PUBLIC src)
  target_link_libraries(openrac_host PUBLIC openrac_ps2 SDL3::SDL3)

  add_executable(openrac-gsdemo src/app/gsdemo.cpp)
  target_link_libraries(openrac-gsdemo PRIVATE openrac_host)
  add_executable(openrac-boot src/app/boot.cpp)
  target_link_libraries(openrac-boot PRIVATE openrac_sys openrac_host)
else()
  add_executable(openrac-boot src/app/boot.cpp)
  target_compile_definitions(openrac-boot PRIVATE OPENRAC_NO_WINDOW)
  target_link_libraries(openrac-boot PRIVATE openrac_sys)
  add_executable(openrac-gsdemo src/app/gsdemo.cpp)
  target_compile_definitions(openrac-gsdemo PRIVATE OPENRAC_NO_WINDOW)
  target_link_libraries(openrac-gsdemo PRIVATE openrac_ps2)
endif()

# Tools that read a file from your own disc and report on it.
add_executable(openrac-vuscan src/app/vuscan.cpp)
target_link_libraries(openrac-vuscan PRIVATE openrac_ps2)

enable_testing()
add_executable(openrac-ps2-tests tests/test_ps2.cpp)
target_link_libraries(openrac-ps2-tests PRIVATE openrac_ps2)
add_test(NAME ps2 COMMAND openrac-ps2-tests)
add_executable(openrac-vu-tests tests/test_vu.cpp)
target_link_libraries(openrac-vu-tests PRIVATE openrac_ps2)
add_test(NAME vu COMMAND openrac-vu-tests)

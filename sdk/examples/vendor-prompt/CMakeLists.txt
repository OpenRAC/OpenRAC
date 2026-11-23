# SPDX-License-Identifier: MIT
# The vendor-prompt example (sdk/README.md):
#   cmake -S sdk/examples/vendor-prompt -B build/vendor-prompt -G Ninja
#   cmake --build build/vendor-prompt
cmake_minimum_required(VERSION 3.20)
project(vendor_prompt LANGUAGES C CXX)

# Inside OpenRAC the SDK is two folders up; elsewhere -DOpenRACMod_DIR=<OpenRAC>/sdk/cmake.
if(NOT OpenRACMod_DIR)
  set(OpenRACMod_DIR "${CMAKE_CURRENT_LIST_DIR}/../../cmake")
endif()
find_package(OpenRACMod 1.31 REQUIRED)

openrac_add_mod(vendor_prompt MANIFEST openrac-mod.json SOURCES src/vendor_prompt.cpp)

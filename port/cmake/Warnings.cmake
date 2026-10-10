# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# Warnings for the port's own code. Generated game code gets its own, looser
# set (games/*.cmake): it is the decompilation's C, read as it is.

function(openrac_warnings target)
  if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wshadow
      $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic -Wconversion -Wno-sign-conversion>)
  endif()
  # The console multiplies and then adds; a fused multiply-add rounds differently.
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE -ffp-contract=off)
  endif()
endfunction()

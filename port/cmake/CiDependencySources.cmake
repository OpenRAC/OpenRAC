# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors

# CI sets the flag only after an exact hit for the dependency declarations.
# Use each restored checkout without restoring FetchContent's generated build state.
file(GLOB openrac_restored_sources LIST_DIRECTORIES true "${CMAKE_BINARY_DIR}/_deps/*-src")
set(openrac_ci_source_overrides "")
foreach(source IN LISTS openrac_restored_sources)
  if(IS_DIRECTORY "${source}")
    get_filename_component(name "${source}" NAME)
    string(REGEX REPLACE "-src$" "" name "${name}")
    string(TOUPPER "${name}" name)
    if(NOT FETCHCONTENT_SOURCE_DIR_${name})
      set(FETCHCONTENT_SOURCE_DIR_${name} "${source}")
      list(APPEND openrac_ci_source_overrides FETCHCONTENT_SOURCE_DIR_${name})
    endif()
  endif()
endforeach()

# FetchContent can persist these values as PATH entries. Keep them out of the
# configuration cache so a later source-cache miss can fetch normally.
function(openrac_clear_ci_source_overrides)
  foreach(override IN LISTS openrac_ci_source_overrides)
    unset(${override} CACHE)
  endforeach()
endfunction()
cmake_language(DEFER CALL openrac_clear_ci_source_overrides)

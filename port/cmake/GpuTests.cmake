# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors

include_guard(GLOBAL)

# Private, initially empty caches let fresh scenes reuse compiled shaders without
# reading a player's cache. ARGN names the matrix shards that may run together,
# after the other GPU tests finish; other GPU processes run one at a time.
function(openrac_configure_gpu_tests)
  get_property(tests DIRECTORY PROPERTY TESTS)
  set(reset "")
  set(serial "")
  set(parallel "${ARGN}")
  foreach(test IN LISTS tests)
    get_property(environment TEST "${test}" PROPERTY ENVIRONMENT)
    if(NOT "OPENRAC_GL_API=vulkan" IN_LIST environment)
      continue()
    endif()
    set_property(TEST "${test}" APPEND PROPERTY LABELS gpu)
    if(NOT test IN_LIST parallel)
      set_property(TEST "${test}" APPEND PROPERTY RESOURCE_LOCK openrac_gpu)
      list(APPEND serial "${test}")
    else()
      set_property(TEST "${test}" APPEND PROPERTY ENVIRONMENT "LP_NUM_THREADS=2")
    endif()
    # Cache-behaviour tests may deliberately select their own cache or disable it.
    if(environment MATCHES "(^|;)OPENRAC_VULKAN_PIPELINE_CACHE=")
      continue()
    endif()
    set(cache "${CMAKE_CURRENT_BINARY_DIR}/test-cache/vulkan/${test}.bin")
    set_property(TEST "${test}" APPEND PROPERTY ENVIRONMENT
      "OPENRAC_VULKAN_PIPELINE_CACHE=${cache}")
    set_property(TEST "${test}" APPEND PROPERTY FIXTURES_REQUIRED vulkan_test_cache)
    string(APPEND reset "file(REMOVE \"${cache}\" \"${cache}.manifest\" \"${cache}.spirv\")\n")
  endforeach()
  foreach(test IN LISTS parallel)
    set_property(TEST "${test}" APPEND PROPERTY DEPENDS "${serial}")
  endforeach()
  if(reset)
    set(script "${CMAKE_CURRENT_BINARY_DIR}/reset_vulkan_test_caches.cmake")
    file(CONFIGURE OUTPUT "${script}" CONTENT "${reset}" @ONLY)
    add_test(NAME vulkan_test_cache_reset COMMAND "${CMAKE_COMMAND}" -P "${script}")
    set_tests_properties(vulkan_test_cache_reset PROPERTIES FIXTURES_SETUP vulkan_test_cache)
  endif()
endfunction()

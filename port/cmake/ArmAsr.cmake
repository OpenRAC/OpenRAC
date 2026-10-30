# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
#
# Arm Accuracy Super Resolution, the temporal upscaler for tile-based GPUs, beside FSR 3.1 in the
# enhanced temporal module (port/enhanced/asr/README.md). Included only by Fsr3.cmake.
include_guard(GLOBAL)
option(OPENRAC_ARM_ASR "Build Arm ASR temporal upscaling beside FSR 3.1 (enhanced Vulkan)" ON)
if(NOT OPENRAC_ARM_ASR)
  add_library(openrac_asr STATIC enhanced/asr/asr_stub.cpp)
  target_link_libraries(openrac_asr PUBLIC openrac_renderer_vulkan)
  openrac_warnings(openrac_asr)
  return()
endif()

# Release 25.06 (MIT), pinned by commit; its shaders come prebuilt as SPIR-V in its headers.
FetchContent_Declare(arm_asr
  GIT_REPOSITORY https://github.com/arm/accuracy-super-resolution-generic-library.git
  GIT_TAG 0501f490bd9946a2e1806b5363d7ab8a9a6a5e0a
  SOURCE_SUBDIR openrac-download-only)
FetchContent_MakeAvailable(arm_asr)
set(asr "${arm_asr_SOURCE_DIR}")

# Two corrections to the memory-type search of its Vulkan backend, made on a copy in the build
# tree: a type must have every requested property, not any one of them (a host-visible request
# took device-only memory and failed to map), and when only device-local memory is asked for, a
# host-visible type is a fallback instead of being skipped (unified memory has no other).
set(asr_vk "${CMAKE_CURRENT_BINARY_DIR}/generated/asr/ffxm_vk.cpp")
file(READ "${asr}/src/backends/vk/ffxm_vk.cpp" asr_source)
set(asr_any "(memProperties.memoryTypes[i].propertyFlags & requestedProperties))")
set(asr_all "(memProperties.memoryTypes[i].propertyFlags & requestedProperties) == requestedProperties)")
set(asr_skip "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))\n                continue;")
set(asr_fallback "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {\n                if (bestCandidate == UINT32_MAX) {\n                    bestCandidate = i;\n                    outProperties = memProperties.memoryTypes[i].propertyFlags;\n                }\n                continue;\n            }")
foreach(pattern IN ITEMS asr_any asr_skip)
  string(FIND "${asr_source}" "${${pattern}}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "Arm ASR: ffxm_vk.cpp no longer matches the memory-type correction")
  endif()
endforeach()
string(REPLACE "${asr_any}" "${asr_all}" asr_source "${asr_source}")
string(REPLACE "${asr_skip}" "${asr_fallback}" asr_source "${asr_source}")
file(CONFIGURE OUTPUT "${asr_vk}" CONTENT "${asr_source}" @ONLY)

add_library(openrac_asr_sdk STATIC
  "${asr}/src/shared/ffxm_assert.cpp"
  "${asr}/src/shared/ffxm_object_management.cpp"
  "${asr}/src/components/fsr2/ffxm_fsr2.cpp"
  "${asr_vk}"
  "${asr}/src/backends/vk/ffxm_hash.cpp"
  "${asr}/src/backends/shared/ffxm_shader_blobs.cpp"
  "${asr}/src/backends/shared/blob_accessors/ffxm_fsr2_shaderblobs.cpp")
target_include_directories(openrac_asr_sdk SYSTEM PUBLIC "${asr}/include")
target_include_directories(openrac_asr_sdk SYSTEM PRIVATE "${asr}" "${asr}/include/host"
  "${asr}/include/host/backends/vk" "${asr}/include/gpu" "${asr}/src/components"
  "${asr}/src/backends/shared" "${asr}/src/backends/vk"
  "${asr}/src/backends/shared/blob_accessors/prebuilt_shaders")
target_link_libraries(openrac_asr_sdk PUBLIC openrac_vulkan)
target_compile_features(openrac_asr_sdk PRIVATE cxx_std_20)
target_compile_definitions(openrac_asr_sdk PRIVATE FFXM_FSR NOMINMAX WIN32_LEAN_AND_MEAN)
# Its Vulkan backend calls the loader's functions directly: volk's, through OpenRAC's loader.
if(MSVC)
  target_compile_options(openrac_asr_sdk PRIVATE
    "/FI${CMAKE_CURRENT_SOURCE_DIR}/enhanced/asr/asr_compat.h" /W0)
else()
  target_compile_options(openrac_asr_sdk PRIVATE
    -include "${CMAKE_CURRENT_SOURCE_DIR}/enhanced/asr/asr_compat.h" -w)
endif()

add_library(openrac_asr STATIC enhanced/asr/asr_context.cpp)
target_link_libraries(openrac_asr PUBLIC openrac_renderer_vulkan PRIVATE openrac_renderer
  openrac_asr_sdk)
openrac_warnings(openrac_asr)

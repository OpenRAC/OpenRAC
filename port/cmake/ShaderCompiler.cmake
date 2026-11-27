# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
include_guard(GLOBAL)
if(NOT TARGET openrac_renderer_vulkan)
  return()
endif()

option(OPENRAC_SHADER_RUNTIME_COMPILER "Allow development-time compilation of missing Vulkan variants" ON)
option(OPENRAC_SHADER_PACKAGE "Generate and embed built-in Vulkan shaders during the build" ON)
set(OPENRAC_SHADER_COMPILER "" CACHE FILEPATH "Host openrac-shader-compiler executable for cross builds")

file(GLOB_RECURSE shader_inputs CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/renderer/*.vert" "${PROJECT_SOURCE_DIR}/renderer/*.frag"
  "${PROJECT_SOURCE_DIR}/renderer/*.comp" "${PROJECT_SOURCE_DIR}/renderer/*.glsl"
  "${PROJECT_SOURCE_DIR}/enhanced/*.vert" "${PROJECT_SOURCE_DIR}/enhanced/*.frag"
  "${PROJECT_SOURCE_DIR}/enhanced/*.comp" "${PROJECT_SOURCE_DIR}/enhanced/*.glsl"
  "${PROJECT_SOURCE_DIR}/*shader*.h" "${PROJECT_SOURCE_DIR}/*shader*.cpp")
file(GLOB_RECURSE shader_assembly_inputs CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/renderer/*.cpp" "${PROJECT_SOURCE_DIR}/renderer/*.h" "${PROJECT_SOURCE_DIR}/renderer/*.inc"
  "${PROJECT_SOURCE_DIR}/scene/*.cpp" "${PROJECT_SOURCE_DIR}/scene/*.h"
  "${PROJECT_SOURCE_DIR}/enhanced/*.cpp" "${PROJECT_SOURCE_DIR}/enhanced/*.h"
  "${PROJECT_SOURCE_DIR}/tools/shader_compiler/*.cpp" "${PROJECT_SOURCE_DIR}/tools/shader_compiler/*.h")
list(APPEND shader_inputs ${shader_assembly_inputs} "${CMAKE_CURRENT_LIST_FILE}" "${CMAKE_CURRENT_LIST_DIR}/ShaderSources.cmake")
list(REMOVE_DUPLICATES shader_inputs)
list(FILTER shader_inputs EXCLUDE REGEX "/(build|tests)/")
list(SORT shader_inputs)
list(APPEND shader_inputs "${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake")
if(TARGET openrac_fsr3_shaders)
  list(APPEND shader_inputs "${CMAKE_CURRENT_BINARY_DIR}/generated/fsr3/fsr3_sources.h")
endif()
set(version "OpenRAC shader package 2\n")
if(OPENRAC_ENHANCED_RENDERING)
  string(APPEND version "enhanced=1\n")
else()
  string(APPEND version "enhanced=0\n")
endif()
if(TARGET openrac_fsr3_shaders)
  string(APPEND version "fsr3=1\n")
else()
  string(APPEND version "fsr3=0\n")
endif()
foreach(input IN LISTS shader_inputs)
  file(SHA256 "${input}" hash)
  if(input STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/generated/fsr3/fsr3_sources.h")
    set(relative "sdk/fsr3_sources.h")
  else()
    file(RELATIVE_PATH relative "${PROJECT_SOURCE_DIR}" "${input}")
  endif()
  string(APPEND version "${relative} ${hash}\n")
endforeach()
string(SHA256 version "${version}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${shader_inputs})
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(CONFIGURE OUTPUT "${generated}/shader_package_version.h"
  CONTENT "// Generated; do not edit.\n#define OPENRAC_SHADER_PACKAGE_VERSION \"${version}\"\n" @ONLY)

add_library(openrac_shader_data STATIC
  renderer/vulkan/pipelines/shader_package.cpp renderer/vulkan/pipelines/shader_package_writer.cpp
  renderer/vulkan/pipelines/shader_package_codec.cpp renderer/vulkan/pipelines/shader_clip_distance.cpp
  renderer/vulkan/cache/cache_file.cpp renderer/vulkan/cache/spirv_cache_file.cpp)
if(NOT TARGET openrac_bake_lz4)
  include(BakeLz4)
endif()
target_include_directories(openrac_shader_data PUBLIC . PRIVATE "${generated}")
target_link_libraries(openrac_shader_data PUBLIC openrac_common PRIVATE openrac_bake_lz4)
openrac_warnings(openrac_shader_data)

set(backend_version "SPIR-V reflection format 3\n")
foreach(input IN ITEMS renderer/vulkan/pipelines/shader_backend.cpp
    renderer/vulkan/pipelines/shader_reflection.cpp
    renderer/vulkan/pipelines/shader_optimization.cpp
    renderer/vulkan/pipelines/native_shader_backend.cpp
    renderer/vulkan/pipelines/shader_compiler_internal.h
    renderer/vulkan/pipelines/shader_specialization.h
    renderer/vulkan/pipelines/shader_clip_distance.cpp renderer/gl/shader_precision.h
    renderer/vulkan/pipelines/shader_compiler.h renderer/vulkan/cache/spirv_cache_file.cpp
    cmake/Dependencies.cmake)
  file(SHA256 "${PROJECT_SOURCE_DIR}/${input}" hash)
  string(APPEND backend_version "${input} ${hash}\n")
endforeach()
if(TARGET openrac_fsr3_shaders)
  file(SHA256 "${CMAKE_CURRENT_BINARY_DIR}/generated/fsr3/fsr3_sources.h" hash)
  string(APPEND backend_version "${hash}")
endif()
string(SHA256 backend_version "${backend_version}")
file(CONFIGURE OUTPUT "${generated}/shader_backend_version.h" CONTENT
  "#define OPENRAC_SHADER_BACKEND_VERSION \"${backend_version}\"\n" @ONLY)

add_library(openrac_shader_backend STATIC
  renderer/vulkan/pipelines/shader_backend.cpp
  renderer/vulkan/pipelines/shader_optimization.cpp
  renderer/vulkan/pipelines/shader_reflection.cpp
  renderer/vulkan/pipelines/native_shader_backend.cpp)
target_include_directories(openrac_shader_backend PUBLIC . PRIVATE "${generated}")
target_link_libraries(openrac_shader_backend PRIVATE openrac_glslang SPIRV-Tools-opt)
openrac_warnings(openrac_shader_backend)

add_library(openrac_shader_recipes STATIC
  tools/shader_compiler/renderer_recipes.cpp
  tools/shader_compiler/scene_recipes.cpp enhanced/renderer/compiled_shader_recipes.cpp
  renderer/vulkan/pipelines/native_shader_recipes.cpp
  enhanced/fsr3/compiled_shader_recipes.cpp)
get_property(shader_text GLOBAL PROPERTY OPENRAC_SHADER_TEXT_TARGETS)
target_link_libraries(openrac_shader_recipes PUBLIC openrac_enhanced_shader_sources PRIVATE ${shader_text})
add_library(openrac_shader_build_report STATIC tools/shader_compiler/build_report.cpp)
target_link_libraries(openrac_shader_build_report PUBLIC openrac_common)
openrac_warnings(openrac_shader_build_report)
openrac_add_test(shader_compiler_report LIBS openrac_shader_build_report)
add_executable(openrac-shader-compiler
  tools/shader_compiler/main.cpp tools/shader_compiler/compiler.cpp
  tools/shader_compiler/compile_variants.cpp
  tools/shader_compiler/file_io.cpp tools/shader_compiler/package_output.cpp)
target_link_libraries(openrac-shader-compiler PRIVATE
  openrac_shader_backend openrac_shader_data openrac_shader_recipes openrac_shader_build_report)
if(TARGET openrac_fsr3_shaders)
  target_compile_definitions(openrac_shader_recipes PRIVATE OPENRAC_SHADER_FSR3=1)
  target_include_directories(openrac_shader_recipes SYSTEM PRIVATE ${sdk_includes})
  target_include_directories(openrac_shader_recipes PRIVATE "${generated}/fsr3")
  target_compile_definitions(openrac-shader-compiler PRIVATE OPENRAC_SHADER_FSR3=1)
  target_include_directories(openrac-shader-compiler SYSTEM PRIVATE ${sdk_includes})
  target_include_directories(openrac-shader-compiler PRIVATE "${generated}/fsr3")
endif()
openrac_warnings(openrac-shader-compiler)
openrac_warnings(openrac_shader_recipes)

target_link_libraries(openrac_renderer_vulkan PRIVATE openrac_shader_data)
target_compile_definitions(openrac_renderer_vulkan PRIVATE
  OPENRAC_SHADER_RUNTIME_COMPILER=$<BOOL:${OPENRAC_SHADER_RUNTIME_COMPILER}>)
if(OPENRAC_SHADER_RUNTIME_COMPILER)
  target_link_libraries(openrac_renderer_vulkan PRIVATE openrac_shader_backend)
endif()

set(package_dir "${generated}/shader_package")
set(package_source "${package_dir}/shader_package_data.cpp")
if(OPENRAC_SHADER_PACKAGE)
  set(shader_needs_host_compiler "${CMAKE_CROSSCOMPILING}")
  # macOS architecture selection does not set CMAKE_CROSSCOMPILING. Universal
  # tools can run locally when they include the host architecture.
  if(APPLE AND CMAKE_OSX_ARCHITECTURES
      AND NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" IN_LIST CMAKE_OSX_ARCHITECTURES)
    set(shader_needs_host_compiler TRUE)
  endif()
  if(OPENRAC_SHADER_COMPILER)
    set(compiler "${OPENRAC_SHADER_COMPILER}")
    set(compiler_dependency "${compiler}")
  elseif(shader_needs_host_compiler)
    include(ExternalProject)
    find_program(shader_host_cc NAMES clang cc gcc NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    find_program(shader_host_cxx NAMES clang++ c++ g++ NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    set(host_dir "${CMAKE_CURRENT_BINARY_DIR}/shader-compiler-host")
    set(host_args
      "-DCMAKE_C_COMPILER:FILEPATH=${shader_host_cc}"
      "-DCMAKE_CXX_COMPILER:FILEPATH=${shader_host_cxx}"
      "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_TOOLCHAIN_FILE:FILEPATH="
      "-DOPENRAC_SHADER_PACKAGE:BOOL=OFF" "-DOPENRAC_PORT_GAMES:BOOL=OFF"
      "-DOPENRAC_ENHANCED_RENDERING:BOOL=${OPENRAC_ENHANCED_RENDERING}"
      "-DOPENRAC_FSR3:BOOL=${OPENRAC_FSR3}"
      "-DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}")
    if(APPLE)
      list(APPEND host_args "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_HOST_SYSTEM_PROCESSOR}")
    elseif(CMAKE_HOST_UNIX)
      # The tool opens no window: a Linux host without X11 or Wayland headers (the Android
      # build's runner) still configures SDL, which otherwise stops there.
      list(APPEND host_args "-DSDL_UNIX_CONSOLE_BUILD:BOOL=ON")
    endif()
    # Reuse the parent's pinned dependency sources, with independent native build files.
    get_cmake_property(shader_variables VARIABLES)
    foreach(variable IN LISTS shader_variables)
      # Read only *_SOURCE_DIR values: reading every variable trips CMake 4's
      # deprecated-variable policy (CMP0218).
      if(NOT variable MATCHES "^([a-zA-Z0-9_]+)_SOURCE_DIR$")
        continue()
      endif()
      string(TOUPPER "${CMAKE_MATCH_1}" dependency)
      if(IS_DIRECTORY "${${variable}}" AND NOT dependency MATCHES "^(FETCHCONTENT|OPENRAC_PORT|CMAKE)")
        list(APPEND host_args "-DFETCHCONTENT_SOURCE_DIR_${dependency}:PATH=${${variable}}")
      endif()
    endforeach()
    if(CMAKE_HOST_WIN32)
      set(host_executable "${host_dir}/openrac-shader-compiler.exe")
    else()
      set(host_executable "${host_dir}/openrac-shader-compiler")
    endif()
    ExternalProject_Add(openrac_shader_compiler_host
      SOURCE_DIR "${PROJECT_SOURCE_DIR}" BINARY_DIR "${host_dir}"
      CMAKE_GENERATOR Ninja CMAKE_ARGS ${host_args} "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
      BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target openrac-shader-compiler
      BUILD_BYPRODUCTS "${host_executable}" BUILD_ALWAYS TRUE INSTALL_COMMAND "")
    set(compiler "${host_executable}")
    set(compiler_dependency "${host_executable}")
  else()
    set(compiler "$<TARGET_FILE:openrac-shader-compiler>")
    set(compiler_dependency openrac-shader-compiler)
  endif()
  add_custom_command(OUTPUT "${package_dir}/shader_package_data.cpp"
    BYPRODUCTS "${package_dir}/shaders.bin" "${package_dir}/shaders.json" "${package_dir}/shader-compiler.log"
    COMMAND "${compiler}" --output "${package_dir}" --expect-fingerprint "${version}"
    DEPENDS ${compiler_dependency} ${shader_inputs} "${generated}/shader_package_version.h"
    COMMENT "Compiling OpenRAC Vulkan shader variants" VERBATIM)
else()
  set(package_source renderer/vulkan/pipelines/empty_shader_package.cpp)
endif()
# The package is the same for every game and version of a build, so it is one shared library,
# libopenrac-shaders, beside the programs (on Android in the APK's library folder), not a copy in
# each program. iOS builds one game into its app, so the package stays built in there.
option(OPENRAC_SHARED_SHADER_PACKAGE "Shaders as one shared library beside the programs" ON)
if(OPENRAC_SHADER_PACKAGE AND OPENRAC_SHARED_SHADER_PACKAGE AND NOT OPENRAC_IOS)
  add_library(openrac_shader_package SHARED "${package_source}")
  # Only bytes and one C function, compiled as C: it needs no C++ runtime.
  set_source_files_properties("${package_source}" PROPERTIES LANGUAGE C)
  set_target_properties(openrac_shader_package PROPERTIES OUTPUT_NAME openrac-shaders)
  if(WIN32)
    target_compile_definitions(openrac_shader_package PRIVATE "OPENRAC_SHADER_PACKAGE_API=__declspec(dllexport)")
    openrac_windows_version_info(openrac_shader_package "OpenRAC Vulkan shaders")
  else()
    target_compile_definitions(openrac_shader_package PRIVATE
      "OPENRAC_SHADER_PACKAGE_API=__attribute__((visibility(\"default\")))")
  endif()
else()
  add_library(openrac_shader_package STATIC "${package_source}")
endif()
openrac_warnings(openrac_shader_package)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # The generated archive is one contiguous literal, beyond the standard's minimum supported size.
  target_compile_options(openrac_shader_package PRIVATE -Wno-overlength-strings)
endif()
add_library(openrac_builtin_shaders STATIC
  renderer/vulkan/pipelines/builtin_shader_package.cpp
  renderer/vulkan/pipelines/error_shader.cpp)
target_link_libraries(openrac_builtin_shaders PUBLIC openrac_shader_data openrac_shader_package)
openrac_warnings(openrac_builtin_shaders)
target_link_libraries(openrac_renderer_vulkan PRIVATE openrac_builtin_shaders)
if(OPENRAC_SHADER_PACKAGE)
  openrac_add_test(shader_package LIBS openrac_builtin_shaders openrac_shader_backend)
  openrac_add_test(shader_package_vulkan LIBS openrac_renderer openrac_platform openrac_shader_recipes)
  openrac_add_test(shader_error INCLUDES "${generated}"
    LIBS openrac_renderer openrac_platform openrac_builtin_shaders)
  add_test(NAME shader_error_strict COMMAND shader_error_test --strict)
  set_tests_properties(shader_error shader_error_strict PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
    "OPENRAC_GL_API=vulkan;OPENRAC_ENHANCED=0;OPENRAC_VULKAN_PIPELINE_CACHE=0")
  set_property(TEST shader_error_strict APPEND PROPERTY ENVIRONMENT "OPENRAC_SHADER_PACKAGE_STRICT=1")
  if(TEST fsr3_shaders)
    set_property(TEST fsr3_shaders APPEND PROPERTY ENVIRONMENT "OPENRAC_SHADER_PACKAGE_STRICT=1")
  endif()
  add_test(NAME shader_package_vulkan_fp32 COMMAND shader_package_vulkan_test)
  add_test(NAME shader_package_vulkan_compatibility COMMAND shader_package_vulkan_test)
  set_tests_properties(shader_package_vulkan_compatibility PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
    "OPENRAC_GL_API=vulkan;OPENRAC_ENHANCED=0;OPENRAC_VULKAN_PIPELINE_CACHE=0;OPENRAC_SHADER_PACKAGE_STRICT=1")
  set_tests_properties(shader_package_vulkan_fp32 PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
    "OPENRAC_GL_API=vulkan;OPENRAC_ENHANCED=1;OPENRAC_VULKAN_PIPELINE_CACHE=0;OPENRAC_SHADER_PACKAGE_STRICT=1;OPENRAC_GPU_MATERIALS_OFF=1;OPENRAC_RENDER_OPT_OFF=shader-float16,vk-draw-uniform")
  set_tests_properties(shader_package_vulkan PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
    "OPENRAC_GL_API=vulkan;OPENRAC_ENHANCED=1;OPENRAC_VULKAN_PIPELINE_CACHE=0;OPENRAC_SHADER_PACKAGE_STRICT=1")
endif()

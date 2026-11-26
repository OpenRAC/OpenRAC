# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
# Source assembly shared by the renderers and the CPU-only shader compiler, on every graphics API.
include_guard(GLOBAL)
add_library(openrac_shader_sources STATIC
  renderer/gl/shader_constants.cpp renderer/gl/shader_sources.cpp renderer/world/shader_sources.cpp)
target_include_directories(openrac_shader_sources PUBLIC . "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(openrac_shader_sources PUBLIC openrac_renderer_shader_text)
openrac_warnings(openrac_shader_sources)
target_link_libraries(openrac_renderer PRIVATE openrac_shader_sources)

add_library(openrac_enhanced_shader_sources STATIC enhanced/viewer/shader_sources.cpp)
target_link_libraries(openrac_enhanced_shader_sources PUBLIC
  openrac_shader_sources openrac_enhanced_viewer_shader_text)
openrac_warnings(openrac_enhanced_shader_sources)
target_link_libraries(openrac_enhanced_viewer PRIVATE openrac_enhanced_shader_sources)
target_sources(openrac_enhanced_viewer PRIVATE enhanced/viewer/shader_source_options.cpp)

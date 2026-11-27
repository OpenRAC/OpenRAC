// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// CPU-only shader compilation entry points shared by development builds and the offline compiler.

#pragma once

#include "renderer/vulkan/pipelines/shader_compiler.h"

// CPU-only compiler entry points. Neither Vulkan nor a rendering context is required.
namespace openrac::vkgl::compiler_detail {
// Returns glslang's version and the compilation/reflection cache identity.
std::string compiler_version();
// Replaces clip-distance writes on devices without a usable clip-distance capability.
std::string vertex_clip_distance(std::string_view source, bool clip_distance);
// Parses a graphics stage, returning compiler diagnostics on failure.
bool check_stage_fresh(bool vertex, std::string_view source, std::string& log);
// Parses a compute stage independently of an archive or renderer context.
bool check_compute_stage_fresh(std::string_view source, std::string& log);
// Emits frontend compute instructions for tests that inspect call placement before optimization.
[[nodiscard]] bool compile_compute_for_analysis(
    std::string_view source, std::vector<std::uint32_t>& code, std::string& log
);
// Compiles, links and reflects a complete GL-shaped graphics or compute program on the CPU.
bool compile_program_fresh(
    std::string_view vertex,
    std::string_view fragment,
    CompiledProgram& out,
    std::string& log,
    std::string_view compute = {}
);
// Compiles explicit Vulkan bindings using the caller's synchronous embedded-header lookup.
bool compile_native_fresh(
    std::string_view source,
    std::string_view definitions,
    const std::function<std::string_view(std::string_view)>& include,
    NativeComputeShader& out,
    std::string& error
);
}  // namespace openrac::vkgl::compiler_detail

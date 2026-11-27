// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared glslang initialization and optimized SPIR-V generation for graphics and compute shaders.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glslang/Public/ShaderLang.h>

namespace openrac::vkgl {
struct CompiledProgram;
}

namespace openrac::vkgl::compiler_detail {

// Validates both shader entry points against Vulkan and SPIR-V rules.
inline constexpr EShMessages kMessages =
    static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

// Retains GL uniform offsets, descriptor bindings and graphics attribute locations after linking.
void reflect_program(glslang::TProgram& program, CompiledProgram& out, EShLanguage stage);

// Initialises glslang once across both compiler entry points and calling threads.
void initialise();

// Emits optimized, validated SPIR-V while preserving bindings and specialization constants.
[[nodiscard]] bool compile_spirv(
    const glslang::TIntermediate& intermediate, std::vector<std::uint32_t>& code, std::string& log
);

// Retains frontend instructions for structural analysis before optimization changes call placement.
[[nodiscard]] bool compile_spirv_for_analysis(
    const glslang::TIntermediate& intermediate, std::vector<std::uint32_t>& code, std::string& log
);

}  // namespace openrac::vkgl::compiler_detail

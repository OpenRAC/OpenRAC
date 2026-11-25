// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared glslang initialisation and validation flags for GL translation and native compute shaders.
#pragma once

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

}  // namespace openrac::vkgl::compiler_detail

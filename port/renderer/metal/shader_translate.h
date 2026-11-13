// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's GLSL as Metal's shading language: glslang makes SPIR-V of it exactly as for
// Vulkan (renderer/vulkan/pipelines/shader_compiler.h, which also answers what an OpenGL program would about
// its uniforms), and SPIRV-Cross writes that SPIR-V as MSL. One source for every API, so a shader
// is never written twice and cannot drift from its other forms.
//
// Rows: Vulkan and OpenGL put a framebuffer's row 0 at NDC y = -1, Metal at +1. The vertex stage
// negates y, so that row 0 stays where GL draws it and viewports, scissors, reads and copies keep
// their numbers (as vkgl.h does for Vulkan). Depth: the SPIR-V already maps GL's -w..w to 0..w,
// unless the clip control function constant says the shader's z is 0..w already. The far half
// constant (kFarHalfConstant, false when not given) negates z, the near half keeps it as it is
// (mtlgl_draw.mm's depth_form).
//
// Slots: Metal binds by kind (buffer, texture, sampler) where GL and SPIR-V bind by number. The
// program's bindings are given Metal slots in the order met, the same in both stages, and the
// tables below say which GL binding each slot is. Vertex buffers take the buffer slots from
// kVertexBufferBase up, so a program may have that many blocks.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "renderer/vulkan/pipelines/shader_compiler.h"

namespace openrac::mtlgl {

// The oldest Metal the renderer is written for: macOS 12 on the AMD GPUs of 2015's MacBook Pros
// (MTLGPUFamilyMac2), MSL 2.4.
inline constexpr std::uint32_t kMslMajor = 2;
inline constexpr std::uint32_t kMslMinor = 4;

// Metal's buffer slots from here up are the vertex buffers'; the program's blocks are below it.
inline constexpr std::uint32_t kVertexBufferBase = 16;
// MTLGPUFamilyMac2's samplers a stage may bind.
inline constexpr std::uint32_t kMaxSamplers = 16;
// Function constants for the halves of depth (mtlgl_draw.mm's depth_form): whether a draw is the
// far half's, and whether depth is clamped, so that each half keeps only its own fragments.
inline constexpr std::uint32_t kFarHalfConstant = 901;
inline constexpr std::uint32_t kClampedConstant = 902;

struct MslTexture {
    int binding = -1;
    // The sampler slot of a combined image sampler (GL's sampler2D); -1 for a texel buffer or a
    // storage image. Samplers are numbered apart: Metal allows far fewer of them than textures.
    int sampler = -1;
};

struct MslProgram {
    // Each stage's source and the name of its function in it. Compute programs have only `compute`.
    std::string vertex, fragment, compute;
    std::string vertex_entry, fragment_entry, compute_entry;
    // By Metal slot: the GL binding of each buffer slot (uniform and storage blocks, the loose
    // uniforms' default block among them) and each texture slot.
    std::vector<int> buffers;
    std::vector<MslTexture> textures;
    std::array<std::uint32_t, 3> workgroup{1, 1, 1};  // A compute program's threads a group.
};

// Translates a program glslang compiled, dividing as the OpenGL of an `apple` GPU or of an AMD one
// does. False, with SPIRV-Cross's words in `log`, when it cannot.
bool translate_program(
    const vkgl::CompiledProgram& spirv, bool apple, MslProgram& out, std::string& log
);

}  // namespace openrac::mtlgl

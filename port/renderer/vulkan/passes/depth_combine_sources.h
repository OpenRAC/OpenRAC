// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Native shadow-depth combination GLSL shared by the Vulkan pass and build recipes.

#pragma once

namespace openrac::shader_sources::renderer_vulkan_passes_depth_combine {
inline constexpr const char* kCombine = R"(#version 450

layout(local_size_x=8, local_size_y=8) in;

layout(binding=0) uniform sampler2D static_depth;

layout(binding=1) uniform sampler2D dynamic_depth;

layout(binding=2, r32f) writeonly uniform image2D combined_depth;

layout(push_constant) uniform Parameters { ivec4 offset_rect; ivec4 extent; } params;

void main() {

    ivec2 q = ivec2(gl_GlobalInvocationID.xy);

    if (any(greaterThanEqual(q, params.extent.xy))) return;

    ivec2 cached = (q + params.offset_rect.xy) % params.extent.xy;

    float a = texelFetch(static_depth, cached, 0).r;

    float b = texelFetch(dynamic_depth, q + params.offset_rect.zw, 0).r;

    imageStore(combined_depth, q, vec4(max(a,b)));

}

)";
}

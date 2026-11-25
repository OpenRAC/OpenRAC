// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture copy and multisample seed GLSL shared by Vulkan draws and build recipes.

#pragma once

namespace openrac::shader_sources::renderer_vulkan_draw_multisample_blit {
inline constexpr const char* kVertex = R"(#version 410 core

uniform vec4 copy_region;

out vec2 uv;

void main() {

    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);

    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);

    uv = copy_region.xy + corner * copy_region.zw;

}

)";
inline constexpr const char* kFragment = R"(#version 410 core

uniform sampler2D copy_source;

in vec2 uv;

layout(location = 0) out vec4 result;

void main() { result = texture(copy_source, uv); }

)";
}  // namespace openrac::shader_sources::renderer_vulkan_draw_multisample_blit

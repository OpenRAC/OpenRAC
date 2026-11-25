// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// ImGui overlay GLSL shared by runtime drawing and the build-time shader compiler.

#pragma once

namespace openrac::shader_sources::renderer_ui_imgui_gl {
inline constexpr const char* kVertex = R"glsl(#version 410 core

layout(location = 0) in vec2 position;

layout(location = 1) in vec2 uv;

layout(location = 2) in vec4 colour;

uniform mat4 projection;

out vec2 v_uv;

out vec4 v_colour;

void main() {

    v_uv = uv;

    v_colour = colour;

    gl_Position = projection * vec4(position, 0.0, 1.0);

}

)glsl";
inline constexpr const char* kFragment = R"glsl(#version 410 core

in vec2 v_uv;

in vec4 v_colour;

uniform sampler2D tex_T0;

out vec4 out_colour;

void main() {

    vec4 texel = texture(tex_T0, v_uv);

    out_colour = vec4(vec3(mp_vec3(v_colour.rgb) * mp_vec3(texel.rgb)), v_colour.a * texel.a);

}

)glsl";
}  // namespace openrac::shader_sources::renderer_ui_imgui_gl

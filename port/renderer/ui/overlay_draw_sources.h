// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Mod overlay GLSL shared by runtime drawing and build-time shader enumeration.

#pragma once

namespace openrac::shader_sources::renderer_ui_overlay_draw {
inline constexpr const char* kVertex = R"glsl(#version 410 core

layout(location = 0) in vec2 position;

layout(location = 1) in vec4 colour;

out vec4 v_colour;

void main() {

    v_colour = colour;

    gl_Position = vec4(position.x / 256.0 - 1.0, 1.0 - position.y / 224.0, 0.0, 1.0);

}

)glsl";
inline constexpr const char* kFragment = R"glsl(#version 410 core

in vec4 v_colour;

out vec4 out_colour;

void main() {

    out_colour = v_colour;

}

)glsl";
}  // namespace openrac::shader_sources::renderer_ui_overlay_draw

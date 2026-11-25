// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Compiled into the built-in package; diagnostics never compile these sources at runtime.
#pragma once

namespace openrac::vkgl::error_shader_sources {
inline constexpr char kVertex[] = R"(#version 450 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

inline constexpr char kFragment[] = R"(#version 450 core
layout(location = 0) out vec4 colour;
void main() {
    ivec2 cell = ivec2(floor(gl_FragCoord.xy / 8.0));
    bool magenta = ((cell.x + cell.y) & 1) == 0;
    colour = magenta ? vec4(1.0, 0.0, 1.0, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
)";
}  // namespace openrac::vkgl::error_shader_sources

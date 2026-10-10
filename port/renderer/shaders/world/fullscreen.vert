#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One triangle covering the viewport, from gl_VertexID (no vertex buffer):
// the full-screen passes (shadow resolve, fades, tints, mirror, copies).

out vec2 v_uv;

void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    v_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}

#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Extracted level geometry, instanced: one matrix per placed object, in the
// game's axes (Z up). The projection has reversed depth.
//
// A live moby is skinned first: its vertex is the weighted sum of its joints'
// matrices from the moby's joint palette (F_j = P_j S_j, as the game's
// MobyAnimEval builds it), applied to the bind-pose vertex. The palettes are
// a float texture, four texels (the matrix's columns) per joint; `palette` is
// the instance's first matrix, -1 for an object drawn as stored.

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 colour;
layout(location = 4) in mat4 model;  // locations 4 to 7
layout(location = 8) in vec4 tint;
layout(location = 9) in float palette;
layout(location = 10) in uvec4 joints;
layout(location = 11) in vec4 weights;

uniform mat4 view_projection;
uniform int skinning;
uniform sampler2D palette_texture;

out vec2 v_uv;
out vec4 v_colour;
out vec3 v_normal;

const int kPaletteWidth = 1024;

mat4 joint_matrix(int index) {
    int texel = 4 * index;
    ivec2 at = ivec2(texel % kPaletteWidth, texel / kPaletteWidth);
    return mat4(
        texelFetch(palette_texture, at, 0),
        texelFetch(palette_texture, at + ivec2(1, 0), 0),
        texelFetch(palette_texture, at + ivec2(2, 0), 0),
        texelFetch(palette_texture, at + ivec2(3, 0), 0)
    );
}

void main() {
    vec4 p = vec4(position, 1.0);
    vec3 n = normal;
    float total = weights.x + weights.y + weights.z + weights.w;
    if (skinning != 0 && palette >= 0.0 && total > 0.0) {
        int base = int(palette + 0.5);
        mat4 skin = weights.x * joint_matrix(base + int(joints.x))
                  + weights.y * joint_matrix(base + int(joints.y))
                  + weights.z * joint_matrix(base + int(joints.z))
                  + weights.w * joint_matrix(base + int(joints.w));
        p = vec4((skin * p).xyz, 1.0);
        n = mat3(skin) * n;
    }
    gl_Position = view_projection * (model * p);
    v_uv = uv;
    v_colour = colour * tint;
    v_normal = mat3(model) * n;
}

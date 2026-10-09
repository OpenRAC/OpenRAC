// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders
// (the fog, depth and mip rules shared by tfrag.wgsl, tie.wgsl, shrub.wgsl,
// moby.wgsl and the others): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// What every world shader shares (inserted after the version line, see
// renderer/world/gpu.h): the WorldView block the renderers write each frame
// (renderer/world/world_context.cpp), and the GS rules the vector-unit
// programs and the GS apply to every world vertex and pixel.
//
// Units: positions in game units, game axes (Z up). The programs work in
// integer units (game units x 1024); "depth" below is the camera depth in
// those units, as the VU's w lane sees it.

layout(std140) uniform WorldView {
    mat4 u_view;             // world to GL view space
    mat4 u_projection;
    mat4 u_view_projection;
    vec4 u_eye;              // camera position, game units
    vec4 u_fog_colour;       // FOGCOL as display bytes / 255; w = 1 when fog is on
    vec4 u_fog_params;       // slope per integer unit, offset, lower clamp, upper clamp
    vec4 u_screen;           // x, y: half the game's frame in pixels; z: near (32); w: LOD tint
    vec4 u_tans;             // x, y: the view's half-angle tangents; z: the game's y ratio
};

// Every vertex of a primitive that is not drawn this frame lands on one
// point outside the clip volume, so the primitive has no fragments.
const vec4 kCulled = vec4(2.0, 2.0, 2.0, 1.0);

vec4 world_to_clip(vec3 p) {
    return u_view_projection * vec4(p, 1.0);
}

// Camera depth in integer units (GL view space looks down -Z).
float depth_int(vec3 p) {
    return -(u_view * vec4(p, 1.0)).z * 1024.0;
}

// The fog coefficient F / 255 a world program writes for a vertex at `depth`
// (integer units): w = depth * slope (the projection's w row), plus the
// offset, clamped between the far and near intensities, truncated by ftoi4.
float fog_coefficient(float depth) {
    float f = max(min(depth * u_fog_params.x + u_fog_params.y, u_fog_params.w), u_fog_params.z);
    return trunc(f) / 255.0;
}

// The GS fog (PRIM FGE): RGB only, C = FOGCOL + (C - FOGCOL) * F / 255.
vec3 apply_fog(vec3 rgb, float f) {
    return u_fog_colour.w > 0.5 ? mix(u_fog_colour.rgb, rgb, f) : rgb;
}

// The GS mip rule (TEX1 LCM = 0, L = 0): LOD = log2(1 / |Q|) + K with
// Q = near / depth, so LOD = log2(depth / near) + K; MMIN
// LINEAR_MIPMAP_NEAREST takes the nearest level, clamped to 0..MXL (below 0
// is magnification on level 0), bilinear inside it. Q is interpolated
// perspective-correctly, so `depth` is the exact per-pixel depth.
float gs_mip_level(float depth, float k, float max_level) {
    return clamp(floor(log2(depth / u_screen.z) + k + 0.5), 0.0, max_level);
}

// The tfrag, tie and shrub vertices' TEX1 K: s12 in 1/16 mip levels.
float tex1_k(uint packed_s12) {
    return float(int(packed_s12 << 20) >> 20) / 16.0;
}

// Sign-extend the low 16 bits.
int s16(int x) {
    return (x << 16) >> 16;
}

vec3 unpack_rgb(uint c) {
    return vec3(float(c & 0xffu), float((c >> 8u) & 0xffu), float((c >> 16u) & 0xffu));
}

vec4 unpack_rgba(uint c) {
    return vec4(unpack_rgb(c), float(c >> 24u));
}

// A record word as a float (records are stored as RGBA32UI texels).
vec4 as_float(uvec4 v) {
    return uintBitsToFloat(v);
}

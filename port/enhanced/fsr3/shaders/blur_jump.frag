#version 410 core
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 sphynx-owner
// Copyright (c) 2026 the OpenRAC contributors
// Adapted from SphynxMotionBlurToolkit/JumpFlood/ShaderFiles/jfp_simple.glsl,
// d0c16fc1581eb757e993ff6eaf5877078c8d24cd. Licence: THIRD_PARTY_NOTICES.md.
// Source-coordinate JFA with velocity-space reach/side tests and foreground-depth priority.
// Changes: full-resolution packed integer-coordinate seeds in R32UI, explicit invalid seeds,
// retained centre candidate, power-of-two jumps, reversed-depth ties and pixel-space units.
uniform usampler2D seed_texture;
uniform sampler2D velocity_texture;
uniform vec2 display_size;
uniform int jump_pixels;
uniform float min_motion;
uniform float side_spread;
layout(location=0) out uint chosen_seed;
const uint invalid_seed = 0xffffffffu;
#ifdef FAST_FLOOD
uint last_seed;
#endif
uint seed_at(ivec2 pixel) {
    // The first flood reads implicit per-pixel seeds, avoiding a separate initialization pass.
#ifdef INITIALIZE_SEEDS
    return uint(pixel.x) | (uint(pixel.y) << 16);
#else
    return texelFetch(seed_texture, pixel, 0).r;
#endif
}
const ivec2 neighbours[8] = ivec2[8](
    ivec2(-1,0), ivec2(1,0), ivec2(0,-1), ivec2(0,1),
    ivec2(-1,1), ivec2(1,-1), ivec2(1,1), ivec2(-1,-1)
);

// Upstream sample_fitness expressed in display pixels. Velocities here already include
// exposure and point towards the previous position, so no extra intensity/sign reversal.
void consider(uint packed, vec2 receiver, inout float best_depth, inout float best_fitness) {
    if (packed == invalid_seed || packed == chosen_seed) return;
#ifdef FAST_FLOOD
    // Re-testing the same rejected source cannot change its fitness or win against a nearer best.
    if (packed == last_seed) return;
    last_seed = packed;
#endif
    vec2 candidate = vec2(packed & 0xffffu, packed >> 16);
    ivec2 size = textureSize(velocity_texture, 0);
    if (any(greaterThanEqual(candidate, vec2(size)))) return;
    vec4 motion = texelFetch(velocity_texture, ivec2(candidate), 0);
    // A farther source cannot replace an already valid foreground candidate.
    if (motion.z < best_depth) return;
    vec2 sample_velocity = motion.xy * display_size;
    float speed2 = dot(sample_velocity, sample_velocity);
#if !defined(FAST_FLOOD) || defined(INITIALIZE_SEEDS)
    // Later flood passes only carry seeds that already passed this immutable-velocity test.
    if (speed2 < min_motion * min_motion) return;
#endif
    vec2 uv_offset = (receiver - candidate) / vec2(size) * display_size;
    float velocity_space_distance = dot(sample_velocity, uv_offset) / speed2;
    float absolute_velocity_space_distance = abs(velocity_space_distance - 0.5);
    float side_offset = abs(dot(vec2(uv_offset.y, -uv_offset.x), sample_velocity)) / speed2;
    // User-controlled side allowance (upstream default 0.5); gather checks actual coverage.
    if (absolute_velocity_space_distance > 0.5001 || side_offset > side_spread + 1.0 / sqrt(speed2)) return;
    // Reversed depth makes a foreground source win over background regardless of speed.
    if (motion.z > best_depth
        || (motion.z == best_depth && absolute_velocity_space_distance < best_fitness)) {
        chosen_seed = packed;
        best_depth = motion.z;
        best_fitness = absolute_velocity_space_distance;
    }
}
#if defined(FAST_FLOOD) && defined(INITIALIZE_SEEDS)
void consider_at(ivec2 at, ivec2 pixel, ivec2 size, inout float best_depth, inout float best_fitness) {
    if (any(lessThan(at, ivec2(0))) || any(greaterThanEqual(at, size))) return;
    consider(seed_at(at), vec2(pixel), best_depth, best_fitness);
}
#endif
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 size = textureSize(seed_texture, 0);
    chosen_seed = invalid_seed;
#ifdef FAST_FLOOD
    last_seed = invalid_seed;
#endif
    float best_depth = -1.0, best_fitness = 1e20;
    // Preserve a candidate across iterations, including isolated thin features at the image edge.
#ifndef FAST_FLOOD
    consider(seed_at(pixel), vec2(pixel), best_depth, best_fitness);
#endif
#ifdef FAST_FLOOD
#ifdef INITIALIZE_SEEDS
    // Initial sources are implicit coordinates, so there are no seed reads to prefetch.
    consider(seed_at(pixel), vec2(pixel), best_depth, best_fitness);
    consider_at(pixel + ivec2(-1,0) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(1,0) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(0,-1) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(0,1) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(-1,1) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(1,-1) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(1,1) * jump_pixels, pixel, size, best_depth, best_fitness);
    consider_at(pixel + ivec2(-1,-1) * jump_pixels, pixel, size, best_depth, best_fitness);
#else
    // Issue independent source-coordinate reads before the dependent velocity lookups.
    // This preserves candidate order while hiding texture latency between neighbours.
    uint centre_seed;
    uint left = invalid_seed, right = invalid_seed, down = invalid_seed, up = invalid_seed;
    uint upper_left = invalid_seed, lower_right = invalid_seed;
    uint upper_right = invalid_seed, lower_left = invalid_seed;
    if (jump_pixels == 1 && all(greaterThan(pixel, ivec2(0))) && all(lessThan(pixel, size - 1))) {
        // Gather returns (lower-left + Y, upper-right, lower-left + X, lower-left).
        // Quarter-texel positions keep selection away from rounding boundaries at large extents.
        vec2 uv = (vec2(pixel) + 0.25) / vec2(size);
        vec2 texel = 1.0 / vec2(size);
        uvec4 a = textureGather(seed_texture, uv);
        uvec4 b = textureGather(seed_texture, uv + vec2(texel.x, 0));
        uvec4 c = textureGather(seed_texture, uv + vec2(0, texel.y));
        uvec4 d = textureGather(seed_texture, uv + texel);
        centre_seed = a.y;
        left = a.x; right = b.y; down = a.z; up = c.y;
        upper_left = c.x; lower_right = b.z; upper_right = d.y; lower_left = a.w;
    } else {
        centre_seed = seed_at(pixel);
        if (pixel.x >= jump_pixels) left = seed_at(pixel + ivec2(-jump_pixels,0));
        if (pixel.x + jump_pixels < size.x) right = seed_at(pixel + ivec2(jump_pixels,0));
        if (pixel.y >= jump_pixels) down = seed_at(pixel + ivec2(0,-jump_pixels));
        if (pixel.y + jump_pixels < size.y) up = seed_at(pixel + ivec2(0,jump_pixels));
        if (pixel.x >= jump_pixels && pixel.y + jump_pixels < size.y)
            upper_left = seed_at(pixel + ivec2(-jump_pixels,jump_pixels));
        if (pixel.x + jump_pixels < size.x && pixel.y >= jump_pixels)
            lower_right = seed_at(pixel + ivec2(jump_pixels,-jump_pixels));
        if (pixel.x + jump_pixels < size.x && pixel.y + jump_pixels < size.y)
            upper_right = seed_at(pixel + ivec2(jump_pixels,jump_pixels));
        if (pixel.x >= jump_pixels && pixel.y >= jump_pixels)
            lower_left = seed_at(pixel + ivec2(-jump_pixels,-jump_pixels));
    }
    consider(centre_seed, vec2(pixel), best_depth, best_fitness);
    consider(left, vec2(pixel), best_depth, best_fitness);
    consider(right, vec2(pixel), best_depth, best_fitness);
    consider(down, vec2(pixel), best_depth, best_fitness);
    consider(up, vec2(pixel), best_depth, best_fitness);
    consider(upper_left, vec2(pixel), best_depth, best_fitness);
    consider(lower_right, vec2(pixel), best_depth, best_fitness);
    consider(upper_right, vec2(pixel), best_depth, best_fitness);
    consider(lower_left, vec2(pixel), best_depth, best_fitness);
#endif
#else
    for (int i = 0; i < 8; ++i) {
        ivec2 at = pixel + neighbours[i] * jump_pixels;
        if (any(lessThan(at, ivec2(0))) || any(greaterThanEqual(at, size))) continue;
        consider(seed_at(at), vec2(pixel), best_depth, best_fitness);
    }
#endif
}

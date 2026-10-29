#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Depth-aware gather extends moving silhouettes without assigning motion to stationary scenery.
uniform sampler2D colour_texture;
uniform sampler2D velocity_texture;
uniform usampler2D seed_texture;
uniform vec2 jitter;
uniform int sample_count;
uniform float min_motion;
uniform float depth_tolerance;
layout(location=0) out vec4 result;
vec4 velocity_at(vec2 uv) {
    vec2 size = vec2(textureSize(velocity_texture, 0));
    ivec2 p = ivec2(floor(uv * size + jitter));
    return texelFetch(velocity_texture, clamp(p, ivec2(0), ivec2(size) - 1), 0);
}
bool inside(vec2 uv) { return all(greaterThanEqual(uv, vec2(0))) && all(lessThan(uv, vec2(1))); }
void main() {
    vec2 display = vec2(textureSize(colour_texture, 0));
    vec2 uv = gl_FragCoord.xy / display;
    vec4 centre = velocity_at(uv);
    vec2 render_size = vec2(textureSize(velocity_texture, 0));
    ivec2 receiver = clamp(ivec2(floor(uv * render_size + jitter)), ivec2(0), ivec2(render_size) - 1);
    uint packed = texelFetch(seed_texture, receiver, 0).r;
    vec2 seed = vec2(packed & 0xffffu, packed >> 16);
    bool has_seed = packed != 0xffffffffu && all(lessThan(seed, render_size));
    vec2 direction = has_seed ? texelFetch(velocity_texture, ivec2(seed), 0).xy : vec2(0);
    vec2 seed_uv = has_seed ? (seed + 0.5 - jitter) / render_size : uv;
    float best = max(dot(direction * display, direction * display), dot(centre.xy * display, centre.xy * display));
    // The display already contains reconstructed colour: untouched pixels need no colour
    // fetch or attachment write. Preserve their exact colour, alpha and HUD separation.
    if (best < min_motion * min_motion) {
#ifdef COPY_UNCHANGED
        result = texelFetch(colour_texture, ivec2(gl_FragCoord.xy), 0);
        return;
#else
        discard;
#endif
    }
    vec4 centre_colour = texelFetch(colour_texture, ivec2(gl_FragCoord.xy), 0);
    vec3 base = centre_colour.rgb;
    // A moving receiver gathers along its own trail, excluding nearer occluders.
    if (length(centre.xy * display) >= min_motion) {
        vec3 sum = centre_colour.rgb;
        float weight = 1;
        for (int i = 1; i <= max(1, sample_count * 2 / 3); ++i) {
            vec2 sample_uv = uv + centre.xy * (float(i) / float(max(1, sample_count * 2 / 3)));
            if (!inside(sample_uv)) continue;
            vec4 v = velocity_at(sample_uv);
            float tolerance = max(0.01, centre.w * depth_tolerance);
            if (v.z != 0.0 && v.w < centre.w - tolerance) continue;
            sum += texture(colour_texture, sample_uv).rgb;
            weight += 1;
        }
        base = sum / weight;
    }
    // The jump-flood source brings motion beyond the current silhouette.
    vec3 incoming = vec3(0);
    float coverage = 0;
    for (int i = 1; i <= sample_count; ++i) {
        // Include the exact propagated source once so a thin feature cannot fall between taps.
        vec2 sample_uv = i == 1 && has_seed ? seed_uv
            : uv - direction * ((float(i) - 0.5) / float(sample_count));
        if (!inside(sample_uv)) continue;
        vec4 v = velocity_at(sample_uv);
        vec2 travel = v.xy * display;
        float speed2 = dot(travel, travel);
        if (speed2 < min_motion * min_motion) continue;
        vec2 delta = (uv - sample_uv) * display;
        float t = dot(delta, travel) / speed2;
        float side = length(delta - travel * t);
        float tolerance = max(0.01, centre.w * depth_tolerance);
        if (t < 0.0 || t > 1.0 || side > 1.5
            || (centre.z != 0.0 && v.w > centre.w + tolerance)) continue;
        incoming += texture(colour_texture, sample_uv).rgb;
        coverage += 1;
    }
    // Keep sample coordinates, rejection, reprojection and the long colour sums FP32.
    // Only blend the final bounded LDR averages in mixed precision.
    if (coverage > 0) base = vec3(mix(mp_vec3(base), mp_vec3(incoming / coverage), mp_float(coverage / float(sample_count))));
    result = vec4(base, centre_colour.a);
}

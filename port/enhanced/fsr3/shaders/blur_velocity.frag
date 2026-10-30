#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// FSR vectors point from current to previous unjittered UV, measured in render pixels.
uniform sampler2D motion_texture;
uniform sampler2D depth_texture;
uniform mat4 inverse_raster;
uniform mat4 previous_view;
uniform vec3 camera_position;
// Current minus previous displayed camera focus position, in world units.
uniform vec3 reference_delta;
uniform vec2 jitter;
uniform vec2 display_size;
uniform float exposure;
uniform float max_radius;
uniform float min_motion;
uniform int camera_relative;
layout(location=0) out vec4 velocity;
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 size = vec2(textureSize(motion_texture, 0));
    vec2 uv = (vec2(pixel) + 0.5) / size;
    float depth = texelFetch(depth_texture, pixel, 0).r;
    vec4 motion = texelFetch(motion_texture, pixel, 0);
    velocity = vec4(0, 0, depth, 65504.0);
    vec2 relative_velocity = motion.xy / size;
    if (depth <= 0.0) {
#ifndef OPENRAC_BLUR_CAMERA
#ifndef OPENRAC_BLUR_GAMEPLAY
        if (camera_relative == 0)
#endif
        { velocity = vec4(0); return; }
#endif
        // In cutscenes, the FSR sky vector includes camera rotation but no translation.
    } else {
        vec4 world = inverse_raster * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1);
        if (abs(world.w) < 1e-6) return;
        world /= world.w;
        velocity.w = min(length(world.xyz - camera_position), 65504.0);
#ifndef OPENRAC_BLUR_CAMERA
#ifndef OPENRAC_BLUR_GAMEPLAY
        if (camera_relative == 0)
#endif
        {
            // Project the tracking focus translation at each receiver's depth during gameplay.
            vec4 old = previous_view * vec4(world.xyz - reference_delta, 1);
            if (old.w <= 0.0) return;
            vec2 reference_velocity = old.xy / old.w * 0.5 + 0.5 - (uv - jitter / size);
            relative_velocity -= reference_velocity;
        }
#endif
    }
    if (motion.z > 0.5) return; // New/teleported objects lack reliable history in either mode.
    relative_velocity *= exposure;
    float pixels = length(relative_velocity * display_size);
    if (pixels < min_motion) return; // Reject FP16 residuals from reference cancellation.
    relative_velocity *= min(1.0, max_radius / pixels);
    velocity.xy = relative_velocity;
}

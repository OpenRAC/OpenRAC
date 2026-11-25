// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Backdrop blur GLSL shared by runtime UI drawing and the build-time shader compiler.

#pragma once

namespace openrac::shader_sources::renderer_ui_backdrop_blur {
inline constexpr const char* kVertex = R"glsl(#version 410 core

out vec2 v_uv;

void main() {

    const vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

    v_uv = corners[gl_VertexID] * 0.5 + 0.5;

    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);

}

)glsl";
inline constexpr const char* kFragment = R"glsl(#version 410 core

in vec2 v_uv;

uniform sampler2D tex_T0;

uniform vec2 texel_step;

uniform float sigma;

uniform float calm;

uniform float upscale;

uniform vec3 shade_colour;

uniform vec4 shade;  // top, bottom, side, side width (shares of the picture)

out vec4 out_colour;

vec4 bspline(float v) {

    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;

    vec4 c = n * n * n;

    float x = c.x;

    float y = c.y - 4.0 * c.x;

    float z = c.z - 4.0 * c.y + 6.0 * c.x;

    return vec4(x, y, z, 6.0 - x - y - z) / 6.0;

}

// A cubic B-spline of the texture from four bilinear reads: smooth where a plain bilinear

// enlargement of a small picture shows a grid of squares.

vec4 smooth_sample(vec2 uv) {

    vec2 size = vec2(textureSize(tex_T0, 0));

    vec2 p = uv * size - 0.5;

    vec2 f = fract(p);

    p -= f;

    vec4 xc = bspline(f.x);

    vec4 yc = bspline(f.y);

    vec4 c = p.xxyy + vec2(-0.5, 1.5).xyxy;

    vec4 w = vec4(xc.xz + xc.yw, yc.xz + yc.yw);

    vec4 o = (c + vec4(xc.yw, yc.yw) / w) / size.xxyy;

    vec4 s0 = texture(tex_T0, o.xz);

    vec4 s1 = texture(tex_T0, o.yz);

    vec4 s2 = texture(tex_T0, o.xw);

    vec4 s3 = texture(tex_T0, o.yw);

    float sx = w.x / (w.x + w.y);

    float sy = w.z / (w.z + w.w);

    return mix(mix(s3, s2, sx), mix(s1, s0, sx), sy);

}

void main() {

    if (upscale > 0.5) {

        // A little noise, under one step of eight bits, so dark gradients do not band.

        float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;

        vec3 c = smooth_sample(v_uv).rgb;

        // The first row is the bottom.

        c = mix(c, shade_colour, mix(shade.x, shade.y, 1.0 - v_uv.y));

        if (shade.w > 0.0) {

            float edge = min(v_uv.x, 1.0 - v_uv.x) / shade.w;

            c = mix(c, shade_colour, shade.z * clamp(1.0 - edge, 0.0, 1.0));

        }

        out_colour = vec4(c + noise * 1.5 / 255.0, 1.0);

        return;

    }

    int radius = int(ceil(sigma * 2.5));

    vec4 sum = texture(tex_T0, v_uv);

    float total = 1.0;

    for (int i = 1; i <= radius; ++i) {

        float w = exp(-float(i * i) / (2.0 * sigma * sigma));

        sum += w * (texture(tex_T0, v_uv + texel_step * float(i))

                  + texture(tex_T0, v_uv - texel_step * float(i)));

        total += 2.0 * w;

    }

    vec3 c = sum.rgb / total;

    // Calmer colours (the last pass only): a little less saturated, the brightest pulled down, so

    // something bright under the menu does not glare through it.

    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));

    c = mix(c, mix(vec3(lum), c, 0.8) * (1.0 - 0.3 * smoothstep(0.5, 1.0, lum)), calm);

    out_colour = vec4(c, 1.0);

}

)glsl";
}  // namespace openrac::shader_sources::renderer_ui_backdrop_blur

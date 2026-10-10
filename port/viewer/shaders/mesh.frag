#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture times vertex colour, alpha-tested for cutout materials, with a
// fixed sun so shapes read where the export has no lighting. The editor's PNGs
// carry alpha from 0 to 255 (it doubles the chip's 0..0x80), so the cutout
// threshold is plain 0.5. A vertex colour of 1.0 is the GS's 0x80: the product
// is the GS's MODULATE, (texel x colour) >> 7, clamped like the chip's.

in vec2 v_uv;
in vec4 v_colour;
in vec3 v_normal;
flat in int v_lit;
noperspective in float v_fog;

uniform sampler2D tex_T0;
uniform vec4 material_colour;
uniform int cutout;
uniform int lighting;
uniform vec4 fog_colour;  // FOGCOL; w = 1: fog on

layout(location = 0) out vec4 out_colour;

// From above and to one side, in game axes (Z up).
const vec3 kSun = vec3(0.36, 0.48, 0.80);

void main() {
    vec4 c = texture(tex_T0, v_uv) * v_colour * material_colour;
    if (cutout != 0 && c.a < 0.5) {
        discard;
    }
    float light = 1.0;
    if (lighting != 0 && v_lit == 0) {
        // Double-sided: a face lit from behind is lit too.
        light = 0.55 + 0.45 * abs(dot(normalize(v_normal), kSun));
    }
    vec3 rgb = min(c.rgb * light, vec3(1.0));
    // The GS's fog, after the texture: RGB only.
    if (fog_colour.w > 0.5) {
        rgb = mix(fog_colour.rgb, rgb, v_fog);
    }
    out_colour = vec4(rgb, 1.0);
}

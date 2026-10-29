#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
in vec3 v_normal;
in vec3 v_colour;
in float v_opacity;
uniform float debug_alpha;
layout(location = 0) out vec4 out_colour;

void main() {
    // Diagnostic colours identify placement state, not irradiance magnitude.
    mp_float shape = mp_float(0.35) + mp_float(0.65) * max(
        dot(mp_vec3(normalize(v_normal)), mp_vec3(normalize(vec3(0.4, -0.6, 0.7)))), mp_float(0));
    out_colour = vec4(vec3(mp_vec3(v_colour) * shape), debug_alpha * v_opacity);
}

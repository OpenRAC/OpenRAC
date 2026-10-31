#version 430 core
// SPDX-License-Identifier: GPL-3.0-or-later
layout(location=0) out vec4 out_colour;
void main() {
    vec4 fog=atmosphere_sky_sample();
    out_colour=vec4(fog.rgb,1.-fog.a);
}

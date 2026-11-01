#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Unlit markers retain source hue while occluded edges fade through geometry.
in vec3 marker_colour;
uniform float debug_alpha;
out vec4 frag_colour;
void main() {
    frag_colour = vec4(marker_colour, debug_alpha);
}

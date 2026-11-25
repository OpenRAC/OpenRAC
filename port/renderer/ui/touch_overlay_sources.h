// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Touch overlay GLSL shared by runtime drawing and the build-time shader compiler.

#pragma once

namespace openrac::shader_sources::renderer_ui_touch_overlay {
inline constexpr const char* kVertex = R"glsl(#version 410 core

uniform vec4 box;     // the control's box in device coordinates: centre x, y, half width, half height

uniform vec2 extent;  // its half width and height in radii

out vec2 v_at;        // across the box in radii from the centre, down positive

void main() {

    vec2 corner = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1)) * 2.0 - 1.0;

    v_at = vec2(corner.x, -corner.y) * extent;

    gl_Position = vec4(box.xy + corner * box.zw, 0.0, 1.0);

}

)glsl";
inline constexpr const char* kFragment = R"glsl(#version 410 core

in vec2 v_at;

uniform vec3 tint;     // the symbol's colour

uniform float held;    // 1 while a finger holds it

uniform float stretch; // half the width over half the height

uniform int symbol;    // see Symbol in touch_overlay.cpp

uniform vec2 knob;     // a stick's knob, -1..1 of the circle

uniform float angle;   // an arrow's direction, in radians clockwise from up

uniform int label0;    // two letters (letter()), -1 for none

uniform int label1;

uniform sampler2D tex_T0;  // the logo's mark, its coverage in alpha; or the icons (touch_icons.h)

uniform vec2 logo_half;    // the mark's half width and height in radii

uniform vec4 icon_rect;    // an icon's cell in the icons' texture: u0, v0, u1, v1

uniform float icon_half;   // its half size in radii

uniform vec3 accent;       // the game's accent (the launcher's, ui_colors.h): a held control's light

out vec4 colour;



// smoothstep() is undefined with equal edges: where d is flat across a pixel (fwidth 0, far from

// any shape, d = 1e3) some drivers return garbage, so the edge keeps a width.

float fill(float d) { float w = max(fwidth(d) * 0.75, 1e-5); return 1.0 - smoothstep(-w, w, d); }

float pill(vec2 p, float half_width, float r) {

    return length(vec2(max(abs(p.x) - half_width, 0.0), p.y)) - r;

}

float segment(vec2 p, vec2 a, vec2 b) {

    vec2 pa = p - a, ba = b - a;

    return length(pa - ba * clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0));

}



vec4 over(vec4 under, vec4 top) {

    float a = top.a + under.a * (1.0 - top.a);

    return vec4(a > 0.0 ? (top.rgb * top.a + under.rgb * under.a * (1.0 - top.a)) / a : vec3(0.0), a);

}

// Every button, round or a pill as wide as `stretch` says: a see-through slate face in the middle

// of a darker base, a clear rim, a light along the top edge. A press shrinks the face a little and

// warms it.

const vec3 kSlateBase = vec3(0.075, 0.075, 0.085);

const vec3 kSlateTop = vec3(0.205, 0.205, 0.235);

const vec3 kSlateBottom = vec3(0.150, 0.150, 0.172);

vec4 slate(vec2 p, float d) {

    vec4 c = vec4(kSlateBase, 0.38 * fill(d));

    c = over(c, vec4(mix(vec3(1.0), accent, 0.6 * held), (0.34 + 0.4 * held) * fill(abs(d + 0.025) - 0.022)));  // the rim

    float f = d + 0.12 + 0.04 * held;

    vec3 face = mix(kSlateTop, kSlateBottom, clamp(p.y * 0.5 + 0.5, 0.0, 1.0));

    face = mix(face, accent * 0.5, 0.30 * held);

    c = over(c, vec4(face, (0.42 + 0.25 * held) * fill(f)));

    float edge = fill(abs(f + 0.025) - 0.016) * (1.0 - smoothstep(-0.2, 0.3, p.y));

    c = over(c, vec4(vec3(1.0), 0.09 * edge));

    return c;

}



// A letter of the shoulders' names in strokes, in a box 1 wide and 2 high centred on p's origin:

// 0 L, 1 R, 2 one, 3 two.

float letter(int code, vec2 p) {

    float d = 1e3;

    if (code == 0) {

        d = min(segment(p, vec2(-0.35, -1.0), vec2(-0.35, 1.0)), segment(p, vec2(-0.35, 1.0), vec2(0.4, 1.0)));

    } else if (code == 1) {

        d = min(segment(p, vec2(-0.4, -1.0), vec2(-0.4, 1.0)), segment(p, vec2(-0.4, -1.0), vec2(0.0, -1.0)));

        d = min(d, segment(p, vec2(-0.4, 0.0), vec2(0.0, 0.0)));

        d = min(d, p.x > 0.0 ? abs(length(p - vec2(0.0, -0.5)) - 0.5) : 1e3);

        d = min(d, segment(p, vec2(0.0, 0.0), vec2(0.45, 1.0)));

    } else if (code == 2) {

        d = min(segment(p, vec2(0.05, -1.0), vec2(0.05, 1.0)), segment(p, vec2(-0.3, -0.65), vec2(0.05, -1.0)));

        d = min(d, segment(p, vec2(-0.3, 1.0), vec2(0.4, 1.0)));

    } else if (code == 3) {

        vec2 c = vec2(0.0, -0.45);

        d = (p.y < c.y || p.x > 0.0) && p.y < 0.0 ? abs(length(p - c) - 0.45) : 1e3;

        d = min(d, segment(p, vec2(0.35, -0.2), vec2(-0.42, 1.0)));

        d = min(d, segment(p, vec2(-0.42, 1.0), vec2(0.45, 1.0)));

    }

    return d;

}



void main() {

    vec2 p = v_at;

    float press = 1.0 - 0.06 * held;  // a held control sinks a little

    vec4 c = vec4(0.0);

    vec4 ink = vec4(tint, 0.92);

    if (symbol == 9) {

        // The d-pad's round base, as the stick's.

        float d = length(p) - 1.0;

        c = vec4(kSlateBase, 0.36 * fill(d));

        c = over(c, vec4(vec3(1.0), 0.30 * fill(abs(d + 0.02) - 0.018)));

    } else if (symbol == 6) {

        // An arm of the d-pad: a key pointed toward the middle, an arrow on it.

        float ca = cos(angle), sa = sin(angle);

        vec2 q = mat2(ca, -sa, sa, ca) * p / press;

        // The pad's middle is 1.5 radii inward (kArm): the keys' outer ends follow the base's circle.

        float key = max(max(abs(q.x) - 0.80, length(q - vec2(0.0, 1.5)) - 2.45),

                        (q.y + abs(q.x) - 1.02) * 0.7071);

        vec3 face = mix(kSlateTop, kSlateBottom, clamp(q.y * 0.5 + 0.5, 0.0, 1.0));

        face = mix(face, accent * 0.5, 0.30 * held);

        c = vec4(face, (0.55 + 0.2 * held) * fill(key));

        c = over(c, vec4(mix(vec3(1.0), accent, 0.6 * held), (0.30 + 0.4 * held) * fill(abs(key + 0.03) - 0.02)));

        vec2 t = q + vec2(0.0, 0.45);

        float tri = max(abs(t.x) * 0.9 - t.y * 0.45, t.y - 0.14) - 0.17;

        c = over(c, vec4(mix(vec3(0.93, 0.91, 0.88), accent, held), 0.9 * fill(tri)));

    } else if (symbol == 5) {

        // The stick: a round base, an arrow each way (the one it leans to lit), a round

        // knob with four dots.

        float d = length(p) - 1.0;

        c = vec4(kSlateBase, 0.36 * fill(d));

        c = over(c, vec4(vec3(1.0), 0.30 * fill(abs(d + 0.02) - 0.018)));

        c = over(c, vec4(mix(vec3(0.165, 0.165, 0.19), vec3(0.13, 0.13, 0.15), clamp(p.y * 0.5 + 0.5, 0.0, 1.0)), 0.38 * fill(d + 0.08)));

        c = over(c, vec4(vec3(1.0), 0.06 * fill(abs(d + 0.10) - 0.012) * (1.0 - smoothstep(-0.3, 0.3, p.y))));

        float lean = clamp(length(knob), 0.0, 1.0);

        for (int i = 0; i < 4; ++i) {

            float a = float(i) * 1.5708;

            vec2 dir = vec2(sin(a), -cos(a));

            vec2 t = vec2(dot(p, vec2(-dir.y, dir.x)), dot(p, dir) - 0.74);

            float tri = max(abs(t.x) * 0.95 + t.y * 0.55, -t.y - 0.05) - 0.055;

            float toward = lean * max(dot(normalize(knob + vec2(1e-5)), dir), 0.0);

            vec3 ink = mix(vec3(0.30, 0.30, 0.34), accent, smoothstep(0.3, 0.9, toward));

            c = over(c, vec4(ink, (0.9) * fill(tri)));

        }

        vec2 k = p - knob * 0.50;

        float kr = 0.46;

        float kd = length(k) - kr;

        c = over(c, vec4(kSlateBase, 0.55 * fill(kd)));

        vec2 kf = k;

        vec3 face = mix(vec3(0.235, 0.235, 0.27), vec3(0.18, 0.18, 0.205), clamp(kf.y / kr * 0.5 + 0.5, 0.0, 1.0));

        face = mix(face, accent * 0.6, 0.12 * held);

        c = over(c, vec4(face, 0.65 * fill(kd + kr * 0.14)));

        c = over(c, vec4(vec3(1.0), 0.08 * fill(abs(kd + kr * 0.16) - 0.012) * (1.0 - smoothstep(-0.2, 0.15, kf.y / kr))));

        for (int i = 0; i < 4; ++i) {

            float a = float(i) * 1.5708;

            c = over(c, vec4(vec3(0.36, 0.36, 0.40), 0.9 * fill(length(kf - vec2(sin(a), -cos(a)) * 0.17) - 0.035)));

        }

    } else {

        // A button: slate, its symbol or icon on it.

        float d = pill(p, stretch - 1.0, 1.0);

        vec2 q = p;

        c = slate(p, d);

        float s = 1e3;

        if (symbol == 1) {         // triangle

            vec2 t = q + vec2(0.0, -0.08);

            float e = max(abs(t.x) * 0.866 - t.y * 0.5, t.y) - 0.27;

            s = abs(e) - 0.045;

        } else if (symbol == 2) {  // circle

            s = abs(length(q) - 0.40) - 0.06;

        } else if (symbol == 3) {  // cross

            vec2 r = abs(q);

            s = min(segment(r, vec2(0.0), vec2(0.34, 0.34)), 1e3) - 0.065;

        } else if (symbol == 4) {  // square

            vec2 r = abs(q);

            s = abs(max(r.x, r.y) - 0.31) - 0.055;

        } else if (symbol == 7) {  // Select: a bar

            s = pill(q, 0.30, 0.13);

            ink.rgb = vec3(0.92);

        } else if (symbol == 8) {  // Start: a play arrow

            vec2 t = vec2(q.y, -q.x) + vec2(0.0, -0.04);

            s = max(abs(t.x) * 0.9 - t.y * 0.45, t.y - 0.16) - 0.20;

            ink.rgb = vec3(0.92);

        }

        // The port's options: the logo's mark, its coverage read from the picture.

        float mark = -1.0;

        if (symbol == 10) {

            vec2 uv = q / logo_half * 0.5 + 0.5;

            bool inside = all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)));

            float read = texture(tex_T0, clamp(uv, 0.0, 1.0)).a;  // outside any branch: see below

            mark = inside ? read : 0.0;

            ink.rgb = vec3(0.92);

        }

        // An action's icon: white ink and the accent's, their coverage read from the icons' texture.

        vec4 icon = vec4(0.0);

        if (symbol == 11) {

            // Read for every pixel, then masked: read only inside the square, the mip level at its

            // edge came from broken derivatives and drew a thin square around the icon (Adreno).

            vec2 uv = q / icon_half * 0.5 + 0.5;

            vec2 cover = texture(tex_T0, mix(icon_rect.xy, icon_rect.zw, clamp(uv, 0.0, 1.0))).rg;

            if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)))) {

                vec3 tint = mix(vec3(0.96), accent, cover.g / max(cover.r + cover.g, 1e-4));

                icon = vec4(tint, max(cover.r, cover.g));

            }

        }

        if (label0 >= 0) {        // the shoulders: their name

            const float size = 0.38;  // half a letter's height

            vec2 at = q / size;

            s = min(letter(label0, at + vec2(0.8, 0.0)), letter(label1, at - vec2(0.8, 0.0))) * size - 0.07;

            ink.rgb = vec3(mix(mp_vec3(0.95), mp_vec3(accent), mp_float(0.5) * mp_float(held)));

        }

        // The symbol: a faint shadow under it on a pill (the shoulders), then the ink.

        if (stretch > 1.0) {

            c = over(c, vec4(vec3(0.0), 0.45 * ink.a * fill(s - 0.02) * (1.0 - fill(s))));

        }

        ink.a *= 0.75 + 0.25 * held;

        if (symbol == 11) {

            c = over(c, vec4(icon.rgb, icon.a * (0.88 + 0.12 * held)));

        } else {

            c = over(c, vec4(ink.rgb, ink.a * (mark >= 0.0 ? mark : fill(s))));

        }

        // The glow of a press, outside the outline.

        c = over(vec4(accent, 0.25 * held * (1.0 - smoothstep(0.0, 0.18, d)) * step(0.0, d)), c);

    }

    if (c.a <= 0.002) { discard; }

    colour = c;

}

)glsl";
}  // namespace openrac::shader_sources::renderer_ui_touch_overlay

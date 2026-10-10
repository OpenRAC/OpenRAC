// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders/world_lights.wgsl:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Point lights on the world geometry: the point-light halves of the game's
// LightTfrags, LightTies and LightShrubs passes (NTSC-U 0x2a8e40, 0x2ab218,
// 0x29e7e8 in ReRAC's reading of level 1's code), added to the baked
// (directional) colours the static records hold. The game relights an object
// every frame its nibble list names a light, always from the baked colours,
// so what is drawn is the baked colour plus the listed lights.
//
// Colours are bytes (0x80 = 1.0): a light colour of 1.0 times a dot product
// of 1.0 adds 128. The game adds each term on the float grid 65536 + c / 128,
// i.e. c + floor(128 * colour * d), and clamps the RGB lanes (255 for tfrags,
// 243 for ties and shrubs). A nibble list names up to four bank slots, low
// nibble first; 0xf ends it, 0xffff names none.

layout(std140) uniform WorldLights {
    vec4 u_light_position[8];  // xyz game units, w radius
    vec4 u_light_colour[8];    // rgb colour, w intensity (the back factor: d -> max(d, d * w))
};

const uint kNoLights = 0xffffu;

// LightTfrags' point pass on one vertex: for each listed light whose radius
// reaches the vertex, d = (-N) . (P - L) * (1 - dist / r) / dist,
// d = max(d, d * w), then rgba += floor(128 * (colour, w) * d) (the
// intensity feeds alpha too), RGB clamped at 255. `c0` = the baked bytes,
// `n` = (N, 1 when the vertex has a light record).
vec4 tfrag_lit(uint list, vec4 c0, vec3 p, vec4 n) {
    if (n.w == 0.0) {
        return c0;
    }
    vec4 c = round(c0);
    uint l = list | 0xf0000u;
    for (int step = 0; step < 5; ++step) {
        uint i = l & 0xfu;
        if (i == 0xfu) {
            break;
        }
        l = l >> 4u;
        vec4 lp = u_light_position[i & 7u];
        vec4 lc = u_light_colour[i & 7u];
        vec3 v = p - lp.xyz;
        float d2 = dot(v, v);
        float r = lp.w;
        if (r * r - d2 < 0.0 || d2 <= 0.0) {
            continue;
        }
        float dist = sqrt(d2);
        float d = -dot(n.xyz, v) * ((1.0 - dist / r) / dist);
        d = max(d, d * lc.w);
        vec3 rgb = clamp(c.rgb + floor(128.0 * lc.rgb * d), vec3(0.0), vec3(255.0));
        float a = c.a + floor(128.0 * lc.w * d);
        c = vec4(rgb, a - 256.0 * floor(a / 256.0));
    }
    return c;
}

// The third light LightTies and LightShrubs merge from the listed lights that
// reach the instance's bounding-sphere centre: direction = the sum of the unit
// vectors (centre - L), renormalised only when two or more reach it; colour =
// sum of colour * (1 - dist / r); back factor = sum of w * (1 - dist / r).
struct InstanceLight {
    vec3 dir;
    vec3 col;
    float back;
    bool hit;
};

InstanceLight instance_light(uint list, vec3 centre) {
    InstanceLight o = InstanceLight(vec3(0.0), vec3(0.0), 0.0, false);
    int n = 0;
    uint l = list | 0xf0000u;
    for (int step = 0; step < 5; ++step) {
        uint i = l & 0xfu;
        if (i == 0xfu) {
            break;
        }
        l = l >> 4u;
        vec4 lp = u_light_position[i & 7u];
        vec4 lc = u_light_colour[i & 7u];
        vec3 v = centre - lp.xyz;
        float d2 = dot(v, v);
        float r = lp.w;
        if (r * r - d2 < 0.0) {
            continue;
        }
        float dist = sqrt(d2);
        float a = 1.0 - dist / r;
        if (dist > 0.0) {
            o.dir += v / dist;
        }
        o.col += lc.rgb * a;
        o.back += lc.w * a;
        n += 1;
    }
    float len2 = dot(o.dir, o.dir);
    if (n >= 2 && len2 > 0.0) {
        o.dir *= inversesqrt(len2);
    }
    o.hit = n > 0;
    return o;
}

// One colour of the instance (bytes) lit by the merged light on the class
// normal `nw` in world axes: d = -dir . nw, d = max(d, d * back),
// rgb += floor(128 * col * d), clamped at `top` (243).
vec3 instance_lit(vec3 c, InstanceLight il, vec3 nw, float top) {
    float d = -dot(il.dir, nw);
    d = max(d, d * il.back);
    return clamp(c + floor(128.0 * il.col * d), vec3(0.0), vec3(top));
}

// A packed RGBA8 colour with the merged light on class normal `n` (s16 /
// 32768) of an instance whose class to world matrix is `m`: the unit axis
// columns drop the scale, as the game's c * rsqrt(|c|^2).
uint instance_lit_packed(uint rgba, InstanceLight il, vec3 n, mat4 m, float top) {
    vec3 nw = normalize(m[0].xyz) * n.x + normalize(m[1].xyz) * n.y + normalize(m[2].xyz) * n.z;
    vec3 c = vec3(float(rgba & 0xffu), float((rgba >> 8u) & 0xffu), float((rgba >> 16u) & 0xffu));
    uvec3 lit = uvec3(instance_lit(c, il, nw, top));
    return lit.x | (lit.y << 8u) | (lit.z << 16u) | (rgba & 0xff000000u);
}

// A class normal packed as the raw s16 (x | y << 16, z).
vec3 class_normal(uvec2 p) {
    return vec3(float(int(p.x << 16u) >> 16), float(int(p.x) >> 16), float(int(p.y << 16u) >> 16)) / 32768.0;
}

#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders/tfrag.wgsl:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Terrain (tfrag): the VU1 terrain program's level-of-detail passes,
// replayed per vertex. Every triangle of all three strip lists is in the
// batch; this frame's draw mode of the vertex's tfrag (TfragProc, on the CPU:
// renderer/world/tfrag_lod.h) picks the list it draws, and a vertex of another
// list is dropped. The rest replays the VU1 program for the mode's entry, as
// ReRAC read it:
//
//   L18 (modes 8, 0xa, 0xe) morphs LOD-01 positions, L17 (0xe, 0x10) LOD-0
//   positions, after L18, so LOD-0 vertices blend toward already-morphed
//   LOD-01 parents in mode 0xe:
//     t.xy = clamp(w * qw66a.xy + qw66b.xy, 0, (0.5, 1)) = (u / 2, 1 - u),
//            w = the vertex's own VU w (depth * slope)
//     pos' = (P1 + P2) * t.x + pos * t.y, col' = ftoi0((C1 + C2) * t.x + col * t.y)
//   The VU blends clip-space xyzw, which is affine in the world position, so
//   blending world positions and projecting afterwards is the same map. UVs
//   do not morph.
//   L26 (mode 8, LOD-01) and L25 (mode 0xe, LOD-0): if both parents have
//   w < qw66a.w (deeper than D0, D1), the morphed position is not stored and
//   the vertex-info entry is replaced by parent 1's (UV and position).
//   L48 / L47: the same collapse test for the "extra" entries, after the morph.
// w is depth * slope with the fog slope (negative), the VU's w lane before
// the fog offset, so "w < threshold" means "deeper than D".
//
// Point lights (LightTfrags' point pass): when the tfrag's nibble list names
// any, every slot the vertex reads gets them added to its baked RGBA before
// the morph blends, as VU1 reads the relit RGBA block.
//
// The colour and F are interpolated in screen space like the GS (IIP = 1
// Gouraud: noperspective), ST perspective-correctly (divided by Q).

layout(location = 0) in uvec2 a_ref;  // vertex info, tfrag | list << 16 | TEX1 K (s12) << 18

uniform usamplerBuffer tex_T1;  // slots: (pos.xyz, rgba), (parent 1, parent 2, tier, normal)
uniform usamplerBuffer tex_T2;  // vertex infos: (s, t, slot, tier), (parent-1 info, parent-2 slot, 0, 0)
uniform usamplerBuffer tex_T3;  // per tfrag: (draw mode, light list, 0, 0)
uniform vec4 u_k666;
uniform vec4 u_k667;
uniform vec4 u_k668;
uniform vec4 u_k669;
uniform float u_w_slope;  // VU w per integer unit of depth

out vec2 v_uv;
noperspective out vec4 v_colour;  // bytes, 0x80 = 1.0
noperspective out float v_fog;
out float v_depth;            // integer units, perspective-correct (= n / Q per pixel)
flat out uint v_tint;         // debug: 0 none, 1 LOD 1, 2 LOD 2, 3 the clipping path
flat out float v_k;           // TEX1 K in mip levels

const uint kTierLod01 = 1u;
const uint kTierLod0 = 2u;

uint g_lights = kNoLights;

struct Vtx {
    vec3 pos;
    vec4 col;
};

// N = (cos az cos el, sin az cos el, sin el), 256 steps a turn; w = 1 when
// the slot has a light record.
vec4 slot_normal(uint p) {
    float a = float(p & 0xffu) * (6.2831853 / 256.0);
    float e = float((p >> 8u) & 0xffu) * (6.2831853 / 256.0);
    return vec4(cos(a) * cos(e), sin(a) * cos(e), sin(e), float((p >> 16u) & 1u));
}

float w_of(vec3 p) {
    return depth_int(p) * u_w_slope;
}

Vtx own(uint s) {
    uvec4 a = texelFetch(tex_T1, int(s * 2u));
    uvec4 b = texelFetch(tex_T1, int(s * 2u + 1u));
    vec3 pos = uintBitsToFloat(a.xyz);
    vec4 c = unpack_rgba(a.w);
    if (g_lights == kNoLights) {
        return Vtx(pos, c);
    }
    return Vtx(pos, tfrag_lit(g_lights, c, pos, slot_normal(b.w)));
}

Vtx blend(Vtx c, Vtx p1, Vtx p2, vec2 t) {
    return Vtx((p1.pos + p2.pos) * t.x + c.pos * t.y, floor((p1.col + p2.col) * t.x + c.col * t.y));
}

// A common or LOD-01 slot after L18 / L26 in mode m.
Vtx value01(uint s, uint m) {
    uvec4 link = texelFetch(tex_T1, int(s * 2u + 1u));
    Vtx c = own(s);
    if (link.z != kTierLod01 || !(m == 8u || m == 10u || m == 14u)) {
        return c;
    }
    Vtx p1 = own(link.x);
    Vtx p2 = own(link.y);
    if (m == 8u && w_of(p1.pos) < u_k666.w && w_of(p2.pos) < u_k666.w) {
        return c;
    }
    vec2 t = clamp(w_of(c.pos) * u_k666.xy + u_k668.xy, vec2(0.0), vec2(0.5, 1.0));
    return blend(c, p1, p2, t);
}

// Any slot after every morph pass of mode m.
Vtx value(uint s, uint m) {
    uvec4 link = texelFetch(tex_T1, int(s * 2u + 1u));
    if (link.z != kTierLod0) {
        return value01(s, m);
    }
    Vtx c = own(s);
    if (!(m == 14u || m == 16u)) {
        return c;
    }
    Vtx p1 = value01(link.x, m);
    Vtx p2 = value01(link.y, m);
    if (m == 14u && w_of(p1.pos) < u_k667.w && w_of(p2.pos) < u_k667.w) {
        return c;
    }
    vec2 t = clamp(w_of(c.pos) * u_k667.xy + u_k669.xy, vec2(0.0), vec2(0.5, 1.0));
    return blend(c, p1, p2, t);
}

void main() {
    uint tf = a_ref.y & 0xffffu;
    uint list = (a_ref.y >> 16u) & 3u;
    uvec4 frame = texelFetch(tex_T3, int(tf));
    uint m = frame.x;
    // The list the mode draws: LOD 2 for MSCAL 6, LOD 1 for 8 / 0xa, LOD 0
    // otherwise (the clipping path included).
    uint mlist = m == 6u ? 2u : ((m == 8u || m == 10u) ? 1u : 0u);
    if (m == 0u || mlist != list) {
        gl_Position = kCulled;
        v_uv = vec2(0.0);
        v_colour = vec4(0.0);
        v_fog = 1.0;
        v_depth = 1.0;
        v_tint = 0u;
        v_k = 0.0;
        return;
    }
    g_lights = frame.y & 0xffffu;
    uvec4 e0 = texelFetch(tex_T2, int(a_ref.x * 2u));
    uvec4 e1 = texelFetch(tex_T2, int(a_ref.x * 2u + 1u));
    // Collapse (L26 / L25, L48 / L47): both parents beyond the tier's distance.
    if ((m == 8u && e0.w == kTierLod01) || (m == 14u && e0.w == kTierLod0)) {
        float threshold = m == 8u ? u_k666.w : u_k667.w;
        uvec4 p0 = texelFetch(tex_T2, int(e1.x * 2u));
        uvec4 p1 = texelFetch(tex_T2, int(e1.x * 2u + 1u));
        if (w_of(value(p0.z, m).pos) < threshold && w_of(value(e1.y, m).pos) < threshold) {
            e0 = p0;
            e1 = p1;
        }
    }
    Vtx r = value(e0.z, m);
    gl_Position = world_to_clip(r.pos);
    v_uv = uintBitsToFloat(e0.xy);
    v_colour = r.col;
    float depth = depth_int(r.pos);
    v_depth = depth;
    v_fog = fog_coefficient(depth);
    v_k = tex1_k(a_ref.y >> 18u);
    v_tint = u_screen.w > 0.5 ? (m == 2u ? 3u : list) : 0u;
}

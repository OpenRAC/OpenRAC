#version 410 core
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-engine/assets/shaders/tie.wgsl:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Ties: instanced class meshes, coloured from the instance's LightTies slot
// table, GS TFX = MODULATE, one GS fog F per instance, the GS mip rule.
//
// Per instance (tex_T1, 54 texels): the class to world matrix (it may scale,
// shear and mirror), the bounding-sphere centre and draw distance, the
// radius, the 64 lit slot colours and the 64 class normals the point lights
// use. Per frame (tex_T2, 2 texels): TieProc's decision (renderer/world/
// tie_lod.h): x = LOD (3 culled) | F << 8, then k, qw4.w = 256k and qw4.z =
// 256 - 256k as float bits; then the instance's point-light list. Vertices
// of the other LODs are dropped.
//
// Fat vertices, as ReRAC read program 13507: position + k * delta; colour per
// lane = the low byte of avg * w + c0 * z with avg = (c1 + c2) * 0.5, all in
// VU floats with the palette lanes as 0x4b000000 + byte. The GPU has no such
// floats, so vu_mul / vu_add do the console's truncating arithmetic in
// integers (checked against the float model on the CPU:
// renderer/world/vu_float.h mul_positive, add_positive). Dinky vertices take
// their slot colour as it is.
//
// Point lights (LightTies' third light): when the instance's list names any,
// every slot colour the vertex reads (c0, and c1 / c2 of a fat vertex, before
// the VU blend) gets the merged light on its class normal, clamped at 243.

layout(location = 0) in vec3 a_position;  // class space
layout(location = 1) in vec3 a_delta;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in uint a_info;      // light slot | TEX1 K (s12) << 20
layout(location = 4) in uint a_morph;     // morph slot 1 | slot 2 << 6 | fat << 12 | LOD << 13
layout(location = 5) in uint a_instance;  // per instance

uniform usamplerBuffer tex_T1;  // instance records
uniform usamplerBuffer tex_T2;  // per-frame records

out vec2 v_uv;
noperspective out vec4 v_colour;
flat out float v_fog;
out float v_depth;
flat out float v_k;
flat out uint v_tint;

const int kRecord = 54;

uint vu_mul(uint a, uint b) {
    int ea = int((a >> 23u) & 0xffu);
    int eb = int((b >> 23u) & 0xffu);
    if (ea == 0 || eb == 0) {
        return 0u;
    }
    uint ma = (a & 0x7fffffu) | 0x800000u;
    uint mb = (b & 0x7fffffu) | 0x800000u;
    uint ah = ma >> 12u;
    uint al = ma & 0xfffu;
    uint bh = mb >> 12u;
    uint bl = mb & 0xfffu;
    uint mid = ah * bl + al * bh;
    uint low = ((mid & 0xfffu) << 12u) + al * bl;
    uint top = ah * bh + (mid >> 12u) + (low >> 24u);
    uint m = top;
    int e = ea + eb - 126;
    if (top < 0x800000u) {
        m = (top << 1u) | ((low >> 23u) & 1u);
        e = ea + eb - 127;
    }
    if (e < 1) {
        return 0u;
    }
    if (e > 255) {
        return 0x7fffffffu;
    }
    return (uint(e) << 23u) | (m & 0x7fffffu);
}

uint vu_add(uint a, uint b) {
    int ea = int(a >> 23u);
    int eb = int(b >> 23u);
    if (ea == 0) {
        return b;
    }
    if (eb == 0) {
        return a;
    }
    uint hi = a;
    uint lo = b;
    int eh = ea;
    int el = eb;
    if (eb > ea) {
        hi = b;
        lo = a;
        eh = eb;
        el = ea;
    }
    uint d = uint(eh - el);
    if (d >= 25u) {
        return hi;
    }
    uint s = ((hi & 0x7fffffu) | 0x800000u) + (((lo & 0x7fffffu) | 0x800000u) >> d);
    if (s >= 0x1000000u) {
        return (uint(eh + 1) << 23u) | ((s >> 1u) & 0x7fffffu);
    }
    return (uint(eh) << 23u) | (s & 0x7fffffu);
}

uint vu_fat_lane(uint c0, uint c1, uint c2, uint w, uint z) {
    uint avg = vu_add(vu_mul(0x4b000000u | c1, 0x3f000000u), vu_mul(0x4b000000u | c2, 0x3f000000u));
    return vu_add(vu_mul(avg, w), vu_mul(0x4b000000u | c0, z)) & 0xffu;
}

uint slot_colour(int base, uint j) {
    uvec4 t = texelFetch(tex_T1, base + 6 + int(j >> 2u));
    return t[j & 3u];
}

uvec2 slot_normal(int base, uint j) {
    uvec4 t = texelFetch(tex_T1, base + 22 + int(j >> 1u));
    return (j & 1u) == 0u ? t.xy : t.zw;
}

void main() {
    int base = int(a_instance) * kRecord;
    uvec4 lod = texelFetch(tex_T2, int(a_instance) * 2);
    uint vlod = (a_morph >> 13u) & 3u;
    v_uv = a_uv;
    v_k = 0.0;
    v_tint = 0u;
    if ((lod.x & 3u) != vlod) {
        // Culled this frame (3), or a vertex of another LOD.
        gl_Position = kCulled;
        v_colour = vec4(0.0);
        v_fog = 1.0;
        v_depth = 1.0;
        return;
    }
    mat4 model = mat4(
        as_float(texelFetch(tex_T1, base)),
        as_float(texelFetch(tex_T1, base + 1)),
        as_float(texelFetch(tex_T1, base + 2)),
        as_float(texelFetch(tex_T1, base + 3))
    );
    vec3 centre = as_float(texelFetch(tex_T1, base + 4)).xyz;
    float k = uintBitsToFloat(lod.y);
    vec3 world = (model * vec4(a_position + k * a_delta, 1.0)).xyz;
    gl_Position = world_to_clip(world);

    uint list = texelFetch(tex_T2, int(a_instance) * 2 + 1).x & 0xffffu;
    InstanceLight il = InstanceLight(vec3(0.0), vec3(0.0), 0.0, false);
    if (list != kNoLights) {
        il = instance_light(list, centre);
    }
    uint s0 = a_info & 63u;
    uint c0 = slot_colour(base, s0);
    if (il.hit) {
        c0 = instance_lit_packed(c0, il, class_normal(slot_normal(base, s0)), model, 243.0);
    }
    uint rgba = c0;
    if (((a_morph >> 12u) & 1u) != 0u) {
        uint s1 = a_morph & 63u;
        uint s2 = (a_morph >> 6u) & 63u;
        uint c1 = slot_colour(base, s1);
        uint c2 = slot_colour(base, s2);
        if (il.hit) {
            c1 = instance_lit_packed(c1, il, class_normal(slot_normal(base, s1)), model, 243.0);
            c2 = instance_lit_packed(c2, il, class_normal(slot_normal(base, s2)), model, 243.0);
        }
        rgba = 0u;
        for (uint i = 0u; i < 32u; i += 8u) {
            uint lane = vu_fat_lane((c0 >> i) & 0xffu, (c1 >> i) & 0xffu, (c2 >> i) & 0xffu, lod.z, lod.w);
            rgba |= lane << i;
        }
    }
    v_colour = unpack_rgba(rgba);
    v_depth = depth_int(world);
    // Program 13507 writes the same F for every vertex of an instance.
    v_fog = float(lod.x >> 8u) / 255.0;
    v_k = tex1_k(a_info >> 20u);
    v_tint = u_screen.w > 0.5 ? vlod : 0u;
}

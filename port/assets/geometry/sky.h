// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sky.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The RAC1 level sky: up to eight concentric shells of small indexed-triangle
// clusters, and the sky's own 8-bit textures (ReRAC
// docs/formats/shrub_sky_rac1.md part 2, docs/plan/sky_render_notes.md).
//
// How the game draws it, which is not Wrench's model: the sky is no VU1
// program. The EE builds GS packets with VU0 macro code and sends them over
// PATH2. sky_draw_shell (NTSC-U boot 0x22b690) draws a shell textured when
// its flags word is 0 and gouraud otherwise. Per cluster it DMAs the header's
// data_size bytes to scratchpad, transforms every vertex, then emits three GS
// vertices per face in stored index order as a plain triangle list: textured,
// ST = u16 / 4096 and RGBAQ = (0x80, 0x80, 0x80, vertex alpha), TEX0 re-sent
// whenever the face texture changes; gouraud, RGBAQ = the vertex's attribute
// word as it is. For gouraud shells the "ST" array is the vertex colour.

#pragma once

#include "assets/bytes.h"
#include "assets/geometry/mesh.h"
#include "assets/geometry/texture.h"

#include <array>
#include <string>
#include <vector>

namespace openrac::assets::rac1 {

// The sky block header (0x40 bytes); offsets are block-relative.
struct SkyHeader {
    std::array<u8, 4> colour{};  // RGBA, 0x80 = 1.0; not read by the shell code
    // 0x04: the frame-clear gate; the loader overwrites it with 1, so the disc
    // value is dead.
    s16 clear_screen = 0;
    s16 shell_count = 0;            // 0x06: 0..8, drawn in order
    s16 sprite_count = 0;           // 0x08: run time
    s16 maximum_sprite_count = 0;   // 0x0a: sprite slots (0x20 bytes each)
    s16 texture_count = 0;          // 0x0c
    s16 fx_count = 0;               // 0x0e: bytes at fx_list
    s32 texture_defs = 0;           // 0x10: SkyTextureDef array
    s32 texture_data = 0;           // 0x14: base of the palettes and pixels
    s32 fx_list = 0;                // 0x18: fx_count texture indices
    s32 sprites = 0;                // 0x1c
    std::array<s32, 8> shells{};    // 0x20: shell header offsets
};
static_assert(sizeof(SkyHeader) == 0x40);

// A texture definition as the disc stores it (0x10 bytes).
struct SkyTextureDef {
    s32 palette_offset = 0;  // 256 x RGBA32, CSM1, relative to texture_data
    s32 texture_offset = 0;  // width * height PSMT8 indices, relative to texture_data
    s32 width = 0;           // a power of two
    s32 height = 0;
};
static_assert(sizeof(SkyTextureDef) == 0x10);

// A cluster header (0x20 bytes).
struct SkyClusterHeader {
    std::array<f32, 4> bsphere{};  // for the per-cluster cull, vertex units
    s32 data = 0;                  // the cluster data, block-relative
    s16 vertex_count = 0;
    s16 tri_count = 0;
    s16 vertex_offset = 0;  // within data (0 on the disc)
    s16 st_offset = 0;      // the attribute array within data
    s16 tri_offset = 0;     // the face array within data
    s16 data_size = 0;      // bytes the game DMAs; every array lies inside it
};
static_assert(sizeof(SkyClusterHeader) == 0x20);

// A vertex (8 bytes); the game uses only its direction (the sky is drawn
// with GS Z = 0).
struct SkyVertex {
    s16 x = 0;
    s16 y = 0;
    s16 z = 0;      // up
    s16 alpha = 0;  // textured shells: the low byte is the vertex alpha (0x80 = 1.0)

    bool operator==(const SkyVertex&) const = default;
};
static_assert(sizeof(SkyVertex) == 8);

// The per-vertex attribute word: ST for textured shells, RGBA for gouraud ones.
struct SkyVertexAttr {
    std::array<u8, 4> bytes{};

    // (s, t) as the game reads them: zero-extended u16 (pextlh with zero), 4.12.
    std::array<u16, 2> st_raw() const {
        return {static_cast<u16>(bytes[0] | bytes[1] << 8), static_cast<u16>(bytes[2] | bytes[3] << 8)};
    }

    std::array<f32, 2> st() const {
        const auto r = st_raw();
        return {r[0] / 4096.0f, r[1] / 4096.0f};
    }

    std::array<u8, 4> rgba() const { return bytes; }
};
static_assert(sizeof(SkyVertexAttr) == 4);

// A face: cluster-local vertex indices and a texture definition index (0xff
// in gouraud shells). The GS draws it with no culling: winding means nothing.
struct SkyFace {
    std::array<u8, 3> indices{};
    u8 texture = 0;
};
static_assert(sizeof(SkyFace) == 4);

struct SkyCluster {
    SkyClusterHeader header;
    std::vector<SkyVertex> vertices;
    std::vector<SkyVertexAttr> attrs;
    std::vector<SkyFace> faces;
};

// A shell: {s32 cluster_count, s32 flags}, 8 zero bytes, the cluster headers.
// RAC1 has no per-shell rotation, angular velocity or bloom in the data.
struct SkyShell {
    s32 cluster_count = 0;
    s32 flags = 0;  // 0 = textured; any other value = gouraud (0 or 1 on the disc)
    std::vector<SkyCluster> clusters;

    // What sky_draw_shell tests: the whole word, not bit 0.
    bool textured() const { return flags == 0; }
};

struct Sky {
    SkyHeader header;
    std::vector<u8> fx_list;
    std::vector<SkyTextureDef> texture_defs;
    std::vector<SkyShell> shells;
};

// One GS vertex of a cluster's triangle list, three per face in face order.
struct SkyGsVertex {
    std::array<s16, 3> position{};  // raw (x, y, z up)
    u8 texture = 0xff;              // the texture definition; 0xff for gouraud shells
    std::array<f32, 2> st{};        // Q = 1; 0 for gouraud shells
    // GS RGBA, 0x80 = 1.0: textured (0x80, 0x80, 0x80, vertex alpha)
    // modulating the texel; gouraud the vertex colour.
    std::array<u8, 4> rgba{};

    bool operator==(const SkyGsVertex&) const = default;
};

// The vertex stream SkyDrawShellTextured / SkyDrawShellGouraud build for one
// cluster, before the view-dependent parts (transform, rejecting a face whose
// three vertices share an outside clip flag). Triangle k is 3k..3k+3.
std::vector<SkyGsVertex> sky_gs_vertices(const SkyShell& shell, const SkyCluster& cluster);

// One shell as a plain mesh: positions are the raw vertex directions / 1024,
// colours and UVs as the GS gets them; triangles carry the texture definition
// (-1 for gouraud).
Mesh sky_shell_mesh(const SkyShell& shell);

// A whole sky block (the core data's "sky" block).
Sky parse_sky(ByteView block);

// A decoded sky texture; `index` is the definition index faces name.
struct SkyTexture {
    std::size_t index = 0;
    SkyTextureDef def;
    RgbaImage image;

    std::string key() const;  // "sky/03_128x128"
};

IndexedImage sky_texture_image(ByteView block, const Sky& sky, std::size_t index);

// Every definition in table order (the FX textures first). PSMT8 with a CSM1
// palette and 0x80 alpha, exactly like level textures.
std::vector<SkyTexture> decode_sky_textures(ByteView block, const Sky& sky);

}  // namespace openrac::assets::rac1

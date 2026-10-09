// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/moby.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// RAC1 moby classes: the animated characters and dynamic objects (ReRAC
// docs/formats/moby_rac1.md with the corrections of
// docs/plan/moby_skinning_lighting.md). A class blob holds a header, a
// packet table of high-LOD, low-LOD and metal ("shine") packets, the
// skeleton and the animation sequences (moby_animation.h).
//
// What the reader resolves at load, so a renderer never replays the PS2
// machinery:
//
//   - the 9-bit vertex-cache ids, stored seven vertices late (spec 2.9), and
//     the duplicate vertices that copy a cached one (2.8);
//   - the VU0 matrix-slot machine that blends joints (skinning doc 4-5) into
//     a per-vertex MobySkin: up to three joint-palette indices with integer
//     weights summing to 256;
//   - the index stream (secret indices, texture switches, the flush trailer)
//     into triangles.
//
// The vertex cache, the VU0 slots and the GS texture carry from packet to
// packet within one LOD list. EE addresses are NTSC-U.

#pragma once

#include "assets/bytes.h"
#include "assets/geometry/core_records.h"
#include "assets/geometry/mesh.h"

#include <array>
#include <optional>
#include <span>
#include <vector>

namespace openrac::assets::rac1 {

// The class header at byte 0 of a class blob (0x48 bytes). Pointers are
// blob-relative; 0 is absent.
struct MobyClassHeader {
    s32 packet_table_offset = 0;  // 0x00: 0 = no mesh
    u8 high_lod_count = 0;        // 0x04: first in the table
    u8 low_lod_count = 0;         // 0x05: after them
    u8 metal_count = 0;           // 0x06
    u8 metal_begin = 0;           // 0x07: high + low on the disc
    u8 joint_count = 0;           // 0x08: with the high LOD (skeleton size)
    u8 low_lod_joint_count = 0;   // 0x09
    u8 glow_high = 0;             // 0x0a: first glow packet of the high LOD (0xff / past the end: none)
    u8 glow_low = 0;              // 0x0b: the same for the low LOD
    u8 sequence_count = 0;        // 0x0c: the pointer list at 0x48
    u8 sound_count = 0;           // 0x0d
    u8 lod_trans = 0;             // 0x0e: low LOD beyond lod_trans * 1024 depth
    u8 shadow = 0;                // 0x0f: quadwords of the shadow block before `skeleton`
    s32 collision = 0;            // 0x10: the collision blob (moby_collision.h)
    s32 skeleton = 0;             // 0x14: joint_count matrices (inverse bind)
    s32 common_trans = 0;         // 0x18: joint_count x MobyTrans
    s32 joints = 0;               // 0x1c: joint lists (spec 3.3)
    s32 gif_usage = 0;            // 0x20
    f32 scale = 0;                // 0x24: world units = packed * scale / 1024
    s32 sound_defs = 0;           // 0x28
    u8 bangles = 0;               // 0x2c: quadwords (0 on every RAC1 class)
    u8 mip_dist = 0;              // 0x2d
    s16 short_2e = 0;
    std::array<f32, 4> bsphere{};  // 0x30: world units
    // 0x40: glow colour (RGBA, R low); non-zero gives the moby mode 0x10.
    s32 glow_rgba = 0;
    s16 mode_bits = 0;  // 0x44
    u8 type = 0;        // 0x46
    u8 mode_bits2 = 0;  // 0x47
};
static_assert(sizeof(MobyClassHeader) == 0x48);

// A packet table entry (0x10 bytes).
struct MobyPacketEntry {
    u32 vif_list_offset = 0;          // blob-relative
    u16 vif_list_size = 0;            // quadwords
    u16 vif_list_texture_unpack = 0;  // quadword of the ad-gif unpack in the list (0 = none)
    u32 vertex_offset = 0;            // blob-relative: the vertex table header
    u8 vertex_data_size = 0;          // quadwords, header included
    u8 positions_qwc = 0;             // (0xf + 6 * transfer_vertex_count) / 16
    u8 colours_qwc = 0;               // (3 + transfer_vertex_count) / 4
    u8 transfer_vertex_count = 0;     // vertices sent to VU1 (in-file + duplicates)
};
static_assert(sizeof(MobyPacketEntry) == 0x10);

// A regular packet's vertex table header (0x20 bytes).
struct MobyVertexTableHeader {
    u32 matrix_transfer_count = 0;
    u32 two_way_blend_vertex_count = 0;    // first
    u32 three_way_blend_vertex_count = 0;  // next
    u32 main_vertex_count = 0;             // single-matrix, last
    u32 duplicate_vertex_count = 0;
    u32 transfer_vertex_count = 0;  // in-file + duplicates
    u32 vertex_table_offset = 0;    // the vertex array, from this header
    u32 multipliers_offset = 0;     // the RGBA multipliers, from this header (ends the epilogue)
};
static_assert(sizeof(MobyVertexTableHeader) == 0x20);

// A metal packet's vertex table header (0x10 bytes).
struct MobyMetalVertexTableHeader {
    s32 vertex_count = 0;  // 16-byte vertices follow
    s32 positions_out = 0;
    s32 colours_out = 0;
    s32 output_size = 0;
};
static_assert(sizeof(MobyMetalVertexTableHeader) == 0x10);

// A pre-loop matrix transfer: VU0[vu0_dest_addr] = palette[joint].
struct MobyMatrixTransfer {
    u8 joint = 0;
    u8 vu0_dest_addr = 0;  // a multiple of 4

    bool operator==(const MobyMatrixTransfer&) const = default;
};
static_assert(sizeof(MobyMatrixTransfer) == 2);

// A joint's common_trans record (0x10 bytes).
struct MobyTrans {
    std::array<f32, 3> vector{};  // the rest translation relative to the parent, packed units
    u16 parent_offset = 0;        // parent * 0x40: with `seventy`, the SPR word 0x70000000 + 0x40 * parent
    u16 seventy = 0;              // 0x7000 for a joint with a parent
};
static_assert(sizeof(MobyTrans) == 0x10);

// A vertex's skin: position = sum_k weights[k] / 256 * (F[joints[k]] * p),
// with F the moby's joint palette (F_j = pose_j * skeleton_j). Only the first
// `count` entries mean anything.
struct MobySkin {
    u8 count = 0;
    std::array<u8, 3> joints{};
    std::array<u16, 3> weights{};  // sum 256

    static MobySkin joint(u8 j) { return {1, {j, 0, 0}, {256, 0, 0}}; }

    bool operator==(const MobySkin&) const = default;
};

enum class MobyVertexKind : u8 {
    TwoWay = 1,
    ThreeWay = 2,
    Single = 3,
    Metal = 4,
};

// A vertex as sent to VU1.
struct MobyVertex {
    // Regular: bytes 0-7 of the record (the skinning bytes). Metal: bytes 8-15
    // (joint[3], count, weight[3], pad). Duplicates carry their source's.
    std::array<u8, 8> raw{};
    u8 normal_azimuth = 0;
    u8 normal_elevation = 0;
    s16 x = 0;  // packed; world units = value * scale / 1024
    s16 y = 0;
    s16 z = 0;
    u16 id = 0;  // the 9-bit cache id (metal: the vertex number)
    MobyVertexKind kind = MobyVertexKind::Single;
    bool duplicate = false;
    MobySkin skin;
    std::array<s16, 2> st{};  // 4.12; 0 for metal (sphere-mapped at run time)

    bool operator==(const MobyVertex&) const = default;
};

// A triangle: indices into the packet's vertices in index-stream order
// (strip positions n-2, n-1, n; winding not normalised) and the texture in
// effect when it was kicked.
struct MobyTriangle {
    u32 a = 0;
    u32 b = 0;
    u32 c = 0;
    // TEX0 data_lo: the class texture slot, or -1 none, -2 chrome, -3 glass.
    s32 texture = -1;

    bool operator==(const MobyTriangle&) const = default;
};

// One packet ("submesh"): up to about 0x60 vertices VU1 processes at once.
struct MobyPacket {
    MobyPacketEntry entry;
    MobyVertexTableHeader vertex_table;      // zero for metal packets
    MobyMetalVertexTableHeader metal_header;  // zero for regular packets
    std::vector<MobyMatrixTransfer> transfers;
    std::vector<u16> duplicates;  // raw entries (cache id << 7)
    std::vector<u8> raw_vertices;  // the records as stored
    // An RGBA lighting multiplier per transfer vertex (0x80 = 1.0, every one
    // on the disc), zero-padded to 16 bytes. Empty for metal.
    std::vector<u8> rgba_multipliers;
    std::vector<MobyVertex> vertices;  // in-file, then duplicates
    std::vector<std::array<s16, 2>> st;  // the raw ST unpack; may be longer than vertices
    std::vector<u8> index_bytes;  // after the 4-byte index header, terminator and padding included
    // Index header byte 2, then byte 0xc of each ad-gif block's quadwords.
    std::vector<u8> secret_indices;
    std::vector<s32> texture_indices;  // TEX0 data_lo of each ad-gif block, in switch order
    s32 initial_texture = -1;  // the GS texture left by the previous packet of the list
    u32 unresolved_duplicates = 0;  // duplicates of a never-written cache slot (0 on the disc)
    std::vector<MobyTriangle> triangles;
    bool is_metal = false;

    // One RGBA multiplier per vertex (in-file then duplicates).
    std::vector<std::array<u8, 4>> rgba_multiplier_records() const;
};

struct MobySkeleton {
    // Row-major as stored: rows 0-2 rotation, row 3 translation in packed units.
    std::vector<std::array<std::array<f32, 4>, 4>> matrices;
    std::vector<MobyTrans> trans;

    // parent_offset / 0x40; joint 0 is the root.
    std::optional<std::size_t> parent(std::size_t joint) const;
};

struct MobyClass {
    MobyClassHeader header;
    std::vector<s32> sequence_pointers;
    std::vector<MobyPacket> high_lod;
    std::vector<MobyPacket> low_lod;
    std::vector<MobyPacket> metal;
    MobySkeleton skeleton;

    // The bind-pose position in world units (no skinning, no instance).
    std::array<f32, 3> position(const MobyVertex& v) const;
};

// The cache id of a duplicate entry (index << 7).
inline u16 duplicate_cache_id(u16 entry) { return static_cast<u16>((entry >> 7) & 0x1ff); }

// Undoes the 7-vertex id shift: record i carries the id of vertex i - 7; the
// last ids continue in the epilogue records and then in the six u16 at bytes
// 4..16 of the last record. `records` are the in-file + epilogue records.
std::vector<u16> vertex_cache_ids(std::span<const std::array<u8, 16>> records, std::size_t in_file);

// VU0 data memory of program 104691 as matrix slots: each quadword address a
// matrix was stored at holds a palette matrix (a transfer) or a 2- or 3-way
// blend of them. Carried across the packets of one LOD list.
class Vu0Slots {
public:
    void store(u8 address, const MobySkin& skin);
    MobySkin load(u8 address) const;

    // Runs one regular vertex record (bytes 0-7) through the slot machine.
    // Two-way: store palette[b1 >> 1] at b6, blend b2/b3 by b4/b5, store at b7.
    // Three-way: blend b2/b3/(b1 & 0xfe) by b4/b5/b6, store at b7. Single:
    // store palette[b1 >> 1] at b3, then load b2. Stores come before loads.
    MobySkin vertex(MobyVertexKind kind, const std::array<u8, 8>& record);

private:
    u8 load_joint(u8 address) const;

    std::array<std::optional<MobySkin>, 256> m_slots{};
};

// A metal vertex's skin from its bytes 8-15: count <= 1 is joint byte 8,
// otherwise `count` joints whose weights sum to 256.
MobySkin metal_skin(const std::array<u8, 8>& record);

// The index stream as triangles (spec 2.4, corrected): indices are 1-based,
// bit 7 means no GS kick; a 0 byte switches to the next ad-gif block's texture
// and pushes the next secret index (never kicked); a 0 secret index ends the
// packet, and the last three pushes (the 1, 1, 1 flush trailer) never reach
// the GS. A kicked push n draws (n-2, n-1, n). Starts with initial_texture.
std::vector<MobyTriangle> moby_triangles(const MobyPacket& packet);

// One moby class blob (a moby_class block of the core data, or a
// decompressed gadget).
MobyClass parse_moby_class(ByteView blob);

// A LOD list as a plain mesh in bind pose, class units (packed * scale /
// 1024), with each vertex's skin alongside. Colours are left at 1.0 (they
// are lit per moby, moby_lighting.h); triangles carry the class texture slot
// or -1 / -2 / -3.
struct MobyMesh {
    Mesh mesh;
    std::vector<MobySkin> skins;  // one per mesh vertex
};

MobyMesh moby_mesh(const MobyClass& moby, std::span<const MobyPacket> packets);

// A level moby class: its core table entry and the parsed blob.
struct LevelMobyClass {
    CoreClassEntry entry;
    MobyClass moby;
};

// Every class of the moby class table with a blob (entries with offset 0 have
// none), in table order; `core_data` is the whole decompressed core data.
std::vector<LevelMobyClass> parse_level_moby_classes(std::span<const CoreClassEntry> table, ByteView core_data);

}  // namespace openrac::assets::rac1

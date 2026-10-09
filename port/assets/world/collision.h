// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/collision.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A level's static collision. Specification: ReRAC's
// docs/formats/collision_rac1.md; the game's queries over it are
// collision_query.h.
//
// rac1 has one collision block per level, at core header +0x14 in the
// decompressed core data. It holds the baked world-space mesh of all static
// geometry, bucketed into a sparse grid of 4 x 4 x 4-unit cells reached
// through a tree indexed Z, then Y, then X, followed by the hero-only
// collision groups (invisible walls the player alone collides with). Every
// cell has its own vertices and faces: a face overlapping N cells is stored N
// times, so a point query reads one cell. Tree offsets (slab, row and leaf) are
// byte offsets from the start of the mesh, not of the block. A moby class's own
// collision is a separate format (moby class header +0x10), not read here.

#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include "assets/bytes.h"
#include "assets/version.h"

namespace openrac::assets {

// A cell spans [4c, 4c + 4) on each axis and has its centre at 4c + 2.
inline constexpr float kCollisionCellSize = 4.0f;
// The game rejects any query not inside [0, 1024)^3 before reading the grid
// (ReRAC's docs/plan/collision_queries.md section 2).
inline constexpr float kCollisionWorldSize = 1024.0f;

// The header of each of the three tree levels: the cell coordinate of entry 0
// on the node's axis (Z for the root, Y for a slab, X for a row) and the entry
// count.
struct CollisionNode {
    s16 base = 0;
    u16 count = 0;
};

// A cell-relative vertex packed into a word. Signed fields: X in bits 0..9 at
// 1/16, Y in bits 10..19 at 1/16, Z in bits 20..31 at 1/64, each relative to
// the cell centre. The scales are Wrench's, confirmed on the disc by the
// collision map lying on the terrain (ReRAC's spec, section 6b).
struct PackedCollisionVertex {
    u32 word = 0;

    // The raw fields {x, y, z}: x and y in -512..511, z in -2048..2047.
    std::array<s32, 3> fields() const;

    // The offset from the cell centre in world units (exact in a float).
    std::array<float, 3> offset() const;

    // centre + offset per component, as the game computes it.
    std::array<float, 3> world(const std::array<float, 3>& centre) const;

    // The inverse of fields() for values in range.
    static PackedCollisionVertex pack(s32 x, s32 y, s32 z);
};

// One face record: three indices into its cell's vertices and the collision
// type byte. Bits 0-4 of the type are the surface id (0x1f: none), bits 5-6 the
// footstep sound class, and bit 7 marks faces queries with flag 0x80 skip.
struct CollisionFace {
    std::array<u8, 3> v{};
    u8 type = 0;

    // The game's surface-id getter (NTSC-U level01 0x2151d8): type & 0x1f, -1
    // for 0x1f.
    int surface_id() const;

    // The game's footstep sound-class getter (NTSC-U level01 0x215208):
    // (type & 0x60) >> 5, with 3 read as 0.
    int sound_class() const;

    bool high_bit() const { return (type & 0x80) != 0; }
};

// A leaf's header. The leaf goes on with vertex_count packed vertices,
// face_count faces (quads first), quad_count fourth indices, and zero padding
// to 16 bytes.
struct CollisionLeafHeader {
    u16 face_count = 0;
    u8 vertex_count = 0;
    u8 quad_count = 0;  // the first quad_count faces are quads

    // 4 + 4V + 4F + Q.
    std::size_t payload_size() const;

    // The leaf's size in 16-byte units, which the tree word's low byte holds.
    std::size_t size_qwords() const { return (payload_size() + 15) / 16; }
};

// One non-empty cell, a tree leaf, with its payload.
struct CollisionCell {
    s16 x = 0;
    s16 y = 0;
    s16 z = 0;
    // The tree entry: leaf offset from the mesh << 8 | leaf size in quadwords.
    u32 leaf_word = 0;
    CollisionLeafHeader header;
    std::vector<PackedCollisionVertex> packed;
    std::vector<std::array<float, 3>> vertices;  // world positions of `packed`
    std::vector<CollisionFace> faces;            // quads first, then triangles
    std::vector<u8> quad_v3;                     // the fourth index of each quad

    u32 leaf_offset() const { return leaf_word >> 8; }

    u32 leaf_qwords() const { return leaf_word & 0xff; }

    // 4c + 2 per axis.
    std::array<float, 3> centre() const;

    // The low corner 4c of [4c, 4c + 4). Faces may reach past the cell (a
    // vertex can lie up to 32 units from the centre); the cell owns what
    // overlaps it.
    std::array<float, 3> low_corner() const;

    bool is_quad(std::size_t face) const { return face < header.quad_count; }

    // The face's four indices; a triangle repeats v0 as the fourth.
    std::array<u8, 4> face_indices(std::size_t face) const;
};

// Hero collision: a flat list of groups with bounding spheres.
struct HeroCollisionGroup {
    std::array<u16, 4> sphere{};  // x, y, z, radius, unsigned at 1/64
    u16 triangle_count = 0;
    u16 vertex_count = 0;
    u32 data = 0;                              // offset of the data from the hero section
    std::vector<std::array<u16, 3>> vertices;  // absolute, unsigned at 1/64
    std::vector<std::array<u8, 3>> triangles;  // group-local indices; no surface type

    std::array<float, 4> sphere_world() const;

    std::array<float, 3> vertex_world(std::size_t i) const;
};

struct CollisionSlab {
    u32 offset = 0;  // from the mesh (the root entry * 4)
    s16 z = 0;
    CollisionNode node;
    std::vector<u32> rows;  // row offsets from the mesh, 0 for none
};

struct CollisionRow {
    u32 offset = 0;  // from the mesh
    s16 z = 0;
    s16 y = 0;
    CollisionNode node;
    std::vector<u32> leaves;  // leaf words, 0 for none
};

// A parsed collision block. Slabs, rows and cells are in tree order (Z, then
// Y, then X ascending).
struct CollisionMesh {
    s32 mesh_offset = 0;  // from the block; 0x40 on every retail level
    s32 hero_offset = 0;  // from the block, 0 for none
    CollisionNode root;
    std::vector<u16> root_entries;  // slab offset / 4 per Z, 0 for none
    std::vector<CollisionSlab> slabs;
    std::vector<CollisionRow> rows;
    std::vector<CollisionCell> cells;
    s32 hero_group_count = 0;
    std::vector<HeroCollisionGroup> hero_groups;

    // The cell at these cell coordinates, or null. Needs `cells` in tree order.
    const CollisionCell* find_cell(s32 x, s32 y, s32 z) const;

    // Face count per type byte.
    std::map<u8, std::size_t> type_counts() const;
};

// Parses a collision block (the bytes at core header +0x14).
CollisionMesh read_collision(Game game, ByteView block);

// The game's cell lookup (NTSC-U level01 0x2117d0) on the raw tree: the
// non-zero leaf word of cell (x, y, z), or nothing when the cell is outside the
// grid or empty. `mesh` is the bytes from the root node on.
std::optional<u32> lookup_collision_leaf(ByteView mesh, s32 x, s32 y, s32 z);

// The cell holding world position p, or nothing outside [0, 1024)^3. The game
// computes trunc(p * 1024) >> 12 for lines and trunc(p) >> 2 for volumes; both
// are floor(p / 4) in range.
std::optional<std::array<s32, 3>> collision_cell_of(const std::array<float, 3>& p);

// One world-space triangle of the mesh.
struct CollisionTriangle {
    std::array<float, 3> a{};
    std::array<float, 3> b{};
    std::array<float, 3> c{};
    u32 cell = 0;  // index into CollisionMesh::cells
    u16 face = 0;  // index into CollisionCell::faces
    u8 type = 0;
    u8 part = 0;  // 0 a triangle, 1 the (v0, v1, v2) half of a quad, 2 the (v0, v2, v3) half
};

// Every face of every cell as triangles, in cell then face order. A quad splits
// along its v0-v2 diagonal as the game's kernels split it. A face stored in
// several cells appears once per cell.
std::vector<CollisionTriangle> collision_triangles(const CollisionMesh& mesh);

}  // namespace openrac::assets

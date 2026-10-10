// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/occlusion.rs
// and crates/rc-engine/src/occlusion.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Precomputed occlusion: which objects can be seen from where the camera is.
// Specification: ReRAC's docs/formats/occlusion_rac1.md; the per-frame rule
// and its use by a renderer: docs/plan/occlusion_culling.md. Addresses are
// NTSC-U level01.
//
// Three pieces of data take part:
// - the core occlusion grid (core header +0x0c): a Z -> Y -> X tree of u16
//   nodes over 4 x 4 x 4-unit cells, each populated cell naming one 0x80-byte
//   mask of 1024 visibility bits;
// - the gameplay occlusion mappings (gameplay section 0x8c; the level's
//   `occlusion` lump is a copy): one (bit, key) record per tfrag, tie instance
//   and occlusion-culled moby;
// - the optional octant override (core header +0xa8): a centre and 8 masks,
//   used only when the camera leaves the grid while the frame's fallback is 2.
//
// At level load (0x255958) every object's mapping becomes a u16 word
// (OcclusionBits), and every frame UpdateOcclusion (0x2193f8) builds the
// 128-byte mask from the camera's cell; the tfrag, tie and moby renderers skip
// an object whose bit is clear before any frustum test. Shrubs never consult it.

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "assets/bytes.h"
#include "assets/version.h"
#include "assets/world/gameplay.h"

namespace openrac::assets {

inline constexpr std::size_t kOcclusionMaskBytes = 0x80;
// The mask's bit b is mask[b >> 3] & (1 << (b & 7)).
using OcclusionMask = std::array<u8, kOcclusionMaskBytes>;
// BuildOcclVisibility (0x219008) scales the camera by 0.25.
inline constexpr float kOcclusionCellSize = 4.0f;
// Core header +0xa8's block: the centre (4 floats) and 8 masks.
inline constexpr std::size_t kOcclusionOctantBlockSize = 0x10 + 8 * kOcclusionMaskBytes;

struct OcclusionCell {
    u16 x = 0;
    u16 y = 0;
    u16 z = 0;
    u16 mask = 0;  // into OcclusionGrid::masks

    bool operator==(const OcclusionCell&) const = default;
};

// The octant override: BuildOcclVisibility picks mask (camera.x > c.x) * 4 +
// (camera.y > c.y) * 2 + (camera.z > c.z), strictly.
struct OcclusionOctants {
    std::array<float, 4> centre{};  // x, y, z and a fourth word (0 in retail)
    std::array<OcclusionMask, 8> masks{};

    const OcclusionMask& mask_for(const std::array<float, 3>& camera) const;
};

// The core occlusion grid, decoded.
struct OcclusionGrid {
    s32 masks_offset = 0;              // +0x00: from the block
    u16 z_base = 0;                    // +0x04: the first Z cell
    u16 z_count = 0;                   // +0x06: Z slots (u16 node offsets / 4, 0 for none)
    std::vector<OcclusionCell> cells;  // every populated cell, in tree order
    // Masks 0 up to the highest one a cell names (the game stores no count).
    std::vector<OcclusionMask> masks;
    // Present when core header +0xa8 is set (levels 11, 13 and 17 in retail).
    std::optional<OcclusionOctants> octants;
    // The block itself, which lookup() walks as the game does.
    std::vector<u8> raw;

    // ParseOcclGrid (0x218e78) on this block: the mask of cell (x, y, z).
    std::optional<u16> lookup(s32 x, s32 y, s32 z) const;

    // The cell holding a world position, mapped as the game maps the camera.
    std::optional<OcclusionCell> cell_for(const std::array<float, 3>& position) const;
};

// The camera's cell as BuildOcclVisibility computes it: cvt.w.s(p * 0.25),
// truncation toward zero (so cell 0 spans -4 < p < 4).
std::array<s32, 3> occlusion_cell_coords(const std::array<float, 3>& position);

// ParseOcclGrid on a raw block: each coordinate rebased by its node's u16 base
// and checked against its u16 count; a Z or Y slot of 0 or an X slot of 0xffff
// means no cell. Reads are bounds-checked here (the game's are not).
std::optional<u16> lookup_occlusion_mask(ByteView block, s32 x, s32 y, s32 z);

// Decodes a grid block (the bytes at core header +0x0c); without the octants.
OcclusionGrid read_occlusion_grid(Game game, ByteView block);

// The octant override block (the bytes at core header +0xa8).
OcclusionOctants read_occlusion_octants(Game game, ByteView block);

// One mapping record: the bit an object tests and the key the loader matches
// it by (tfrag: header byte 0x3d; tie: the instance's occlusion index; moby:
// the instance's spawn id).
struct OcclusionMapping {
    s32 bit_index = 0;
    s32 occlusion_id = 0;
};

struct OcclusionMappings {
    std::vector<OcclusionMapping> tfrag;
    std::vector<OcclusionMapping> tie;
    std::vector<OcclusionMapping> moby;
};

// `s32 tfrag_count, tie_count, moby_count, pad`, then the records in that order.
OcclusionMappings read_occlusion_mappings(Game game, ByteView section);

// The mappings of a gameplay file; nothing when section 0x8c is absent (the
// loader then prints "no occlusion", makes everything visible and switches
// occlusion off).
std::optional<OcclusionMappings> read_gameplay_occlusion_mappings(const GameplayFile& file);

// An object's load-time occlusion word: the high byte is the mask byte, the
// low byte the bit in it.
struct OcclusionBits {
    u16 word = 0;

    // Bit 1023, which every frame mask has set: the loader's "always visible".
    static constexpr u16 kAlways = 0x7f80;

    // (bit >> 3) << 8 | 1 << (bit & 7), truncated to 16 bits as the loader's sh.
    static OcclusionBits from_bit(s32 bit);

    static OcclusionBits always() { return {kAlways}; }

    u8 byte() const { return static_cast<u8>(word >> 8); }

    u8 bit() const { return static_cast<u8>(word); }

    // The renderers' test: mask[byte] & bit. A byte past 0x7f (a bit index of
    // 1024 or more, never in retail) counts as not visible.
    bool visible(const OcclusionMask& frame_mask) const;

    bool operator==(const OcclusionBits&) const = default;
};

// Every object's word after the loader's resolution (0x255958).
struct LevelOcclusion {
    std::vector<OcclusionBits> tfrag;  // per tfrag, in header table order (tfrag header 0x3a)
    std::vector<OcclusionBits> tie;    // per tie instance, gameplay order (runtime tie +0x18)
    std::vector<OcclusionBits> moby;   // per static moby instance (runtime moby +0x36)
    // The tfrag count or a key differed: every tfrag always visible ("occlusion
    // out of date on tfrag").
    bool tfrag_out_of_date = false;
    // The ties took the positional path.
    bool ties_positional = false;
    // Ties / culled mobys without a mapping ("occlusion out of date on
    // ties/mobys").
    std::size_t ties_not_found = 0;
    std::size_t mobys_not_found = 0;
};

// Tfrags: mapping i belongs to tfrag i, taken only when the counts match and
// every tfrag's header byte 0x3d equals its mapping's key (compared as u32);
// otherwise every tfrag is always visible.
void resolve_tfrag_occlusion(
    const OcclusionMappings& mappings, std::span<const u8> tfrag_keys, LevelOcclusion& out
);

// Ties: positional when the counts match and every (s16) occlusion index
// equals its mapping's (s16) key; otherwise each tie takes the first mapping
// whose key (u32) equals its u16 occlusion index, or is always visible.
void resolve_tie_occlusion(
    const OcclusionMappings& mappings, std::span<const s32> tie_occlusion_index, LevelOcclusion& out
);

// Mobys: an instance takes part only when its occlusion word is 0 (else always
// visible); it takes the first mapping whose key equals its (s16) spawn id, or
// is always visible. Mobys created later start always visible.
void resolve_moby_occlusion(
    const OcclusionMappings& mappings, std::span<const MobyInstance> mobys, LevelOcclusion& out
);

// All three; null mappings is the loader's "no occlusion" path.
LevelOcclusion resolve_level_occlusion(
    const OcclusionMappings* mappings,
    std::span<const u8> tfrag_keys,
    std::span<const s32> tie_occlusion_index,
    std::span<const MobyInstance> mobys
);

// The indices of the objects whose word passes under `frame_mask`.
std::vector<u32> visible_objects(
    std::span<const OcclusionBits> words, const OcclusionMask& frame_mask
);

// The mask with bit 1023 forced on, as the last store of BuildOcclVisibility does.
OcclusionMask with_always_bit(OcclusionMask mask);

// The debug menu's "occlusion" item (0x16c4f4). The loader sets Active when the
// level has a grid and mappings, else Off; retail never changes it.
enum class OcclusionMode : u8 {
    Off = 0,
    Freeze = 1,
    Active = 2,
};

// What to do when the camera's cell is not in the grid (0x15f608). Reset to 0
// after every frame and at level load; set to 1 by the Visibomb's missile and to
// 2 by UpdateModeFreeze for the frame that follows.
enum class OcclusionFallback : u8 {
    Neighbours = 0,  // the 6 face neighbours (nearer side first), else the previous mask
    AllVisible = 1,
    Octants = 2,  // the octant override if the level has one, else the previous mask
};

// Where the "previous mask" (0x15f610) points.
enum class OcclusionPrevious : u8 {
    None,
    Grid,
    Union,
};

// The per-frame state of UpdateOcclusion / BuildOcclVisibility.
class OcclusionState {
public:
    OcclusionState();

    // UpdateOcclusion (0x2193f8) for one frame: `camera` is the render
    // camera's position (0x167240) in world units; `debug_camera` the debug
    // menu's camera mode (0 in play). Returns the frame's mask (0x174180).
    const OcclusionMask& update(
        const OcclusionGrid* grid,
        OcclusionMode mode,
        OcclusionFallback fallback,
        bool debug_camera,
        const std::array<float, 3>& camera
    );

    const OcclusionMask& mask() const { return m_mask; }

    OcclusionPrevious previous() const { return m_previous; }

    u16 previous_grid_mask() const { return m_previous_mask; }

    // The camera's own cell was not in the grid (0x15f60c).
    bool missed() const { return m_missed; }

private:
    void build(
        const OcclusionGrid& grid,
        OcclusionFallback fallback,
        bool debug_camera,
        const std::array<float, 3>& camera
    );

    OcclusionMask m_mask;
    OcclusionMask m_union;  // 0x174200: the neighbour cells' masks or'd
    OcclusionPrevious m_previous = OcclusionPrevious::None;
    u16 m_previous_mask = 0;
    bool m_missed = false;
};

}  // namespace openrac::assets

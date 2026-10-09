// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/level.rs
// (spec: docs/formats/wad_layouts_rac1.md sections 2.3 to 2.5): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// RAC1's level data: the uncompressed data container a level header points
// at (its 0x58-byte table of byte ranges: the overlay, sound bank, core index,
// GS RAM image, HUD and the WAD-compressed core data), the core index (the
// 0xbc-byte core header and its tables) and the named blocks of the
// decompressed core data. The same layout holds on NTSC-U and PAL
// (editor/level.py reads the PAL disc with the same table).

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

struct ByteRange {
    s32 offset;
    s32 size;

    bool present() const { return offset >= 0 && size > 0; }
};

// The header at the start of a level's data container.
struct LevelDataHeader {
    ByteRange overlay;
    ByteRange sound_bank;
    ByteRange core_index;
    ByteRange gs_ram;
    ByteRange hud_header;
    std::array<ByteRange, 5> hud_banks;
    ByteRange core_data;
};

static_assert(sizeof(LevelDataHeader) == 0x58);

LevelDataHeader parse_level_data_header(ByteView data);

// {count, offset}.
struct ArrayRange {
    s32 count;
    s32 offset;
};

// The core header at byte 0 of the core index. "index" offsets are relative
// to the core index, "data" offsets to the decompressed core data.
struct LevelCoreHeader {
    ArrayRange gs_ram;
    s32 tfrags;
    s32 occlusion;
    s32 sky;
    s32 collision;
    ArrayRange moby_classes;
    ArrayRange tie_classes;
    ArrayRange shrub_classes;
    ArrayRange tfrag_textures;
    ArrayRange moby_textures;
    ArrayRange tie_textures;
    ArrayRange shrub_textures;
    ArrayRange part_textures;
    ArrayRange fx_textures;
    s32 textures_base_offset;
    s32 part_bank_offset;
    s32 fx_bank_offset;
    s32 part_defs_offset;
    s32 sound_remap_offset;
    s32 unknown_74;
    s32 ratchet_seqs;
    s32 scene_view_size;
    s32 gadget_count;
    s32 gadget_offset;
    s32 assets_compressed_size;
    s32 assets_decompressed_size;
    s32 chrome_map_texture;
    s32 chrome_map_palette;
    s32 glass_map_texture;
    s32 glass_map_palette;
    s32 unknown_a0;
    s32 heightmap_offset;
    s32 occlusion_oct_offset;
    s32 moby_gs_stash_list;
    s32 occlusion_rad_offset;
    s32 moby_sound_remap_offset;
    s32 occlusion_rad2_offset;
};

static_assert(sizeof(LevelCoreHeader) == 0xbc);

struct GsRamEntry {
    s32 psm;
    s16 width;
    s16 height;
    s32 address;
    s32 offset;
};

struct ClassEntry {
    s32 offset_in_asset_wad;
    s32 o_class;
    s32 unknown_8;
    s32 unknown_c;
    std::array<u8, 16> textures;
};

struct ShrubBillboardInfo {
    s16 width;
    s16 height;
    s16 max_mip;
    s16 palette_offset;
    s16 texture_offset;
    s16 mip1;
    s16 mip2;
    s16 mip3;
};

struct ShrubClassEntry {
    ClassEntry base;
    ShrubBillboardInfo billboard;
};

static_assert(sizeof(ShrubClassEntry) == 0x30);

struct TextureEntry {
    s32 data_offset;
    s16 width;
    s16 height;
    s16 type;
    s16 palette;
    s16 mipmap;
    s16 pad;
};

struct GadgetEntry {
    s32 offset_in_asset_wad;
    s32 class_number;
    s32 compressed_size;
    s32 pad;
};

// A block of the decompressed core data.
struct CoreBlock {
    std::string name;  // tfrags, sky, moby_class/0042, ratchet_seq/007, ...
    std::size_t offset;
    std::size_t size;
};

struct LevelCore {
    LevelCoreHeader header;
    std::vector<GsRamEntry> gs_ram;
    std::vector<ClassEntry> moby_classes;
    std::vector<ClassEntry> tie_classes;
    std::vector<ShrubClassEntry> shrub_classes;
    std::vector<TextureEntry> tfrag_textures;
    std::vector<TextureEntry> moby_textures;
    std::vector<TextureEntry> tie_textures;
    std::vector<TextureEntry> shrub_textures;
    std::vector<GadgetEntry> gadgets;
    std::vector<s32> ratchet_seqs;
    // Every block, sized by the boundary rule: a block runs to the next known
    // start offset.
    std::vector<CoreBlock> blocks;

    // The bytes of the first block named `name` in the decompressed core data.
    std::optional<ByteView> block(ByteView data, std::string_view name) const;
};

// The core index and the decompressed core data's size.
LevelCore parse_level_core(ByteView index, std::size_t data_size);

// The level height grid (core header +0xa4, a data offset): s32 width, s32
// rows, f32 low, f32 high, then u8 cells[rows][width]. ReRAC finds it on
// levels 08, 12 and 14 of NTSC-U, whose weather reads it (NTSC-U 0x278020).
struct HeightGrid {
    s32 width;
    s32 rows;
    f32 low;   // the height of cell value 255
    f32 high;  // the height of cell value 0
    std::vector<u8> cells;

    // Nothing when the offset is 0 or the grid does not fit the data.
    static std::optional<HeightGrid> parse(const LevelCoreHeader& header, ByteView data);

    // (high - low) * (1 - cell / 255) + low of the cell under (x, y), the
    // coordinates truncated as the game's cvt.w.s does. Nothing outside.
    std::optional<f32> height(f32 x, f32 y) const;
};

}  // namespace openrac::assets::disc

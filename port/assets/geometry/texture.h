// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/texture.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Level textures. Every texture the level geometry uses is 8-bit indexed
// (PSMT8) with a 256-entry RGBA32 palette in the GS's CSM1 order and alpha
// 0x80 = opaque; decode_indexed8 turns one into RGBA8. The RAC1 readers find
// the indices and palettes the way the game does (ReRAC
// docs/formats/textures_rac1.md 3, 4, 9, 12): the tfrag, moby, tie and shrub
// tables' pixels in the core data's textures block and their palettes in
// gs_ram, shrub billboards wholly in gs_ram, tfrag mip levels 2 and 3 in
// gs_ram too.

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "assets/bytes.h"
#include "assets/geometry/core_records.h"

namespace openrac::assets {

// An RGBA8 image, rows top-down, alpha already scaled to 0..255.
struct RgbaImage {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> rgba;

    bool operator==(const RgbaImage&) const = default;
};

// CSM1 palette order to linear: within each 32-entry group the two middle
// 8-entry blocks swap (bits 3 and 4 of the index). Its own inverse.
u32 clut_index(u32 i);

// GS alpha 0..0x80 to 0..0xff; 0x80 and above is opaque.
u8 scale_alpha(u8 a);

// `width * height` palette indices with a 256-entry RGBA32 palette (1024
// bytes, CSM1 order).
RgbaImage decode_indexed8(ByteView indices, u32 width, u32 height, ByteView clut);

// One stored 8-bit image: the indices and the palette as the disc holds them.
// Exporters that keep the indices use the parts; decode() is what every
// reader here applies.
struct IndexedImage {
    u32 width = 0;
    u32 height = 0;
    ByteView indices;
    ByteView clut;

    RgbaImage decode() const { return decode_indexed8(indices, width, height, clut); }
};

}  // namespace openrac::assets

namespace openrac::assets::rac1 {

// The core index table a texture came from.
enum class TextureTable : u8 {
    Tfrag,
    Moby,
    Tie,
    Shrub,
    Billboard,
};

std::string_view texture_table_name(TextureTable table);  // "tfrag", ...

struct LevelTexture {
    TextureTable table = TextureTable::Tfrag;
    // The index in its table, what class texture slots and tfrag ad-gifs name;
    // for billboards, the index of the shrub class.
    std::size_t index = 0;
    CoreTextureEntry entry;                      // tables other than Billboard
    std::optional<CoreBillboardInfo> billboard;  // Billboard
    s32 billboard_class = 0;                     // Billboard: the shrub class number
    RgbaImage image;

    // A stable name: "tfrag/000_128x128_t4" or "billboard/0123_32x64".
    std::string key() const;
};

// The tables decode_level_textures reads.
struct TextureTables {
    std::span<const CoreTextureEntry> tfrag;
    std::span<const CoreTextureEntry> moby;
    std::span<const CoreTextureEntry> tie;
    std::span<const CoreTextureEntry> shrub;
    std::span<const CoreShrubClassEntry> shrub_classes;
};

// Level 0 of a table entry: pixels in `textures_block` (the core data's
// textures block), palette in gs_ram.
IndexedImage entry_image(ByteView textures_block, ByteView gs_ram, const CoreTextureEntry& entry);

// Level 0 of a shrub billboard: pixels and palette in gs_ram.
IndexedImage billboard_image(ByteView gs_ram, const CoreBillboardInfo& info);

// Every level texture in table order (tfrag, moby, tie, shrub, then the shrub
// billboards). Entries with no size or negative offsets are skipped; any
// other entry that does not decode is an error (none does on the RAC1 disc).
std::vector<LevelTexture> decode_level_textures(
    const TextureTables& tables, ByteView textures_block, ByteView gs_ram
);

// The stored mip chain of a tfrag texture, level 0 first, every level with the
// entry's one palette (TEX0.CBP; MIPTBP1 carries only TBP/TBW per level).
// `levels` is the level count. Level 0 is at textures[data_offset] (w x h),
// level 1 right after it (w/2 x h/2: the NTSC-U boot's tfrag texture DMA at
// 0x234d48 sends it from the base address + w*h; tfrag textures are square),
// levels 2 and 3 in gs_ram at mip2_block / mip3_block. Only verified for the
// tfrag table (ReRAC tfrag_rac1.md 2.5.1).
std::vector<IndexedImage> tfrag_mip_images(
    ByteView textures_block, ByteView gs_ram, const CoreTextureEntry& entry
);
std::vector<RgbaImage> decode_tfrag_mip_levels(
    ByteView textures_block, ByteView gs_ram, const CoreTextureEntry& entry
);

// The stored mip chain of a shrub billboard, level 0 first, with the
// descriptor's palette. The class init (NTSC-U boot 0x203b08) builds TEX1 MXL
// = levels - 1, TEX0 TBP0 = texture_block and MIPTBP1 TBP1..3 = mip1..3, all
// resident in gs_ram in the same linear layout as level 0.
std::vector<IndexedImage> billboard_mip_images(ByteView gs_ram, const CoreBillboardInfo& info);
std::vector<RgbaImage> decode_billboard_mip_levels(ByteView gs_ram, const CoreBillboardInfo& info);

// The GS register words the level loader writes over a geometry ad-gif block
// at load (the tfrag init, NTSC-U boot 0x2040e0, and the shrub class init,
// boot 0x203b08, use one rule). `gs_base` is the level texture area's GS byte
// address; TBP and CBP fields are 256-byte blocks relative to gs_base >> 8.
// TEX0 TBP0 and MIPTBP1 TBP1 stay 0: the game ORs them in every frame from its
// texture paging tables, which a renderer that keeps every texture resident
// can ignore.
//
//   TEX0    = TBW max(1, w/64) << 14 | PSMT8 << 20 | log2 w << 26 | log2 h << 30
//             | TCC << 34 | CBP (palette + base) << 37 | CLD 4 << 61
//   TEX1    = MXL (levels - 1) << 2 | MMAG 1 << 5 | MMIN hi << 6 | K lo << 32
//   CLAMP   = WMS lo | WMT hi << 2 | texture_index << 24
//   MIPTBP1 = TBW1 max(1, w/128) << 14 | TBP2 (mip2 + base) << 20 | 1 << 34
//             | TBP3 (mip3 + base) << 40 | 1 << 54
u64 loaded_tex0(const CoreTextureEntry& texture, u32 gs_base);
u64 loaded_tex1(s16 levels, s32 stored_lo, s32 stored_hi);
u64 loaded_clamp(s32 stored_lo, s32 stored_hi, s32 texture_index);
u64 loaded_miptbp1(const CoreTextureEntry& texture, u32 gs_base);

}  // namespace openrac::assets::rac1

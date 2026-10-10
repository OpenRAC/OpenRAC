// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/save_game.rs
// (spec: docs/plan/game_state.md sections 1 to 4): ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// RAC1's saved game as it is on the memory card: the chunk descriptor tables
// (read from the boot executable), the save%d.bin file (a header, the global
// section and 20 level sections), its CRC-16, the disc's save_game lump (the
// card's icon files and the blank template) and the item tables the
// level-start rules read. Nothing here is copied from the disc: only layouts
// and the CRC are code.
//
//   Descriptor (16 bytes): {u32 address, u32 size, s32 id, s32 restore
//     status}, ended by address 0. A per-level chunk of level slot L lives at
//     address + L * size.
//   Section (NTSC-U PrepData 0x20ad78): {u32 data size, u32 crc16(data)}, then
//     chunks {s32 id, u32 size, bytes[size], pad to 4} and the terminator
//     {s32 -1, u32 0}. PrepData does not write the pad bytes (they are whatever
//     the buffer held), so Chunk keeps them for byte-exact round trips.
//   File: {u32 global size, u32 level size}, the global section at 8, level
//     section L at 8 + global size + L * level size.
//
// The addresses of the descriptor and item tables are NTSC-U's
// (SCUS_971.99); save_layout() returns nothing for the other versions.

#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "assets/bytes.h"
#include "assets/disc/overlay.h"
#include "assets/version.h"

namespace openrac::assets::disc {

// Where a build keeps the tables the save code reads.
struct SaveLayout {
    u32 global_chunks;  // the global chunk descriptor table
    u32 level_chunks;   // the per-level one
    u32 vendor_items;   // u32[20]: the item each level's vendor sells
    u32 item_records;   // 0x18-byte item price records [37]
};

// NTSC-U only; nothing for the versions whose addresses are not known yet.
const SaveLayout* save_layout(const GameVersion& version);

inline constexpr std::size_t kSaveLevelSlots = 20;  // 19 levels are used
inline constexpr std::size_t kSaveSlots = 5;        // save0.bin .. save4.bin
inline constexpr std::size_t kCrcMaxLength = 0x1800;
inline constexpr s32 kChunkEndId = -1;
inline constexpr std::size_t kDescriptorSize = 16;

struct ChunkDesc {
    u32 address;  // level slot 0 for per-level chunks
    u32 size;
    s32 id;

    u32 address_for(std::size_t slot) const { return address + static_cast<u32>(slot) * size; }

    bool operator==(const ChunkDesc&) const = default;
};

std::vector<ChunkDesc> read_chunk_table(ByteView elf, u32 vaddr);

// NTSC-U memcard_GetDataSize 0x20ac88: 8 + sum(align4(size) + 8) + 8, the
// section's size with its header.
std::size_t section_size(std::span<const ChunkDesc> descs);

struct ChunkTables {
    std::vector<ChunkDesc> global;
    std::vector<ChunkDesc> level;

    static ChunkTables from_boot_executable(ByteView elf, const SaveLayout& layout);

    std::size_t global_size() const { return section_size(global); }

    std::size_t level_size() const { return section_size(level); }

    std::size_t file_size() const { return 8 + global_size() + kSaveLevelSlots * level_size(); }
};

// NTSC-U memcard_Checksum 0x20acc0: CRC-16, MSB first, the register seeded
// with 0xedb88320 (only its low 16 bits matter: init 0x8320), polynomial
// 0x1f45, no final xor; 0 above kCrcMaxLength bytes.
u16 save_crc16(std::span<const u8> data);

struct Chunk {
    s32 id;
    std::vector<u8> data;
    std::vector<u8> pad;  // the bytes after data up to 4-byte alignment

    static Chunk make(s32 id, std::vector<u8> data);
    bool operator==(const Chunk&) const = default;
};

struct Section {
    std::vector<Chunk> chunks;
    // Decoded: the stored CRC was non-zero and matched (NTSC-U TestChecksum
    // 0x20ad38); the game restores nothing from a section that fails.
    bool crc_ok = true;

    const Chunk* chunk(s32 id) const;
    std::size_t encoded_size() const;
    std::vector<u8> encode() const;
    static Section decode(ByteView bytes);

    bool operator==(const Section&) const = default;
};

struct SaveFile {
    Section global;
    std::array<Section, kSaveLevelSlots> levels;

    static SaveFile parse(ByteView bytes);
    std::vector<u8> to_bytes() const;  // NTSC-U MakeWholeSave 0x20abb0
    bool all_crcs_ok() const;

    // The incremental save (NTSC-U memcard_Save 0x20b178): writes the global
    // section at 8 and level section `level` in place; the header and the
    // other level sections keep what the card held.
    void write_incremental(std::size_t level, std::span<u8> card_file) const;
};

std::string save_file_name(std::size_t slot);  // "save0.bin"
inline constexpr std::string_view kIconSysName = "icon.sys";
inline constexpr std::string_view kStaticIcoName = "static.ico";

// NTSC-U memcard_GetName 0x209030: the card directory from SYSTEM.CNF's bytes
// (BOOT2 = cdrom0:\SCUS_971.99;1 gives /BASCUS-97199RATCHET; a 'E' at byte 18
// gives /BE...).
std::string card_directory(ByteView system_cnf);

// The global save_game lump: {icon.sys offset, size, static.ico offset, size,
// template offset, size} then the parts.
struct SaveGameLump {
    std::vector<u8> icon_sys;
    std::vector<u8> static_ico;
    std::vector<u8> blank_save;  // the template every new save slot starts as

    static SaveGameLump parse(ByteView lump);
};

inline constexpr std::size_t kItemCount = 37;
inline constexpr std::size_t kVendorTableLength = 20;
inline constexpr std::size_t kItemDefinitionSize = 0x4c;

struct ItemRecord {
    std::array<u8, 0x18> bytes;

    // +8 non-zero: the item uses ammo.
    bool has_ammo() const;
    // +0x12: ammo granted with the item.
    u16 grant_ammo() const;

    bool operator==(const ItemRecord&) const = default;
};

struct ItemTables {
    std::array<u32, kVendorTableLength> vendor_items;
    std::array<ItemRecord, kItemCount> records;
    // Item definition +8: slot type (0 hand, 1 feet, 2 head, 3 back, 4 and 5
    // extras, -1 none), read from the level overlay.
    std::array<s32, kItemCount> slot_type;
    u32 item_definitions;  // the table's address in that overlay

    static ItemTables load(ByteView boot_elf, ByteView overlay, const SaveLayout& layout);
};

// The item definition table of a level overlay, whose address differs per
// overlay: found by the vendor table's bytes and GiveItem's code reading it
// (NTSC-U 0x275838: lui/addiu of the vendor table, a lui, the first addiu on
// it followed within 3 words by lw rX, 8(rD)). Every candidate must agree.
u32 find_item_definitions(std::span<const OverlaySection> sections, ByteView vendor_table);

}  // namespace openrac::assets::disc

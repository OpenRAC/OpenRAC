// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/disc.rs
// (spec: docs/formats/disc_layout.md sections 1 and 2): ISC License, Copyright (c) 2026
// ReRAC contributors.
//
// A game disc: its ISO 9660 file system, its table of contents and the lumps
// the table points at, read straight from the player's image.
//
// The lump plan (archive_files) is ReRAC's "Tier 0" layout, the one its
// per-file SHA-1 table of the NTSC-U disc names: boot/<file> for every file of
// the file system, toc.bin, global/<field>[/NNN].bin for the global header and
// levels/NN/ per level: level_header.bin, the members of the level's data
// container (overlay.bin, core_index.bin, core_data.bin, ...), the gameplay
// and occlusion ranges, then its audio and scene lumps. Every file is one
// contiguous byte range of the image, kept exactly as on the disc.
//
// Only games whose layout is known can be opened (toc.h: RAC1, both regions).

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "assets/disc/iso9660.h"
#include "assets/disc/level.h"
#include "assets/disc/toc.h"

namespace openrac::assets::disc {

struct TocLevel {
    std::size_t table_index;  // slot in the level table
    u32 header_sector;
    LevelHeader header;
};

// One file of the lump plan: `bytes` bytes of the image at byte `offset`,
// written as `path` (relative, "/"-separated).
struct DiscFile {
    std::string path;
    u64 offset;
    u64 bytes;

    bool operator==(const DiscFile&) const = default;
};

// The raw lumps of one level's "level group", as on the disc. A member is
// absent when its range is empty.
struct LevelFiles {
    u32 id = 0;
    LevelHeader header{};
    std::vector<u8> level_header;            // the header's sectors' first 0x2434 bytes
    std::optional<std::vector<u8>> overlay;  // the level's program
    std::optional<std::vector<u8>> sound_bank;
    std::optional<std::vector<u8>> core_index;
    std::optional<std::vector<u8>> gs_ram;
    std::optional<std::vector<u8>> hud_header;
    std::array<std::optional<std::vector<u8>>, 5> hud_banks;
    std::optional<std::vector<u8>> core_data;      // still WAD-compressed
    std::optional<std::vector<u8>> gameplay_ntsc;  // whole sectors
    std::optional<std::vector<u8>> gameplay_pal;
    std::optional<std::vector<u8>> occlusion;

    // (file name under levels/NN/, bytes) of every present member, in plan order.
    std::vector<std::pair<std::string, ByteView>> files() const;
    std::optional<ByteView> file(std::string_view name) const;
};

class Disc {
public:
    // Reads the table of contents and every level header it names. Throws
    // AssetError when the layout is not known or the table is not there.
    Disc(IsoImage iso, const DiscLayout& layout);
    static Disc open(const std::filesystem::path& image, const DiscLayout& layout);

    const IsoImage& iso() const { return m_iso; }

    const DiscLayout& layout() const { return *m_layout; }

    // The table of contents as the game keeps it.
    const std::vector<u8>& toc() const { return m_toc; }

    const std::vector<TocLevel>& levels() const { return m_levels; }

    // Level ids (= NN of levels/NN) in table order.
    std::vector<u32> level_ids() const;

    // The boot executable's disc path, from SYSTEM.CNF's BOOT2.
    std::string boot_executable_path() const;

    // The boot executable; falls back to the first root file starting with
    // "\x7fELF" when SYSTEM.CNF names no existing file.
    std::vector<u8> boot_executable() const;

    // The level group of level `id`.
    LevelFiles level(u32 id) const;

    // The level group's files in plan order (levels/NN/level_header.bin, the
    // data container's members, gameplay and occlusion), without reading them.
    std::vector<DiscFile> level_group_files(u32 id) const;

    // Every global lump in table order; `name` is the path under global/
    // without ".bin". Empty entries are skipped; a repeated name gets ".2", ".3".
    std::vector<StreamLump> global_lumps() const;

    // The audio and scene lumps of level `id`.
    std::vector<StreamLump> level_stream_lumps(u32 id) const;

    std::vector<u8> read_lump(const StreamLump& lump) const;

    // The whole lump plan (header comment), each range checked to lie inside
    // the image.
    std::vector<DiscFile> archive_files() const;

    // The global save_game lump (whole sectors).
    std::vector<u8> save_game_lump() const;

    // Scene k's region file of level `id` (chunks and sentinel), or nothing.
    std::optional<std::vector<u8>> scene_region(u32 id, std::size_t k, bool pal) const;

    // Scene k's speech VAG of level `id` in language `lang`, or nothing.
    std::optional<std::vector<u8>> scene_speech(u32 id, std::size_t k, std::size_t lang) const;

private:
    const TocLevel& toc_level(u32 id) const;
    std::optional<std::vector<u8>> sector_range(SectorRange r) const;
    u64 probe(u32 sector) const;

    IsoImage m_iso;
    const DiscLayout* m_layout;
    std::vector<u8> m_toc;
    std::vector<TocLevel> m_levels;
};

}  // namespace openrac::assets::disc

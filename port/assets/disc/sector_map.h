// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The disc as the extractor's files: the sector map (sectors.tsv) that
// tools/openrac_common/disc_files.py writes into iso_data/<game>/ beside the
// files it extracts. The games read their data by absolute sector, most of it
// outside the file system; the extractor writes the file system's files at
// their paths and every other sector as lumps cut where the game's table of
// contents points, and the map says for each run of sectors which file holds
// it:
//
//   # openrac sector map 1: first sector, sectors, bytes, file
//   0<TAB>289<TAB>591872<TAB>lumps/sectors/0000000.bin
//   289<TAB>1<TAB>58<TAB>SYSTEM.CNF
//
// A run's file holds `bytes` bytes; its sectors past them read as zeros (the
// padding after a file of the file system, which the extractor checked). The
// runs cover every sector, in order, so a read anywhere gives the bytes the
// disc has there without the image being opened.
//
// `open_sector_map` is an ImageSource over the map: IsoImage and Disc read
// through it as through an image, and IsoImage::open takes the folder in place
// of an image.

#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

#include "assets/disc/iso9660.h"

namespace openrac::assets::disc {

inline constexpr std::string_view kSectorMapName = "sectors.tsv";
inline constexpr std::string_view kSectorMapHeader =
    "# openrac sector map 1: first sector, sectors, bytes, file";

struct SectorRun {
    u32 first = 0;
    u32 sectors = 0;
    u64 bytes = 0;
    std::filesystem::path file;
};

class SectorMap {
public:
    // The map in `folder`. Throws IoError when it cannot be read, or is not a
    // map of this version, or its runs are not in order.
    static SectorMap load(const std::filesystem::path& folder);

    // Whether `folder` has a map.
    static bool present(const std::filesystem::path& folder);

    const std::vector<SectorRun>& runs() const { return m_runs; }

    // The sectors the runs cover, from 0.
    u32 sector_count() const { return m_sectors; }

    u64 size() const { return static_cast<u64>(m_sectors) * kSectorSize; }

    // Exactly dst.size() bytes from byte `offset` of the disc; throws IoError
    // past the last run or when a file is missing or short. Safe from any
    // thread (the music is read on threads of its own).
    void read(u64 offset, std::span<u8> dst) const;

    // As read, but stops at the end of the map: the bytes read.
    std::size_t read_some(u64 offset, std::span<u8> dst) const;

private:
    // A file kept open, by its run: the last few read.
    struct Open {
        std::size_t run = 0;
        std::unique_ptr<std::ifstream> file;
    };

    std::size_t run_at(u32 sector) const;
    void read_run(std::size_t run, u64 at, std::span<u8> dst) const;

    std::vector<SectorRun> m_runs;
    u32 m_sectors = 0;
    std::unique_ptr<std::mutex> m_lock = std::make_unique<std::mutex>();
    mutable std::vector<Open> m_open;
};

// The disc of the map in `folder`, as an image (throws as SectorMap::load).
std::unique_ptr<ImageSource> open_sector_map(const std::filesystem::path& folder);

}  // namespace openrac::assets::disc

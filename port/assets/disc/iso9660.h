// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/iso9660.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A small ISO 9660 reader for PlayStation 2 disc images. The Ratchet & Clank
// games name only a few files in the file system (RAC1: SYSTEM.CNF, the boot
// executable and IOPRP243.IMG) and read everything else by absolute sector
// through their table of contents, so the reader gives both the directory
// tree and raw sector reads. It reads exactly the bytes asked for: a 4 GB
// image is never read whole.
//
// Plain 2048-byte-sector images (.iso) and raw 2352-byte CD dumps (.bin, mode
// 1 or mode 2 form 1, told apart by the 12-byte sync pattern) are both read;
// only the 2048 user bytes of each sector are returned.
//
// Failures: a damaged or unexpected image throws AssetError, a file that
// cannot be read (missing, permissions, an I/O error, a short read) throws
// IoError, so a caller can tell "not a disc" from "cannot read the disc".

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets::disc {

inline constexpr std::size_t kSectorSize = 2048;
inline constexpr std::size_t kRawSectorSize = 2352;

// The image file could not be read.
class IoError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Where an image's bytes come from: a file, or memory in tests.
class ImageSource {
public:
    virtual ~ImageSource() = default;
    virtual u64 size() const = 0;
    // Reads exactly dst.size() bytes at `offset`; throws IoError otherwise.
    virtual void read(u64 offset, std::span<u8> dst) = 0;
};

std::unique_ptr<ImageSource> open_image_file(const std::filesystem::path& path);
std::unique_ptr<ImageSource> image_in_memory(std::vector<u8> bytes);

struct IsoEntry {
    std::string name;  // upper case, without the ";1" version suffix
    std::string path;  // "/DIR/FILE.EXT", upper case
    u32 lba = 0;
    u32 size = 0;
    bool is_directory = false;
};

class IsoImage {
public:
    explicit IsoImage(std::unique_ptr<ImageSource> source);
    static IsoImage open(const std::filesystem::path& path);

    u32 sector_count() const { return m_sector_count; }

    // 2048 for a plain image, 2352 for a raw CD dump.
    std::size_t raw_sector_size() const { return m_raw_sector_size; }

    const std::string& volume_id() const { return m_volume_id; }

    // The primary volume descriptor's volume space size (logical blocks). An
    // image with fewer sectors than this is truncated.
    u32 volume_sectors() const { return m_volume_sectors; }

    // Every file and directory, in directory order (depth first).
    const std::vector<IsoEntry>& entries() const { return m_entries; }

    // Case-insensitive lookup; the leading "/" and a ";1" suffix are optional.
    const IsoEntry* find(std::string_view path) const;

    std::vector<u8> read_file(const IsoEntry& entry) const;
    std::vector<u8> read_sectors(u32 lba, u32 count) const;

    // `size` user-data bytes from logical byte `offset` (sector * 2048 + offset
    // in the sector).
    std::vector<u8> read_bytes(u64 offset, u64 size) const;
    void read_into(u64 offset, std::span<u8> dst) const;

private:
    void walk_directory(u32 lba, u32 size, const std::string& prefix, int depth);

    std::unique_ptr<ImageSource> m_source;
    std::unique_ptr<std::mutex> m_lock = std::make_unique<std::mutex>();
    u32 m_sector_count = 0;
    std::size_t m_raw_sector_size = kSectorSize;
    std::size_t m_raw_user_offset = 0;
    std::string m_volume_id;
    u32 m_volume_sectors = 0;
    std::vector<IsoEntry> m_entries;
};

}  // namespace openrac::assets::disc

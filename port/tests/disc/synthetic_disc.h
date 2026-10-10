// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/iso9660.rs
// (tests) and crates/rc-extract/tests/extract/synthetic.rs: ISC License, Copyright (c)
// 2026 ReRAC contributors.
//
// Synthetic disc images for the tests, built in memory from the documented
// layouts: an ISO 9660 file system, and a RAC1-shaped table of contents with
// one level. No byte of a real disc.

#pragma once

#include <algorithm>
#include <bit>
#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "assets/bytes.h"

namespace openrac::test {

using assets::s32;
using assets::u32;
using assets::u8;

inline constexpr std::size_t kSs = 2048;

struct IsoFile {
    std::string path;  // "SYSTEM.CNF" or "DATA/A.BIN" (one directory level)
    std::vector<u8> bytes;
};

inline void both_endian32(u8* b, u32 v) {
    std::memcpy(b, &v, 4);
    b[4] = static_cast<u8>(v >> 24);
    b[5] = static_cast<u8>(v >> 16);
    b[6] = static_cast<u8>(v >> 8);
    b[7] = static_cast<u8>(v);
}

inline std::vector<u8> dir_record(u32 lba, u32 size, bool dir, std::string_view name) {
    const std::size_t len = (33 + name.size() + 1) & ~std::size_t{1};  // even length
    std::vector<u8> r(len, 0);
    r[0] = static_cast<u8>(len);
    both_endian32(r.data() + 2, lba);
    both_endian32(r.data() + 10, size);
    r[25] = dir ? 2 : 0;
    r[32] = static_cast<u8>(name.size());
    std::memcpy(r.data() + 33, name.data(), name.size());
    return r;
}

template <typename T>
inline void put32(std::vector<u8>& img, std::size_t at, T value) {
    const s32 v = static_cast<s32>(value);
    std::memcpy(img.data() + at, &v, 4);
}

// An ISO 9660 image of at least `sectors` sectors. The root directory is at
// sector 20, subdirectories follow, then each file from a sector of its own.
// Files are listed under their names with ";1".
inline std::vector<u8> make_iso(
    std::size_t sectors,
    const std::vector<IsoFile>& files,
    std::string_view volume = "RATCHETANDCLANK"
) {
    std::vector<std::string> dirs;
    for (const IsoFile& f : files) {
        if (const auto slash = f.path.find('/'); slash != std::string::npos) {
            const std::string d = f.path.substr(0, slash);
            if (std::find(dirs.begin(), dirs.end(), d) == dirs.end()) {
                dirs.push_back(d);
            }
        }
    }
    std::size_t next = 21 + dirs.size();
    std::vector<std::size_t> lba;
    for (const IsoFile& f : files) {
        lba.push_back(next);
        next += std::max<std::size_t>(1, (f.bytes.size() + kSs - 1) / kSs);
    }
    sectors = std::max(sectors, next);
    std::vector<u8> img(sectors * kSs, 0);
    u8* pvd = img.data() + 16 * kSs;
    pvd[0] = 1;
    std::memcpy(pvd + 1, "CD001", 5);
    pvd[6] = 1;
    std::memset(pvd + 8, ' ', 64);
    std::memcpy(pvd + 8, "PLAYSTATION", 11);
    std::memcpy(pvd + 40, volume.data(), volume.size());
    both_endian32(pvd + 80, static_cast<u32>(sectors));
    pvd[128] = 0x00;
    pvd[129] = 0x08;
    const auto root = dir_record(20, kSs, true, std::string_view("\0", 1));
    std::memcpy(pvd + 156, root.data(), 34);

    auto write_dir = [&](std::size_t sector, std::size_t parent, const std::string& prefix) {
        std::vector<u8> d;
        auto add = [&](const std::vector<u8>& r) {
            d.insert(d.end(), r.begin(), r.end());
        };
        add(dir_record(static_cast<u32>(sector), kSs, true, std::string_view("\0", 1)));
        add(dir_record(static_cast<u32>(parent), kSs, true, std::string_view("\1", 1)));
        if (prefix.empty()) {
            for (std::size_t i = 0; i < dirs.size(); ++i) {
                add(dir_record(static_cast<u32>(21 + i), kSs, true, dirs[i]));
            }
        }
        for (std::size_t i = 0; i < files.size(); ++i) {
            const std::string& p = files[i].path;
            const auto slash = p.find('/');
            const std::string dir = slash == std::string::npos ? "" : p.substr(0, slash);
            if (dir != prefix) {
                continue;
            }
            const std::string name = (slash == std::string::npos ? p : p.substr(slash + 1)) + ";1";
            add(dir_record(
                static_cast<u32>(lba[i]), static_cast<u32>(files[i].bytes.size()), false, name
            ));
        }
        std::memcpy(img.data() + sector * kSs, d.data(), d.size());
    };
    write_dir(20, 20, "");
    for (std::size_t i = 0; i < dirs.size(); ++i) {
        write_dir(21 + i, 20, dirs[i]);
    }
    for (std::size_t i = 0; i < files.size(); ++i) {
        std::copy(
            files[i].bytes.begin(),
            files[i].bytes.end(),
            img.begin() + static_cast<std::ptrdiff_t>(lba[i] * kSs)
        );
    }
    return img;
}

// Re-encodes a 2048-byte image as raw 2352-byte mode 2 form 1 sectors.
inline std::vector<u8> to_raw(const std::vector<u8>& img) {
    static constexpr u8 kSync[12] =
        {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0};
    std::vector<u8> out;
    for (std::size_t s = 0; s + kSs <= img.size(); s += kSs) {
        std::vector<u8> raw(2352, 0xaa);  // EDC/ECC stand-in: must never reach user data
        std::memcpy(raw.data(), kSync, 12);
        raw[12] = raw[13] = raw[14] = 0;
        raw[15] = 2;
        std::memset(raw.data() + 16, 0, 8);
        std::memcpy(raw.data() + 24, img.data() + s, kSs);
        out.insert(out.end(), raw.begin(), raw.end());
    }
    return out;
}

inline std::vector<u8> bytes_of(std::string_view s) {
    return {s.begin(), s.end()};
}

inline std::vector<u8> fake_elf(std::string_view tag, std::size_t size = 5000) {
    std::vector<u8> e = bytes_of("\x7f"
                                 "ELF");
    const std::string t = std::format("..{}..", tag);
    e.insert(e.end(), t.begin(), t.end());
    e.resize(size, 0x11);
    return e;
}

inline std::vector<u8> system_cnf(std::string_view serial, std::string_view vmode = "NTSC") {
    return bytes_of(
        std::format("BOOT2 = cdrom0:\\{};1\r\nVER = 1.00\r\nVMODE = {}\r\n\r\n", serial, vmode)
    );
}

// A small RAC1-shaped disc (after ReRAC's synthetic test): the table of
// contents at sector 1500, one level (id 1, table slot 0) at sector 1510 with
// NTSC and PAL gameplay ranges and scene regions; the global save_game lump,
// an NTSC and a PAL movie, a PAL credits image. Sectors from 1500 on are
// filled with a pattern first.
inline std::vector<u8> rac1_disc(std::string_view serial, const std::vector<u8>& elf) {
    std::vector<u8> img =
        make_iso(1600, {{"SYSTEM.CNF", system_cnf(serial)}, {serial.data(), elf}});
    for (std::size_t i = 1500 * kSs; i < img.size(); ++i) {
        img[i] = static_cast<u8>(std::rotr(static_cast<u32>(i - 1500 * kSs) * 2654435761u, 13));
    }
    const std::size_t toc = 1500 * kSs;
    std::fill(
        img.begin() + static_cast<std::ptrdiff_t>(toc),
        img.begin() + static_cast<std::ptrdiff_t>(toc + 0x2960),
        u8{0}
    );
    put32(img, toc, 1);
    put32(img, toc + 4, 0x2960);
    put32(img, toc + 0x10, 1530);  // save_game: 1 sector
    put32(img, toc + 0x14, 1);
    put32(img, toc + 0x1748, 1533);  // credits_images_pal[0]
    put32(img, toc + 0x174c, 1);
    put32(img, toc + 0x17f8 + 21 * 8, 1531);  // mpegs[21], 1000 bytes
    put32(img, toc + 0x17f8 + 21 * 8 + 4, 1000);
    put32(img, toc + 0x17f8 + 40 * 8, 1532);  // mpegs[40], 500 bytes
    put32(img, toc + 0x17f8 + 40 * 8 + 4, 500);
    put32(img, toc + 0x28c8, 1510);  // level table slot 0
    put32(img, toc + 0x28cc, 5);
    const std::size_t h = 1510 * kSs;
    std::fill(
        img.begin() + static_cast<std::ptrdiff_t>(h),
        img.begin() + static_cast<std::ptrdiff_t>(h + 0x2434),
        u8{0}
    );
    const std::pair<std::size_t, s32> header[] = {
        {0, 1},
        {4, 0x2434},
        {8, 1516},
        {12, 2},
        {16, 1518},
        {20, 1},
        {24, 1519},
        {28, 1},
        {0x19c, 1520},
        {0x1a0, 1521},  // scene 0 NTSC: chunk 1520, sentinel 1521
        {0x2b8, 1522},
        {0x2bc, 1523},  // scene 0 PAL
    };
    for (const auto& [o, v] : header) {
        put32(img, h + o, v);
    }
    const std::size_t d = 1516 * kSs;
    std::fill(
        img.begin() + static_cast<std::ptrdiff_t>(d),
        img.begin() + static_cast<std::ptrdiff_t>(d + 0x58),
        u8{0}
    );
    put32(img, d + 0, 0x80);  // overlay
    put32(img, d + 4, 0x20);
    put32(img, d + 0x50, 0x100);  // core_data
    put32(img, d + 0x54, 0x50);
    return img;
}

}  // namespace openrac::test

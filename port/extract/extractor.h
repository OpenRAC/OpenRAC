// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-extract/src (identify.rs,
// extract.rs, space.rs, prepare.rs): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// openrac-extractor: sets a game up from the image of the player's own disc,
// the way OpenGOAL's extractor does for Jak and Daxter. It is the C++ form of
// tools/extractor.py, with the same command line, the same layout and the
// same failures, and does what that one could not yet:
//
//   --extract    copy the disc's files into PROJ/iso_data/GAME/, checking each
//                as it is copied, and keep the image there (disc.iso, a hard
//                link where the file system allows): the games read most of
//                their data by sector, not by file
//   --validate   refuse an image whose boot executable is not a build OpenRAC
//                knows (assets/version.h, from games/*/game.json); implied by
//                --extract
//   --decompile  turn the disc's data into what the port reads, in
//                PROJ/decompiler_out/GAME/: the table of contents, every
//                level's files, their WAD-compressed lumps unpacked, and an
//                index of every other lump (movies, music, speech) by its
//                place in disc.iso. Games whose disc layout is not known yet
//                fail with 4060.
//   --compile    build the native port (not available: 4060)
//
// Every file is written as <name>.partial and renamed once complete, and a
// step's folder is built under a temporary name and renamed at the end, so a
// run that is killed never leaves a complete-looking wrong file.

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include "assets/disc/iso9660.h"
#include "assets/version.h"
#include "extract/report.h"

namespace openrac::extract {

// OpenGOAL's is_iso_file: a PS2 DVD image is larger than this.
inline constexpr std::uint64_t kMinImageSize = 1'000'000'000;

// Kept free beyond what a step writes.
inline constexpr std::uint64_t kSpaceMargin = 64ull << 20;

struct Options {
    std::filesystem::path image;
    std::string game;  // "rac1"
    std::filesystem::path proj = ".";
    bool extract = false;
    bool validate = false;
    bool decompile = false;
    bool compile = false;
    std::uint64_t minimum_size = kMinImageSize;
    // The builds OpenRAC knows; tests pass their own.
    std::span<const assets::GameVersion> known = assets::versions();
};

// What a disc is, from SYSTEM.CNF and its boot executable.
struct Identity {
    std::string serial;
    std::string disc_version;  // SYSTEM.CNF's VER
    std::string vmode;         // SYSTEM.CNF's VMODE
    std::string volume;
    std::uint64_t boot_size = 0;
    std::string boot_sha1;                         // lower-case hex
    std::optional<assets::Game> game;              // the series' game the serial belongs to
    const assets::GameVersion* version = nullptr;  // the known build, if it is one
};

// Throws 4040 or 4041.
void check_image_file(const std::filesystem::path& image, std::uint64_t minimum_size);

// Opens an image for reading; throws 4020 or 4063.
assets::disc::IsoImage open_image(const std::filesystem::path& image);

// Reads SYSTEM.CNF and hashes the boot executable; throws 4000 or 4063.
Identity identify(const assets::disc::IsoImage& iso, std::span<const assets::GameVersion> known);

DiscReport disc_report(const Identity& identity);

// The build `identity` is, when it is a version of `game`; throws 4001 or 4002.
const assets::GameVersion& validate(const Identity& identity, std::string_view game);

// One hash for a set of files whatever order they were read in, as
// tools/extractor.py computes it: SHA-256 of "path\0sha1\n" per file, sorted.
std::string contents_hash(std::span<const std::pair<std::string, std::string>> file_sha1s);

struct BuildInfo {
    std::string serial;
    std::string version;  // "rac1/pal"
    std::string boot_sha1;
    std::size_t files = 0;
    std::string contents;
    std::string image;  // the image's file name
};

// --extract (and --validate): PROJ/iso_data/GAME.
BuildInfo extract_disc(const Options& options, Reporter& report);

// --decompile: PROJ/decompiler_out/GAME from PROJ/iso_data/GAME/disc.iso.
void decompile_disc(const Options& options, Reporter& report);

// The steps `options` asks for (all of them when none is named). Returns the
// exit status: 0, or 1 after reporting the failure.
int run(const Options& options, Reporter& report);

}  // namespace openrac::extract

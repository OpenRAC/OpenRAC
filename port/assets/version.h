// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game versions OpenRAC knows: every disc the port can be set up from,
// with the facts that identify it. The table is written by CMake from
// games/<game>/game.json (cmake/Assets.cmake), so the port, the launcher and
// the decompilations name the same builds with the same checksums.

#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace openrac::assets {

enum class Game : std::uint8_t {
    Rac1,  // Ratchet & Clank (2002)
    Rac2,  // Going Commando (2003)
    Rac3,  // Up Your Arsenal (2004)
    Rac4,  // Deadlocked (2005)
};

std::string_view game_name(Game game);   // "rac1"
std::string_view game_title(Game game);  // "Ratchet & Clank"

enum class Region : std::uint8_t {
    NtscU,
    Pal,
    NtscJ,
};

std::string_view region_name(Region region);  // "NTSC-U"

// One build of one game, as it is on its disc.
struct GameVersion {
    std::string_view id;   // "rac1-pal": the port's name (port/game/<id>)
    std::string_view key;  // "rac1/pal": the decompilation's (games/<game>/<version>)
    Game game;
    Region region;
    std::string_view title;
    std::string_view serial;        // "SCES_509.16": the boot executable's name on the disc
    std::string_view volume;        // the ISO 9660 volume identifier
    std::string_view disc_version;  // SYSTEM.CNF's VER
    std::uint64_t disc_size;        // bytes of a good 2048-byte-sector image
    std::string_view disc_sha1;
    std::uint64_t boot_size;  // bytes of the boot executable
    std::string_view boot_sha1;
    int frame_rate;  // 50 or 60
};

// Every version, in the order of games/*/game.json.
std::span<const GameVersion> versions();

// The version with this boot executable, or null. `boot_sha1` is lower-case hex.
const GameVersion* find_version_by_boot(std::string_view serial, std::string_view boot_sha1);

// The version with this id ("rac1-pal") or key ("rac1/pal"), or null.
const GameVersion* find_version(std::string_view id_or_key);

// The game a serial belongs to when the build itself is not known (another
// region, a demo): from the serials of every known release of the series.
// Returns false when the serial is not Ratchet & Clank.
bool game_of_serial(std::string_view serial, Game& game);

}  // namespace openrac::assets

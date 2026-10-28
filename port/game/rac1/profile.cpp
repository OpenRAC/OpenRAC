// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Ratchet & Clank's profiles (game/common/profile.h): one per disc, PAL (SCES-50916) and NTSC-U
// (SCUS-97199), each answering the host program with game_state.h's reads over its own tables of
// addresses (game_state.cpp), the boot program's and each level program's, and with the
// executable's addresses the frontend reads for the hero's shadow and state.
//
// It defines game_profile() for the programs that link it, and so picks which disc's profile a
// game id names, as rac1_addresses() does.

#include "game/common/profile.h"

#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "game/rac1/game_state.h"
#include "game/rac1/gameplay_lighting.h"
#include "openrac/guest.h"
#include "renderer/math.h"

// Developer tools: host/collision.c's note of the last particle line query.
extern "C" int openrac_rac1_coll_note(std::uint32_t* words);

namespace openrac::game::rac1 {
namespace {

// The number of level programs, each with its own copy of the engine's globals and so its own
// table in game_state.h.
constexpr int kLevels = 19;

// What one disc's executable keeps at addresses of its own, outside game_state.h's tables.
struct ExecutableAddresses {
    std::uint32_t shadow_directions;  // The four directions the hero's shadow is cast along.
    std::uint32_t hero_state;         // The hero's record (games/rac1's D_0013F450 on PAL).
    std::uint32_t world_update_time;  // The frame clock create_moby compares a free slot's +0x38 with.
};

// Each disc's executable addresses: PAL's as games/rac1 has them, NTSC-U's the same data in
// SCUS-97199 (rac1-ntsc's rac1_version.h pairs the hero's record).
constexpr ExecutableAddresses kPalExecutable{0x001CAE00u, 0x0013F450u, 0x0015F6F0u};
constexpr ExecutableAddresses kNtscExecutable{0x001CAA80u, 0x0013F350u, 0x0015F60Cu};

std::string drone_notes(
    std::span<const std::uint8_t> ram, const GameAddresses& a, std::uint32_t hero, std::uint32_t clock
);

// One disc's profile: its boot program's table, its level programs' tables and its executable's
// addresses. Ratchet & Clank's levels are extracted for the port, and its frame is one of two list
// buffers it alternates (the defaults of host::GameProfile).
class Profile final : public host::GameProfile {
public:
    // Makes the profile of the disc whose tables are `boot`, `levels` and `executable`, which
    // outlive it.
    constexpr Profile(
        const GameAddresses& boot,
        const GameAddresses (&levels)[kLevels],
        const ExecutableAddresses& executable
    ) noexcept
        : m_boot(boot),
          m_levels(levels),
          m_executable(executable) {}

    host::GameState read_state(std::span<const std::uint8_t> ram, int level) const override {
        return read_game_state(ram, addresses(level));
    }

    std::optional<host::LiveMoby> read_moby(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const override {
        return read_live_moby(ram, moby);
    }

    bool beyond_draw_distance(
        const std::uint8_t* moby, const renderer::Vec3& eye, const renderer::Vec3& forward
    ) const override {
        return moby_beyond_draw_distance(moby, eye, forward);
    }

    std::uint32_t shown_chain(std::span<const std::uint8_t> ram, int level) const override {
        return rac1::shown_chain(ram, addresses(level));
    }

    std::uint32_t shadow_directions() const override { return m_executable.shadow_directions; }

    std::uint32_t hero_state() const override { return m_executable.hero_state; }

    bool has_levels() const override { return true; }

    bool gameplay_essential(int class_id) const override { return essential_moby(class_id); }

    std::string diagnostics(std::span<const std::uint8_t> ram, int level, std::uint32_t hero) const override {
        return drone_notes(ram, addresses(level), hero, OPENRAC_DATA(m_executable.world_update_time));
    }

private:
    // Returns the table of the program loaded: a level's, or the boot program's when `level` is
    // not one (-1 before the first level).
    const GameAddresses& addresses(int level) const {
        // Only a level number has a table of its own; the title and the menus run the boot program.
        if (level >= 0 && level < kLevels) {
            return m_levels[level];
        }

        return m_boot;
    }

    const GameAddresses& m_boot;               // The boot program's table, a constant.
    const GameAddresses (&m_levels)[kLevels];  // The level programs' tables, constants.
    const ExecutableAddresses& m_executable;   // The executable's addresses, a constant.
};

// Developer tools: the Drone Device's state for a diagnostics recording. The hero's record keeps
// the drones' count at +0x1FF4 to +0x1FF7 (the update's counter, the pending use, the drones made,
// the drones orbiting) and their six mobys at +0x2020; the ammo is the word of item 0x18 in the
// counts 0x1F28 below it. Each drone moby: its state (+0x20), class record (+0x24), scale (+0x2C),
// mode (+0x34), class id (+0xA6) and position; and every moby of the drone class (479) in the
// table, with the table's free slots (state 0xFE or more).
std::string drone_notes(
    std::span<const std::uint8_t> ram, const GameAddresses& a, std::uint32_t hero, std::uint32_t clock
) {
    const auto word = [&](std::uint32_t at) -> std::uint32_t {
        at &= 0x01FFFFFFu;
        if (at + 4 > ram.size()) {
            return 0;
        }
        std::uint32_t v;
        std::memcpy(&v, ram.data() + at, 4);
        return v;
    };
    const auto byte = [&](std::uint32_t at) -> unsigned {
        at &= 0x01FFFFFFu;
        return at < ram.size() ? ram[at] : 0u;
    };
    const auto real = [&](std::uint32_t at) -> float {
        const std::uint32_t v = word(at);
        float f;
        std::memcpy(&f, &v, 4);
        return f;
    };
    const auto moby = [&](std::uint32_t m) {
        return std::format(
            "{{\"at\":\"{:06X}\",\"state\":{},\"class\":{},\"record\":\"{:08X}\",\"scale\":{:.4g},"
            "\"mode\":\"{:04X}\",\"pos\":[{:.4g},{:.4g},{:.4g}]}}",
            m, byte(m + 0x20), static_cast<std::int16_t>(word(m + 0xA4) >> 16), word(m + 0x24), real(m + 0x2C),
            word(m + 0x34) & 0xFFFFu, real(m + 0x10), real(m + 0x14), real(m + 0x18));
    };
    std::string out = std::format(
        "{{\"counter\":{},\"pending\":{},\"made\":{},\"orbiting\":{},\"ammo\":{},\"slots\":[",
        byte(hero + 0x1FF4), byte(hero + 0x1FF5), byte(hero + 0x1FF6), byte(hero + 0x1FF7),
        static_cast<std::int32_t>(word(hero - 0x1F28 + 0x18 * 4)));
    for (int i = 0; i < 6; ++i) {
        const std::uint32_t m = word(hero + 0x2020 + 4u * static_cast<std::uint32_t>(i)) & 0x01FFFFFFu;
        out += i ? "," : "";
        out += m != 0 ? moby(m) : std::string("null");
    }
    out += "],\"class_479\":[";
    auto first = word(a.moby_first) & 0x01FFFFFFu;
    const auto end = word(a.moby_end) & 0x01FFFFFFu;
    unsigned free = 0;
    bool any = false;
    std::string slots;
    if (first != 0 && end > first && end <= ram.size() && (end - first) % 0x100 == 0) {
        for (std::uint32_t m = first; m < end; m += 0x100) {
            if (byte(m + 0x20) >= 0xFE) {
                if (free < 24) {
                    slots += std::format("{}[\"{:06X}\",{},{},{}]", free ? "," : "", m, byte(m + 0x20),
                                         word(m + 0x38), word(m + 0x3C));
                }
                ++free;
                continue;
            }
            if (static_cast<std::int16_t>(word(m + 0xA4) >> 16) == 479) {
                out += any ? "," : "";
                out += moby(m);
                any = true;
            }
        }
    }
    out += std::format("],\"free_slots\":{},\"free\":[{}],\"clock\":[\"{:06X}\",{}],\"range\":[\"{:06X}\",\"{:06X}\"]",
                       free, slots, clock, word(clock), first, end);
    // The last particle line query: its counts this tick, where it wrote CollOutput and where the
    // particle's C reads the hit normal, the particle, the result, the segment and CollOutput +0x18..
    std::uint32_t note[38];
    openrac_rac1_coll_note(note);
    out += std::format(",\"coll\":{{\"calls\":{},\"hits\":{},\"out\":\"{:06X}\",\"normal_at\":\"{:06X}\","
                       "\"particle\":\"{:06X}\",\"result\":{},\"words\":\"",
                       note[0], note[1], note[2], note[3], note[4], note[5]);
    for (int i = 6; i < 38; ++i) {
        out += std::format("{}{:08X}", i > 6 ? " " : "", note[i]);
    }
    // The moby hit (CollOutput +0x18) and its class.
    const std::uint32_t hit = note[12] & 0x01FFFFFFu;
    out += std::format("\",\"hit_class\":{}", hit != 0 ? static_cast<std::int16_t>(word(hit + 0xA4) >> 16) : -1);
    out += std::format(",\"at_normal\":\"{:08X} {:08X} {:08X}\",\"at_point\":\"{:08X} {:08X} {:08X}\"}}}}",
                       word(note[3]), word(note[3] + 4), word(note[3] + 8), word(note[3] - 0x20),
                       word(note[3] - 0x1C), word(note[3] - 0x18));
    return out;
}

// The two discs' profiles, constants for the program's life.
const Profile kPalProfile{kRac1Pal, kRac1PalLevels, kPalExecutable};
const Profile kNtscProfile{kRac1Ntsc, kRac1NtscLevels, kNtscExecutable};

}  // namespace
}  // namespace openrac::game::rac1

namespace openrac::host {

const GameProfile& game_profile(std::string_view game) {
    // The NTSC-U disc's id picks its tables; any other id reads as PAL, as rac1_addresses() does.
    if (game == "rac1-ntsc") {
        return game::rac1::kNtscProfile;
    }

    return game::rac1::kPalProfile;
}

}  // namespace openrac::host

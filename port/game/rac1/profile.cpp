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

#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "game/rac1/game_state.h"
#include "game/rac1/gameplay_lighting.h"
#include "game/rac1/host/collision_notes.h"
#include "openrac/guest.h"
#include "renderer/math.h"

namespace openrac::game::rac1 {
namespace {

// The number of level programs, each with its own copy of the engine's globals and so its own
// table in game_state.h.
constexpr int kLevels = 19;

// What one disc's executable keeps at addresses of its own, outside game_state.h's tables.
struct ExecutableAddresses {
    std::uint32_t shadow_directions;  // The four directions the hero's shadow is cast along.
    std::uint32_t hero_state;         // The hero's record (games/rac1's D_0013F450 on PAL).
    std::uint32_t world_update_time;  // The frame clock create_moby compares a slot's +0x38 to.
    std::uint32_t particle_updates;   // The particle updates, by a particle record's type byte.
};

// Each disc's executable addresses: PAL's as games/rac1 has them, NTSC-U's the same data in
// SCUS-97199 (rac1-ntsc's rac1_version.h pairs the hero's record).
constexpr ExecutableAddresses kPalExecutable{0x001CAE00u, 0x0013F450u, 0x0015F6F0u, 0x001CE100u};
constexpr ExecutableAddresses kNtscExecutable{0x001CAA80u, 0x0013F350u, 0x0015F60Cu, 0x001CDD80u};

// Developer tools, for a diagnostics recording: the moby table's spawn range (create_moby takes
// a slot there whose state is 0xFE or more and whose +0x38 the frame clock has reached), with its
// free slots and the classes of the mobys spawned in it; and, with host/collision.c's watch, the
// collision queries that started near Ratchet since the last tick, after which it watches him
// again.
std::string notes(
    std::span<const std::uint8_t> ram,
    const GameAddresses& a,
    const host::GameState& state,
    std::uint32_t clock,
    OpenracCollWatch* watch
) {
    const auto word = [&](std::uint32_t at) -> std::uint32_t {
        at = openrac_guest_ram_address_for_size(at, static_cast<std::uint32_t>(ram.size()));
        if (at + 4 > ram.size()) {
            return 0;
        }
        std::uint32_t v;
        std::memcpy(&v, ram.data() + at, 4);
        return v;
    };
    const auto byte = [&](std::uint32_t at) -> unsigned {
        at = openrac_guest_ram_address_for_size(at, static_cast<std::uint32_t>(ram.size()));
        return at < ram.size() ? ram[at] : 0u;
    };
    const auto number = [](float v) {
        return std::isfinite(v) ? std::format("{:.6g}", v) : std::string("null");
    };
    const auto class_of = [&](std::uint32_t moby) {
        return static_cast<std::int16_t>(word(moby + 0xA4) >> 16);
    };

    const std::uint32_t first = openrac_guest_ram_address_for_size(
        word(a.moby_first), static_cast<std::uint32_t>(ram.size())
    );
    const std::uint32_t end = openrac_guest_ram_address_for_size(
        word(a.moby_end), static_cast<std::uint32_t>(ram.size())
    );
    std::string free_slots;
    unsigned free = 0;
    std::map<int, unsigned> classes;
    if (first != 0 && end > first && end <= ram.size() && (end - first) % 0x100 == 0) {
        for (std::uint32_t m = first; m < end; m += 0x100) {
            if (byte(m + 0x20) < 0xFE) {
                ++classes[class_of(m)];
                continue;
            }
            if (free < 24) {
                // The slot, its state and its spawn frame (+0x38, both words).
                free_slots += std::format(
                    "{}[\"{:06X}\",{},{},{}]",
                    free ? "," : "",
                    m,
                    byte(m + 0x20),
                    word(m + 0x38),
                    word(m + 0x3C)
                );
            }
            ++free;
        }
    }
    std::string out = std::format("{{\"mobys\":{{\"range\":[\"{:06X}\",\"{:06X}\"]", first, end);
    out += std::format(",\"clock\":{},\"free\":{}", static_cast<std::int32_t>(word(clock)), free);
    out += ",\"free_slots\":[" + free_slots + "],\"classes\":{";
    bool any = false;
    for (const auto& [id, count] : classes) {
        out += std::format("{}\"{}\":{}", any ? "," : "", id, count);
        any = true;
    }
    out += "}}";

    if (watch != nullptr) {
        out += std::format(
            ",\"collision\":{{\"calls\":{},\"hits\":{},\"near\":[", watch->calls, watch->hits
        );
        for (std::uint32_t i = 0; i < watch->count && i < std::size(watch->notes); ++i) {
            const OpenracCollNote& n = watch->notes[i];
            out += std::format(
                "{}{{\"entry\":{},\"flags\":\"{:X}\",\"result\":{},\"out\":\"{:06X}\"",
                i ? "," : "",
                n.entry,
                n.flags,
                n.result,
                n.out
            );
            out += std::format(
                ",\"ignore\":[\"{:06X}\",\"{:08X}\"],\"template\":\"{:06X}\"",
                n.ignore,
                n.ignore_word,
                n.tmpl
            );
            out += std::format(
                ",\"a\":[{},{},{}],\"b\":[{},{},{}],\"output\":\"",
                number(n.a[0]),
                number(n.a[1]),
                number(n.a[2]),
                number(n.b[0]),
                number(n.b[1]),
                number(n.b[2])
            );
            for (std::size_t k = 0; k < std::size(n.output); ++k) {
                out += std::format("{}{:08X}", k ? " " : "", n.output[k]);
            }
            const std::uint32_t hit = openrac_guest_ram_address_for_size(
                n.output[0], static_cast<std::uint32_t>(ram.size())
            );
            out += std::format("\",\"hit_class\":{}}}", hit != 0 ? class_of(hit) : -1);
        }
        out += "]}";
        // The next tick's queries, near where Ratchet is now.
        watch->calls = watch->hits = watch->count = 0;
        watch->rr = 0.0f;
        for (const host::LiveMoby& m : state.mobys) {
            if (m.class_id == 0) {
                watch->point[0] = m.matrix[12];
                watch->point[1] = m.matrix[13];
                watch->point[2] = m.matrix[14];
                watch->rr = 3.0f * 3.0f;
                break;
            }
        }
    }
    out += "}";
    return out;
}

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

    std::uint32_t particle_updates() const override { return m_executable.particle_updates; }

    std::string diagnostics(
        std::span<const std::uint8_t> ram,
        int level,
        const host::GameState& state,
        const host::DiagnosticsInput& input
    ) const override {
        return notes(
            ram,
            addresses(level),
            state,
            input.relocate(m_executable.world_update_time),
            static_cast<OpenracCollWatch*>(input.game)
        );
    }

    void diagnostics_off(const host::DiagnosticsInput& input) const override {
        if (auto* watch = static_cast<OpenracCollWatch*>(input.game)) {
            watch->rr = 0.0f;
        }
    }

    // The water lily class is shared by PAL and NTSC (editor/moby_classes.tsv).
    bool floats_on_water(int class_id) const override { return class_id == 695; }

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

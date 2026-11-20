// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "program/moby_states.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include "common/log.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

namespace openrac::program::moby_states {
namespace {

std::filesystem::path g_out;
long g_ticks = 0;
// Ticks the level has run so far, and ticks waited for it.
long g_in_level = 0;
long g_waited = 0;

// A level that never comes (a wrong number, a broken file) ends the run after this many ticks.
constexpr long kMostWait = 60 * 60 * 2;

constexpr std::uint32_t kPlaced = 0x4u;
constexpr std::uint32_t kDead = 0x10u;
constexpr std::uint32_t kHidden = 0x40u;

// A coordinate as JSON can hold it (a dead moby's may be anything).
float finite(float v) {
    return std::isfinite(v) ? v : 0.0f;
}

void write(const std::vector<openrac_game_moby_state>& mobys) {
    std::ofstream out(g_out, std::ios::binary | std::ios::trunc);
    out << "{\"format\": 1, \"game\": \"" << openrac_game.id
        << "\", \"level\": " << openrac_game_start_level << ", \"ticks\": " << g_ticks
        << ", \"mobys\": [";
    bool first = true;
    for (const openrac_game_moby_state& m : mobys) {
        // Only the level's own mobys, by their record: the editor places those.
        if (!(m.flags & kPlaced) || m.key < 0) {
            continue;
        }
        out << (first ? "\n" : ",\n") << "  {\"key\": " << m.key << ", \"class\": " << m.oclass
            << ", \"state\": " << m.state << ", \"hidden\": " << ((m.flags & kHidden) ? 1 : 0)
            << ", \"dead\": " << ((m.flags & kDead) ? 1 : 0) << ", \"pos\": ["
            << finite(m.position[0]) << ", " << finite(m.position[1]) << ", "
            << finite(m.position[2]) << "]}";
        first = false;
    }
    out << "\n]}\n";
    if (!out) {
        log::error("--moby-states: cannot write {}", g_out.string());
        std::exit(1);
    }
}

}  // namespace

void request(const std::filesystem::path& out, long ticks) {
    g_out = out;
    g_ticks = ticks < 1 ? 1 : ticks;
}

bool requested() {
    return !g_out.empty();
}

void tick() {
    if (g_out.empty()) {
        return;
    }
    const openrac_game_play* play = openrac_game_play_host;
    // The level asked for, loaded and holding its mobys: its ticks count from here.
    std::vector<openrac_game_moby_state> mobys;
    if (play != nullptr && play->mobys != nullptr
        && openrac_game_loaded_overlay() == openrac_game_start_level) {
        mobys.resize(4096);
        mobys.resize(static_cast<std::size_t>(play->mobys(mobys.data(), 4096)));
    }
    if (mobys.empty()) {
        if (++g_waited > kMostWait) {
            log::error("--moby-states: level {} never came", openrac_game_start_level);
            std::exit(1);
        }
        return;
    }
    if (++g_in_level < g_ticks) {
        return;
    }
    write(mobys);
    log::info("--moby-states: {} after {} ticks", g_out.string(), g_ticks);
    std::exit(0);
}

}  // namespace openrac::program::moby_states

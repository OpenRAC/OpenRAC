// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The developer tools' switches, once a vertical blank (docs/port/DEBUGGING.md, Going anywhere):
// what the hero is given, kept alive, or moved to, and the check that a scripted run has not stuck.
// What they do in a game is that game's (openrac_game_tools, game_host.h); a game that installs
// nothing has none of them but the stuck check and the state dump. Frames count from control, as
// OPENRAC_PRESS's c entries do, so the same script means the same moment in every run.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "common/crash.h"
#include "common/env.h"
#include "common/log.h"
#include "openrac/game_host.h"
#include "openrac/memory.h"
#include "program/program.h"

#ifdef OPENRAC_FRONTEND
#include "frontend.h"
#endif

#if !defined(_WIN32)
#include <csignal>
#include <pthread.h>
#endif

namespace openrac::program {
namespace {

// How long a teleport is held: a shorter write was undone by the game on rac1's Novalis
// (RAC1_PAL_TESTING.md, section 4).
constexpr int kTeleportHold = 10;

// The value of a variable, as a string that ends in a NUL (sscanf and fopen want one).
std::optional<std::string> env_string(env::Var var) {
    const std::optional<std::string_view> found = env::value(var);
    return found ? std::optional<std::string>(std::string(*found)) : std::nullopt;
}

// The frames since the player got control, or -1 before and without a window.
std::int64_t since_control() {
#ifdef OPENRAC_FRONTEND
    return g_window ? frontend::since_control() : -1;
#else
    return -1;
#endif
}

const openrac_game_tools* tools() {
    return openrac_game_tools_host;
}

// OPENRAC_GIVE=WHAT[@FRAME],...: each entry once, FRAME frames after control (0 by default).
void apply_give(std::int64_t since) {
    static const std::optional<std::string> script = env_string(env::Var::Give);
    static std::uint64_t done = 0;  // a bit per entry, the first 64

    if (!script || since < 0 || tools() == nullptr || tools()->give == nullptr) {
        return;
    }

    std::string_view rest = *script;

    for (unsigned entry = 0; !rest.empty() && entry < 64; ++entry) {
        const std::size_t comma = rest.find(',');
        std::string_view item = rest.substr(0, comma);
        rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
        const std::size_t at = item.find('@');
        const long frame = at == std::string_view::npos
                               ? 0
                               : std::strtol(std::string(item.substr(at + 1)).c_str(), nullptr, 10);

        if ((done >> entry & 1u) != 0 || since < frame) {
            continue;
        }

        done |= std::uint64_t{1} << entry;
        const std::string what(item.substr(0, at));
        const int changed = tools()->give(what.c_str());

        if (changed < 0) {
            log::warn("OPENRAC_GIVE: this game has no {}", what);
        } else {
            log::info(
                "OPENRAC_GIVE: {}, {} changed, {} frames after control", what, changed, since
            );
        }
    }
}

// OPENRAC_TELEPORT=FRAME:X:Y:Z,...: the hero held there kTeleportHold frames, from FRAME frames
// after control.
void apply_teleports(std::int64_t since) {
    static const std::optional<std::string> script = env_string(env::Var::Teleport);

    if (!script || since < 0 || tools() == nullptr || tools()->teleport == nullptr) {
        return;
    }

    for (const char* at = script->c_str(); at && *at;) {
        long frame = 0;
        float xyz[3]{};

        if (std::sscanf(at, "%ld:%f:%f:%f", &frame, &xyz[0], &xyz[1], &xyz[2]) == 4
            && since >= frame && since < frame + kTeleportHold) {
            tools()->teleport(xyz);
        }

        at = std::strchr(at, ',');
        at = at ? at + 1 : nullptr;
    }
}

// ---- OPENRAC_STUCK=SECONDS[:FRAMES] ----

// The time of the last vertical blank, on the clock of steady_clock, in milliseconds.
std::atomic<std::int64_t> g_last_blank{0};
long g_stuck_seconds = 0;

std::int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

#if !defined(_WIN32)
pthread_t g_game_thread;

// On the game's thread, wherever it spins: the report's backtrace is the function that does.
void on_stuck(int) {
    crash::Text reason;
    reason.str("stuck: no vertical blank for ").dec(g_stuck_seconds).str(" seconds");
    crash::report(reason.c_str(), crash::Kind::Fatal);
    std::_Exit(4);
}
#endif

// Starts the watch over the blanks, once, from the game's thread. A pause (F5) holds the game
// inside a blank, which looks the same: the switch is for scripted runs.
void watch_blanks() {
#if !defined(_WIN32)
    g_game_thread = pthread_self();
    struct sigaction action{};
    action.sa_handler = on_stuck;
    action.sa_flags = SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    sigaction(SIGUSR2, &action, nullptr);
    std::thread([] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (now_ms() - g_last_blank.load() > g_stuck_seconds * 1000) {
                pthread_kill(g_game_thread, SIGUSR2);
                return;
            }
        }
    }).detach();
#endif
}

// A stuck run: the tick goes on but the game does not (a load that waits for what no stub ever
// sets). FRAMES window frames with no new level and no control.
void check_progress(int level, std::int64_t since, long frames) {
    static int seen_level = -2;
    static long seen_at = 0;

    if (level != seen_level) {
        seen_level = level;
        seen_at = g_frame;
    }

    if (frames > 0 && since < 0 && g_frame - seen_at > frames) {
        const std::string reason = std::format(
            "stuck: {} frames with no control in {}",
            g_frame - seen_at,
            openrac_game_level_name(level)
        );
        crash::report(reason.c_str(), crash::Kind::Fatal);
        std::_Exit(4);
    }
}

void check_stuck(int level, std::int64_t since) {
    static const std::optional<std::string> script = env_string(env::Var::Stuck);
    static long frames = 0;

    if (!script) {
        return;
    }

    if (g_stuck_seconds == 0) {
        g_stuck_seconds = std::max(1l, std::strtol(script->c_str(), nullptr, 10));
        const std::size_t colon = script->find(':');
        frames =
            colon == std::string::npos ? 0 : std::strtol(script->c_str() + colon + 1, nullptr, 10);
        g_last_blank = now_ms();
        watch_blanks();
    }

    g_last_blank = now_ms();
    check_progress(level, since, frames);
}

}  // namespace

void run_tools() {
    const std::int64_t since = since_control();
    const int level = openrac_game_loaded_overlay();
    runtime::rearm_soft_faults();
    check_stuck(level, since);
    apply_give(since);
    apply_teleports(since);

    if (tools() != nullptr) {
        static const bool invincible = env::flag(env::Var::Invincible);
        static const bool skip_scenes = env::flag(env::Var::SkipScenes);

        if (invincible && tools()->heal != nullptr) {
            tools()->heal();
        }

        if (skip_scenes && tools()->end_scene != nullptr) {
            tools()->end_scene();
        }
    }
}

void write_state_dump() {
    static const std::optional<std::string> path = env_string(env::Var::StateDump);

    if (!path || path->empty()) {
        return;
    }

    const int level = openrac_game_loaded_overlay();
    std::string json = std::format(
        "{{\"game\":\"{}\",\"frame\":{},\"level\":{},\"level_name\":\"{}\",\"since_control\":{}",
        openrac_game.id,
        g_frame,
        level,
        openrac_game_level_name(level),
        since_control()
    );
    std::uint32_t mode = 0;

    if (openrac_game_mode != nullptr && openrac_game_mode(&mode) != 0) {
        json += std::format(",\"mode\":{}", mode);
    }

    float position[3]{}, rotation[3]{};
    std::int32_t state = 0;

    if (openrac_game_hero != nullptr && openrac_game_hero(position, rotation, &state) != 0) {
        json += std::format(
            ",\"hero\":{{\"position\":[{},{},{}],\"rotation\":[{},{},{}],\"state\":{}}}",
            position[0],
            position[1],
            position[2],
            rotation[0],
            rotation[1],
            rotation[2],
            state
        );
    }

    if (tools() != nullptr && tools()->state_json != nullptr) {
        static char members[8192];

        if (tools()->state_json(members, sizeof members) > 0) {
            json += ",";
            json += members;
        }
    }
#ifdef OPENRAC_FRONTEND
    if (g_window) {
        json += ",\"walk\":" + frontend::walk_state();
    }
#endif
    json += "}\n";

    if (std::FILE* f = std::fopen(path->c_str(), "w")) {
        std::fwrite(json.data(), 1, json.size(), f);
        std::fclose(f);
    } else {
        log::warn("OPENRAC_STATE_DUMP: cannot write {}", *path);
    }
}

}  // namespace openrac::program

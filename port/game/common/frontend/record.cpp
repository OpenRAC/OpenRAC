// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A run's pad and path in a file (docs/port/DEBUGGING.md, Going anywhere). OPENRAC_RECORD writes
// one; OPENRAC_REPLAY plays its pad back, the same buttons at the same frames, which repeats the
// run where the run is deterministic (OPENRAC_AUDIO_TICKED); OPENRAC_WALK walks through its points
// instead, steering the left stick at the next one, so it gets there however the level's timing
// differs, and a path recorded on one build still walks on the next.
//
// The file is text, a line for each change of the pad and one for each point:
//
//   openrac-play 1 rac2-ntsc 4           the game and the level recorded
//   f 120 bfff 128 128 128 128           from window frame 120, before control: buttons (active
//                                        low), right x y, left x y
//   c 30 ffff 128 128 128 0              the same, 30 frames after control
//   p 45 12.50 80.25 31.00               at 45 frames after control the hero was at x y z

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "common/env.h"
#include "common/log.h"
#include "frontend.h"
#include "game/common/frontend/state.h"
#include "openrac/game_host.h"

namespace openrac::frontend {
namespace {

// A pad as the game reads it.
struct PadState {
    std::uint16_t buttons = 0xFFFF;
    std::uint8_t analog[4] = {128, 128, 128, 128};

    bool operator==(const PadState& o) const {
        return buttons == o.buttons && std::memcmp(analog, o.analog, sizeof analog) == 0;
    }
};

// A pad held from a frame on.
struct Held {
    bool from_control = false;
    std::int64_t frame = 0;
    PadState pad;
};

// A point of a walk.
struct Point {
    float x, y, z;
};

// Every this many frames a recording notes where the hero is.
constexpr std::int64_t kPointEvery = 15;
// A walk counts a point reached this near it, in game units.
constexpr float kArrived = 2.0f;
// A hero that came no nearer the point for this many frames jumps...
constexpr int kJumpAfter = 45;
// ...and after this many the point is given up, and the walk goes on to the next.
constexpr int kGiveUpAfter = 300;
constexpr std::uint16_t kCross = 0x4000;

// Reads OPENRAC_REPLAY's or OPENRAC_WALK's file.
void read_play(const std::string& path, std::vector<Held>* pad, std::vector<Point>* points) {
    std::ifstream in(path);

    if (!in) {
        log::warn("cannot read {}", path);
        return;
    }

    std::string line;

    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::string kind;
        row >> kind;

        if ((kind == "f" || kind == "c") && pad != nullptr) {
            Held held;
            std::string buttons;
            unsigned a[4]{};
            row >> held.frame >> buttons >> a[0] >> a[1] >> a[2] >> a[3];
            held.from_control = kind == "c";
            held.pad.buttons =
                static_cast<std::uint16_t>(std::strtoul(buttons.c_str(), nullptr, 16));

            for (int i = 0; i < 4; ++i) {
                held.pad.analog[i] = static_cast<std::uint8_t>(a[i]);
            }

            pad->push_back(held);
        } else if (kind == "p" && points != nullptr) {
            std::int64_t frame = 0;
            Point p{};
            row >> frame >> p.x >> p.y >> p.z;
            points->push_back(p);
        }
    }

    log::info(
        "{}: {} pad changes, {} points", path, pad ? pad->size() : 0, points ? points->size() : 0
    );
}

// OPENRAC_RECORD: the pad's changes and the hero's path, as they happen.
void record(const PadState& now) {
    static const std::optional<std::string> path = env_string(env::Var::Record);
    static std::FILE* file = nullptr;
    static PadState last;
    static std::uint64_t written_frame = ~std::uint64_t{0};
    static bool first = true;

    if (!path) {
        return;
    }

    if (first) {
        first = false;
        file = std::fopen(path->c_str(), "w");

        if (file == nullptr) {
            log::warn("OPENRAC_RECORD: cannot write {}", *path);
        }
    }

    if (file == nullptr || written_frame == g->index) {
        return;
    }

    written_frame = g->index;
    static bool header = false;

    if (!header && g_route.level >= 0) {
        header = true;
        std::fprintf(file, "openrac-play 1 %s %d\n", openrac_game.id, g_route.level);
    }

    const std::int64_t since = since_control();

    if (!(now == last)) {
        last = now;
        std::fprintf(
            file,
            "%c %lld %04x %u %u %u %u\n",
            since >= 0 ? 'c' : 'f',
            static_cast<long long>(since >= 0 ? since : static_cast<std::int64_t>(g->index)),
            now.buttons,
            now.analog[0],
            now.analog[1],
            now.analog[2],
            now.analog[3]
        );
    }

    float p[3]{}, r[3]{};
    std::int32_t state = 0;

    if (since >= 0 && since % kPointEvery == 0 && openrac_game_hero != nullptr
        && openrac_game_hero(p, r, &state) != 0) {
        std::fprintf(
            file, "p %lld %.2f %.2f %.2f\n", static_cast<long long>(since), p[0], p[1], p[2]
        );
    }

    std::fflush(file);  // a crash keeps every frame before it
}

// OPENRAC_REPLAY: overrides the pad with the file's, frame by frame.
bool replay(PadState* pad) {
    static const std::optional<std::string> path = env_string(env::Var::Replay);
    static std::vector<Held> held;
    static std::size_t next = 0;
    static PadState now;
    static bool read = false;

    if (!path) {
        return false;
    }

    if (!read) {
        read = true;
        read_play(*path, &held, nullptr);
    }

    const std::int64_t since = since_control();

    while (next < held.size()) {
        const Held& h = held[next];
        const bool due =
            h.from_control ? since >= h.frame : static_cast<std::int64_t>(g->index) >= h.frame;

        if (!due) {
            break;
        }

        now = h.pad;
        ++next;
    }

    *pad = now;
    return true;
}

// OPENRAC_WALK: where the walk is.
struct Walk {
    std::vector<Point> points;
    bool read = false;
    std::size_t next = 0;
    unsigned reached = 0, gave_up = 0;
    float nearest = 0.0f;
    int waited = 0;
    float forward[2] = {1.0f, 0.0f};
};

Walk g_walk;

// Steers the left stick at the next point of OPENRAC_WALK.
void walk(PadState* pad) {
    static const std::optional<std::string> path = env_string(env::Var::Walk);

    if (!path || since_control() < 0) {
        return;
    }

    if (!g_walk.read) {
        g_walk.read = true;
        read_play(*path, nullptr, &g_walk.points);
    }

    float hero[3]{}, rotation[3]{};
    std::int32_t state = 0;

    if (g_walk.next >= g_walk.points.size() || openrac_game_hero == nullptr
        || openrac_game_hero(hero, rotation, &state) == 0) {
        return;
    }

    const Point& target = g_walk.points[g_walk.next];
    const float dx = target.x - hero[0], dy = target.y - hero[1];
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < kArrived) {
        ++g_walk.reached;
    } else if (g_walk.waited >= kGiveUpAfter) {
        ++g_walk.gave_up;
        log::info(
            "walk: point {} ({:.1f} {:.1f} {:.1f}) given up, {:.1f} away",
            g_walk.next,
            target.x,
            target.y,
            target.z,
            distance
        );
    } else {
        if (g_walk.waited == 0 || distance < g_walk.nearest - 0.25f) {
            g_walk.nearest = distance;
            g_walk.waited = 1;
        } else {
            ++g_walk.waited;
        }

        // The left stick is relative to the camera: up its forward, right its right.
        const float ux = dx / distance, uy = dy / distance;
        const float fx = g_walk.forward[0], fy = g_walk.forward[1];
        const float ahead = ux * fx + uy * fy, across = ux * fy - uy * fx;
        pad->analog[2] =
            static_cast<std::uint8_t>(std::clamp(128.0f + across * 127.0f, 0.0f, 255.0f));
        pad->analog[3] =
            static_cast<std::uint8_t>(std::clamp(128.0f - ahead * 127.0f, 0.0f, 255.0f));

        if (g_walk.waited > kJumpAfter && g_walk.waited % kJumpAfter < 5) {
            pad->buttons = static_cast<std::uint16_t>(pad->buttons & ~kCross);
        }

        return;
    }

    ++g_walk.next;
    g_walk.waited = 0;
}

}  // namespace

void scripted_pad(std::uint16_t& buttons, std::uint8_t analog[4]) {
    PadState now;
    now.buttons = buttons;
    std::memcpy(now.analog, analog, sizeof now.analog);
    replay(&now);
    walk(&now);
    record(now);
    buttons = now.buttons;
    std::memcpy(analog, now.analog, sizeof now.analog);
}

void scripted_view(const float forward[3]) {
    const float length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1]);

    if (length > 0.001f) {
        g_walk.forward[0] = forward[0] / length;
        g_walk.forward[1] = forward[1] / length;
    }
}

std::string walk_state() {
    if (!g_walk.read) {
        return "null";
    }

    return std::format(
        "{{\"points\":{},\"reached\":{},\"gave_up\":{},\"next\":{}}}",
        g_walk.points.size(),
        g_walk.reached,
        g_walk.gave_up,
        g_walk.next
    );
}

}  // namespace openrac::frontend

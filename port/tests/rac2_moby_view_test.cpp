// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's profile gives MobyProc's view (GameState::moby_view) from each program's own
// globals, and leaves a moby past its draw distance or the program's cap undrawn, as the game does.
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

#include "game/common/profile.h"
#include "tests/check.h"

namespace {
using namespace openrac;

void put_float(std::vector<std::uint8_t>& ram, std::uint32_t address, float value) {
    std::memcpy(ram.data() + address, &value, sizeof(value));
}

void put_word(std::vector<std::uint8_t>& ram, std::uint32_t address, std::int32_t value) {
    std::memcpy(ram.data() + address, &value, sizeof(value));
}

// One program's globals, USA v1.01's (game/rac2/profile.cpp kNtsc): BuildViewMatrices's rows,
// UpdateViewContext's block and CreateMoby's table end.
struct Program {
    int level;
    std::uint32_t camera_rows;
    std::uint32_t view_context;
    std::uint32_t moby_end;
};

// The camera at x = 100 looking down +x; the near plane 0.1 units, the cap `cap` units.
std::vector<std::uint8_t> memory(const Program& p, std::int32_t cap) {
    std::vector<std::uint8_t> ram(32u << 20);
    const std::uint32_t block = p.camera_rows - 0x370;
    put_float(ram, block + 0x08, 1.0f);  // the view matrix's third column: the depth axis
    put_float(ram, p.camera_rows - 0x230, 100.0f);
    put_float(ram, p.camera_rows, 1.0f);
    put_float(ram, p.camera_rows + 0x18, 1.0f);
    put_float(ram, p.camera_rows + 0x28, 1.0f);
    put_float(ram, p.view_context + 0xA0, 0.1f * 1024.0f);
    put_word(ram, p.moby_end + 0x10, cap);
    return ram;
}

// A moby `depth` units ahead of the camera, of draw distance `dd`.
std::optional<std::uint8_t> fade_at(
    const host::GameProfile& profile, const host::GameState& state, float depth, std::int16_t dd
) {
    std::uint8_t moby[0x100] = {};
    const float sphere[4] = {(100.0f + depth) * 1024.0f, 0.0f, 0.0f, 0.0f};
    std::memcpy(moby, sphere, sizeof(sphere));
    std::memcpy(moby + 0x32, &dd, sizeof(dd));
    return profile.moby_distance_fade(moby, state);
}

void reads_each_programs_view() {
    const host::GameProfile& profile = host::game_profile("rac2-ntsc");
    for (const Program& p :
         {Program{-1, 0x001B5570, 0x001B9040, 0x001B1B64},
          Program{1, 0x001B96F0, 0x001BD9C0, 0x001B2564},
          Program{24, 0x001B88F0, 0x001BCBC0, 0x001B17A4}}) {
        const auto ram = memory(p, 960);
        const host::GameState state = profile.read_state(ram, p.level);
        CHECK(state.moby_view.has_value());
        if (!state.moby_view) {
            continue;
        }
        CHECK(state.moby_view->draw_cap == 960);
        CHECK(state.moby_view->near_plane == 0.1f * 1024.0f);
        CHECK(state.moby_view->depth_axis[0] == 1.0f);
        CHECK(state.moby_view->depth_axis[1] == 0.0f);
        CHECK(state.moby_view->depth_axis[2] == 0.0f);
    }
}

void culls_past_the_distance_and_the_cap() {
    const host::GameProfile& profile = host::game_profile("rac2-ntsc");
    const Program boot{-1, 0x001B5570, 0x001B9040, 0x001B1B64};
    auto ram = memory(boot, 960);
    host::GameState state = profile.read_state(ram, -1);
    CHECK(fade_at(profile, state, 40.0f, 64) == std::uint8_t{0x80});
    CHECK(fade_at(profile, state, 56.0f, 64) == std::uint8_t{0x40});  // the last 16 units fade
    CHECK(!fade_at(profile, state, 65.0f, 64));                       // past +0x32
    CHECK(fade_at(profile, state, 900.0f, 4000) == std::uint8_t{0x80});
    CHECK(!fade_at(profile, state, 970.0f, 4000));  // past the cap
    CHECK(!fade_at(profile, state, -5.0f, 64));     // behind the eye
    // The cap lowered (every level has a routine that sets 144 for a while): culled sooner.
    ram = memory(boot, 144);
    state = profile.read_state(ram, -1);
    CHECK(!fade_at(profile, state, 150.0f, 4000));
    CHECK(fade_at(profile, state, 100.0f, 4000) == std::uint8_t{0x80});
}
}  // namespace

int main() {
    reads_each_programs_view();
    culls_past_the_distance_and_the_cap();
    return openrac::test::result();
}

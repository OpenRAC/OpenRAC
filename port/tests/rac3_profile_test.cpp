// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Up Your Arsenal's PAL profile (game/rac3-pal/profile.cpp) reads each program's own globals at
// rac3's offsets where its blocks moved from Going Commando's (the eye, the view block, the fog,
// the cap), and a moby record at Going Commando's offsets, which MobyProc shows are rac3's too.
#include <array>
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

void put_word(std::vector<std::uint8_t>& ram, std::uint32_t address, std::uint32_t value) {
    std::memcpy(ram.data() + address, &value, sizeof(value));
}

// One program's globals (profile.cpp kPal): BuildViewMatrices' rows, UpdateFog's view context,
// moby_spawn's table and MobyProc's cap.
struct Program {
    int level;
    std::uint32_t camera_rows;
    std::uint32_t view_context;
    std::uint32_t moby_end;
    std::uint32_t moby_first;
    std::uint32_t moby_cap;  // 0: the program's MobyProc is not known
};

constexpr Program kBoot{-1, 0x00222420, 0x00225900, 0x001DA4A4, 0x001DA4A0, 0x001DA4D0};
constexpr Program kFrontbin{100, 0x00222420, 0x00225900, 0x001DA4A4, 0x001DA4A0, 0x001DA4D0};
constexpr Program kVeldin{1, 0x001E29E0, 0x001E7580, 0x001E0024, 0x001E0020, 0x001E0050};
constexpr Program kMultiplayerMenu{39, 0x0024DE20, 0x0025D6C0, 0x0024BF64, 0x0024BF60, 0};

// Where the mobys go in the test's memory: the table's start, two placed mobys, then the spawned.
constexpr std::uint32_t kTable = 0x00800000;

// The camera at (100, 200, 300) looking down +x; the fog and the colours rac3's offsets apart.
std::vector<std::uint8_t> memory(const Program& p) {
    std::vector<std::uint8_t> ram(32u << 20);
    const std::uint32_t rows = p.camera_rows;
    put_float(ram, rows - 0x20, 100.0f);  // the eye (the view block's +0x140)
    put_float(ram, rows - 0x1C, 200.0f);
    put_float(ram, rows - 0x18, 300.0f);
    put_float(ram, rows + 0x00, 1.0f);          // forward
    put_float(ram, rows + 0x14, 1.0f);          // left
    put_float(ram, rows + 0x28, 1.0f);          // up
    put_float(ram, rows - 0x160 + 0x08, 1.0f);  // the view matrix's third column: the depth axis
    const std::uint32_t vc = p.view_context;
    put_float(ram, vc + 0xA0, 0.1f * 1024.0f);
    put_float(ram, vc + 0xB0, 0.62f);
    put_float(ram, vc + 0xB4, 0.4805f);
    put_float(ram, vc + 0xC0, 413.0f / 32.0f);
    put_float(ram, vc + 0xD4, 433.0f / 32.0f);
    put_float(ram, vc + 0x228, 1000.0f);  // UpdateFog: near, far, F at each
    put_float(ram, vc + 0x22C, 5000.0f);
    put_float(ram, vc + 0x238, 250.0f);
    put_float(ram, vc + 0x23C, 40.0f);
    put_word(ram, vc + 0x250, 0x33);  // FOGCOL r, g, b
    put_word(ram, vc + 0x254, 0x66);
    put_word(ram, vc + 0x258, 0x99);
    put_word(ram, vc + 0x25C, 0xFF);  // the background r, g, b
    put_word(ram, vc + 0x260, 0x00);
    put_word(ram, vc + 0x264, 0x80);
    if (p.moby_cap != 0) {
        put_word(ram, p.moby_cap, 960);
    }
    put_word(ram, p.moby_first - 4, kTable);
    put_word(ram, p.moby_first, kTable + 0x200);
    put_word(ram, p.moby_end, kTable + 0x400);
    return ram;
}

// A moby record laid out as Going Commando's (rac2_moby_view_test, game/rac2/profile.cpp): the
// class at +0xAA, the position +0x10, the state +0x20, the scale +0x2C, the occlusion +0x36, the
// rows +0xC0.
void put_moby(std::vector<std::uint8_t>& ram, std::uint32_t m, std::int16_t oclass, float x) {
    std::memcpy(ram.data() + m + 0xAA, &oclass, sizeof(oclass));
    put_float(ram, m + 0x00, x * 1024.0f);  // MobyProc's sphere, 1/1024 units
    put_float(ram, m + 0x0C, 2.0f * 1024.0f);
    put_float(ram, m + 0x10, x);
    put_float(ram, m + 0x14, 5.0f);
    put_float(ram, m + 0x18, 6.0f);
    ram[m + 0x20] = 1;
    put_float(ram, m + 0x2C, 2.0f);
    const std::uint16_t occlusion = 0x1234;
    std::memcpy(ram.data() + m + 0x36, &occlusion, sizeof(occlusion));
    put_float(ram, m + 0xC0, 1.0f);
    put_float(ram, m + 0xD4, 1.0f);
    put_float(ram, m + 0xE8, 1.0f);
}

void reads_each_programs_camera_and_view() {
    const host::GameProfile& profile = host::game_profile("rac3-pal");
    for (const Program& p : {kBoot, kFrontbin, kVeldin, kMultiplayerMenu}) {
        const auto ram = memory(p);
        const host::GameState s = profile.read_state(ram, p.level);
        CHECK(s.camera_position == (renderer::Vec3{100.0f, 200.0f, 300.0f}));
        CHECK(s.forward == (renderer::Vec3{1.0f, 0.0f, 0.0f}));
        CHECK(s.left == (renderer::Vec3{0.0f, 1.0f, 0.0f}));
        CHECK(s.up == (renderer::Vec3{0.0f, 0.0f, 1.0f}));
        CHECK(s.tan_half_fov_x == 0.62f);
        CHECK(s.tan_half_fov_y == 0.4805f);
        CHECK(s.focal_x == 413.0f);
        CHECK(s.focal_y == 433.0f);
        CHECK(s.fog_near == 1000.0f);
        CHECK(s.fog_far == 5000.0f);
        CHECK(s.fog_near_f == 250.0f);
        CHECK(s.fog_far_f == 40.0f);
        CHECK(s.fog_colour[0] == 0x33 / 255.0f);
        CHECK(s.fog_colour[1] == 0x66 / 255.0f);
        CHECK(s.fog_colour[2] == 0x99 / 255.0f);
        CHECK(s.background_colour.has_value());
        if (s.background_colour) {
            CHECK((*s.background_colour)[0] == 1.0f);
            CHECK((*s.background_colour)[1] == 0.0f);
            CHECK((*s.background_colour)[2] == 0x80 / 255.0f);
        }
        CHECK(s.moby_view.has_value() == (p.moby_cap != 0));
        if (s.moby_view) {
            CHECK(s.moby_view->draw_cap == 960);
            CHECK(s.moby_view->near_plane == 0.1f * 1024.0f);
            CHECK(s.moby_view->depth_axis == (std::array<float, 3>{1.0f, 0.0f, 0.0f}));
        }
    }
}

void reads_the_moby_table() {
    const host::GameProfile& profile = host::game_profile("rac3-pal");
    for (const Program& p : {kBoot, kVeldin, kMultiplayerMenu}) {
        auto ram = memory(p);
        put_moby(ram, kTable, 0, 10.0f);  // placed: the hero's class
        put_moby(ram, kTable + 0x100, 7, 20.0f);
        ram[kTable + 0x100 + 0x20] = 0xFE;           // deleted
        put_moby(ram, kTable + 0x200, 4321, 30.0f);  // spawned
        put_moby(ram, kTable + 0x300, 99, 40.0f);
        put_float(ram, kTable + 0x300 + 0x2C, 0.0f);  // no scale: not placed yet
        const host::GameState s = profile.read_state(ram, p.level);
        CHECK(s.mobys.size() == 2u);
        if (s.mobys.size() != 2u) {
            continue;
        }
        CHECK(s.mobys[0].address == kTable);
        CHECK(s.mobys[0].class_id == 0);
        CHECK(s.mobys[1].address == kTable + 0x200);
        CHECK(s.mobys[1].class_id == 4321);
        const host::LiveMoby& m = s.mobys[1];
        CHECK(m.model_class == -1);
        CHECK(m.occlusion == 0x1234);
        CHECK(m.world_sphere == (std::array<float, 4>{30.0f, 0.0f, 0.0f, 2.0f}));
        CHECK(m.matrix[0] == 2.0f);  // the rows times the scale
        CHECK(m.matrix[5] == 2.0f);
        CHECK(m.matrix[10] == 2.0f);
        CHECK(m.matrix[12] == 30.0f);  // the position
        CHECK(m.matrix[13] == 5.0f);
        CHECK(m.matrix[14] == 6.0f);
        const auto one = profile.read_moby(ram, kTable + 0x200);
        CHECK(one.has_value() && one->matrix == m.matrix);
    }
}

// MobyProc's distance test on the boot program's cap (moby_end + 0x2C, not Going Commando's
// + 0x10).
void culls_past_the_cap() {
    const host::GameProfile& profile = host::game_profile("rac3-pal");
    auto ram = memory(kBoot);
    put_float(ram, kBoot.camera_rows - 0x20, 0.0f);  // the eye at the origin, looking down +x
    put_float(ram, kBoot.camera_rows - 0x1C, 0.0f);
    put_float(ram, kBoot.camera_rows - 0x18, 0.0f);
    const host::GameState state = profile.read_state(ram, -1);
    std::uint8_t moby[0x100] = {};
    const auto at = [&](float depth, std::int16_t dd) {
        const float sphere[4] = {depth * 1024.0f, 0.0f, 0.0f, 0.0f};
        std::memcpy(moby, sphere, sizeof(sphere));
        std::memcpy(moby + 0x32, &dd, sizeof(dd));
        return profile.moby_distance_fade(moby, state);
    };
    CHECK(at(40.0f, 64) == std::uint8_t{0x80});
    CHECK(!at(65.0f, 64));
    CHECK(at(900.0f, 4000) == std::uint8_t{0x80});
    CHECK(!at(970.0f, 4000));  // past the cap
}

// The fields this game shares with Going Commando's profile (game/rac2/profile.cpp), which
// MobyProc and the hero code read at the same offsets.
void shares_going_commandos_moby_fields() {
    const host::GameProfile& profile = host::game_profile("rac3-pal");
    CHECK(profile.moby_anim_fields() == 0x40);
    CHECK(profile.moby_glow_word() == 0x78);
    CHECK(profile.stick_dead_zone() == (std::array<int, 2>{0x30, 0x4C}));
    CHECK(profile.hero_state() == 0x001A4BE0u);
    CHECK(profile.hero_air_frames() == 0x36E);
    CHECK(profile.cutscene_mode() == std::optional<std::uint32_t>{2});
    CHECK(profile.pause_mode() == std::optional<std::uint32_t>{4});
    CHECK(profile.sends_display_list());
}

// The levels are drawn natively from the extracted ones (host/render.c); the title is a picture,
// with no world under it.
void draws_the_extracted_levels() {
    const host::GameProfile& profile = host::game_profile("rac3-pal");
    CHECK(profile.has_levels());
    CHECK(!profile.has_title_world());
}

}  // namespace

int main() {
    reads_each_programs_camera_and_view();
    reads_the_moby_table();
    culls_past_the_cap();
    shares_going_commandos_moby_fields();
    draws_the_extracted_levels();
    return openrac::test::result();
}

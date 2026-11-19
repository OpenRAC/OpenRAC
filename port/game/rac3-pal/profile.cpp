// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Up Your Arsenal's profile for PAL (game/common/profile.h): the camera, the view context and its
// fog, the live mobys and the hero, read from game memory by program (the boot program's, frontbin's
// and each level's copy of the engine's globals), and the frame shown being the display list it
// sends through VIF1 DMA (the bootstrap the native renderers replace). It defines game_profile() for
// openrac-rac3-pal.
//
// The engine is Going Commando's (game/rac2/profile.cpp): each function read here is that game's,
// instruction for instruction, at other addresses. Where a block's layout moved, the offsets below
// say so; the rest are rac2's (rac3_profile_test checks both).

#include "game/common/profile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "game/common/moby_proc.h"
#include "openrac/guest.h"
#include "renderer/math.h"

namespace openrac::game::rac3 {
namespace {

// One program's copy of the engine's globals. Every value was read from the instructions of that
// program's copy of the same function (rac3-decomp config/levels/functions.tsv places; boot_elf's
// copies by config/frontend_pairs.tsv), all programs' copies being the same code at other
// addresses:
// - camera_rows: BuildViewMatrices (boot 0x003865D0, frontbin 0x003861F0; rac2 func_00279D18)
//   keeps it in $s2: the camera's rotation rows forward, left, up. The view block (the view
//   matrix, MobyProc's eye at its +0x140) is 0x160 before them, where Going Commando's is 0x370.
// - view_context: UpdateFog (boot 0x003868A0, frontbin 0x003864C0; rac2 func_00279FF0): the block
//   UpdateViewContext (view_projection_setup) fills. BuildViewMatrices' $s3, its +0xC0, agrees in
//   every program.
// - moby_first, moby_end: moby_spawn (boot 0x003C2868, frontbin 0x003C24B0; rac2's CreateMoby,
//   func_0029EFD0) walks the spawned mobys between them; the word before moby_first is the table's
//   start.
// - moby_cap: MobyProc (hand-written; boot 0x003CB798, frontbin 0x003CB3E0; rac2 func_002A4E68)
//   loads its cap on a moby's draw distance into $s7 from $gp: 0x2C past moby_end in every
//   program that has it (rac2: 0x10). The multiplayer levels' MobyProc is not in the level
//   catalogue: 0 there, and no moby view.
struct Addresses {
    std::uint32_t camera_rows;
    std::uint32_t view_context;
    std::uint32_t moby_end;
    std::uint32_t moby_first;
    std::uint32_t moby_cap;
};

// BuildViewMatrices scales the eye at camera_rows - 0x20 by 1024 into the view context's +0x280,
// and world_to_screen reads it there too (the view block's +0x140).
constexpr std::uint32_t kEyeBeforeRows = 0x20;
constexpr std::uint32_t kViewBeforeRows = 0x160;

// The view context's fields (view_projection_setup and UpdateFog write them): the near plane
// (+0xA0), the half-angle tangents (+0xB0, +0xB4) and the focal lengths (+0xC0, +0xD4) where Going
// Commando keeps them; UpdateFog's fog 0x10 later (rac2 +0x218, +0x21C, +0x228, +0x22C) and its
// colour 0x20 later (rac2 +0x230..+0x238), and the background colour the level's loader writes
// beside it (0x003BBDC0: +0x25C..+0x264; rac2 +0x23C).
constexpr std::uint32_t kNearPlane = 0xA0;
constexpr std::uint32_t kTanX = 0xB0;
constexpr std::uint32_t kTanY = 0xB4;
constexpr std::uint32_t kFocalX = 0xC0;
constexpr std::uint32_t kFocalY = 0xD4;
constexpr std::uint32_t kFogNear = 0x228;
constexpr std::uint32_t kFogFar = 0x22C;
constexpr std::uint32_t kFogNearF = 0x238;
constexpr std::uint32_t kFogFarF = 0x23C;
constexpr std::uint32_t kFogColour = 0x250;
constexpr std::uint32_t kBackground = 0x25C;

// The programs: the boot program (boot_elf's own .text, the main menu), frontbin (the front end's
// program, hostgen's overlay 100, whose globals are the boot program's) and the levels by number.
struct Release {
    Addresses boot;
    Addresses levels[54];
};

constexpr int kFrontbinOverlay = 100;

// PAL's (SCES-52456).
// clang-format off
constexpr Release kPal{
    {0x00222420, 0x00225900, 0x001DA4A4, 0x001DA4A0, 0x001DA4D0},
    {
    {},  // 0: no program
    {0x001E29E0, 0x001E7580, 0x001E0024, 0x001E0020, 0x001E0050},  // 1 Veldin
    {0x001E12A0, 0x001E5E40, 0x001DE9A4, 0x001DE9A0, 0x001DE9D0},  // 2
    {0x001E2460, 0x001E7000, 0x001DF9A4, 0x001DF9A0, 0x001DF9D0},  // 3
    {0x001E27E0, 0x001E7380, 0x001DFDA4, 0x001DFDA0, 0x001DFDD0},  // 4
    {0x001E26A0, 0x001E7240, 0x001DFBA4, 0x001DFBA0, 0x001DFBD0},  // 5
    {0x001E16A0, 0x001E6240, 0x001DEDA4, 0x001DEDA0, 0x001DEDD0},  // 6
    {0x001E1E60, 0x001E6A00, 0x001DF424, 0x001DF420, 0x001DF450},  // 7
    {0x001E1CE0, 0x001E6880, 0x001DF224, 0x001DF220, 0x001DF250},  // 8
    {0x001E2B20, 0x001E76C0, 0x001E00A4, 0x001E00A0, 0x001E00D0},  // 9
    {0x001E1B60, 0x001E6700, 0x001DF2A4, 0x001DF2A0, 0x001DF2D0},  // 10
    {0x001E1720, 0x001E62C0, 0x001DEE24, 0x001DEE20, 0x001DEE50},  // 11
    {0x001E2720, 0x001E72C0, 0x001DFD24, 0x001DFD20, 0x001DFD50},  // 12
    {0x001E20E0, 0x001E6C80, 0x001DF5A4, 0x001DF5A0, 0x001DF5D0},  // 13
    {0x001E20E0, 0x001E6C80, 0x001DF724, 0x001DF720, 0x001DF750},  // 14
    {},  // 15: no program
    {0x001E2B20, 0x001E76C0, 0x001E0164, 0x001E0160, 0x001E0190},  // 16
    {0x001E1920, 0x001E64C0, 0x001DF0A4, 0x001DF0A0, 0x001DF0D0},  // 17
    {0x001E2A60, 0x001E7600, 0x001E00A4, 0x001E00A0, 0x001E00D0},  // 18
    {0x001E20A0, 0x001E6C40, 0x001DF724, 0x001DF720, 0x001DF750},  // 19
    {0x001E2E20, 0x001E79C0, 0x001E02A4, 0x001E02A0, 0x001E02D0},  // 20
    {0x001E16A0, 0x001E6240, 0x001DEDA4, 0x001DEDA0, 0x001DEDD0},  // 21
    {0x001E2AA0, 0x001E7640, 0x001DFDA4, 0x001DFDA0, 0x001DFDD0},  // 22
    {0x001E19E0, 0x001E6580, 0x001DF164, 0x001DF160, 0x001DF190},  // 23
    {0x001E21A0, 0x001E6D40, 0x001DF524, 0x001DF520, 0x001DF550},  // 24
    {},  // 25: no program
    {0x001E27E0, 0x001E7380, 0x001DFE24, 0x001DFE20, 0x001DFE50},  // 26
    {0x001E1620, 0x001E61C0, 0x001DED24, 0x001DED20, 0x001DED50},  // 27
    {0x001E14E0, 0x001E6080, 0x001DECA4, 0x001DECA0, 0x001DECD0},  // 28
    {0x001E2520, 0x001E70C0, 0x001DFB24, 0x001DFB20, 0x001DFB50},  // 29
    {0x001E2260, 0x001E6E00, 0x001DF724, 0x001DF720, 0x001DF750},  // 30
    {0x001E23E0, 0x001E6F80, 0x001DF7A4, 0x001DF7A0, 0x001DF7D0},  // 31
    {0x001E2BE0, 0x001E7780, 0x001DFE24, 0x001DFE20, 0x001DFE50},  // 32
    {0x001E2860, 0x001E7400, 0x001DFB24, 0x001DFB20, 0x001DFB50},  // 33
    {0x001E23E0, 0x001E6F80, 0x001DF7A4, 0x001DF7A0, 0x001DF7D0},  // 34
    {0x001E2520, 0x001E70C0, 0x001DF8A4, 0x001DF8A0, 0x001DF8D0},  // 35
    {0x001E27A0, 0x001E7340, 0x001DFB24, 0x001DFB20, 0x001DFB50},  // 36
    {},  // 37: no program
    {},  // 38: no program
    {0x0024DE20, 0x0025D6C0, 0x0024BF64, 0x0024BF60, 0},  // 39 multiplayer menu
    {0x00249F60, 0x00259900, 0x00248464, 0x00248460, 0},  // 40
    {0x0024A0A0, 0x00259A40, 0x00248664, 0x00248660, 0},  // 41
    {0x00249FA0, 0x00259940, 0x00248564, 0x00248560, 0},  // 42
    {0x00249E20, 0x002597C0, 0x002483E4, 0x002483E0, 0},  // 43
    {0x00249E60, 0x00259800, 0x00248464, 0x00248460, 0},  // 44
    {0x00249E20, 0x002597C0, 0x002483E4, 0x002483E0, 0},  // 45
    {0x002499E0, 0x00259380, 0x00247FE4, 0x00247FE0, 0},  // 46
    {0x00249B20, 0x002594C0, 0x002480E4, 0x002480E0, 0},  // 47
    {0x00249B60, 0x00259500, 0x002480E4, 0x002480E0, 0},  // 48
    {0x00249AE0, 0x00259480, 0x002480E4, 0x002480E0, 0},  // 49
    {0x00249F60, 0x00259900, 0x00248464, 0x00248460, 0},  // 50
    {0x0024A0A0, 0x00259A40, 0x00248664, 0x00248660, 0},  // 51
    {0x00249FA0, 0x00259940, 0x00248564, 0x00248560, 0},  // 52
    {0x00249E20, 0x002597C0, 0x002483E4, 0x002483E0, 0},  // 53
    },
};
// clang-format on

template <typename T>
T at(std::span<const std::uint8_t> ram, std::uint32_t address) {
    T value{};
    address = openrac_guest_ram_address_for_size(address, static_cast<std::uint32_t>(ram.size()));
    if (address + sizeof(T) <= ram.size()) {
        std::memcpy(&value, ram.data() + address, sizeof(T));
    }
    return value;
}

renderer::Vec3 vec3(std::span<const std::uint8_t> ram, std::uint32_t address) {
    return {at<float>(ram, address), at<float>(ram, address + 4), at<float>(ram, address + 8)};
}

// The moby's fields are Going Commando's (include/structs/moby.h; MobyProc reads them at the same
// offsets): the position +0x10, the state +0x20, the scale +0x2C, the draw distance +0x32, the
// occlusion +0x36, the class +0xAA and the rotation rows +0xC0.
constexpr std::uint32_t kMobyClass = 0xAA;

// Reads a complete record's sphere in world units (MobyProc's lqc2 of +0x00, in 1/1024 units).
std::array<float, 4> moby_sphere(std::span<const std::uint8_t> ram, std::uint32_t address) {
    std::array<float, 4> sphere;
    std::memcpy(sphere.data(), ram.data() + address, sizeof(sphere));
    if (!(sphere[3] > 0) || !std::all_of(sphere.begin(), sphere.end(), [](float value) {
            return std::isfinite(value);
        })) {
        return {};
    }
    for (float& component : sphere) {
        component /= 1024.0f;
    }
    return sphere;
}

// Returns a moby's placement from its complete record, none when it has none yet.
std::optional<host::LiveMoby> read_live_moby(std::span<const std::uint8_t> ram, std::uint32_t m) {
    if (m == 0 || m + 0x100 > ram.size()) {
        return std::nullopt;
    }
    const renderer::Vec3 pos = vec3(ram, m + 0x10);
    const float scale = at<float>(ram, m + 0x2C);
    if (!std::isfinite(pos[0]) || !std::isfinite(scale) || scale <= 0.0f || scale > 1000.0f) {
        return std::nullopt;
    }
    host::LiveMoby live;
    live.address = m;
    live.class_id = at<std::int16_t>(ram, m + kMobyClass);
    live.occlusion = at<std::uint16_t>(ram, m + 0x36);
    live.world_sphere = moby_sphere(ram, m);
    const renderer::Vec3 r0 = vec3(ram, m + 0xC0), r1 = vec3(ram, m + 0xD0),
                         r2 = vec3(ram, m + 0xE0);
    const auto usable = [](const renderer::Vec3& r) {
        return std::isfinite(r[0]) && std::isfinite(r[1]) && std::isfinite(r[2])
               && r[0] * r[0] + r[1] * r[1] + r[2] * r[2] > 1.0e-6f;
    };
    if (!usable(r0) || !usable(r1) || !usable(r2)) {
        return std::nullopt;  // Not built since it was made: nothing to place it by.
    }
    live.matrix =
        {r0[0] * scale,
         r0[1] * scale,
         r0[2] * scale,
         0,
         r1[0] * scale,
         r1[1] * scale,
         r1[2] * scale,
         0,
         r2[0] * scale,
         r2[1] * scale,
         r2[2] * scale,
         0,
         pos[0],
         pos[1],
         pos[2],
         1};
    return live;
}

float colour_byte(std::span<const std::uint8_t> ram, std::uint32_t address) {
    return static_cast<float>(at<std::uint32_t>(ram, address) & 0xFF) / 255.0f;
}

host::GameState read_game_state(std::span<const std::uint8_t> ram, const Addresses& a) {
    host::GameState s;
    s.camera_position = vec3(ram, a.camera_rows - kEyeBeforeRows);
    s.forward = vec3(ram, a.camera_rows);
    s.left = vec3(ram, a.camera_rows + 0x10);
    s.up = vec3(ram, a.camera_rows + 0x20);

    // What MobyProc tests each moby's distance against (host::moby_proc_distance_fade): the view
    // matrix's third column, the near plane and the cap.
    if (a.moby_cap != 0) {
        host::GameState::MobyView moby_view;
        const std::uint32_t view = a.camera_rows - kViewBeforeRows;
        for (int i = 0; i < 3; ++i) {
            moby_view.depth_axis[static_cast<std::size_t>(i)] =
                at<float>(ram, view + 0x08 + 0x10 * i);
        }
        moby_view.near_plane = at<float>(ram, a.view_context + kNearPlane);
        moby_view.draw_cap = at<std::int32_t>(ram, a.moby_cap);
        s.moby_view = moby_view;
    }

    const float tx = at<float>(ram, a.view_context + kTanX);
    const float ty = at<float>(ram, a.view_context + kTanY);
    if (tx > 0.05f && tx < 10.0f && ty > 0.05f && ty < 10.0f) {
        s.tan_half_fov_x = tx;
        s.tan_half_fov_y = ty;
    }
    const float fx = at<float>(ram, a.view_context + kFocalX) * 32.0f;
    const float fy = at<float>(ram, a.view_context + kFocalY) * 32.0f;
    if (fx > 16.0f && fx < 4096.0f && fy > 16.0f && fy < 4096.0f) {
        s.focal_x = fx;
        s.focal_y = fy;
    }
    for (std::uint32_t c = 0; c < 3; ++c) {
        s.fog_colour[c] = colour_byte(ram, a.view_context + kFogColour + 4 * c);
    }
    s.fog_near = at<float>(ram, a.view_context + kFogNear);
    s.fog_far = at<float>(ram, a.view_context + kFogFar);
    s.fog_near_f = at<float>(ram, a.view_context + kFogNearF);
    s.fog_far_f = at<float>(ram, a.view_context + kFogFarF);
    std::array<float, 3> background{};
    for (std::uint32_t c = 0; c < 3; ++c) {
        background[c] = colour_byte(ram, a.view_context + kBackground + 4 * c);
    }
    s.background_colour = background;

    // The table starts at the word before moby_first (the mobys the level places); moby_first is
    // where the spawned ones begin, as in Going Commando.
    const auto ram_size = static_cast<std::uint32_t>(ram.size());
    auto first = openrac_guest_ram_address_for_size(at<std::uint32_t>(ram, a.moby_first), ram_size);
    const auto end =
        openrac_guest_ram_address_for_size(at<std::uint32_t>(ram, a.moby_end), ram_size);
    const auto start =
        openrac_guest_ram_address_for_size(at<std::uint32_t>(ram, a.moby_first - 4), ram_size);
    if (start != 0 && start < first && (first - start) % 0x100 == 0) {
        first = start;
    }
    if (first == 0 || end <= first || end > ram.size() || (end - first) % 0x100 != 0) {
        return s;
    }
    for (std::uint32_t m = first; m < end; m += 0x100) {
        // A state with its sign bit set is no moby any more (deleted, or the end of the used part).
        if (at<std::uint8_t>(ram, m + 0x20) >= 0x80) {
            continue;
        }
        if (const auto live = read_live_moby(ram, m)) {
            s.mobys.push_back(*live);
        }
    }
    return s;
}

// The game's profile: its memory read by program, and the display list it sends.
class Profile final : public host::GameProfile {
public:
    explicit constexpr Profile(const Release& release) : release_(release) {}

    host::GameState read_state(std::span<const std::uint8_t> ram, int level) const override {
        return read_game_state(ram, addresses(level));
    }

    std::optional<host::LiveMoby> read_moby(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const override {
        return read_live_moby(ram, moby);
    }

    // MobyProc leaves a moby past its draw distance (+0x32, at most the program's cap) undrawn and
    // fades it over the last 16 units, Going Commando's instructions (boot 0x003CBB90).
    std::optional<std::uint8_t> moby_distance_fade(
        const std::uint8_t* moby, const host::GameState& state
    ) const override {
        return host::moby_proc_distance_fade(moby, state);
    }

    bool sends_display_list() const override { return true; }

    // Going Commando's: the frames at +0x40, the sequences at +0x42, the keys at +0x58 and the
    // pose layer link at +0x50 (structs/moby.h; MobyProc reads them there).
    std::uint32_t moby_anim_fields() const override { return 0x40; }

    // The glow word MobyProc copies to its job: +0x78 (boot 0x003CCD38, lw a2 0x78($fp)), as Going
    // Commando's; the bangles' word is +0x7C there too (0x003CBEDC).
    std::uint32_t moby_glow_word() const override { return 0x78; }

    // The hero's record, D_001A4BE0 (the hero code's base: his position at +0x80, 0x001A4C60),
    // whose short at +0x36E counts the updates off the ground (D_001A4F4E, hero_air_frames; read
    // as lh 0x36E in the levels' hero code).
    std::uint32_t hero_state() const override { return 0x001A4BE0u; }

    std::uint32_t hero_air_frames() const override { return 0x36E; }

    // A level's game mode (host/tools.c): 2 a level's scene, as Going Commando numbers them and
    // hero_camera_allowed refuses the hero camera in; 4 the overlay modes (the pause menus and the
    // quick-select pause, overlay_update), over the world as it was paused.
    std::optional<std::uint32_t> cutscene_mode() const override { return 2; }

    std::optional<std::uint32_t> pause_mode() const override { return 4; }

    // The pad's read (boot 0x003A31C8): |byte - 0x7F| from 0x30, over 76, as Going Commando's.
    std::array<int, 2> stick_dead_zone() const override { return {0x30, 0x4C}; }

private:
    const Addresses& addresses(int level) const {
        return level >= 0 && static_cast<std::size_t>(level) < std::size(release_.levels)
                       && release_.levels[level].camera_rows != 0
                   ? release_.levels[level]
                   : release_.boot;  // the boot program, and frontbin (kFrontbinOverlay)
    }

    const Release& release_;
};

static_assert(kFrontbinOverlay >= static_cast<int>(std::size(kPal.levels)));

// The one instance, a constant for the program's life.
const Profile kPalProfile{kPal};

}  // namespace
}  // namespace openrac::game::rac3

namespace openrac::host {

const GameProfile& game_profile(std::string_view /*game*/) {
    return game::rac3::kPalProfile;
}

}  // namespace openrac::host

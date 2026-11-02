// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's profile for NTSC-U (game/common/profile.h): the camera, the fog and the live
// mobys read from game memory, by program (the boot's and each level's copy of the engine's
// globals), and the frame shown being the display list it sends through VIF1 DMA, without Ratchet
// & Clank's two alternating list buffers. It defines game_profile() for openrac-rac2-ntsc.

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
#include <utility>

#include "openrac/guest.h"
#include "renderer/math.h"

namespace openrac::game::rac2 {
namespace {

// Each program's copy of the engine's globals. Every value was read from the instructions of that
// program's copy of the same function, as levels' copies differ only in addresses:
// - camera_rows: BuildViewMatrices (func_00279D18) keeps it in $s2: the camera's rotation rows
//   forward, left, up. The camera block is 0x370 before it and the eye at its +0x140, 0x230 before
//   the rows (what the function scales by 1024 for the view context).
// - view_context: UpdateFog (func_00279FF0): the block UpdateViewContext fills.
// - moby_first, moby_end: CreateMoby (func_0029EFD0) walks the spawned mobys between them.
struct Addresses {
    std::uint32_t camera_rows;
    std::uint32_t view_context;
    std::uint32_t moby_first;
    std::uint32_t moby_end;
};

constexpr std::uint32_t kEyeBeforeRows = 0x230;

const Addresses kBoot{0x001B5570, 0x001B9040, 0x001B1B60, 0x001B1B64};
const Addresses kLevels[31] = {
    {0x001B8FF0, 0x001BD2C0, 0x001B1EA0, 0x001B1EA4},  // 0
    {0x001B96F0, 0x001BD9C0, 0x001B2560, 0x001B2564},  // 1
    {0x001BA730, 0x001BEA00, 0x001B35A0, 0x001B35A4},  // 2
    {0x001B9BB0, 0x001BDE80, 0x001B2A20, 0x001B2A24},  // 3
    {0x001B98B0, 0x001BDB80, 0x001B24E0, 0x001B24E4},  // 4
    {0x001B98F0, 0x001BDBC0, 0x001B2520, 0x001B2524},  // 5
    {0x001B9A70, 0x001BDD40, 0x001B2820, 0x001B2824},  // 6
    {0x001B9430, 0x001BD700, 0x001B22E0, 0x001B22E4},  // 7
    {0x001B9D30, 0x001BE000, 0x001B29E0, 0x001B29E4},  // 8
    {0x001B9C30, 0x001BDF00, 0x001B2860, 0x001B2864},  // 9
    {0x001B98B0, 0x001BDB80, 0x001B2520, 0x001B2524},  // 10
    {0x001BAAF0, 0x001BEDC0, 0x001B35A0, 0x001B35A4},  // 11
    {0x001BA030, 0x001BE300, 0x001B2EA0, 0x001B2EA4},  // 12
    {0x001B9AF0, 0x001BDDC0, 0x001B2920, 0x001B2924},  // 13
    {0x001B9730, 0x001BDA00, 0x001B25A0, 0x001B25A4},  // 14
    {0x001B99F0, 0x001BDCC0, 0x001B2620, 0x001B2624},  // 15
    {0x001B9D30, 0x001BE000, 0x001B2B20, 0x001B2B24},  // 16
    {0x001B9DF0, 0x001BE0C0, 0x001B2C20, 0x001B2C24},  // 17
    {0x001B99F0, 0x001BDCC0, 0x001B2820, 0x001B2824},  // 18
    {0x001B9C70, 0x001BDF40, 0x001B2A20, 0x001B2A24},  // 19
    {0x001BA1F0, 0x001BE4C0, 0x001B2FA0, 0x001B2FA4},  // 20
    {},                                                // 21: no level
    {0x001B9EB0, 0x001BE180, 0x001B2120, 0x001B2124},  // 22
    {0x001BA870, 0x001BEB40, 0x001B27A0, 0x001B27A4},  // 23
    {0x001B88F0, 0x001BCBC0, 0x001B17A0, 0x001B17A4},  // 24
    {0x001B9430, 0x001BD700, 0x001B20A0, 0x001B20A4},  // 25
    {0x001B8DB0, 0x001BD080, 0x001B1C20, 0x001B1C24},  // 26
    {},                                                // 27: no level
    {},                                                // 28: no level
    {},                                                // 29: no level
    {0x001B9FF0, 0x001BE2C0, 0x001B2D20, 0x001B2D24},  // 30
};

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

// The moby's fields this game moves from Ratchet & Clank's: the class id (CreateMoby's set-up,
// func_0029F0C8) is at +0xAA. The position (+0x10), state (+0x20), scale (+0x2C), draw distance
// (+0x32), occlusion (+0x36) and rotation rows (+0xC0) are where they were.
constexpr std::uint32_t kMobyClass = 0xAA;

// Ratchet's class holds only his skeleton: the game loads his packets and textures over it from a
// pack of the disc, the armour's unless one of four flags names another (func_00292078). The export
// has each pack past the first as a class of its own (editor/moby_class.py, HERO_LOOK_CLASSES).
constexpr std::int32_t kHeroLookClasses = 6000;
constexpr std::uint32_t kArmour = 0x001A7A18;
constexpr std::array<std::pair<std::uint32_t, std::int32_t>, 4> kLookFlags{
    {{0x001A7A49, 6}, {0x001A7A42, 8}, {0x001A7A4B, 7}, {0x001A7A4A, 5}}
};

// The wrench is loaded the same way, by the form the game has (func_00292408).
constexpr std::int32_t kWrenchClass = 71;
constexpr std::int32_t kWrenchFormClasses = 6020;
constexpr std::uint32_t kWrenchForm = 0x001A72C0;

std::int32_t hero_class(std::span<const std::uint8_t> ram) {
    std::int32_t look = at<std::int16_t>(ram, kArmour);
    for (const auto& [flag, pack] : kLookFlags) {
        if (at<std::uint8_t>(ram, flag) != 0) {
            look = pack;
            break;
        }
    }
    return look > 0 && look <= 8 ? kHeroLookClasses + look : -1;
}

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
    if (live.class_id == 0) {
        live.model_class = hero_class(ram);
    } else if (live.class_id == kWrenchClass) {
        const auto form = at<std::int32_t>(ram, kWrenchForm);
        live.model_class = form > 0 && form < 3 ? kWrenchFormClasses + form : -1;
    }
    live.occlusion = at<std::uint16_t>(ram, m + 0x36);
    std::array<float, 4> sphere;
    std::memcpy(sphere.data(), ram.data() + m, sizeof(sphere));
    if (sphere[3] > 0
        && std::all_of(sphere.begin(), sphere.end(), [](float x) { return std::isfinite(x); })) {
        for (std::size_t i = 0; i < sphere.size(); ++i) {
            live.world_sphere[i] = sphere[i] / 1024.0f;
        }
    }
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

host::GameState read_game_state(std::span<const std::uint8_t> ram, const Addresses& a) {
    host::GameState s;
    s.camera_position = vec3(ram, a.camera_rows - kEyeBeforeRows);
    s.forward = vec3(ram, a.camera_rows);
    s.left = vec3(ram, a.camera_rows + 0x10);
    s.up = vec3(ram, a.camera_rows + 0x20);

    // The view context as in Ratchet & Clank: the tangents, the focal lengths, UpdateFog's fog
    // (func_00279FF0 writes these offsets) and the level's background colour.
    const float tx = at<float>(ram, a.view_context + 0xB0);
    const float ty = at<float>(ram, a.view_context + 0xB4);
    if (tx > 0.05f && tx < 10.0f && ty > 0.05f && ty < 10.0f) {
        s.tan_half_fov_x = tx;
        s.tan_half_fov_y = ty;
    }
    const float fx = at<float>(ram, a.view_context + 0xC0) * 32.0f;
    const float fy = at<float>(ram, a.view_context + 0xD4) * 32.0f;
    if (fx > 16.0f && fx < 4096.0f && fy > 16.0f && fy < 4096.0f) {
        s.focal_x = fx;
        s.focal_y = fy;
    }
    for (int c = 0; c < 3; ++c) {
        s.fog_colour[c] =
            static_cast<float>(at<std::uint32_t>(ram, a.view_context + 0x230 + 4 * c) & 0xFF)
            / 255.0f;
    }
    s.fog_near = at<float>(ram, a.view_context + 0x218);
    s.fog_far = at<float>(ram, a.view_context + 0x21C);
    s.fog_near_f = at<float>(ram, a.view_context + 0x228);
    s.fog_far_f = at<float>(ram, a.view_context + 0x22C);
    std::array<float, 3> background{};
    for (int c = 0; c < 3; ++c) {
        background[c] =
            static_cast<float>(at<std::uint32_t>(ram, a.view_context + 0x23C + 4 * c) & 0xFF)
            / 255.0f;
    }
    s.background_colour = background;

    // The table starts at the word before moby_first (the mobys the level places); moby_first is
    // where the spawned ones begin, as in Ratchet & Clank.
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
    host::GameState read_state(std::span<const std::uint8_t> ram, int level) const override {
        return read_game_state(ram, addresses(level));
    }

    std::optional<host::LiveMoby> read_moby(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const override {
        return read_live_moby(ram, moby);
    }

    bool sends_display_list() const override { return true; }

    bool images_outlast_list() const override { return true; }

    // The VRAM base of the frame (D_001A72DC, 0x1E8000 in bytes / 256 / 8): past the two 448-line
    // buffers, where the world's passes set FRAME (func_002DF1B8: FBP 0xF4).
    std::uint32_t draw_buffer_block() const override { return 0x1E80; }

    // 0x10 before Ratchet & Clank's: ResolveMobyAnimFramePtrs (func_0029F390) reads the frames at
    // +0x40, the sequences at +0x42 and writes the keys at +0x58, and the pose layer slots
    // (func_L00_00310BC8) link at +0x50.
    std::uint32_t moby_anim_fields() const override { return 0x40; }

    // The glow word MobyProc (func_002A4E68) copies to its job when the mode has 0x10, and
    // func_00274D90 writes over the colours of each packet's last vertices (its vertex table's
    // last count): +0x78, where Ratchet & Clank's is +0x90. The lift's button (class 0xC16) pulses
    // grey and turns green there (func_L00_003CD8E0).
    std::uint32_t moby_glow_word() const override { return 0x78; }

    // func_L00_00316F60 blends a layer's buffer (+0x10), not keys: his weapon arm.
    bool moby_layer_records() const override { return true; }

    bool has_levels() const override { return true; }

    // The pad's read (func_002B88F0): |byte - 0x7F| from 0x30, over 0x4C, as Ratchet & Clank's.
    std::array<int, 2> stick_dead_zone() const override { return {0x30, 0x4C}; }

private:
    static const Addresses& addresses(int level) {
        return level >= 0 && static_cast<std::size_t>(level) < std::size(kLevels)
                       && kLevels[level].camera_rows != 0
                   ? kLevels[level]
                   : kBoot;
    }
};

// The one instance, a constant for the program's life.
const Profile kProfile;

}  // namespace
}  // namespace openrac::game::rac2

namespace openrac::host {

const GameProfile& game_profile(std::string_view /*game*/) {
    return game::rac2::kProfile;
}

}  // namespace openrac::host

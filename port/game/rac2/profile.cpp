// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's profiles (game/common/profile.h), one per disc: USA v1.01 (SCUS-97268), PAL
// (SCES-51607) and USA v2.00 (SCUS-97268, Greatest Hits). The camera, the fog and the live mobys
// read from game memory, by program (the boot's and each level's copy of the engine's globals), and
// the frame shown being the display list it sends through VIF1 DMA, without Ratchet & Clank's two
// alternating list buffers. Each disc's addresses are its table (Release): v1.01's below, the other
// releases' in profile_releases.h, carried from v1.01's by port/tools/hostgen/release_carry.py. The
// fields' offsets are v1.01's on every disc: the functions that read them are the same code there
// (release_carry.json). It defines game_profile() for openrac-rac2-ntsc, -pal and -ntsc2.

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
#include <vector>

#include "game/common/moby_proc.h"
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
//
// MobyProc (func_002A4E68; each level's copy, level 1's func_L01_0030DB30) reads the same block:
// the camera block 0x370 before camera_rows (lqc2 its +0x140, the eye, and its rows +0x00..+0x20,
// the view matrix), the near plane at view_context + 0xA0, and its cap on a moby's draw distance,
// the word 0x10 past moby_end (lw s7 from $gp; checked in every program of USA v1.01).
struct Addresses {
    std::uint32_t camera_rows;
    std::uint32_t view_context;
    std::uint32_t moby_first;
    std::uint32_t moby_end;
};

constexpr std::uint32_t kEyeBeforeRows = 0x230;
constexpr std::uint32_t kViewBeforeRows = 0x370;
constexpr std::uint32_t kMobyCapPastEnd = 0x10;

// One disc's addresses: the engine's globals by program, and the executable's words the profile
// reads (each named where the profile reads it, below).
struct Release {
    Addresses boot;
    Addresses levels[31];
    std::uint32_t armour;
    std::array<std::pair<std::uint32_t, std::int32_t>, 4> look_flags;
    std::uint32_t wrench_form;
    std::uint32_t ship_body;
    std::uint32_t ship_paint;
    std::array<std::uint32_t, 4> scenery_matrices;
    std::uint32_t shadow_directions;
    std::uint32_t hero_state;
};

// USA v1.01's, as the decompilation names them.
constexpr Release kNtsc{
    {0x001B5570, 0x001B9040, 0x001B1B60, 0x001B1B64},
    {
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
    },
    0x001A7A18,
    {{{0x001A7A49, 6}, {0x001A7A42, 8}, {0x001A7A4B, 7}, {0x001A7A4A, 5}}},
    0x001A72C0,
    0x00152C18,
    0x00152C30,
    {0x001B21BCu, 0x001B20A0u, 0x001B21B4u, 0x001B2098u},
    0x001B1B90u,
    0x00189E20u,
};

}  // namespace
}  // namespace openrac::game::rac2

// PAL's and v2.00's (kPal, kNtsc2).
#include "game/rac2/profile_releases.h"

namespace openrac::game::rac2 {
namespace {

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

// The wrench is loaded the same way, by the form the game has (func_00292408).
constexpr std::int32_t kWrenchClass = 71;
constexpr std::int32_t kWrenchFormClasses = 6020;

// His ship's hull is loaded over class 3412 on each load, by the hull he has (func_002925F0, which
// keeps its number at +0x18 of 0x00152C00). The export has hull 0 as the class and the others as
// classes of their own (editor/moby_class.py, SHIP_BODY_CLASSES).
constexpr std::int32_t kShipClass = 3412;
constexpr std::int32_t kShipBodyClasses = 6030;
constexpr std::int32_t kShipBodies = 3;
// And its picture, one of 18 paint jobs, by func_002926B8, which keeps its number at +0x30
// (editor/port.py's "ship_paints").
constexpr std::int32_t kShipPaints = 18;

// A class's bangles: parts of its mesh past its LOD lists, which a moby draws by the bits of its
// bangle word (+0x7C, an s16; MobyProc, func_002A4E68 at 0x2A5E78): bit 15 clear, the class's mesh
// and the bangles of the bits set; set, those bangles alone (bangle 0 for none). A broken crate's
// pieces and a destroyed enemy's are mobys of a class drawn so, one bit each. The export has each
// bangle as a class of its own (editor/moby_class.py, BANGLE_CLASSES).
constexpr std::uint32_t kMobyBangles = 0x7C;
constexpr int kBangleClasses = 100000;
constexpr int kBangles = 15;

// Adds `live` to `mobys` once for each part the game draws of it: its class's mesh, its bangles.
void add_drawn_parts(
    std::span<const std::uint8_t> ram,
    const host::LiveMoby& live,
    std::vector<host::LiveMoby>& mobys
) {
    const auto word = at<std::int16_t>(ram, live.address + kMobyBangles);
    if (word == 0 || live.class_id <= 0 || live.model_class >= 0) {
        mobys.push_back(live);
        return;
    }
    unsigned mask = static_cast<std::uint16_t>(word) & 0x7FFFu;
    if (word < 0) {
        mask = std::max(mask, 1u);
    } else {
        mobys.push_back(live);
    }
    for (int bangle = 0; bangle < kBangles; ++bangle) {
        if ((mask & (1u << bangle)) != 0) {
            host::LiveMoby part = live;
            part.model_class = kBangleClasses + live.class_id * 16 + bangle;
            part.model_only = true;
            mobys.push_back(part);
        }
    }
}

std::int32_t hero_class(std::span<const std::uint8_t> ram, const Release& r) {
    std::int32_t look = at<std::int16_t>(ram, r.armour);
    for (const auto& [flag, pack] : r.look_flags) {
        if (at<std::uint8_t>(ram, flag) != 0) {
            look = pack;
            break;
        }
    }
    return look > 0 && look <= 8 ? kHeroLookClasses + look : -1;
}

std::optional<host::LiveMoby> read_live_moby(
    std::span<const std::uint8_t> ram, std::uint32_t m, const Release& r
) {
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
        live.model_class = hero_class(ram, r);
    } else if (live.class_id == kWrenchClass) {
        const auto form = at<std::int32_t>(ram, r.wrench_form);
        live.model_class = form > 0 && form < 3 ? kWrenchFormClasses + form : -1;
    } else if (live.class_id == kShipClass) {
        const auto body = at<std::int32_t>(ram, r.ship_body);
        live.model_class = body > 0 && body < kShipBodies ? kShipBodyClasses + body : -1;
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

host::GameState read_game_state(
    std::span<const std::uint8_t> ram, const Addresses& a, const Release& r
) {
    host::GameState s;
    s.camera_position = vec3(ram, a.camera_rows - kEyeBeforeRows);
    s.forward = vec3(ram, a.camera_rows);
    s.left = vec3(ram, a.camera_rows + 0x10);
    s.up = vec3(ram, a.camera_rows + 0x20);

    // What MobyProc tests each moby's distance against (host::moby_proc_distance_fade): the view
    // matrix's third column, the near plane and the cap. Every level's loader sets the cap to 960
    // units (Oozla's at 0x2F6404), and a routine every level has lowers it to 144 for a while and
    // puts 960 back (Oozla's at 0x386C98 and 0x386DB0), so it is read each tick.
    host::GameState::MobyView moby_view;
    const std::uint32_t view = a.camera_rows - kViewBeforeRows;
    for (int i = 0; i < 3; ++i) {
        moby_view.depth_axis[static_cast<std::size_t>(i)] = at<float>(ram, view + 0x08 + 0x10 * i);
    }
    moby_view.near_plane = at<float>(ram, a.view_context + 0xA0);
    moby_view.draw_cap = at<std::int32_t>(ram, a.moby_end + kMobyCapPastEnd);
    s.moby_view = moby_view;

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
        if (const auto live = read_live_moby(ram, m, r)) {
            add_drawn_parts(ram, *live, s.mobys);
        }
    }
    return s;
}

// The classes enhanced lighting's rim marks as gameplay subjects, as Ratchet & Clank's
// game/rac1/gameplay_lighting.h does for its own: the ones identified in this game so far (the
// crates' numbers are Ratchet & Clank's; IDENTIFIED rows of the port's notes).
constexpr std::array kEssentialMobys{
    0,     // Ratchet
    11,    // vendor (func_00390AB8)
    500,   // bolt crate
    501,   // nanotech crate (0x1F5)
    505,   // crate
    511,   // ammo crate
    512,   // crate
    3291,  // a crate's ammo pickup, flying to the hero (0xCDB)
};

// The Electrolyzer puzzles' boards: the controller (0x896, func_L02_003C26F0), the gates (0xA24),
// platforms (0xA55) and rotators (0x873) its group holds, and the board's other pieces, every one
// of them only in the levels with a controller. The game parks each board out of sight (Barlow's
// at z 31 under its floor) and its pieces' draw distances (64, 128) keep them out of view there.
constexpr std::array kBoardMobys{2163, 2198, 2596, 2645, 2809, 2810, 2811, 2812, 2813, 2819, 2894};

bool essential_moby(int class_id) {
    return std::find(kEssentialMobys.begin(), kEssentialMobys.end(), class_id)
           != kEssentialMobys.end();
}

// The game's profile: its memory read by program, and the display list it sends.
class Profile final : public host::GameProfile {
public:
    explicit constexpr Profile(const Release& release) : release_(release) {}

    host::GameState read_state(std::span<const std::uint8_t> ram, int level) const override {
        return read_game_state(ram, addresses(level), release_);
    }

    std::optional<host::LiveMoby> read_moby(
        std::span<const std::uint8_t> ram, std::uint32_t moby
    ) const override {
        return read_live_moby(ram, moby, release_);
    }

    // MobyProc leaves a moby past its draw distance (+0x32, at most the level's cap) undrawn and
    // fades it over the last 16 units, as Ratchet & Clank's does.
    std::optional<std::uint8_t> moby_distance_fade(
        const std::uint8_t* moby, const host::GameState& state
    ) const override {
        return host::moby_proc_distance_fade(moby, state);
    }

    bool sends_display_list() const override { return true; }

    bool own_mobys_split_the_2d() const override { return true; }

    bool own_mobys_into_narrow_buffers() const override { return true; }

    bool images_outlast_list() const override { return true; }

    bool repeated_ticks_send_images() const override { return true; }

    bool mobys_take_point_lights() const override { return true; }

    // The paint job on his ship: func_002926B8 loads one of the 18 over the ship's picture (the
    // level's load by 0x001A7AF2, the Ship Shack's preview by the one shown) and keeps its number.
    int texture_variant(std::span<const std::uint8_t> ram) const override {
        const auto paint = at<std::int32_t>(ram, release_.ship_paint);
        return paint >= 0 && paint < kShipPaints ? paint : -1;
    }

    bool interpolates_effect_quads() const override { return true; }

    bool terrain_takes_point_lights() const override { return true; }

    // The ties' matrices: gp + 0x31CC (PatchTieVertexLighting, 0x2F3FEC; level 0's copy
    // D_L00_001B251C), the shrubs': gp + 0x30B0 (LightShrubs, 0x2E2C54; level 0's D_L00_001B2400),
    // the ties' records: gp + 0x31C4 (0x2F3FE0; level 0's D_L00_001B2514), the shrubs' records:
    // gp + 0x30A8 (LightShrubs, 0x2E2C4C).
    std::array<std::uint32_t, 4> scenery_matrices() const override {
        return release_.scenery_matrices;
    }

    // Maktar's dome: light strips (tie class 2713) lie on the dome's band (class 2675) within a few
    // centimetres over large areas, and from the landing pad (about 150 units) they flickered in
    // and out of it with GL's depth (GameProfile::precise_depth). Without glClipControl (macOS's
    // OpenGL) a near of 0.25 parts them five times farther away (GameProfile::depth_near).
    bool precise_depth() const override { return true; }

    float depth_near() const override { return 0.25f; }

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

    // The hero's shadow is Ratchet & Clank's shadow volumes (shadowproc, func_002DFBB0), fed by
    // MobyProc's queue (D_001EF160), which the port's MobyProc (host/render.c) does not fill: the
    // window draws them from the moby instead. MobyProc copies the four directions from 0x1B1B90
    // (slot 0 aimed by func_0029FFB8) to the scratchpad; the slab is across the moby's up
    // axis.
    std::uint32_t shadow_directions() const override { return release_.shadow_directions; }

    bool shadow_slab_on_up_axis() const override { return true; }

    // MobyProc queues every moby with mode 0x400 and a slab: Clank (class 87) where he is the
    // hero (Aranos Prison), the platforms and crates that carry one.
    bool every_moby_casts_shadow() const override { return true; }

    bool has_levels() const override { return true; }

    // The hero's record, D_00189E20 (the motion of func_L00_002B5B60: position +0x80, velocity
    // +0xF0), whose shorts at +0x34C and +0x34E count the updates off the ground and are 0 on
    // landing (Ratchet & Clank's +0x30C and +0x30E: a jump in level 4 read 15, 25, 35, then 0).
    // Enhanced lighting tilts his own shadow map down by it.
    std::uint32_t hero_state() const override { return release_.hero_state; }

    std::uint32_t hero_air_frames() const override { return 0x34E; }

    // The gameplay subjects enhanced lighting gives its GI-coloured rim (port/enhanced/README.md,
    // Gameplay rim lighting); held items and the hero's attachments are added by the frontend.
    bool gameplay_essential(int class_id) const override { return essential_moby(class_id); }

    // An Electrolyzer board keeps the game's draw distance at every Draw distance (kBoardMobys).
    bool keeps_game_draw_distance(int class_id) const override {
        return std::find(kBoardMobys.begin(), kBoardMobys.end(), class_id) != kBoardMobys.end();
    }

    // Game state 2 is a level's scene (host/tools.c STATE_SCENE, which skip_cutscenes ends).
    std::optional<std::uint32_t> cutscene_mode() const override { return 2; }

    // The pad's read (func_002B88F0): |byte - 0x7F| from 0x30, over 0x4C, as Ratchet & Clank's.
    std::array<int, 2> stick_dead_zone() const override { return {0x30, 0x4C}; }

private:
    const Addresses& addresses(int level) const {
        return level >= 0 && static_cast<std::size_t>(level) < std::size(release_.levels)
                       && release_.levels[level].camera_rows != 0
                   ? release_.levels[level]
                   : release_.boot;
    }

    const Release& release_;
};

// One instance for each disc, constants for the program's life.
const Profile kNtscProfile{kNtsc};
const Profile kPalProfile{kPal};
const Profile kNtsc2Profile{kNtsc2};

}  // namespace
}  // namespace openrac::game::rac2

namespace openrac::host {

const GameProfile& game_profile(std::string_view game) {
    // The disc's program id picks its tables; any other id reads as USA v1.01.
    if (game == "rac2-pal") {
        return game::rac2::kPalProfile;
    }
    if (game == "rac2-ntsc2") {
        return game::rac2::kNtsc2Profile;
    }
    return game::rac2::kNtscProfile;
}

}  // namespace openrac::host

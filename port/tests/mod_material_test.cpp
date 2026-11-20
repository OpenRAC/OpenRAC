// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A mod's material on one moby (interface 1.23, openrac_api.moby_material): the call's checks, the
// list the renderer reads (mods::moby_materials) and when an entry ends, with a play host of the
// test's own and no game. The renderer's side is moby_material_pack_test.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "check.h"
#include "mod_play.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/memory.h"

// mod_play.cpp asks the game for its play host; the test sets its own.
extern "C" {
const openrac_game_play* openrac_game_play_host = nullptr;
}

using namespace openrac;

static_assert(
    offsetof(openrac_api, moby_material) == offsetof(openrac_api, pickup_animation) + sizeof(void*),
    "the moby material entry (1.23) must follow the reward entries (1.22)"
);

namespace {

// Bolt coins' classes in Going Commando, the test's mobys.
constexpr int kBolt = 0x0D;
constexpr int kBigBolt = 0x13;
constexpr openrac_gaddr kCoin = 0x1C0000;
constexpr openrac_gaddr kOther = 0x1C0100;

std::uint32_t g_load = 1;

// The test's level loads (openrac_game_play.loads).
std::uint32_t loads() {
    return g_load;
}

// Returns the material the list has for `moby`, or none's.
mods::MobyMaterial material_of(const std::vector<mods::MobyMaterial>& list, openrac_gaddr moby) {
    const auto found =
        std::find_if(list.begin(), list.end(), [&](const auto& m) { return m.moby == moby; });
    return found != list.end() ? *found : mods::MobyMaterial{};
}

}  // namespace

int main() {
    runtime::Memory::create();
    openrac_api api{};
    openrac_game_play play{};
    play.loads = loads;
    mods::fill_play_api(api, &play, {});
    mods::fill_motion_api(api, mods::Host{});
    auto* first = reinterpret_cast<openrac_mod*>(1);
    auto* second = reinterpret_cast<openrac_mod*>(2);

    // What the call refuses: no mod, no moby, an unknown material, an rgb past 0xFFFFFF.
    CHECK(
        api.moby_material(nullptr, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material(first, 0, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(api.moby_material(first, kCoin, 3, 0xFFFFFF) == OPENRAC_HOOK_UNKNOWN);
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_TINT, 0x1000000)
        == OPENRAC_HOOK_UNKNOWN
    );

    // Render-only: setting one writes nothing into the moby's record (0x100 bytes) in game memory.
    const auto* record = static_cast<const std::uint8_t*>(G(kCoin));
    std::vector<std::uint8_t> before(record, record + 0x100);
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xFFFFFF) == OPENRAC_HOOK_OK
    );
    CHECK(std::equal(before.begin(), before.end(), record));

    // A frame from before the game listed the moby keeps it; the first that lists it draws it.
    CHECK(mods::moby_materials({}).empty());
    auto list = mods::moby_materials({{kCoin, kBolt}, {kOther, kBolt}});
    CHECK(list.size() == 1 && material_of(list, kCoin).material == OPENRAC_MOBY_MATERIAL_PLATINUM);
    CHECK(material_of(list, kCoin).rgb == 0xFFFFFF);
    // The game's own effects of the moby ask the same (openrac_moby_platinum); another has none.
    CHECK(openrac_moby_platinum(kCoin) == 1 && openrac_moby_platinum(kOther) == 0);

    // Set again, it changes; two mods on one moby, the last set is drawn.
    CHECK(api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_TINT, 0x80FF80) == OPENRAC_HOOK_OK);
    CHECK(material_of(mods::moby_materials({{kCoin, kBolt}}), kCoin).rgb == 0x80FF80);
    CHECK(
        api.moby_material(second, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xC0C0FF)
        == OPENRAC_HOOK_OK
    );
    list = mods::moby_materials({{kCoin, kBolt}});
    CHECK(list.size() == 1 && material_of(list, kCoin).rgb == 0xC0C0FF);
    CHECK(openrac_moby_platinum(kCoin) == 1);

    // NONE takes one mod's off, the other's is drawn again; unloading a mod takes its off.
    CHECK(api.moby_material(second, kCoin, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);
    CHECK(material_of(mods::moby_materials({{kCoin, kBolt}}), kCoin).rgb == 0x80FF80);
    // A tint is no platinum.
    CHECK(openrac_moby_platinum(kCoin) == 0);
    mods::drop_figures(first);
    CHECK(mods::moby_materials({{kCoin, kBolt}}).empty());

    // Listed once, then not: the game removed the moby, and its slot's next moby has none.
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xFFFFFF) == OPENRAC_HOOK_OK
    );
    CHECK(mods::moby_materials({{kCoin, kBolt}}).size() == 1);
    CHECK(mods::moby_materials({{kOther, kBolt}}).empty());
    CHECK(mods::moby_materials({{kCoin, kBolt}}).empty());

    // Its slot holding a moby of another class is another moby too.
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM, 0xFFFFFF) == OPENRAC_HOOK_OK
    );
    CHECK(mods::moby_materials({{kCoin, kBolt}}).size() == 1);
    CHECK(mods::moby_materials({{kCoin, kBigBolt}}).empty());

    // Another level load ends them.
    CHECK(
        api.moby_material(first, kOther, OPENRAC_MOBY_MATERIAL_TINT, 0x808080) == OPENRAC_HOOK_OK
    );
    ++g_load;
    CHECK(mods::moby_materials({{kOther, kBolt}}).empty());

    // At most OPENRAC_MOBY_MATERIALS_MAX a mod.
    for (int i = 0; i < OPENRAC_MOBY_MATERIALS_MAX; ++i) {
        CHECK(
            api.moby_material(first, kCoin + 0x100u * i, OPENRAC_MOBY_MATERIAL_TINT, 0)
            == OPENRAC_HOOK_OK
        );
    }

    CHECK(
        api.moby_material(first, 0x400000, OPENRAC_MOBY_MATERIAL_TINT, 0) == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(api.moby_material(second, 0x400000, OPENRAC_MOBY_MATERIAL_TINT, 0) == OPENRAC_HOOK_OK);

    // The play's reset forgets them all.
    mods::reset_play();
    CHECK(mods::moby_materials({{kCoin, kBolt}, {0x400000, kBolt}}).empty());
    return test::result();
}

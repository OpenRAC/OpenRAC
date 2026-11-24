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
    CHECK(api.moby_material(first, kCoin, 6, 0xFFFFFF) == OPENRAC_HOOK_UNKNOWN);
    // A level is energy's and paint's alone, at most 15, in bits 8 to 11.
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_TINT | 0x100u, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_ENERGY | 0x1000u, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
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
    CHECK(openrac_moby_obsidian(kCoin, nullptr) == 0);

    // Obsidian (1.24): drawn, with its glow colour for the game's own effects of the moby; the
    // last set wins over another mod's tint, and is no platinum.
    CHECK(
        api.moby_material(second, kCoin, OPENRAC_MOBY_MATERIAL_OBSIDIAN, 0x9020FF)
        == OPENRAC_HOOK_OK
    );
    list = mods::moby_materials({{kCoin, kBolt}});
    CHECK(list.size() == 1 && material_of(list, kCoin).material == OPENRAC_MOBY_MATERIAL_OBSIDIAN);
    std::uint32_t glow = 0;
    CHECK(openrac_moby_obsidian(kCoin, &glow) == 1 && glow == 0x9020FF);
    CHECK(openrac_moby_obsidian(kCoin, nullptr) == 1);
    CHECK(openrac_moby_obsidian(kOther, &glow) == 0 && glow == 0x9020FF);
    CHECK(openrac_moby_platinum(kCoin) == 0);
    CHECK(api.moby_material(second, kCoin, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);
    CHECK(openrac_moby_obsidian(kCoin, nullptr) == 0);

    // Energy (1.25) at a level: drawn, and listed for the effects the moby makes with its colour
    // and level; another material set later on the moby takes it out of that list.
    CHECK(mods::energy_materials().empty());
    CHECK(
        api.moby_material(second, kOther, OPENRAC_MOBY_MATERIAL_ENERGY_AT(7), 0x20FF40)
        == OPENRAC_HOOK_OK
    );
    list = mods::moby_materials({{kCoin, kBolt}, {kOther, kBolt}});
    CHECK(material_of(list, kOther).material == OPENRAC_MOBY_MATERIAL_ENERGY_AT(7));
    auto energy = mods::energy_materials();
    CHECK(energy.size() == 1 && energy.contains(kOther));
    CHECK(energy[kOther].rgb == 0x20FF40);
    CHECK(OPENRAC_MOBY_MATERIAL_KIND(energy[kOther].material) == OPENRAC_MOBY_MATERIAL_ENERGY);
    CHECK(openrac_moby_platinum(kOther) == 0 && openrac_moby_obsidian(kOther, nullptr) == 0);
    CHECK(
        api.moby_material(second, kOther, OPENRAC_MOBY_MATERIAL_TINT, 0x20FF40) == OPENRAC_HOOK_OK
    );
    CHECK(mods::energy_materials().empty());
    CHECK(api.moby_material(second, kOther, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);
    mods::drop_figures(first);
    CHECK(mods::moby_materials({{kCoin, kBolt}}).empty());

    // Paint (1.34) at a level: drawn as set, and no energy, platinum or obsidian.
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PAINT | 0x1000u, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PAINT_AT(12), 0xFFB020)
        == OPENRAC_HOOK_OK
    );
    list = mods::moby_materials({{kCoin, kBolt}});
    CHECK(material_of(list, kCoin).material == OPENRAC_MOBY_MATERIAL_PAINT_AT(12));
    CHECK(material_of(list, kCoin).rgb == 0xFFB020);
    CHECK(mods::energy_materials().empty() && openrac_moby_platinum(kCoin) == 0);
    CHECK(api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);
    CHECK(mods::moby_materials({{kCoin, kBolt}}).empty());

    // Paint with an accent (1.38): the macro keeps the accent's top 3+ bits a channel, the port
    // takes it, and refuses a colour without the flag, bits past it, and an accent on other kinds.
    constexpr std::uint32_t kAccent = OPENRAC_MOBY_MATERIAL_PAINT_ACCENT_AT(9, 0xFF2010);
    CHECK((kAccent & 0xFFu) == OPENRAC_MOBY_MATERIAL_PAINT && (kAccent >> 8 & 0xFu) == 9);
    CHECK((kAccent & OPENRAC_MOBY_MATERIAL_PAINT_ACCENT) != 0);
    CHECK(
        (kAccent >> 20 & 0xFu) == 0xF && (kAccent >> 16 & 0xFu) == 0x2
        && (kAccent >> 12 & 0xFu) == 0x1
    );
    CHECK(api.moby_material(first, kCoin, kAccent, 0x2080FF) == OPENRAC_HOOK_OK);
    list = mods::moby_materials({{kCoin, kBolt}});
    CHECK(material_of(list, kCoin).material == kAccent && material_of(list, kCoin).rgb == 0x2080FF);
    CHECK(
        OPENRAC_MOBY_MATERIAL_KIND(material_of(list, kCoin).material) == OPENRAC_MOBY_MATERIAL_PAINT
    );
    // A colour with no flag, a bit above the flag, and an accent on platinum or energy.
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PAINT | 0x2000u, 0xFFFFFF)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(api.moby_material(first, kCoin, kAccent | 0x4000000u, 0) == OPENRAC_HOOK_UNKNOWN);
    CHECK(
        api.moby_material(
            first, kCoin, OPENRAC_MOBY_MATERIAL_PLATINUM | OPENRAC_MOBY_MATERIAL_PAINT_ACCENT, 0
        )
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material(
            first, kCoin, OPENRAC_MOBY_MATERIAL_ENERGY_AT(3) | OPENRAC_MOBY_MATERIAL_PAINT_ACCENT, 0
        )
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);

    // Paint whose effects take its colour (1.39): taken with or without an accent, its effects
    // listed with the energy materials; refused on energy, platinum or the tint, and plain paint
    // stays out of the list.
    constexpr std::uint32_t kPaintEffects =
        OPENRAC_MOBY_MATERIAL_PAINT_AT(10) | OPENRAC_MOBY_MATERIAL_PAINT_EFFECTS;
    CHECK(api.moby_material(first, kCoin, kPaintEffects, 0x60C8FF) == OPENRAC_HOOK_OK);
    energy = mods::energy_materials();
    CHECK(energy.size() == 1 && energy[kCoin].material == kPaintEffects);
    CHECK(energy[kCoin].rgb == 0x60C8FF);
    CHECK(
        api.moby_material(first, kCoin, kAccent | OPENRAC_MOBY_MATERIAL_PAINT_EFFECTS, 0x60C8FF)
        == OPENRAC_HOOK_OK
    );
    CHECK(mods::energy_materials().size() == 1);
    CHECK(
        api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_PAINT_AT(10), 0x60C8FF)
        == OPENRAC_HOOK_OK
    );
    CHECK(mods::energy_materials().empty());
    for (const std::uint32_t other :
         {OPENRAC_MOBY_MATERIAL_ENERGY_AT(3),
          static_cast<std::uint32_t>(OPENRAC_MOBY_MATERIAL_TINT),
          static_cast<std::uint32_t>(OPENRAC_MOBY_MATERIAL_PLATINUM)}) {
        CHECK(
            api.moby_material(first, kCoin, other | OPENRAC_MOBY_MATERIAL_PAINT_EFFECTS, 0)
            == OPENRAC_HOOK_UNKNOWN
        );
    }
    CHECK(api.moby_material(first, kCoin, OPENRAC_MOBY_MATERIAL_NONE, 0) == OPENRAC_HOOK_OK);

    // A class's material (1.34): what it refuses, one a class and scope, the last set wins,
    // NONE takes the mod's off in the scope named, a mod's unload and a level load end it.
    const auto by_class = [](int oclass, std::uint32_t scope) {
        for (const mods::ClassMaterialDraw& d : mods::class_materials()) {
            if (d.oclass == oclass && d.scope == scope) {
                return d.material;
            }
        }
        return mods::MobyMaterial{};
    };
    constexpr std::uint32_t kBoth = OPENRAC_MODEL_WORLD | OPENRAC_MODEL_MENU;
    CHECK(
        api.moby_material_class(nullptr, 0, kBoth, OPENRAC_MOBY_MATERIAL_TINT, 0)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material_class(first, -1, kBoth, OPENRAC_MOBY_MATERIAL_TINT, 0)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material_class(first, 0, 0, OPENRAC_MOBY_MATERIAL_TINT, 0) == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(
        api.moby_material_class(first, 0, 4, OPENRAC_MOBY_MATERIAL_TINT, 0) == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(api.moby_material_class(first, 0, kBoth, 6, 0) == OPENRAC_HOOK_UNKNOWN);
    CHECK(
        api.moby_material_class(first, 0, kBoth, OPENRAC_MOBY_MATERIAL_TINT, 0x1000000)
        == OPENRAC_HOOK_UNKNOWN
    );
    CHECK(mods::class_materials().empty());
    CHECK(
        api.moby_material_class(first, 0, kBoth, OPENRAC_MOBY_MATERIAL_PAINT_AT(3), 0x40C0FF)
        == OPENRAC_HOOK_OK
    );
    CHECK(mods::class_materials().size() == 2);
    CHECK(by_class(0, OPENRAC_MODEL_WORLD).material == OPENRAC_MOBY_MATERIAL_PAINT_AT(3));
    CHECK(by_class(0, OPENRAC_MODEL_MENU).rgb == 0x40C0FF);
    CHECK(
        api.moby_material_class(second, 0, OPENRAC_MODEL_MENU, OPENRAC_MOBY_MATERIAL_TINT, 0x808080)
        == OPENRAC_HOOK_OK
    );
    CHECK(by_class(0, OPENRAC_MODEL_MENU).rgb == 0x808080);
    CHECK(by_class(0, OPENRAC_MODEL_WORLD).rgb == 0x40C0FF);
    // Another mod's NONE leaves the first's; its own takes it off.
    CHECK(
        api.moby_material_class(second, 0, OPENRAC_MODEL_WORLD, OPENRAC_MOBY_MATERIAL_NONE, 0)
        == OPENRAC_HOOK_OK
    );
    CHECK(by_class(0, OPENRAC_MODEL_WORLD).rgb == 0x40C0FF);
    CHECK(
        api.moby_material_class(first, 0, OPENRAC_MODEL_WORLD, OPENRAC_MOBY_MATERIAL_NONE, 0)
        == OPENRAC_HOOK_OK
    );
    CHECK(mods::class_materials().size() == 1);
    mods::drop_figures(second);
    CHECK(mods::class_materials().empty());
    CHECK(
        api.moby_material_class(first, 7, kBoth, OPENRAC_MOBY_MATERIAL_TINT, 0) == OPENRAC_HOOK_OK
    );
    ++g_load;
    CHECK(mods::class_materials().empty());

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

// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "check.h"
#include "mod_play.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/memory.h"
extern "C" {
const openrac_game_play* openrac_game_play_host = nullptr;
}
using namespace openrac;
static_assert(
    offsetof(openrac_api, moby_visuals)
        == offsetof(openrac_api, setting_set_bool) + 9 * sizeof(void*),
    "released-SDK compatibility must skip the official settings, save-data and collision slots"
);

namespace {
std::uint32_t load = 1;

std::uint32_t loads() {
    return load;
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
    const openrac_moby_visual bolt{0x10000, 0x46E, 1.0f};
    CHECK(api.moby_visuals(first, &bolt, 1) == OPENRAC_HOOK_OK);
    CHECK(mods::moby_visuals().size() == 1);
    const auto generation = mods::visual_classes_generation();
    CHECK(api.moby_visuals(first, &bolt, 1) == OPENRAC_HOOK_OK);
    CHECK(mods::visual_classes_generation() == generation);
    auto invalid = bolt;
    invalid.height = std::numeric_limits<float>::quiet_NaN();
    CHECK(api.moby_visuals(first, &invalid, 1) == OPENRAC_HOOK_UNKNOWN);
    CHECK(mods::moby_visuals().size() == 1);
    CHECK(api.moby_visuals(first, nullptr, 1) == OPENRAC_HOOK_UNKNOWN);
    CHECK(api.moby_visuals(first, &bolt, OPENRAC_MOBY_VISUALS_MAX + 1) == OPENRAC_HOOK_UNKNOWN);
    const openrac_moby_visual bot{0x10100, 0x2EE, 0};
    CHECK(api.moby_visuals(second, &bot, 1) == OPENRAC_HOOK_OK);
    CHECK(mods::moby_visuals().size() == 2);
    mods::drop_figures(first);
    CHECK(mods::moby_visuals().size() == 1 && mods::moby_visuals()[0].oclass == 0x2EE);
    ++load;
    CHECK(mods::moby_visuals().empty());
    CHECK(api.moby_visuals(second, nullptr, 0) == OPENRAC_HOOK_OK);
    std::string text = "Buy Gold Bolt for 500 bolts";
    const auto address = api.guest_text(first, text.c_str());
    CHECK(address != 0);
    text.assign("changed native buffer");
    CHECK(
        std::strcmp(
            reinterpret_cast<const char*>(G(GSTR("Buy Gold Bolt for 500 bolts"))),
            reinterpret_cast<const char*>(G(address))
        )
        == 0
    );
    CHECK(api.guest_text(first, "Buy Gold Bolt for 500 bolts") == address);
    CHECK(api.guest_text(first, std::string(1024, 'x').c_str()) == 0);
    auto& memory = runtime::Memory::get();
    std::vector<std::uint8_t> image(runtime::Memory::snapshot_size());
    CHECK(memory.save_snapshot(image));
    openrac_guest_allocation_state allocations{};
    openrac_guest_get_allocation_state(&allocations);
    std::vector<std::uint8_t> metadata(openrac_guest_snapshot_metadata_size());
    std::size_t used = 0;
    CHECK(openrac_guest_snapshot_metadata_save(metadata.data(), metadata.size(), &used) == 0);
    std::strcpy(reinterpret_cast<char*>(G(address)), "changed guest buffer");
    CHECK(memory.restore_snapshot(image, allocations));
    CHECK(openrac_guest_snapshot_metadata_load(metadata.data(), used, &allocations) == 0);
    const auto restored = api.guest_text(first, "Buy Gold Bolt for 500 bolts");
    CHECK(
        restored != 0
        && std::strcmp(reinterpret_cast<const char*>(G(restored)), "Buy Gold Bolt for 500 bolts")
               == 0
    );
    CHECK(api.sprite_tint(first, 0x20647Fu) == UINT32_MAX);
    CHECK(mods::sprite_tint() == 0x20647Fu);
    CHECK(api.sprite_tint(first, UINT32_MAX) == 0x20647Fu);
    mods::reset_play();
    return test::result();
}

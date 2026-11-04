// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <limits>

#include "enhanced/renderer/water_impulse_contact.h"
#include "game/common/frontend/water_interactions.h"
#include "tests/check.h"
using namespace openrac;

int main() {
    for (const auto region : {"rac1-pal", "rac1-ntsc"}) {
        const auto& profile = host::game_profile(region);
        for (int projectile : {121, 305, 153, 172, 41}) {
            CHECK(profile.water_projectile(projectile));
        }
        for (int other : {0, 695, 168, 192, 226}) {
            CHECK(!profile.water_projectile(other));
        }
        host::LiveMoby old;
        old.address = 123;
        old.class_id = 305;
        auto current = old;
        current.matrix[14] = 2;
        std::vector<renderer::WaterImpulse> impulses;
        frontend::water_projectile_impulses(
            profile, std::array{current}, std::array{old}, impulses
        );
        CHECK(impulses.size() == 1 && impulses[0].from[2] == 0 && impulses[0].to[2] == 2);
        impulses.clear();
        frontend::water_projectile_impulses(profile, std::array{old}, std::array{old}, impulses);
        CHECK(impulses.empty());
        for (const auto value : {100.f, std::numeric_limits<float>::quiet_NaN()}) {
            current.matrix[14] = value;
            frontend::water_projectile_impulses(
                profile, std::array{current}, std::array{old}, impulses
            );
            CHECK(impulses.empty());
        }
        current = old;
        current.matrix[14] = 1;
        current.address = 124;
        frontend::water_projectile_impulses(
            profile, std::array{current}, std::array{old}, impulses
        );
        CHECK(impulses.empty());  // Spawn slots never sweep from an unrelated position.
        current.address = old.address;
        current.class_id = 121;
        frontend::water_projectile_impulses(
            profile, std::array{current}, std::array{old}, impulses
        );
        CHECK(impulses.empty());  // Slot reuse by another projectile type.
        current.class_id = old.class_id;
        impulses.resize(renderer::kWaterImpulseLimit);
        frontend::water_projectile_impulses(
            profile, std::array{current}, std::array{old}, impulses
        );
        CHECK(impulses.size() == renderer::kWaterImpulseLimit);
    }
    std::array<std::array<float, 4>, 5> bodies{};
    bodies[1][3] = bodies[2][3] = 4;
    std::array<std::array<float, 4>, 16> coast;
    coast.fill({514, 1, 0, 0});  // Wet, rest Z=2.
    renderer::WaterImpulse shot{{.75f, .75f, 10}, {.75f, .75f, -10}, .25f, .5f};
    CHECK(
        enhanced::water_impulse_contact(bodies, coast, shot)
    );  // Swept crossing, neither endpoint wet.
    shot.from = shot.to = {.75f, .75f, 4};
    CHECK(!enhanced::water_impulse_contact(bodies, coast, shot));
    shot.radius = 2;
    CHECK(
        enhanced::water_impulse_contact(bodies, coast, shot)
    );  // Nearby blast reaches the surface.
    shot.from[0] = shot.to[0] = 100;
    CHECK(!enhanced::water_impulse_contact(bodies, coast, shot));
    shot.from = shot.to = {.75f, .75f, 2};
    coast.fill({});
    CHECK(!enhanced::water_impulse_contact(bodies, coast, shot));  // Dry cell.
    shot.radius = std::numeric_limits<float>::quiet_NaN();
    CHECK(!enhanced::water_impulse_contact(bodies, coast, shot));
    return test::result();
}

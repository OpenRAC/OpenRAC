// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <cstring>
#include <limits>
#include <vector>

#include "game/rac1/game_state.h"
#include "tests/check.h"

namespace {
template <class T>
void put(std::vector<std::uint8_t>& ram, std::uint32_t address, T value) {
    std::memcpy(ram.data() + address, &value, sizeof(value));
}

void test_focus(const openrac::game::rac1::GameAddresses& addresses) {
    using namespace openrac;
    std::vector<std::uint8_t> ram(32u * 1024 * 1024);
    constexpr std::uint32_t record = 0x200000, saved = 0x201000;
    const auto read = [&] {
        return game::rac1::read_game_state(ram, addresses);
    };
    CHECK(!read().camera_target);
    put(ram, addresses.camera_position + 0x40, record);
    put(ram, record + 0x70, saved);
    put(ram, saved + 0x40, 1.f);
    put(ram, saved + 0x44, 2.f);
    put(ram, saved + 0x48, 3.f);
    const auto focus = read();
    CHECK(focus.camera_target && *focus.camera_target == renderer::Vec3({1, 2, 3}));
    CHECK(focus.camera_target_id == (std::uint64_t(record) << 32 | saved));
    // Moving/orbiting the eye must not be mistaken for movement of its tracking target.
    put(ram, addresses.camera_position, 100.f);
    put(ram, addresses.camera_position + 8, 200.f);
    CHECK(read().camera_target == focus.camera_target);
    put(ram, saved + 0x48, 3.1f);
    CHECK(read().camera_target && (*read().camera_target)[2] == 3.1f);
    put(ram, record + 0x86, std::int16_t{6});
    CHECK(!read().camera_target);  // Other camera records do not own the follow layout.
    put(ram, record + 0x86, std::int16_t{0});
    put(ram, saved + 0x40, std::numeric_limits<float>::quiet_NaN());
    CHECK(!read().camera_target);
    put(ram, record + 0x70, std::uint32_t{0x1ffffff0});
    CHECK(!read().camera_target);
    put(ram, addresses.camera_position + 0x40, std::uint32_t{0x1ffffff0});
    CHECK(!read().camera_target);
}
}  // namespace

int main() {
    test_focus(openrac::game::rac1::kRac1Ntsc);
    test_focus(openrac::game::rac1::kRac1Pal);
    for (int level = 0; level < 19; ++level) {
        test_focus(openrac::game::rac1::kRac1NtscLevels[level]);
        test_focus(openrac::game::rac1::kRac1PalLevels[level]);
    }
    return openrac::test::result();
}

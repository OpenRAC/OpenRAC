// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the game hands over at its addresses (game/common/frontend/game_image_sources.h): kept as it
// was when handed over, found by the address and its uncached view, replaced by bytes kept over
// (part of) it, nothing past the memory.
#include <cstdint>
#include <vector>

#include "game/common/frontend/game_image_sources.h"
#include "tests/check.h"

namespace {

using openrac::frontend::GameImageSources;

void test_kept_as_handed_over() {
    GameImageSources sources;
    std::vector<std::uint8_t> ram(0x1000, 0);
    ram[0x100] = 3;
    sources.keep(ram, 0x100, 4);
    CHECK(sources.bytes() == 4);
    ram[0x100] = 9;  // the game reuses its buffer
    const auto kept = sources.find(0x100, 4);
    CHECK(kept.size() == 4 && kept[0] == 3);
    CHECK(sources.find(0x100, 2).size() == 2);  // a shorter claim is answered
    CHECK(sources.find(0x100, 8).empty());      // a longer one is not
    CHECK(sources.find(0x104, 4).empty());      // nor another address inside
    // The uncached views of the same bytes.
    CHECK(!sources.find(0x20000100u, 4).empty());
    CHECK(!sources.find(0x30000100u, 4).empty());
}

void test_kept_over() {
    GameImageSources sources;
    std::vector<std::uint8_t> ram(0x1000, 0);
    ram[0x100] = 1;
    sources.keep(ram, 0x100, 4);
    const std::uint64_t first = sources.generation();
    ram[0x100] = 2;
    sources.keep(ram, 0x100, 4);
    CHECK(sources.generation() != first);
    CHECK(sources.find(0x100, 4)[0] == 2);
    CHECK(sources.bytes() == 4);
    // Over part of it, from either side: the older goes.
    sources.keep(ram, 0x102, 8);
    CHECK(sources.find(0x100, 4).empty());
    CHECK(sources.bytes() == 8);
    sources.keep(ram, 0x0F0, 0x14);
    CHECK(sources.find(0x102, 8).empty());
    CHECK(!sources.find(0x0F0, 0x14).empty());
    // Beside it, both stay.
    sources.keep(ram, 0x104, 4);
    CHECK(!sources.find(0x0F0, 0x14).empty() && !sources.find(0x104, 4).empty());
    CHECK(sources.bytes() == 0x18);
}

void test_outside_memory() {
    GameImageSources sources;
    std::vector<std::uint8_t> ram(0x1000, 0);
    sources.keep(ram, 0x2000, 4);
    sources.keep(ram, 0xFFE, 4);
    sources.keep(ram, 0x100, 0);
    CHECK(sources.bytes() == 0);
    sources.keep(ram, 0x100, 4);
    sources.clear();
    CHECK(sources.bytes() == 0 && sources.find(0x100, 4).empty());
}

}  // namespace

int main() {
    test_kept_as_handed_over();
    test_kept_over();
    test_outside_memory();
    return openrac::test::result();
}

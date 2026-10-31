// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// The larger guest arena used only by mounts with expanded level streams.

#include "check.h"
#include "openrac/guest.h"
#include "openrac/memory.h"

int main() {
    using openrac::runtime::Memory;
    Memory& memory = Memory::create(GUEST_RAM_EXPANDED_SIZE);

    CHECK(memory.ram_size() == GUEST_RAM_EXPANDED_SIZE);
    CHECK(openrac_guest_ram_size == GUEST_RAM_EXPANDED_SIZE);
    CHECK(memory.mapped(GUEST_RAM_EXPANDED_SIZE - 4, 4));
    CHECK(!memory.mapped(GUEST_RAM_EXPANDED_SIZE, 4));
    CHECK(Memory::snapshot_size() > GUEST_RAM_SIZE);
    CHECK(openrac_guest_ram_address_for_size(0x03FFFFFFu, GUEST_RAM_EXPANDED_SIZE)
          == 0x03FFFFFFu);
    CHECK(openrac_guest_ram_address_for_size(
              GUEST_RAM_UNCACHED + 0x03FFFFFFu, GUEST_RAM_EXPANDED_SIZE
          ) == 0x03FFFFFFu);
    CHECK(openrac_guest_ram_address_for_size(
              GUEST_RAM_UNCACHED_ACCEL + 0x03FFFFFFu, GUEST_RAM_EXPANDED_SIZE
          ) == 0x03FFFFFFu);
    GREF(std::uint32_t, 0x03FFFFFCu) = 0xC0DEC0DEu;
    CHECK(GREF(std::uint32_t, 0x03FFFFFCu) == 0xC0DEC0DEu);
    if (memory.has_aliases()) {
        CHECK(GREF(std::uint32_t, GUEST_RAM_UNCACHED + 0x03FFFFFCu) == 0xC0DEC0DEu);
        CHECK(GREF(std::uint32_t, GUEST_RAM_UNCACHED_ACCEL + 0x03FFFFFCu) == 0xC0DEC0DEu);
    }
    return openrac::test::result();
}

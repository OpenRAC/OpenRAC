// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The table of game versions, and the byte reader.

#include "assets/version.h"

#include "assets/bytes.h"
#include "tests/check.h"

using namespace openrac::assets;

int main() {
    CHECK(versions().size() == 5);
    const GameVersion* pal = find_version("rac1-pal");
    CHECK(pal != nullptr);
    if (pal) {
        CHECK(pal->key == "rac1/pal");
        CHECK(pal->serial == "SCES_509.16");
        CHECK(pal->region == Region::Pal);
        CHECK(pal->frame_rate == 50);
        CHECK(find_version_by_boot(pal->serial, pal->boot_sha1) == pal);
    }
    const GameVersion* rac4 = find_version("rac4/ntsc");
    CHECK(rac4 != nullptr && rac4->game == Game::Rac4 && rac4->frame_rate == 60);
    CHECK(find_version_by_boot("SCES_509.16", "00") == nullptr);
    Game game{};
    CHECK(game_of_serial("SCES_524.56", game) && game == Game::Rac3);
    CHECK(!game_of_serial("SLUS_200.00", game));

    ByteWriter w;
    w.put<u32>(0x11223344);
    w.put<s16>(-2);
    w.put_bytes(std::span<const u8>(reinterpret_cast<const u8*>("abc\0"), 4));
    ByteView v(w.bytes());
    CHECK(v.u32_at(0) == 0x11223344);
    CHECK(v.s16_at(4) == -2);
    CHECK(v.string_at(6, 16) == "abc");
    bool threw = false;
    try {
        (void)v.u32_at(8);
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The table of game versions, and the byte reader.

#include "assets/version.h"

#include "assets/bytes.h"
#include "tests/check.h"

using namespace openrac::assets;

int main() {
    CHECK(versions().size() == 10);
    const GameVersion* pal = find_version("rac1-pal");
    CHECK(pal != nullptr);
    if (pal) {
        CHECK(pal->key == "rac1/pal");
        CHECK(pal->serial == "SCES_509.16");
        CHECK(pal->region == Region::Pal);
        CHECK(pal->frame_rate == 50);
        CHECK(find_version_by_boot(pal->serial, pal->boot_sha1) == pal);
    }
    // Ratchet & Clank's two PAL releases share SCES_509.16, told apart by the executable's bytes.
    const GameVersion* pal1 = find_version("rac1/pal1");
    const GameVersion* jp = find_version("rac1/jp");
    CHECK(pal1 && jp);
    if (pal && pal1 && jp) {
        CHECK(pal1->serial == pal->serial && pal1->boot_sha1 != pal->boot_sha1);
        CHECK(find_version_by_boot(pal1->serial, pal1->boot_sha1) == pal1);
        CHECK(pal1->region == Region::Pal && pal1->frame_rate == 50);
        CHECK(jp->serial == "SCPS_150.37" && jp->region == Region::NtscJ && jp->frame_rate == 60);
    }
    // Going Commando's three releases: two of them boot SCUS_972.68, told apart by its bytes.
    const GameVersion* gc = find_version("rac2/ntsc");
    const GameVersion* gc2 = find_version("rac2-ntsc2");
    const GameVersion* gc_pal = find_version("rac2/pal");
    CHECK(gc && gc2 && gc_pal);
    if (gc && gc2 && gc_pal) {
        CHECK(gc->serial == gc2->serial && gc->boot_sha1 != gc2->boot_sha1);
        CHECK(find_version_by_boot(gc2->serial, gc2->boot_sha1) == gc2);
        CHECK(find_version_by_boot(gc->serial, gc->boot_sha1) == gc);
        CHECK(gc2->region == Region::NtscU && gc2->frame_rate == 60);
        CHECK(gc_pal->region == Region::Pal && gc_pal->frame_rate == 50);
    }
    // Up Your Arsenal's PAL disc, rac3-decomp's own target, at 50 Hz.
    const GameVersion* uya_pal = find_version("rac3-pal");
    CHECK(uya_pal != nullptr);
    if (uya_pal) {
        CHECK(uya_pal->game == Game::Rac3 && uya_pal->serial == "SCES_524.56");
        CHECK(uya_pal->region == Region::Pal && uya_pal->frame_rate == 50);
        CHECK(find_version_by_boot(uya_pal->serial, uya_pal->boot_sha1) == uya_pal);
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

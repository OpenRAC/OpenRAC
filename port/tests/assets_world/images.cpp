// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Images outside the level data: PIF pictures, the spaceships files and the
// HUD's banks of icons, on small invented pictures.

#include "assets/world/hud.h"
#include "assets/world/pif.h"
#include "assets/world/spaceships.h"
#include "tests/check.h"

using namespace openrac::assets;

namespace {

template <typename F>
bool throws(F&& f) {
    try {
        f();
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

// A PIF of `side` x `side` pixels, all of index `index`, whose palette
// entry for that index (stored in CSM1 order) is `rgba`.
std::vector<u8> pif(u32 side, u8 index, std::array<u8, 4> rgba, u32 format = kPifFormatPsmt8) {
    ByteWriter w;
    w.put_bytes(std::vector<u8>{'2', 'F', 'I', 'P'});
    w.put<u32>(0);
    w.put(side);
    w.put(side);
    w.put(format);
    w.resize(kPifPaletteOffset);
    std::vector<u8> palette(0x400, 0);
    std::copy(rgba.begin(), rgba.end(), palette.begin() + clut_index(index) * 4);
    w.put_bytes(palette);
    w.put_bytes(std::vector<u8>(std::size_t{side} * side, index));
    return w.bytes();
}

void pifs() {
    const auto bytes = pif(8, 9, {200, 100, 50, 0x40});
    const PifImage p = read_pif(Game::Rac1, bytes);
    CHECK(p.width == 8 && p.height == 8 && p.pixels.size() == 64);
    const RgbaImage raw = p.image().decode(GsAlpha::Raw);
    CHECK(raw.rgba[0] == 200 && raw.rgba[1] == 100 && raw.rgba[3] == 0x40);
    CHECK(p.image().decode().rgba[3] == 0x80);  // scaled alpha
    std::vector<u8> bad = bytes;
    bad[0] = 'X';
    CHECK(throws([&] { read_pif(Game::Rac1, bad); }));
    CHECK(throws([&] { read_pif(Game::Rac1, pif(8, 0, {}, 0x14)); }));  // not PSMT8
    CHECK(throws([&] { read_pif(Game::Rac1, pif(6, 0, {})); }));        // not a power of two
    CHECK(throws([&] { read_pif(Game::Rac4, bytes); }));                // not known for Deadlocked
}

// A spaceships file: header, ship class, extra class, the two texture lists.
std::vector<u8> spaceships(u32 ship_side, u32 extra_side) {
    auto texture_list = [](u32 side) {
        ByteWriter w;
        w.put<u32>(1);
        w.put<u32>(0x10);
        w.resize(0x10);
        w.put_bytes(pif(side, 1, {1, 2, 3, 0x80}));
        return w.bytes();
    };
    const std::vector<u8> ship_class(0x40, 0x11);
    const std::vector<u8> extra_class(0x20, 0x22);
    const auto ship_texture = texture_list(ship_side);
    const auto extra_texture = texture_list(extra_side);
    const u32 ship = 0x10;
    const auto extra = static_cast<u32>(ship + ship_class.size());
    const auto texture = static_cast<u32>(extra + extra_class.size());
    const auto extra_tex = static_cast<u32>(texture + ship_texture.size());
    ByteWriter w;
    w.put(ship);
    w.put(texture);
    w.put(extra);
    w.put(extra_tex);
    for (const auto* part : {&ship_class, &extra_class, &ship_texture, &extra_texture}) {
        w.put_bytes(*part);
    }
    return w.bytes();
}

void spaceship_files() {
    const auto bytes = spaceships(256, 128);
    const SpaceshipFile f = read_spaceship_file(Game::Rac1, bytes);
    CHECK(f.ship_class.size() == 0x40 && f.ship_class.u8_at(0) == 0x11);
    CHECK(f.extra_class.size() == 0x20 && f.extra_class.u8_at(0) == 0x22);
    CHECK(f.ship_texture.width == 256 && f.extra_texture.width == 128);
    CHECK(rac1_spaceships_entry(2) == 3);
    CHECK(throws([&] { read_spaceship_file(Game::Rac1, spaceships(128, 128)); }));
    std::vector<u8> swapped = bytes;
    std::swap_ranges(
        swapped.begin(), swapped.begin() + 4, swapped.begin() + 8
    );  // extra before ship
    CHECK(throws([&] { read_spaceship_file(Game::Rac1, swapped); }));
}

// A HUD with two icons: icon 7 animates frames 0..1, icon 9 shows frame 1.
// Frame 0 is a 4x4 texture in bank 1 with its palette in bank 0; frame 1 an
// 8x2 texture with both in bank 1.
struct Hud {
    std::vector<u8> header;
    std::array<std::vector<u8>, kHudBanks> banks;
};

Hud invented_hud() {
    ByteWriter h;
    h.resize(kHudHeaderSize);
    const u32 icons = static_cast<u32>(kHudHeaderSize);
    const u32 frames = icons + 3 * 8;
    const u32 palettes = frames + 2 * 4;
    const u32 textures = palettes + 2 * 8;
    h.put_at<u16>(0, 3);  // icons, with the terminator
    h.put_at<u16>(2, 2);
    h.put_at(4, icons);
    h.put_at(8, frames);
    h.put_at(0xc, palettes);
    h.put_at(0x10, textures);
    // Cumulative counts per bank: palette 0 in bank 0, palette 1 in bank 1;
    // both textures in bank 1.
    const std::array<u32, 5> palette_total = {1, 2, 2, 2, 2};
    const std::array<u32, 5> texture_total = {0, 2, 2, 2, 2};
    const std::array<u32, 5> sizes = {0x400, 0x500, 0, 0, 0};
    for (std::size_t b = 0; b < 5; ++b) {
        h.put_at(0x14 + 4 * b, palette_total[b]);
        h.put_at(0x34 + 4 * b, texture_total[b]);
        h.put_at(0x54 + 4 * b, sizes[b]);
    }
    // Icons: id, frame count, first frame, animation mode, ticks per frame.
    for (const std::array<u16, 3>& icon :
         {std::array<u16, 3>{7, 2, 0}, {9, 1, 1}, {0xffff, 0, 0}}) {
        h.put(icon[0]);
        h.put(icon[1]);
        h.put(icon[2]);
        h.put<u8>(1);
        h.put<u8>(4);
    }
    // Frames: palette, texture.
    for (const std::array<s16, 2>& f : {std::array<s16, 2>{0, 0}, {1, 1}}) {
        h.put(f[0]);
        h.put(f[1]);
    }
    // Palettes: offset in their bank, CBP.
    for (const u32 at : {0u, 0x100u}) {
        h.put(at);
        h.put<u16>(0);
        h.put<u16>(0);
    }
    // Textures: offset, TBP, log2 width, log2 height.
    for (const std::array<u32, 3>& t : {std::array<u32, 3>{0, 2, 2}, {0x10, 3, 1}}) {
        h.put(t[0]);
        h.put<u16>(0);
        h.put(static_cast<u8>(t[1]));
        h.put(static_cast<u8>(t[2]));
    }
    Hud hud{h.bytes(), {}};
    hud.banks[0].assign(0x400, 0);
    hud.banks[0][clut_index(5) * 4] = 77;  // palette 0, index 5: red 77
    hud.banks[0][clut_index(5) * 4 + 3] = 0x80;
    hud.banks[1].assign(0x500, 5);                 // every pixel index 5
    hud.banks[1][0x100 + clut_index(5) * 4] = 33;  // palette 1, index 5: red 33
    return hud;
}

void hud_sets() {
    Hud h = invented_hud();
    auto read = [&] {
        std::array<ByteView, kHudBanks> banks;
        for (std::size_t b = 0; b < kHudBanks; ++b) {
            banks[b] = h.banks[b];
        }
        return HudSet::read(Game::Rac1, h.header, banks);
    };
    const HudSet hud = read();
    CHECK(hud.icons().size() == 3 && hud.frames().size() == 2);
    CHECK(hud.icon_index(9) == 1);
    CHECK(hud.icon_index(1234) == 2);  // unknown ids find the terminator
    CHECK(hud.icon_frame(7, 1) == 1 && hud.icon_frame(7, 5) == 0 && hud.icon_frame(9, 0) == 1);
    CHECK(hud.palette_bank(0) == 0u && hud.palette_bank(1) == 1u && !hud.palette_bank(2));
    CHECK(hud.texture_bank(0) == 1u);
    CHECK((hud.frame_size(1) == std::pair<u32, u32>{8, 2}));
    CHECK(!hud.frame_size(5));
    const RgbaImage f0 = hud.decode_frame(0, GsAlpha::Raw);
    CHECK(f0.width == 4 && f0.height == 4 && f0.rgba[0] == 77 && f0.rgba[3] == 0x80);
    CHECK(hud.decode_frame(1, GsAlpha::Raw).rgba[0] == 33);

    h.header[kHudHeaderSize + 2 * 8] = 0;  // the terminator's id
    CHECK(throws(read));
    h = invented_hud();
    h.banks[1].resize(0x100);  // shorter than its header says
    CHECK(throws(read));
}

}  // namespace

int main() {
    pifs();
    spaceship_files();
    hud_sets();
    return openrac::test::result();
}

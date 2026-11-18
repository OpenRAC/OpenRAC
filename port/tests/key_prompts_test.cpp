// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts (game/common/key_prompts.h) on fonts made here: found through the bank and table,
// painted as the game's shoulder tabs with their letters taken out, put back byte for byte, left
// alone when the game reloads them, and refused when the tabs are not ones it knows. Key labels (key_labels.h) too.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "game/common/key_labels.h"
#include "game/common/key_prompts.h"
#include "key_prompts_fonts.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c);                    \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

namespace kp = openrac::key_prompts;

namespace {

constexpr std::uint32_t kRam = 0x100000;
constexpr std::uint32_t kBankWord = 0x100;
constexpr std::uint32_t kTable = 0x200;
constexpr std::uint32_t kBank = 0x10000;

void put16(std::vector<std::uint8_t>& m, std::uint32_t a, std::uint16_t v) {
    std::memcpy(m.data() + a, &v, 2);
}

// Three fonts and their palette: texels use entries 0..`used`-1, entry 0 clear.
std::vector<std::uint8_t> game_memory(int fonts, int used, bool tabs = true) {
    std::vector<std::uint8_t> m(kRam);
    std::memcpy(m.data() + kBankWord, &kBank, 4);
    const std::uint16_t clut = (3 * kp::kFontBytes) / 16;
    for (int f = 0; f < fonts; ++f) {
        const std::uint32_t record = kTable + 16u * static_cast<std::uint32_t>(f + 1);
        put16(m, record + 8, static_cast<std::uint16_t>(f * kp::kFontBytes / 16));
        put16(m, record + 0xA, clut);
        for (std::uint32_t i = 0; i < kp::kFontBytes; ++i) {
            m[kBank + f * kp::kFontBytes + i] = static_cast<std::uint8_t>((i * 7u + f) % used);
        }
        if (tabs) {
            draw_tabs(m.data() + kBank + f * kp::kFontBytes);
        }
    }
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint8_t* e = m.data() + kBank + clut * 16u + i * 4u;
        e[0] = e[1] = e[2] = static_cast<std::uint8_t>(i);
        e[3] = i == 0 ? 0 : 0x80;
    }
    return m;
}

void labels() {
    using openrac::keys::icon_label;
    using openrac::keys::menu_label;
    CHECK(menu_label("KeyW") == "W");
    CHECK(menu_label("Digit4") == "4");
    CHECK(menu_label("MouseLeft") == "Left click");
    CHECK(menu_label("Numpad4") == "Num 4");
    CHECK(menu_label("F5") == "F5");
    CHECK(icon_label("KeyW") == "W");
    CHECK(icon_label("Space") == "SPACE");
    CHECK(icon_label("MouseLeft") == "LMB");
    CHECK(icon_label("Numpad4") == "NUM4");
    CHECK(icon_label("NumpadEnter").size() <= 5);
    CHECK(icon_label("Semicolon").size() <= 5);
    using openrac::keys::move_label;
    CHECK(move_label({"KeyW", "KeyA", "KeyS", "KeyD"}) == "WASD");
    CHECK(move_label({"ArrowUp", "ArrowLeft", "ArrowDown", "ArrowRight"}) == "MOVE");
}

void find() {
    std::vector<std::uint8_t> m = game_memory(3, 64);
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    CHECK(f.count == 3);
    CHECK(f.font[0] == m.data() + kBank);
    CHECK(f.palette == m.data() + kBank + 3 * kp::kFontBytes);

    // The title's menus: no third font.
    std::vector<std::uint8_t> two = game_memory(2, 64);
    CHECK(kp::find_fonts(two.data(), kRam, kBankWord, kTable, f));
    CHECK(f.count == 2);

    // No bank, a bank past memory, a palette that differs between fonts: none.
    std::vector<std::uint8_t> none(kRam);
    CHECK(!kp::find_fonts(none.data(), kRam, kBankWord, kTable, f));
    std::vector<std::uint8_t> far = game_memory(3, 64);
    const std::uint32_t past = kRam;
    std::memcpy(far.data() + kBankWord, &past, 4);
    CHECK(!kp::find_fonts(far.data(), kRam, kBankWord, kTable, f));
    std::vector<std::uint8_t> odd = game_memory(3, 64);
    put16(odd, kTable + 32 + 0xA, 1);
    CHECK(!kp::find_fonts(odd.data(), kRam, kBankWord, kTable, f));
    CHECK(!kp::find_fonts(nullptr, kRam, kBankWord, kTable, f));
}

void paint_and_restore() {
    std::vector<std::uint8_t> m = game_memory(3, 64);
    const std::vector<std::uint8_t> before = m;
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));

    kp::Labels l{};
    l[kp::Cross] = "Space";
    l[kp::Circle] = "MouseLeft";
    kp::Painter p;
    p.update(f, l);
    CHECK(p.painted());
    CHECK(m != before);
    // Only the Cross and Circle cells changed.
    for (int font = 0; font < 3; ++font) {
        for (std::uint32_t v = 0; v < 128; ++v) {
            for (std::uint32_t u = 0; u < 256; ++u) {
                const std::uint32_t a = kBank + font * kp::kFontBytes + v * 256 + u;
                const bool cell = v >= 96 && v < 112 && (u / 24 == 0 || u / 24 == 3) && u < 192;
                if (!cell) {
                    CHECK(m[a] == before[a]);
                }
            }
        }
    }
    // The palette is the game's.
    const std::uint32_t clut = kBank + 3 * kp::kFontBytes;
    CHECK(std::memcmp(&m[clut], &before[clut], kp::kPaletteBytes) == 0);
    const std::uint32_t after = kp::hash(f);

    // The same labels again: nothing more to do.
    p.update(f, l);
    CHECK(kp::hash(f) == after);

    // New labels: the game's cells first, then the new ones.
    l[kp::Circle] = "";
    l[kp::Square] = "F";
    p.update(f, l);
    for (std::uint32_t y = 0; y < 16; ++y) {
        const std::uint32_t a = kBank + (96 + y) * 256 + 72;
        CHECK(std::memcmp(&m[a], &before[a], 24) == 0);
    }

    // The gamepad again: every byte as the game had it.
    p.update(f, kp::Labels{});
    CHECK(!p.painted());
    CHECK(m == before);
}

void game_reloads() {
    std::vector<std::uint8_t> m = game_memory(3, 64);
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    kp::Labels l{};
    l[kp::Cross] = "W";
    kp::Painter p;
    p.update(f, l);

    // A level loads the fonts anew: they are the game's, painted again, and put back as loaded.
    const std::vector<std::uint8_t> reloaded = game_memory(3, 48);
    m = reloaded;
    p.update(f, l);
    CHECK(p.painted());
    CHECK(m != reloaded);
    p.update(f, kp::Labels{});
    CHECK(m == reloaded);

    // Forgotten (the level is gone): nothing is put back.
    p.update(f, l);
    p.forget();
    const std::vector<std::uint8_t> painted = m;
    p.update(f, kp::Labels{});
    CHECK(m == painted);
}

void no_tabs() {
    // Fonts whose shoulder tabs are not the game's: nothing is painted.
    std::vector<std::uint8_t> m = game_memory(3, 64, false);
    const std::vector<std::uint8_t> before = m;
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    kp::Labels l{};
    l[kp::Cross] = "KeyW";
    CHECK(!kp::paint(f, l));
    kp::Painter p;
    p.update(f, l);
    CHECK(!p.painted());
    CHECK(m == before);
}

// The value of font 0's texel (u, v) of the icons' row.
std::uint8_t at(const std::vector<std::uint8_t>& m, std::uint32_t u, std::uint32_t v) {
    return m[kBank + (96 + v) * 256 + u];
}

void blank_keys_and_mice() {
    std::vector<std::uint8_t> m = game_memory(3, 64);
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    // A label with nothing to draw: the tab with its letters taken out, none of their white left.
    kp::Labels l{};
    l[kp::Cross] = ".";
    l[kp::Square] = "MouseLeft";
    l[kp::Triangle] = "MouseRight";
    l[kp::LeftStick] = "WASD";
    l[kp::RightStick] = "Mouse";
    CHECK(kp::paint(f, l));
    for (std::uint32_t v = 3; v < 14; ++v) {
        for (std::uint32_t u = 4; u < 23; ++u) {
            CHECK(at(m, u, v) != 250);
        }
    }
    // The mice: the left button lit on one, the right on the other.
    bool differ = false;
    for (std::uint32_t v = 0; v < 16; ++v) {
        for (std::uint32_t u = 0; u < 24; ++u) {
            differ |= at(m, 24 + u, v) != at(m, 48 + u, v);
        }
    }
    CHECK(differ);
    // The sticks' cells are painted too, the glyph between them left alone.
    const std::vector<std::uint8_t> game = game_memory(3, 64);
    bool sticks = false;
    for (std::uint32_t v = 0; v < 16; ++v) {
        for (std::uint32_t u = 192; u < 208; ++u) {
            CHECK(at(m, u, v) == at(game, u, v));
        }
        for (std::uint32_t u = 208; u < 256; ++u) {
            sticks |= at(m, u, v) != at(game, u, v);
        }
    }
    CHECK(sticks);
}

void long_labels() {
    // A label longer than a key holds, or with letters the font has not: drawn as far as it fits.
    std::vector<std::uint8_t> m = game_memory(3, 64);
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    kp::Labels l{};
    l[kp::R2] = "ABCDEFGHIJ";
    l[kp::L1] = "[;]";
    CHECK(kp::paint(f, l));
}

}  // namespace

int main() {
    labels();
    find();
    paint_and_restore();
    game_reloads();
    no_tabs();
    blank_keys_and_mice();
    long_labels();
    std::puts("key_prompts: ok");
    return 0;
}

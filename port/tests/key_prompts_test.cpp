// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Key prompts (game/common/key_prompts.h) on fonts made here: found through the bank and table,
// painted as the game's shoulder tabs with their letters taken out, put back byte for byte, left
// alone when the game reloads them, and refused when the tabs are not ones it knows. Key labels (key_labels.h) too.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "game/common/key_labels.h"
#include "game/common/key_prompts.h"
#include "key_prompts_fonts.h"
#include "openrac/keys.h"
#include "renderer/textures/sharp_cells.h"

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
    CHECK(icon_label("Numpad4") == "NUM\n4");
    CHECK(icon_label("NumpadEnter") == "NUM\nENTER");
    CHECK(icon_label("Semicolon") == ";");
    CHECK(icon_label("F12") == "F12");
    CHECK(menu_label("PageDown") == "Page Down");
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
    l[kp::Cross] = "~";
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

// The white texels of font 0's cell `cell` (the letters' white of draw_tabs is 250).
int whites(const std::vector<std::uint8_t>& m, std::uint32_t u) {
    int n = 0;
    for (std::uint32_t v = 0; v < 16; ++v) {
        for (std::uint32_t x = 0; x < 24; ++x) {
            n += at(m, u + x, v) == 250 ? 1 : 0;
        }
    }
    return n;
}

void every_key() {
    // Every key and mouse button a bind can name shows on a key: its label fits (one or two lines
    // of at most 5), each of its letters and signs has a glyph, and no two keys look alike unless
    // they read the same (left and right Shift).
    std::set<std::vector<std::uint8_t>> pictures;
    std::set<std::string> texts;
    for (int i = 0; i < OPENRAC_BIND_NAME_COUNT; ++i) {
        const std::string name = openrac_bind_names[i].name;
        const std::string label = openrac::keys::icon_label(name);
        const std::size_t split = label.find('\n');
        CHECK(label.find('\n', split == std::string::npos ? 0 : split + 1) == std::string::npos);
        CHECK(
            split == std::string::npos ? label.size() <= 5
                                       : split <= 5 && label.size() - split - 1 <= 5
        );
        for (char c : label) {
            if (c == '\n') {
                continue;
            }
            std::vector<std::uint8_t> m = game_memory(3, 64);
            kp::Fonts f;
            CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
            kp::Labels one{};
            one[kp::Cross] = std::string(1, c);
            CHECK(kp::paint(f, one));
            if (whites(m, 0) == 0) {
                std::fprintf(stderr, "no glyph for '%c' (%s)\n", c, name.c_str());
            }
            CHECK(whites(m, 0) > 0);
        }
        std::vector<std::uint8_t> m = game_memory(3, 64);
        kp::Fonts f;
        CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
        kp::Labels l{};
        l[kp::Cross] = name;
        CHECK(kp::paint(f, l));
        CHECK(whites(m, 0) > 0);
        std::vector<std::uint8_t> cell;
        for (std::uint32_t v = 0; v < 16; ++v) {
            cell.insert(cell.end(), &m[kBank + (96 + v) * 256], &m[kBank + (96 + v) * 256 + 24]);
        }
        const bool same_text = !texts.insert(label).second;
        const bool same_picture = !pictures.insert(cell).second;
        if (same_picture && !same_text) {
            std::fprintf(stderr, "%s looks like another key\n", name.c_str());
        }
        CHECK(!same_picture || same_text);
    }
}

// Font 0 as the renderer converts it: each texel its palette entry's colour (CSM1 order).
std::vector<std::uint8_t> converted(const std::vector<std::uint8_t>& m) {
    std::vector<std::uint8_t> out(kp::kFontBytes * 4);
    const std::uint32_t clut = kBank + 3 * kp::kFontBytes;
    for (std::uint32_t i = 0; i < kp::kFontBytes; ++i) {
        const std::uint32_t t = m[kBank + i];
        const std::uint32_t e = (t & ~0x18u) | ((t & 8u) << 1) | ((t & 0x10u) >> 1);
        std::memcpy(&out[i * 4], &m[clut + e * 4], 4);
    }
    return out;
}

void sharp_pictures() {
    namespace sharp = openrac::renderer::sharp_cells;
    std::vector<std::uint8_t> m = game_memory(3, 64);
    const std::vector<std::uint8_t> game = converted(m);
    kp::Fonts f;
    CHECK(kp::find_fonts(m.data(), kRam, kBankWord, kTable, f));
    sharp::Picture big;
    // The game's own fonts: drawn as they are.
    CHECK(!sharp::picture(256, 128, game.data(), big));

    kp::Labels l{};
    l[kp::Cross] = "KeyW";
    l[kp::R2] = "MouseLeft";
    kp::Painter p;
    p.update(f, l);
    std::vector<std::uint8_t> painted = converted(m);
    CHECK(sharp::picture(256, 128, painted.data(), big));
    CHECK(big.width == 256 * 6 && big.height == 128 * 6);
    CHECK(big.pixels.size() == static_cast<std::size_t>(big.width * big.height * 4));
    // Outside the cells, the texture scaled up: a texel's middle keeps its colour and opacity.
    const auto pixel = [&](int x, int y) {
        return &big.pixels[static_cast<std::size_t>((y * big.width + x) * 4)];
    };
    for (int y = 0; y < 128; y += 9) {
        for (int x = 0; x < 256; x += 7) {
            if (y >= 96 && y < 112) {
                continue;
            }
            const std::uint8_t* want = &painted[static_cast<std::size_t>((y * 256 + x) * 4)];
            const std::uint8_t* got = pixel(x * 6 + 3, y * 6 + 3);
            for (int k = 0; k < 4; ++k) {
                CHECK(std::abs(int(got[k]) - int(want[k])) <= (int(want[k]) + 64) / 4);
            }
        }
    }
    // In the W's key: white strokes and the grey edge, sharp (pixels between the two), and the
    // opacity is the texture's.
    int white = 0, between = 0;
    for (int y = 96 * 6; y < 112 * 6; ++y) {
        for (int x = 0; x < 24 * 6; ++x) {
            const std::uint8_t* q = pixel(x, y);
            white += q[0] >= 249 ? 1 : 0;
            between += q[0] > 140 && q[0] < 240 ? 1 : 0;
        }
    }
    CHECK(white > 200);
    CHECK(between > 0);
    // A texel of a cell not as painted (another mod's, a game reload): not ours.
    std::vector<std::uint8_t> other = painted;
    other[(100 * 256 + 10) * 4] ^= 1;
    CHECK(!sharp::picture(256, 128, other.data(), big));
    // Another size: not ours.
    CHECK(!sharp::picture(128, 256, painted.data(), big));
    // Back on the pad: nothing is sharper.
    p.update(f, kp::Labels{});
    CHECK(!sharp::picture(256, 128, painted.data(), big));
    CHECK(!sharp::picture(256, 128, game.data(), big));
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
    every_key();
    sharp_pictures();
    std::puts("key_prompts: ok");
    return 0;
}

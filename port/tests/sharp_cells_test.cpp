// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's sharper cells (renderer/textures/sharp_cells.h) of several owners: the built-ins'
// and each mod's pages kept apart, the matching pages of one scale drawn together, the first on
// top, and one owner's change leaving the others'.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "check.h"
#include "renderer/textures/sharp_cells.h"

namespace sharp = openrac::renderer::sharp_cells;

namespace {

constexpr int kWidth = 8;
constexpr int kHeight = 4;

// The test's texture as the renderer converts it: texel (x, y) is (10x, 10y, 7), opaque.
std::vector<std::uint8_t> texture() {
    std::vector<std::uint8_t> t(kWidth * kHeight * 4);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            std::uint8_t* p = &t[static_cast<std::size_t>((y * kWidth + x) * 4)];
            p[0] = static_cast<std::uint8_t>(x * 10);
            p[1] = static_cast<std::uint8_t>(y * 10);
            p[2] = 7;
            p[3] = 255;
        }
    }
    return t;
}

// A page of the test's texture with one cell at (x, y), 2 x 2 texels, in the texture's colours,
// its sharper picture all `grey`.
sharp::Page page(int x, int y, int scale, std::uint8_t grey) {
    sharp::Cell c{x, y, 2, 2, {}, {}};
    const std::vector<std::uint8_t> t = texture();
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            const std::uint8_t* p = &t[static_cast<std::size_t>(((y + dy) * kWidth + x + dx) * 4)];
            c.rgb.insert(c.rgb.end(), p, p + 3);
        }
    }
    c.sharp.assign(static_cast<std::size_t>(2 * scale * 2 * scale * 3), grey);
    return {kWidth, kHeight, scale, {c}};
}

// The red of the picture's pixel at (x, y).
int red(const sharp::Picture& p, int x, int y) {
    return p.pixels[static_cast<std::size_t>((y * p.width + x) * 4)];
}

void owners_are_kept_apart() {
    const std::vector<std::uint8_t> t = texture();
    int first = 0, second = 0;
    const sharp::Owner mod = &first;
    const sharp::Owner other = &second;
    sharp::Picture big;
    CHECK(!sharp::picture(kWidth, kHeight, t.data(), big));

    // The built-ins' page alone: its cell sharp, the rest scaled up (texel 4's red is 40).
    sharp::set({page(0, 0, 2, 200)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(big.width == kWidth * 2 && big.height == kHeight * 2);
    CHECK(red(big, 0, 0) == 200 && red(big, 3, 3) == 200);
    CHECK(red(big, 9, 1) < 60);

    // A mod's page of the same texture: both owners' cells, and the built-ins' stay.
    sharp::set(mod, {page(4, 0, 2, 100)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(red(big, 0, 0) == 200 && red(big, 9, 1) == 100);

    // The built-ins clear theirs (the key prompts back on the pad): the mod's stays.
    sharp::set({});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(red(big, 0, 0) == 0 && red(big, 9, 1) == 100);

    // Another mod's page of another scale: the first matching page's scale is drawn, so it waits.
    sharp::set(other, {page(4, 2, 3, 50)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(big.width == kWidth * 2 && red(big, 9, 5) != 50);
    // The mod clears its own (its unload): the other's is drawn, at its scale.
    sharp::set(mod, {});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(big.width == kWidth * 3 && red(big, 12, 6) == 50 && red(big, 0, 0) == 0);
    sharp::set(other, {});
    CHECK(!sharp::picture(kWidth, kHeight, t.data(), big));
}

void the_first_is_drawn_on_top() {
    const std::vector<std::uint8_t> t = texture();
    int first = 0, second = 0;
    const sharp::Owner mod = &first;
    const sharp::Owner other = &second;
    sharp::Picture big;

    // Two mods on one cell: the one that set pages first is on top, and setting them again keeps
    // its place.
    sharp::set(other, {page(2, 2, 2, 60)});
    sharp::set(mod, {page(2, 2, 2, 90)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(red(big, 4, 4) == 60);
    sharp::set(other, {page(2, 2, 2, 70)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(red(big, 4, 4) == 70);

    // The built-ins come first whenever they set theirs.
    sharp::set({page(2, 2, 2, 210)});
    CHECK(sharp::picture(kWidth, kHeight, t.data(), big));
    CHECK(red(big, 4, 4) == 210);

    // A texel of a cell not as given (another owner painted over it since): that page is not
    // drawn (nor the mods', whose cell has the texel too), the others are.
    std::vector<std::uint8_t> changed = t;
    changed[static_cast<std::size_t>((2 * kWidth + 3) * 4)] ^= 1;
    sharp::set({page(2, 0, 2, 210), page(3, 2, 2, 230)});
    CHECK(sharp::picture(kWidth, kHeight, changed.data(), big));
    CHECK(red(big, 4, 0) == 210 && red(big, 6, 4) != 230 && red(big, 4, 4) != 70);
    // None matches: drawn as before.
    sharp::set({page(3, 2, 2, 230)});
    CHECK(!sharp::picture(kWidth, kHeight, changed.data(), big));

    sharp::set({});
    sharp::set(mod, {});
    sharp::set(other, {});
    CHECK(!sharp::picture(kWidth, kHeight, t.data(), big));
}

// A page that knows the game's texels under its cell: the texture as the game made it, with the
// cell found; a page that does not know them is left out.
void the_game_texels_come_back() {
    const std::vector<std::uint8_t> t = texture();
    std::vector<std::uint8_t> game;
    std::vector<sharp::Found> cells;
    sharp::set({page(2, 0, 2, 90)});
    CHECK(!sharp::unpainted(kWidth, kHeight, t.data(), game, cells) && cells.empty());

    sharp::Page known = page(2, 0, 2, 90);
    known.cells[0].game.assign(2 * 2 * 4, 200);
    sharp::set({known, page(4, 2, 2, 50)});
    CHECK(sharp::unpainted(kWidth, kHeight, t.data(), game, cells));
    CHECK(game.size() == t.size() && cells.size() == 1);
    CHECK(
        cells.size() == 1 && cells[0].x == 2 && cells[0].y == 0 && cells[0].w == 2
        && cells[0].scale == 2 && cells[0].sharp.size() == 4 * 4 * 3
    );
    // The cell's texels are the game's, the rest the texture's.
    const auto texel = [&](int x, int y) {
        return game[static_cast<std::size_t>((y * kWidth + x) * 4)];
    };
    CHECK(texel(2, 0) == 200 && texel(3, 1) == 200 && texel(4, 0) == 40 && texel(1, 1) == 10);
    // Painted over since: not found.
    std::vector<std::uint8_t> changed = t;
    changed[static_cast<std::size_t>((0 * kWidth + 2) * 4)] ^= 1;
    CHECK(!sharp::unpainted(kWidth, kHeight, changed.data(), game, cells));
    sharp::set({});
}

}  // namespace

int main() {
    owners_are_kept_apart();
    the_first_is_drawn_on_top();
    the_game_texels_come_back();
    std::puts("sharp_cells: ok");
    return openrac::test::result();
}

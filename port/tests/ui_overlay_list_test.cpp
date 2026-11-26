// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game, on the CPU (renderer/ui/overlay_list.h): rectangles, text that keeps
// its shape on a wide picture, a mod's bad numbers left out, and a point of the level found on the
// 512 x 448 picture with the renderer's own reversed-depth camera.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "check.h"
#include "renderer/math.h"
#include "renderer/ui/canvas.h"
#include "renderer/ui/overlay_list.h"

namespace {

using namespace openrac::renderer;
using namespace openrac::renderer::ui;

bool near(float a, float b, float within = 0.01f) {
    return std::fabs(a - b) <= within;
}

void a_rectangle_is_two_triangles_of_its_colour() {
    OverlayList list;
    list.begin(4.0f / 3.0f);
    list.rect(10, 20, 30, 40, 0x11223344u);
    CHECK(list.vertices().size() == 6);
    const OverlayVertex& first = list.vertices()[0];
    CHECK(first.x == 10.0f && first.y == 20.0f);
    CHECK(
        first.rgba[0] == 0x11 && first.rgba[1] == 0x22 && first.rgba[2] == 0x33
        && first.rgba[3] == 0x44
    );
    float right = 0, bottom = 0;
    for (const OverlayVertex& v : list.vertices()) {
        right = std::max(right, v.x);
        bottom = std::max(bottom, v.y);
    }
    CHECK(right == 40.0f && bottom == 60.0f);
}

void bad_numbers_are_left_out() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    OverlayList list;
    list.begin(1.0f);
    list.rect(nan, 0, 1, 1, 0xFFFFFFFFu);
    list.rect(0, 0, -1, 1, 0xFFFFFFFFu);
    list.text(0, nan, 8, 0xFFFFFFFFu, "A");
    list.text(0, 0, 0, 0xFFFFFFFFu, "A");
    CHECK(list.vertices().empty());
    list.begin(nan);  // an aspect that is no number: the picture's own
    list.text(0, 0, 8, 0xFFFFFFFFu, "A");
    CHECK(!list.vertices().empty());
}

void the_list_has_an_end() {
    OverlayList list;
    list.begin(1.0f);
    for (std::size_t i = 0; i < kOverlayVerticesMax; ++i) {
        list.rect(0, 0, 1, 1, 0xFFFFFFFFu);
    }
    CHECK(list.vertices().size() <= kOverlayVerticesMax);
}

void a_run_of_rectangles_and_text_is_one_batch() {
    OverlayList list;
    list.begin(1.0f);
    list.rect(0, 0, 4, 4, 0xFF0000FFu);
    list.text(10, 10, 8, 0x00FF00FFu, "HI");
    list.rect(50, 50, 4, 4, 0x0000FFFFu);
    CHECK(list.batches().size() == 1);
    CHECK(list.batches()[0].first == 0 && list.batches()[0].blend == OverlayBlend::Alpha);
    CHECK(list.batches()[0].count == list.vertices().size());
}

void rectangles_and_text_are_blended_as_set() {
    OverlayList list;
    list.begin(1.0f);
    list.rect(0, 0, 4, 4, 0xFF0000FFu);  // alpha until told otherwise
    CHECK(list.set_blend(2));
    list.rect(10, 0, 4, 4, 0xFF0000FFu);
    list.text(20, 0, 8, 0xFF0000FFu, "A");  // text is rectangles: blended as set
    CHECK(list.set_blend(1));
    list.rect(60, 0, 4, 4, 0xFF0000FFu);
    const auto batches = list.batches();
    CHECK(batches.size() == 3);
    CHECK(batches[0].blend == OverlayBlend::Alpha);
    CHECK(batches[1].blend == OverlayBlend::Subtract);
    CHECK(batches[2].blend == OverlayBlend::Add);
    CHECK(overlay_blend_known(0) && overlay_blend_known(1) && overlay_blend_known(2));
    CHECK(!overlay_blend_known(3) && !overlay_blend_known(0xFFFFFFFFu));
}

void a_blend_this_build_lacks_is_refused_and_keeps_the_last() {
    OverlayList list;
    list.begin(1.0f);
    CHECK(list.set_blend(2));
    CHECK(!list.set_blend(3));
    CHECK(!list.set_blend(0xFFFFFFFFu));
    list.rect(0, 0, 4, 4, 0xFFFFFFFFu);
    CHECK(list.batches().size() == 1 && list.batches()[0].blend == OverlayBlend::Subtract);
    // A new frame starts as alpha again, whatever the last one set.
    list.begin(1.0f);
    list.rect(0, 0, 4, 4, 0xFFFFFFFFu);
    CHECK(list.batches().size() == 1 && list.batches()[0].blend == OverlayBlend::Alpha);
}

// The bounds of what was drawn.
struct Box {
    float left = 1e9f, top = 1e9f, right = -1e9f, bottom = -1e9f;
};

Box box_of(const OverlayList& list) {
    Box b;
    for (const OverlayVertex& v : list.vertices()) {
        b.left = std::min(b.left, v.x);
        b.top = std::min(b.top, v.y);
        b.right = std::max(b.right, v.x);
        b.bottom = std::max(b.bottom, v.y);
    }
    return b;
}

void text_lights_the_font_and_keeps_its_shape() {
    // 'H' fills its 5 x 7 pixels: at height 8 a pixel is 1 down.
    OverlayList square;
    square.begin(kOverlayWidth / kOverlayHeight);  // a picture's unit is square as shown
    square.text(100, 50, 8, 0xFFFFFFFFu, "H");
    const Box h = box_of(square);
    CHECK(near(h.left, 100) && near(h.top, 50));
    CHECK(near(h.right, 105) && near(h.bottom, 57));
    CHECK(near(square.text_width(8, "H"), 6));

    // On a wider picture the same text is narrower in the picture's units and as wide as shown.
    OverlayList wide;
    wide.begin(2.0f * kOverlayWidth / kOverlayHeight);
    wide.text(100, 50, 8, 0xFFFFFFFFu, "H");
    const Box w = box_of(wide);
    CHECK(near(w.right - w.left, 2.5f));
    CHECK(near(w.bottom - w.top, 7));
    CHECK(near(wide.text_width(8, "HH"), 6));
}

void a_character_the_font_lacks_is_one_blank_cell() {
    OverlayList list;
    list.begin(kOverlayWidth / kOverlayHeight);
    // é is two bytes, one character: the H after it is one cell along.
    CHECK(near(list.text_width(8, "\xC3\xA9H"), 12));
    list.text(0, 0, 8, 0xFFFFFFFFu, "\xC3\xA9H");
    const Box b = box_of(list);
    CHECK(near(b.left, 6));
}

// A camera at the origin looking along +X, Z up, as the game's axes have it.
Mat4 view_along_x() {
    // World X goes to view -Z, world Y to view -X, world Z to view Y.
    return {0, 0, -1, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1};
}

void a_point_is_found_on_the_picture() {
    const Mat4 view = view_along_x();
    const Mat4 projection = perspective_reversed(1.2f, 4.0f / 3.0f, 0.1f, 1000.0f);
    float x = -1, y = -1;
    // Straight ahead: the middle.
    CHECK(overlay_project(view, projection, {10, 0, 0}, x, y));
    CHECK(near(x, 256) && near(y, 224));
    // Up in the level is up on the picture, left (+Y) is left.
    CHECK(overlay_project(view, projection, {10, 0, 2}, x, y));
    CHECK(near(x, 256) && y < 224);
    CHECK(overlay_project(view, projection, {10, 2, 0}, x, y));
    CHECK(x < 256 && near(y, 224));
    // Behind the camera, off to the side, or no number at all: not on the picture.
    CHECK(!overlay_project(view, projection, {-10, 0, 0}, x, y));
    CHECK(!overlay_project(view, projection, {10, 100, 0}, x, y));
    CHECK(
        !overlay_project(view, projection, {10, 0, std::numeric_limits<float>::quiet_NaN()}, x, y)
    );
}

void fade_falls_between_its_distances() {
    CHECK(overlay_fade(5, 10, 20) == 1.0f);
    CHECK(overlay_fade(10, 10, 20) == 1.0f);
    CHECK(near(overlay_fade(15, 10, 20), 0.5f));
    CHECK(overlay_fade(20, 10, 20) == 0.0f);
    CHECK(overlay_fade(30, 10, 20) == 0.0f);
    CHECK(overlay_fade(10, 10, 10) == 0.0f);  // no room to fall: no division by it
}

}  // namespace

int main() {
    a_rectangle_is_two_triangles_of_its_colour();
    bad_numbers_are_left_out();
    the_list_has_an_end();
    a_run_of_rectangles_and_text_is_one_batch();
    rectangles_and_text_are_blended_as_set();
    a_blend_this_build_lacks_is_refused_and_keeps_the_last();
    text_lights_the_font_and_keeps_its_shape();
    a_character_the_font_lacks_is_one_blank_cell();
    a_point_is_found_on_the_picture();
    fade_falls_between_its_distances();
    return openrac::test::result();
}

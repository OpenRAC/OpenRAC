// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What mods draw over the game, on the CPU (renderer/ui/overlay_list.h): rectangles, text that keeps
// its shape on a wide picture, a mod's bad numbers left out, a point of the level found on the
// 512 x 448 picture with the renderer's own reversed-depth camera, and the mods' pictures (1.41):
// placed, turned and kept in shape, in runs of their own.

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

// The corner of the image with texture place (u, v).
const OverlayVertex* corner(const OverlayList& list, std::size_t first, float u, float v) {
    for (std::size_t i = first; i < first + 6; ++i) {
        if (list.vertices()[i].u == u && list.vertices()[i].v == v) {
            return &list.vertices()[i];
        }
    }
    return nullptr;
}

void a_picture_is_placed_with_its_texture_corners() {
    OverlayList list;
    // On a 4:3 frame a unit across is 448/512 * 4/3 = 7/6 of one down.
    list.begin(4.0f / 3.0f);
    list.image(5, 256, 224, 70, 70, 0, 0x80FF40C0u);
    CHECK(list.vertices().size() == 6);
    CHECK(list.batches().size() == 1 && list.batches()[0].image == 5);
    const OverlayVertex* top_left = corner(list, 0, 0, 0);
    const OverlayVertex* bottom_right = corner(list, 0, 1, 1);
    CHECK(top_left != nullptr && bottom_right != nullptr);
    // 70 down is 60 across: square as shown.
    CHECK(near(top_left->x, 226) && near(top_left->y, 189));
    CHECK(near(bottom_right->x, 286) && near(bottom_right->y, 259));
    CHECK(top_left->rgba[0] == 0x80 && top_left->rgba[3] == 0xC0);
}

void a_picture_turns_counter_clockwise_as_shown() {
    OverlayList list;
    list.begin(16.0f / 9.0f);
    const float across = 448.0f / 512.0f * 16.0f / 9.0f;
    // A quarter turn: the picture's right edge (u = 1) goes to the top.
    list.image(1, 100, 100, 40, 20, 3.14159265f / 2.0f, 0xFFFFFFFFu);
    const OverlayVertex* top_right = corner(list, 0, 1, 0);
    const OverlayVertex* bottom_right = corner(list, 0, 1, 1);
    CHECK(top_right != nullptr && bottom_right != nullptr);
    // (20, -10) from the centre, as shown, turns to (-10, -20): 10 / across to the left, 20 up.
    CHECK(near(top_right->x, 100 - 10 / across) && near(top_right->y, 80));
    // (20, 10) turns to (10, -20).
    CHECK(near(bottom_right->x, 100 + 10 / across) && near(bottom_right->y, 80));
    // Its length along the turn stays 40 down: its far corners are 40 apart as shown.
    const OverlayVertex* top_left = corner(list, 0, 0, 0);
    CHECK(top_left != nullptr && near(top_left->y - top_right->y, 40));
}

void a_quad_takes_its_corners_and_places_as_given() {
    OverlayList list;
    list.begin(16.0f / 9.0f);
    list.image_quad(
        3, {0, 0, 100, 0, 0, 50, 100, 50}, {0.25f, 0, 0.75f, 0, 0.25f, 1, 0.75f, 1}, 0xFFFFFFFFu
    );
    CHECK(list.vertices().size() == 6 && list.batches()[0].image == 3);
    const OverlayVertex* v = corner(list, 0, 0.75f, 1);
    CHECK(v != nullptr && v->x == 100.0f && v->y == 50.0f);
}

void pictures_break_runs_and_bad_ones_are_left_out() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    OverlayList list;
    list.begin(1.0f);
    list.rect(0, 0, 1, 1, 0xFFFFFFFFu);
    list.image(2, 5, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.image(2, 9, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.image(4, 5, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.rect(1, 1, 1, 1, 0xFFFFFFFFu);
    // A rectangle, two of picture 2 in one run, picture 4, a rectangle: four runs, in order.
    CHECK(list.batches().size() == 4);
    CHECK(list.batches()[0].image == 0 && list.batches()[1].image == 2);
    CHECK(list.batches()[1].count == 12 && list.batches()[2].image == 4);
    CHECK(list.batches()[3].image == 0);
    const std::size_t before = list.vertices().size();
    list.image(0, 5, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.image(-1, 5, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.image(2, nan, 5, 4, 4, 0, 0xFFFFFFFFu);
    list.image(2, 5, 5, 0, 4, 0, 0xFFFFFFFFu);
    list.image(2, 5, 5, 4, 4, nan, 0xFFFFFFFFu);
    list.image_quad(2, {0, 0, 1, 0, 0, nan, 1, 1}, {0, 0, 1, 0, 0, 1, 1, 1}, 0xFFFFFFFFu);
    CHECK(list.vertices().size() == before);
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
    a_picture_is_placed_with_its_texture_corners();
    a_picture_turns_counter_clockwise_as_shown();
    a_quad_takes_its_corners_and_places_as_given();
    pictures_break_runs_and_bad_ones_are_left_out();
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native 2D core must preserve item data and placement without a renderer or game process.

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "openrac/frame.h"
#include "openrac/ui.h"
#include "tests/check.h"

namespace {

// The recorder stand-in keeps exactly what the core handed across its C boundary.
std::vector<openrac_game_ui_item> g_items;
int g_record_result = 1;
bool g_full_scissor = false;
std::array<float, 4> g_scissor{};

// Documented fallback screen dimensions do not depend on an installed game adapter.
constexpr float kDefaultWidth = 512.0f;
constexpr float kDefaultHeight = 448.0f;

// The synthetic adapter can leave its dimensions invalid while a program is loading.
void invalid_screen(float* width, float* height) {
    *width = 0.0f;
    *height = std::numeric_limits<float>::quiet_NaN();
}

// A large adapter screen proves the item carries its own units, rather than a scaled rectangle.
void wide_screen(float* width, float* height) {
    *width = 1024.0f;
    *height = 768.0f;
}

// A flat synthetic quad avoids all image decoding and GPU state in these core tests.
openrac_ui_quad flat_quad() {
    openrac_ui_quad quad{};
    quad.flags = OPENRAC_UI_FLAT;
    return quad;
}

// The core's documented fallback is usable before any game registers an adapter.
void test_default_screen() {
    openrac_ui_set_adapter(nullptr);
    float width = 0.0f;
    float height = 0.0f;
    openrac_ui_screen(&width, &height);
    CHECK(width == kDefaultWidth);
    CHECK(height == kDefaultHeight);
}

// Invalid adapter dimensions cannot reach scissor scaling or become non-finite item data.
void test_invalid_adapter_screen() {
    const openrac_ui_adapter adapter{invalid_screen, nullptr, nullptr};
    openrac_ui_set_adapter(&adapter);
    float width = 1.0f;
    float height = 1.0f;
    openrac_ui_screen(&width, &height);
    CHECK(width == kDefaultWidth);
    CHECK(height == kDefaultHeight);
    const openrac_ui_quad quad = flat_quad();
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    CHECK(std::isfinite(g_items.back().screen_width));
    CHECK(std::isfinite(g_items.back().screen_height));
    openrac_ui_set_adapter(nullptr);
}

// Texture wrapping and per-corner colour are native item fields, not graphics-register words.
void test_quad_flags_and_screen() {
    const openrac_ui_adapter adapter{wide_screen, nullptr, nullptr};
    openrac_ui_set_adapter(&adapter);
    openrac_ui_quad quad{};
    quad.flags = OPENRAC_UI_SHADED | OPENRAC_UI_REPEAT | OPENRAC_UI_NEAREST;
    quad.image.pixels = 1;
    quad.image.width = 1;
    quad.image.height = 1;
    quad.rgba[0] = 0x80402010u;
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    const openrac_game_ui_item& item = g_items.back();
    CHECK(item.flags == quad.flags);
    CHECK(item.image.pixels == quad.image.pixels);
    CHECK(item.rgba[0] == quad.rgba[0]);
    CHECK(item.screen_width == 1024.0f && item.screen_height == 768.0f);
    openrac_ui_set_adapter(nullptr);
}

// An absent image makes a rectangle flat and leaves no stale texture descriptor in the item.
void test_rect_without_image_is_flat() {
    const float rect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    const std::uint32_t colours[4] = {1, 1, 1, 1};
    CHECK(openrac_ui_draw_rect(nullptr, rect, nullptr, colours, OPENRAC_UI_REPEAT) == 1);
    const openrac_game_ui_item& item = g_items.back();
    CHECK((item.flags & OPENRAC_UI_FLAT) != 0);
    CHECK((item.flags & OPENRAC_UI_REPEAT) != 0);
    CHECK(item.image.pixels == 0);
    CHECK(item.x[0] == rect[0] && item.x[1] == rect[2]);
}

// Nested placement belongs to the outer HUD element and ends only with its matching outer end.
void test_nested_placement() {
    const openrac_ui_quad quad = flat_quad();
    openrac_ui_place_begin(24);  // A synthetic HUD side shift in adapter pixels.
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    const std::uint32_t outer = g_items.back().place;
    CHECK(outer != 0);
    CHECK(g_items.back().side == 24.0f);
    openrac_ui_place_begin(48);  // The nested shift must not replace its outer placement.
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    CHECK(g_items.back().place == outer);
    CHECK(g_items.back().side == 24.0f);
    openrac_ui_place_end();
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    CHECK(g_items.back().place == outer);
    openrac_ui_place_end();
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    CHECK(g_items.back().place == 0 && g_items.back().side == 0.0f);
    openrac_ui_place_end();  // An unmatched end cannot create a negative nesting depth.
    openrac_ui_place_begin(24);
    CHECK(openrac_ui_draw_quad(&quad) == 1);
    CHECK(g_items.back().place != 0 && g_items.back().place != outer);
    openrac_ui_place_end();
}

// A renderer refusing a native item remains visible to the caller's transition policy.
void test_recorder_failure_is_returned() {
    const openrac_ui_quad quad = flat_quad();
    g_record_result = 0;
    CHECK(openrac_ui_draw_quad(&quad) == 0);
    g_record_result = 1;
}

// Removing a scissor forwards a full-target request rather than a stale rectangle.
void test_full_scissor() {
    openrac_ui_scissor(nullptr);
    CHECK(g_full_scissor);
}

}  // namespace

extern "C" {

// Capture the C contract without starting a renderer, window or game.
int openrac_frame_ui_item(const openrac_game_ui_item* item) {
    g_items.push_back(*item);
    return g_record_result;
}

// A null scissor is distinct from any zero-sized clipping rectangle.
void openrac_frame_scissor(const float* rect) {
    g_full_scissor = rect == nullptr;
    if (rect != nullptr) {
        for (std::size_t i = 0; i < g_scissor.size(); ++i) {
            g_scissor[i] = rect[i];
        }
    }
}

}  // extern "C"

// Every case operates only on synthetic item data and the production 2D core.
int main() {
    test_default_screen();
    test_invalid_adapter_screen();
    test_quad_flags_and_screen();
    test_rect_without_image_is_flat();
    test_nested_placement();
    test_recorder_failure_is_returned();
    test_full_scissor();
    return openrac::test::result();
}

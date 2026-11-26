// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's core (openrac/ui.h): records the quads the games' 2D routines hand it through the
// frame recorder, in the game's screen pixels, with images named by the game's own memory.

#include "openrac/ui.h"

#include <cstring>

#include "openrac/frame.h"

namespace {

const openrac_ui_adapter* g_adapter = nullptr;

// An axis-aligned quad over `rect` with the texels of `cell`, as a strip.
void place(openrac_game_ui_item& item, const float rect[4], const float cell[4]) {
    item.x[0] = item.x[2] = rect[0];
    item.x[1] = item.x[3] = rect[2];
    item.y[0] = item.y[1] = rect[1];
    item.y[2] = item.y[3] = rect[3];
    item.u[0] = item.u[2] = cell[0];
    item.u[1] = item.u[3] = cell[2];
    item.v[0] = item.v[1] = cell[1];
    item.v[2] = item.v[3] = cell[3];
}

}  // namespace

extern "C" {

void openrac_ui_set_adapter(const openrac_ui_adapter* adapter) {
    g_adapter = adapter;
}

void openrac_ui_screen(float* width, float* height) {
    *width = 512.0f;
    *height = 448.0f;
    if (g_adapter != nullptr && g_adapter->screen != nullptr) {
        g_adapter->screen(width, height);
    }
}

int openrac_ui_draw_glyph(const openrac_ui_glyph* glyph) {
    openrac_game_ui_item item;
    std::memset(&item, 0, sizeof(item));
    place(item, glyph->rect, glyph->cell);
    std::memcpy(item.rgba, glyph->rgba, sizeof(item.rgba));
    item.flags = OPENRAC_UI_GLYPH;
    item.image = glyph->page;
    openrac_ui_screen(&item.screen_width, &item.screen_height);
    return openrac_frame_ui_item(&item);
}

int openrac_ui_draw_quad(const openrac_ui_quad* quad) {
    openrac_game_ui_item item;
    std::memset(&item, 0, sizeof(item));
    std::memcpy(item.x, quad->x, sizeof(item.x));
    std::memcpy(item.y, quad->y, sizeof(item.y));
    std::memcpy(item.u, quad->u, sizeof(item.u));
    std::memcpy(item.v, quad->v, sizeof(item.v));
    std::memcpy(item.rgba, quad->rgba, sizeof(item.rgba));
    item.flags = quad->flags & (OPENRAC_UI_FLAT | OPENRAC_UI_SHADED | OPENRAC_UI_NEAREST);
    if ((item.flags & OPENRAC_UI_FLAT) == 0) {
        item.image = quad->image;
    }
    openrac_ui_screen(&item.screen_width, &item.screen_height);
    return openrac_frame_ui_item(&item);
}

int openrac_ui_draw_rect(
    const openrac_game_ui_image* image,
    const float rect[4],
    const float cell[4],
    const uint32_t rgba[4],
    uint32_t flags
) {
    openrac_game_ui_item item;
    std::memset(&item, 0, sizeof(item));
    const float none[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    place(item, rect, cell != nullptr ? cell : none);
    std::memcpy(item.rgba, rgba, sizeof(item.rgba));
    item.flags = flags & (OPENRAC_UI_FLAT | OPENRAC_UI_SHADED | OPENRAC_UI_NEAREST);
    if (image != nullptr && (item.flags & OPENRAC_UI_FLAT) == 0) {
        item.image = *image;
    } else {
        item.flags |= OPENRAC_UI_FLAT;
    }
    openrac_ui_screen(&item.screen_width, &item.screen_height);
    return openrac_frame_ui_item(&item);
}

void openrac_ui_scissor(const float* rect) {
    if (rect == nullptr) {
        openrac_frame_scissor(nullptr);
        return;
    }
    // In the game's own screen (Up Your Arsenal's 2D after the picture's copy is 512 x 512): the
    // recorder takes its size and the rectangle as they are.
    float width = 512.0f;
    float height = 448.0f;
    openrac_ui_screen(&width, &height);
    openrac_frame_screen(width, height);
    openrac_frame_scissor(rect);
}

int openrac_ui_image(int table, int index, openrac_game_ui_image* out) {
    if (g_adapter == nullptr || g_adapter->image == nullptr) {
        return 0;
    }
    return g_adapter->image(table, index, out);
}

}  // extern "C"

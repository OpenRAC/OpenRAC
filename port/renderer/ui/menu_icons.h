// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' icons (touch_icon) as OpenRAC's menu draws them beside a row of a mod's tab
// (developer_menu_look, mod interface 1.28). The touch pad's texture (touch_icons.h) keeps both
// inks in one texel, for its own shader; ImGui only multiplies a texel by a colour. So here each
// icon is two white cells of one texture, its white ink's and its accent's coverage in their
// alpha, which the menu draws one over the other, tinted the label's colour and the accent's.
// Drawn with touch_icons::raster, kCell square, with mipmaps.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openrac::renderer::ui::menu_icons {

// Where one ink of an icon is in texture(), in texture coordinates.
struct Cell {
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
};

struct Icon {
    Cell white;   // drawn in the label's colour
    Cell accent;  // what is #ff8000 in the SVG, drawn in the accent's
};

// The icons the menu can draw (name, SVG), all of them, the last of a name winning. No GL needed:
// the texture is made again when next asked for, and only when they changed.
void set(const std::vector<std::pair<std::string, std::string>>& icons);

// The icon's cells, none for an unknown icon or one nanosvg cannot draw. Makes the texture when
// the icons changed (the context is current).
std::optional<Icon> find(std::string_view name);

// The texture (0 before find() made it).
unsigned texture();

// Frees the texture (the context is current); the icons stay, to be drawn again.
void release();

}  // namespace openrac::renderer::ui::menu_icons

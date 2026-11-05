// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The on-screen pad's icons (platform/touch_pad.h, the Arc style): SVG pictures, drawn once into one
// texture when the pad first needs them. An icon is drawn in two inks: what is #ff8000 in the SVG
// in the game's accent, everything else white (its coverage only; the shader tints it). Each icon
// is made kCell pixels square from its viewBox, with mipmaps, so it stays sharp from the smallest
// button to the largest. The game's icons are game/common/touch/icons/*.svg; a mod adds its own.

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "renderer/gl.h"

namespace openrac::renderer::ui::touch_icons {

// An icon's square in the texture, in texture coordinates.
struct Cell {
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
};

inline constexpr int kCell = 128;

// Adds or replaces the icon `name` (no GL needed; the texture is made again when next asked for).
// False, and the icon left as it was, when `svg` is not an SVG nanosvg can draw.
bool add(std::string_view name, std::string_view svg);

bool has(std::string_view name);

// The icon's cell in texture(), none for an unknown icon. Makes the texture when icons changed
// (the context is current).
std::optional<Cell> cell(std::string_view name);

// The texture: red the white ink's coverage, green the accent's (0 before cell() made it).
gl::GLuint texture();

// Frees the texture (the context is current); the icons stay, to be drawn again.
void release();

// The icon drawn on the CPU, kCell square, as cell() puts it in the texture: two bytes a pixel,
// the white ink's coverage and the accent's. Empty for an unknown icon. For tests.
std::string coverage(std::string_view name);

}  // namespace openrac::renderer::ui::touch_icons

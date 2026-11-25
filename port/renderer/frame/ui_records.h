// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The 2D layer's records as the window draws them: the older UiQuad (the chip's 12.4 coordinates
// and a TEX0, drawn in the 2D state the game's packets leave, until P4) and UiItem, the game's data
// alone (openrac/ui_item.h: screen pixels, an image named by the game's addresses, no GS word),
// with the images they sample (UiImage). CPU only: the frame recorder (frame_record.h) and the
// renderer both use them.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace openrac::renderer {

struct Rgba8Image;

// A textured quad of the 2D layer the game drew through one of its 2D routines (rac1:
// DrawTexturedQuad, the HUD's sprite routines), drawn natively where its marker comes in the 2D
// path (GifInterpreter): four corners in strip order (0-1-2, 1-2-3) in the chip's 12.4 screen
// coordinates as the game computed them, with their texel coordinates (12.4 both ways), the depth,
// the colour (RGBAQ's low word) and the texture (TEX0).
struct UiQuad {
    std::uint32_t x[4] = {}, y[4] = {};
    std::uint32_t u[4] = {}, v[4] = {};
    std::uint32_t z = 0xFFFFF0;
    std::uint32_t rgba = 0;
    std::uint64_t tex0 = 0;
    std::uint32_t flags = 0;  // kUiGlyph: a glyph of text (drawn from a distance field); kUiSprite
    std::uint32_t corners[4] = {};  // with kUiCorners: each corner's colour, in place of rgba
    std::uint64_t clamp = 0;        // with kUiClamp: CLAMP_1, written before the quad
};

// UiQuad::flags: kUiSprite, an axis-aligned rectangle the game sends as a sprite (corners 0 and 3),
// whose texel mapping follows the sprite's rules (a panel drawn off-screen and sampled back).
inline constexpr std::uint32_t kUiSprite = 2;

inline constexpr std::uint32_t kUiGlyph = 1;

// UiQuad::flags: kUiShadow, a glyph drawn as text's drop shadow: the game draws the text again in
// black at (+1, +1) before it. It keeps the font's own alpha, soft as the console sampled it,
// rather than the sharp distance field the text itself is drawn from: a shadow sharpened the same
// way put its hard edge against the text's antialiased edge, and the text let it show through as
// black strokes over the letters.
inline constexpr std::uint32_t kUiShadow = 64;

// UiQuad::flags: kUiCorners, a colour for each corner (`corners`), each triangle in its last
// corner's colour as the GS draws one unshaded, or shaded between them with kUiShaded (the GS's
// Gouraud); kUiFlat, no texture (flat colour; TEX0 stays as the game left it); kUiClamp, the
// texture's wrap set first (`clamp`, CLAMP_1), as the game's packet does, so what follows sees it.
inline constexpr std::uint32_t kUiCorners = 4;
inline constexpr std::uint32_t kUiFlat = 8;
inline constexpr std::uint32_t kUiClamp = 16;
inline constexpr std::uint32_t kUiShaded = 32;

// An 8-bit image's indices and its 256 colours in index order, as the game keeps them: what a
// PCSX2-format texture pack names a texture by (renderer/textures/pcsx2_pack.h).
struct UiIndexed {
    std::vector<std::uint8_t> indices;
    std::vector<std::uint32_t> palette;
};

// An image the 2D layer draws natively (UiItem, pictures), decoded from the game's memory once per
// content: its hash names it, and the texture pool keeps one texture for each.
struct UiImage {
    std::uint64_t hash = 0;
    std::shared_ptr<const Rgba8Image> image;  // alpha in the game's units (0x80 opaque)
    // Its indices and palette, kept only while a PCSX2-format pack is loaded; null otherwise.
    std::shared_ptr<const UiIndexed> indexed;
};

// A quad of the 2D layer the game drew natively from its data (openrac_game_ui_item): no GS
// packet carries it. Corners in strip order in the game's screen pixels (its 2D screen,
// screen_width x screen_height), texel coordinates in the image's pixels, a colour each corner
// (0x80 = 1.0).
struct UiItem {
    float x[4] = {}, y[4] = {};
    float u[4] = {}, v[4] = {};
    std::uint32_t rgba[4] = {};
    float clip[4] = {};
    float screen_width = 512.0f;
    float screen_height = 448.0f;
    std::uint32_t flags = 0;  // kUiItem*
    std::uint32_t image = 0;  // index into the recording's images; unused when flat
};

inline constexpr std::uint32_t kUiItemGlyph = 1;
inline constexpr std::uint32_t kUiItemFlat = 2;
inline constexpr std::uint32_t kUiItemShaded = 4;
inline constexpr std::uint32_t kUiItemClip = 8;
inline constexpr std::uint32_t kUiItemNearest = 16;

}  // namespace openrac::renderer

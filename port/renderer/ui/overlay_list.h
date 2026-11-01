// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What a mod draws over the game (openrac_overlay, game/common/mod_overlay.cpp), gathered for a
// frame: filled rectangles and text as coloured triangles in the game's 512 x 448 picture, and a
// point of the level found on that picture. No GL here: it is plain memory, so it is tested
// without a window; OverlayDraw puts the triangles over the frame.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "renderer/math.h"

namespace openrac::renderer::ui {

// The picture the positions are in, whatever the frame's size.
constexpr float kOverlayWidth = 512.0f;
constexpr float kOverlayHeight = 448.0f;

// What one frame may hold: past it a mod's drawing is left out, never the frame's time.
constexpr std::size_t kOverlayVerticesMax = 1u << 16;

// A corner of a triangle: where in the picture, and R G B A.
struct OverlayVertex {
    float x = 0;
    float y = 0;
    std::array<std::uint8_t, 4> rgba{};
};

// One frame's rectangles and text, in the order drawn.
class OverlayList {
public:
    // Starts a frame shown `aspect` wide to 1 tall; text keeps its shape on any aspect.
    void begin(float aspect);

    // A rectangle from (x, y), its top left, of `colour` (0xRRGGBBAA).
    void rect(float x, float y, float w, float h, std::uint32_t colour);

    // One line of UTF-8 from (x, y), its top left, `height` tall, in the font of canvas.h: letters
    // as capitals, a character the font lacks as a blank cell.
    void text(float x, float y, float height, std::uint32_t colour, std::string_view line);

    // How wide `line` is at `height`, in the picture's units.
    float text_width(float height, std::string_view line) const;

    std::span<const OverlayVertex> vertices() const { return m_vertices; }

private:
    float m_aspect = 4.0f / 3.0f;
    std::vector<OverlayVertex> m_vertices;
};

// Where `world` is on the picture with this camera, into `x` and `y`: true when it is in front of
// the camera and on the picture.
bool overlay_project(const Mat4& view, const Mat4& projection, const Vec3& world, float& x, float& y);

// 1 up to `near_distance`, falling to 0 at `far_distance`.
float overlay_fade(float distance, float near_distance, float far_distance);

}  // namespace openrac::renderer::ui

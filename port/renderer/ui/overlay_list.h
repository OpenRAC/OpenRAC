// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What a mod draws over the game (openrac_overlay, game/common/mod_overlay.cpp), gathered for a
// frame: filled rectangles and text as triangles in the game's 512 x 448 picture, in the order
// they were asked, and a point of the level found on that picture. No GL here: it is plain memory,
// so it is tested without a window; OverlayDraw puts the triangles over the frame.

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

// A corner of a triangle: where in the picture and R G B A.
struct OverlayVertex {
    float x = 0;
    float y = 0;
    std::array<std::uint8_t, 4> rgba{};
};

// How rectangles and text blend over the frame. A value this build does not know is refused, never
// drawn some other way.
enum class OverlayBlend : std::uint32_t {
    Alpha = 0,     // the colour by its alpha over the frame's (straight, not premultiplied)
    Add = 1,       // the frame's colour plus the colour times its alpha
    Subtract = 2,  // the frame's colour minus the colour times its alpha, not below 0
};

// Whether `blend` is one this build draws.
bool overlay_blend_known(std::uint32_t blend);

// A run of vertices that share one blend mode: one call to the GPU. The list keeps the order of
// what was asked, merging neighbours that share a blend, so a frame with one blend stays one run.
struct OverlayBatch {
    std::uint32_t first = 0;  // the first vertex
    std::uint32_t count = 0;
    OverlayBlend blend = OverlayBlend::Alpha;
};

// One frame's rectangles and text, in the order drawn.
class OverlayList {
public:
    // Starts a frame shown `aspect` wide to 1 tall; text keeps its shape on any aspect. Rectangles
    // and text are blended as Alpha until set_blend says otherwise.
    void begin(float aspect);

    // How the rectangles and text that follow are laid over the frame. False, and nothing changed,
    // for a blend this build lacks.
    bool set_blend(std::uint32_t blend);

    // A rectangle from (x, y), its top left, of `colour` (0xRRGGBBAA).
    void rect(float x, float y, float w, float h, std::uint32_t colour);

    // One line of UTF-8 from (x, y), its top left, `height` tall, in the font of canvas.h: letters
    // as capitals, a character the font lacks as a blank cell.
    void text(float x, float y, float height, std::uint32_t colour, std::string_view line);

    // How wide `line` is at `height`, in the picture's units.
    float text_width(float height, std::string_view line) const;

    std::span<const OverlayVertex> vertices() const { return m_vertices; }

    std::span<const OverlayBatch> batches() const { return m_batches; }

private:
    // Appends the quad's two triangles to the batch of `blend`.
    void quad(OverlayBlend blend, const std::array<OverlayVertex, 4>& corners);

    float m_aspect = 4.0f / 3.0f;
    OverlayBlend m_blend = OverlayBlend::Alpha;
    std::vector<OverlayVertex> m_vertices;
    std::vector<OverlayBatch> m_batches;
};

// Where `world` is on the picture with this camera, into `x` and `y`: true when it is in front of
// the camera and on the picture.
bool overlay_project(
    const Mat4& view, const Mat4& projection, const Vec3& world, float& x, float& y
);

// 1 up to `near_distance`, falling to 0 at `far_distance`.
float overlay_fade(float distance, float near_distance, float far_distance);

}  // namespace openrac::renderer::ui

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/overlay_list.h"

#include <algorithm>
#include <cmath>

#include "renderer/ui/canvas.h"

namespace openrac::renderer::ui {

namespace {

// A line longer than this is cut: more than any name or sentence fits on the picture.
constexpr std::size_t kLineMax = 128;

// A clip w this close to the camera's plane is behind it, for the division.
constexpr float kNearestW = 1e-4f;

constexpr std::size_t kVerticesPerQuad = 6;

std::array<std::uint8_t, 4> bytes_of(std::uint32_t colour) {
    return {
        static_cast<std::uint8_t>(colour >> 24),
        static_cast<std::uint8_t>(colour >> 16),
        static_cast<std::uint8_t>(colour >> 8),
        static_cast<std::uint8_t>(colour)
    };
}

// Whether byte `c` starts a character: UTF-8's other bytes are 10xxxxxx.
bool starts_character(char c) {
    return (static_cast<unsigned char>(c) & 0xC0u) != 0x80u;
}

std::size_t characters(std::string_view line) {
    const auto count = std::count_if(line.begin(), line.end(), starts_character);
    return std::min(static_cast<std::size_t>(count), kLineMax);
}

// A cell's width on the picture at `height`, square as shown: a picture's unit across is
// `aspect` * 448 / 512 of one down.
float cell_width(float height, float aspect) {
    const float pixel = height / static_cast<float>(kGlyphCellHeight);
    const float across = kOverlayHeight / kOverlayWidth * aspect;
    return pixel / across * static_cast<float>(kGlyphCellWidth);
}

bool finite(float v) {
    return std::isfinite(v);
}

}  // namespace

void OverlayList::begin(float aspect) {
    m_aspect = finite(aspect) && aspect > 0.0f ? aspect : 4.0f / 3.0f;
    m_vertices.clear();
}

void OverlayList::rect(float x, float y, float w, float h, std::uint32_t colour) {
    if (!finite(x) || !finite(y) || !finite(w) || !finite(h) || w <= 0.0f || h <= 0.0f
        || m_vertices.size() + kVerticesPerQuad > kOverlayVerticesMax) {
        return;
    }
    const std::array<std::uint8_t, 4> c = bytes_of(colour);
    const OverlayVertex a{x, y, c};
    const OverlayVertex b{x + w, y, c};
    const OverlayVertex d{x, y + h, c};
    const OverlayVertex e{x + w, y + h, c};
    m_vertices.insert(m_vertices.end(), {a, b, d, b, e, d});
}

void OverlayList::text(float x, float y, float height, std::uint32_t colour, std::string_view line) {
    if (!finite(x) || !finite(y) || !finite(height) || height <= 0.0f) {
        return;
    }
    const float cell = cell_width(height, m_aspect);
    const float pixel_w = cell / static_cast<float>(kGlyphCellWidth);
    const float pixel_h = height / static_cast<float>(kGlyphCellHeight);
    float at = x;
    std::size_t shown = 0;
    for (std::size_t i = 0; i < line.size() && shown < kLineMax; ++i) {
        if (!starts_character(line[i])) {
            continue;
        }
        const auto byte = static_cast<unsigned char>(line[i]);
        const char* rows = byte < 0x80u ? glyph_rows(line[i]) : nullptr;
        // The lit pixels of a row, run by run.
        for (int r = 0; rows != nullptr && r < kGlyphRows; ++r) {
            for (int col = 0; col < kGlyphColumns; ++col) {
                if (rows[r * kGlyphColumns + col] != '#') {
                    continue;
                }
                const int first = col;
                while (col + 1 < kGlyphColumns && rows[r * kGlyphColumns + col + 1] == '#') {
                    ++col;
                }
                rect(at + static_cast<float>(first) * pixel_w,
                     y + static_cast<float>(r) * pixel_h,
                     static_cast<float>(col - first + 1) * pixel_w,
                     pixel_h,
                     colour);
            }
        }
        at += cell;
        ++shown;
    }
}

float OverlayList::text_width(float height, std::string_view line) const {
    if (!finite(height) || height <= 0.0f) {
        return 0.0f;
    }
    return static_cast<float>(characters(line)) * cell_width(height, m_aspect);
}

bool overlay_project(const Mat4& view, const Mat4& projection, const Vec3& world, float& x, float& y) {
    if (!finite(world[0]) || !finite(world[1]) || !finite(world[2])) {
        return false;
    }
    const Mat4 m = multiply(projection, view);
    const float clip_x = m[0] * world[0] + m[4] * world[1] + m[8] * world[2] + m[12];
    const float clip_y = m[1] * world[0] + m[5] * world[1] + m[9] * world[2] + m[13];
    const float clip_w = m[3] * world[0] + m[7] * world[1] + m[11] * world[2] + m[15];
    if (!finite(clip_w) || clip_w < kNearestW) {
        return false;
    }
    const float ndc_x = clip_x / clip_w;
    const float ndc_y = clip_y / clip_w;
    if (!(std::fabs(ndc_x) <= 1.0f) || !(std::fabs(ndc_y) <= 1.0f)) {
        return false;
    }
    x = (ndc_x + 1.0f) * 0.5f * kOverlayWidth;
    y = (1.0f - ndc_y) * 0.5f * kOverlayHeight;
    return true;
}

float overlay_fade(float distance, float near_distance, float far_distance) {
    if (!finite(distance) || distance >= far_distance) {
        return 0.0f;
    }
    if (distance <= near_distance) {
        return 1.0f;
    }
    return (far_distance - distance) / (far_distance - near_distance);
}

}  // namespace openrac::renderer::ui

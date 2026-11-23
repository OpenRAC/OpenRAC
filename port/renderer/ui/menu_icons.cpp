// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/menu_icons.h"

#include <algorithm>
#include <cstdint>
#include <map>

#include "renderer/gl/gl.h"
#include "renderer/ui/touch_icons.h"

namespace openrac::renderer::ui::menu_icons {
namespace {

using namespace openrac::gl;

constexpr int kCell = touch_icons::kCell;
// Icons a row of the texture, each two cells wide (its white ink, its accent).
constexpr int kColumns = 4;
// Empty texels around each cell: a row draws an icon at about a fifth of kCell, sampled from mip
// level 2 or 3, whose texels are 4 or 8 of level 0's; with less room the next cell bled in.
constexpr int kGutter = 16;
constexpr int kStride = kCell + 2 * kGutter;

struct State {
    std::map<std::string, std::string, std::less<>> svgs;  // by name
    std::map<std::string, Icon, std::less<>> icons;        // where each is in the texture
    GLuint texture = 0;
    bool dirty = true;
};

State& state() {
    static State s;
    return s;
}

void make_texture() {
    State& s = state();
    s.icons.clear();
    const int count = std::max(1, static_cast<int>(s.svgs.size()));
    const int rows = (count + kColumns - 1) / kColumns;
    const int width = kColumns * 2 * kStride;
    const int height = rows * kStride;
    // White everywhere; each ink's coverage in its cell's alpha.
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4, 0xFF);
    for (std::size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = 0;
    }
    std::vector<std::uint8_t> raster;
    int index = 0;
    const float half_u = 0.5f / static_cast<float>(width);
    const float half_v = 0.5f / static_cast<float>(height);
    const auto cell = [&](int cx, int cy) {
        // Half a texel in; the gutter keeps the coarser levels from reading the next cell.
        return Cell{
            static_cast<float>(cx) / width + half_u,
            static_cast<float>(cy) / height + half_v,
            static_cast<float>(cx + kCell) / width - half_u,
            static_cast<float>(cy + kCell) / height - half_v
        };
    };
    for (const auto& [name, svg] : s.svgs) {
        if (!touch_icons::raster(svg, raster)) {
            continue;
        }
        const int cx = (index % kColumns) * 2 * kStride + kGutter;
        const int cy = (index / kColumns) * kStride + kGutter;
        for (int ink = 0; ink < 2; ++ink) {
            for (int y = 0; y < kCell; ++y) {
                for (int x = 0; x < kCell; ++x) {
                    const std::size_t from = (static_cast<std::size_t>(y) * kCell + x) * 2;
                    const std::size_t to =
                        (static_cast<std::size_t>(cy + y) * width + cx + ink * kStride + x) * 4;
                    pixels[to + 3] = raster[from + static_cast<std::size_t>(ink)];
                }
            }
        }
        s.icons[name] = Icon{cell(cx, cy), cell(cx + kStride, cy)};
        ++index;
    }
    if (s.texture == 0) {
        glGenTextures(1, &s.texture);
    }
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(GL_RGBA8),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR)
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
    glBindTexture(GL_TEXTURE_2D, 0);
    s.dirty = false;
}

}  // namespace

void set(const std::vector<std::pair<std::string, std::string>>& icons) {
    std::map<std::string, std::string, std::less<>> svgs;
    for (const auto& [name, svg] : icons) {
        svgs[name] = svg;
    }
    State& s = state();
    if (svgs != s.svgs) {
        s.svgs = std::move(svgs);
        s.dirty = true;
    }
}

std::optional<Icon> find(std::string_view name) {
    State& s = state();
    if (name.empty() || !s.svgs.contains(name)) {
        return std::nullopt;
    }
    if (s.dirty || s.texture == 0) {
        make_texture();
    }
    const auto it = s.icons.find(name);
    return it != s.icons.end() ? std::optional<Icon>(it->second) : std::nullopt;
}

unsigned texture() {
    return state().texture;
}

void release() {
    State& s = state();
    if (s.texture != 0) {
        glDeleteTextures(1, &s.texture);
        s.texture = 0;
    }
    s.dirty = true;
}

}  // namespace openrac::renderer::ui::menu_icons

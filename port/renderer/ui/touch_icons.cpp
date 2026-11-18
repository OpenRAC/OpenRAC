// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/touch_icons.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

#include "nanosvg.h"
#include "nanosvgrast.h"

namespace openrac::renderer::ui::touch_icons {
namespace {

using namespace openrac::gl;

// The accent's ink in an icon's SVG (#ff8000), as nanosvg packs a colour: red in the low byte.
constexpr unsigned kAccent = 0x0080FFu;
// Icons a row of the texture.
constexpr int kColumns = 8;

struct State {
    std::map<std::string, std::string, std::less<>> svgs;  // by name
    std::map<std::string, Cell, std::less<>> cells;        // where each is in the texture
    GLuint texture = 0;
    bool dirty = true;
};

State& state() {
    static State s;
    return s;
}

bool is_accent(const NSVGpaint& paint) {
    return paint.type == NSVG_PAINT_COLOR && (paint.color & 0xFFFFFFu) == kAccent;
}

// Draws `svg` kCell square into `out` (2 bytes a pixel: the white ink's coverage, the accent's).
bool draw(std::string_view svg, std::vector<std::uint8_t>& out) {
    std::string text(svg);  // nanosvg parses in place
    NSVGimage* image = nsvgParse(text.data(), "px", 96.0f);
    if (image == nullptr || image->width <= 0.0f || image->height <= 0.0f) {
        if (image != nullptr) {
            nsvgDelete(image);
        }
        return false;
    }
    NSVGrasterizer* raster = nsvgCreateRasterizer();
    const float scale = static_cast<float>(kCell) / std::max(image->width, image->height);
    out.assign(static_cast<std::size_t>(kCell) * kCell * 2, 0);
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(kCell) * kCell * 4);
    // Once with the white shapes only, once with the accent's: each pass's alpha is that ink.
    for (int pass = 0; pass < 2; ++pass) {
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            const bool accent = is_accent(shape->fill) || is_accent(shape->stroke);
            if (accent == (pass == 1)) {
                shape->flags |= NSVG_FLAGS_VISIBLE;
            } else {
                shape->flags &= static_cast<unsigned char>(~NSVG_FLAGS_VISIBLE);
            }
        }
        std::fill(rgba.begin(), rgba.end(), 0);
        nsvgRasterize(raster, image, 0.0f, 0.0f, scale, rgba.data(), kCell, kCell, kCell * 4);
        for (std::size_t i = 0; i < static_cast<std::size_t>(kCell) * kCell; ++i) {
            out[i * 2 + static_cast<std::size_t>(pass)] = rgba[i * 4 + 3];
        }
    }
    nsvgDeleteRasterizer(raster);
    nsvgDelete(image);
    return true;
}

void make_texture() {
    State& s = state();
    s.cells.clear();
    const int count = static_cast<int>(s.svgs.size());
    const int rows = std::max(1, (count + kColumns - 1) / kColumns);
    const int width = kColumns * kCell;
    const int height = rows * kCell;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4, 0);
    std::vector<std::uint8_t> icon;
    int index = 0;
    for (const auto& [name, svg] : s.svgs) {
        if (!draw(svg, icon)) {
            continue;
        }
        const int cx = (index % kColumns) * kCell;
        const int cy = (index / kColumns) * kCell;
        for (int y = 0; y < kCell; ++y) {
            for (int x = 0; x < kCell; ++x) {
                const std::size_t from = (static_cast<std::size_t>(y) * kCell + x) * 2;
                const std::size_t to = (static_cast<std::size_t>(cy + y) * width + cx + x) * 4;
                pixels[to] = icon[from];
                pixels[to + 1] = icon[from + 1];
                pixels[to + 3] = std::max(icon[from], icon[from + 1]);
            }
        }
        // Half a texel in, so that linear filtering never reads the next icon.
        const float half_u = 0.5f / static_cast<float>(width);
        const float half_v = 0.5f / static_cast<float>(height);
        s.cells[name] = Cell{
            static_cast<float>(cx) / width + half_u,
            static_cast<float>(cy) / height + half_v,
            static_cast<float>(cx + kCell) / width - half_u,
            static_cast<float>(cy + kCell) / height - half_v
        };
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

bool add(std::string_view name, std::string_view svg) {
    std::vector<std::uint8_t> check;
    if (name.empty() || !draw(svg, check)) {
        return false;
    }
    State& s = state();
    s.svgs[std::string(name)] = std::string(svg);
    s.dirty = true;
    return true;
}

bool has(std::string_view name) {
    return state().svgs.contains(name);
}

std::optional<Cell> cell(std::string_view name) {
    State& s = state();
    if (!s.svgs.contains(name)) {
        return std::nullopt;
    }
    if (s.dirty || s.texture == 0) {
        make_texture();
    }
    const auto it = s.cells.find(name);
    return it != s.cells.end() ? std::optional<Cell>(it->second) : std::nullopt;
}

GLuint texture() {
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

std::string coverage(std::string_view name) {
    const State& s = state();
    const auto it = s.svgs.find(name);
    std::vector<std::uint8_t> icon;
    if (it == s.svgs.end() || !draw(it->second, icon)) {
        return {};
    }
    return std::string(icon.begin(), icon.end());
}

}  // namespace openrac::renderer::ui::touch_icons

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/fsr3/palette_history.h"

#include <algorithm>
#include <cstring>

#include "renderer/gl.h"
#include "renderer/render_opt.h"

namespace openrac::enhanced::fsr3_detail {
using namespace gl;

PaletteHistory::~PaletteHistory() {
    release();
}

void PaletteHistory::release() {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
    m_texture = 0;
    m_count = m_rows = 0;
    m_data.clear();
}

void PaletteHistory::update(std::span<const renderer::Mat4> palette) {
    constexpr std::size_t kMatricesPerRow = 256;
    const std::size_t rows =
        std::max(std::size_t{1}, (palette.size() + kMatricesPerRow - 1) / kMatricesPerRow);
    const bool reuse = renderer::render_opt::enabled(renderer::render_opt::Opt::FsrPaletteReuse);
    const bool unchanged =
        reuse && m_texture && palette.size() == m_count && rows == m_rows
        && (palette.empty()
            || std::memcmp(palette.data(), m_data.data(), palette.size_bytes()) == 0);
    if (!m_texture) {
        glGenTextures(1, &m_texture);
    }
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    if (!unchanged) {
        if (reuse) {
            m_data.resize(rows * kMatricesPerRow);
            std::copy(palette.begin(), palette.end(), m_data.begin());
            std::fill(m_data.begin() + palette.size(), m_data.end(), renderer::Mat4{});
        } else {
            // Keep the original allocation and copy path for byte-for-byte comparisons.
            std::vector<renderer::Mat4> padded(rows * kMatricesPerRow);
            std::copy(palette.begin(), palette.end(), padded.begin());
            m_data.swap(padded);
        }
        // Keep logical height exact on shrink as well: out-of-range sampling must not
        // acquire stale rows, and the final partial row must have the original zero padding.
        if (!reuse || rows != m_rows) {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                0x8814,
                1024,
                static_cast<int>(rows),
                0,
                GL_RGBA,
                GL_FLOAT,
                m_data.data()
            );
        } else {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                1024,
                static_cast<int>(rows),
                GL_RGBA,
                GL_FLOAT,
                m_data.data()
            );
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        m_count = palette.size();
        m_rows = rows;
    }
    glActiveTexture(GL_TEXTURE0);
}
}  // namespace openrac::enhanced::fsr3_detail

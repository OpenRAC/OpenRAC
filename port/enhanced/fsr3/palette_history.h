// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <span>
#include <vector>

#include "renderer/math.h"

namespace openrac::enhanced::fsr3_detail {
// Exact previous displayed joints, including deterministic padding at the end of each image.
class PaletteHistory {
public:
    ~PaletteHistory();
    PaletteHistory() = default;
    PaletteHistory(const PaletteHistory&) = delete;
    PaletteHistory& operator=(const PaletteHistory&) = delete;
    void update(std::span<const renderer::Mat4> palette);
    void release();

    unsigned texture() const { return m_texture; }

private:
    unsigned m_texture = 0;
    std::size_t m_count = 0, m_rows = 0;
    std::vector<renderer::Mat4> m_data;
};
}  // namespace openrac::enhanced::fsr3_detail

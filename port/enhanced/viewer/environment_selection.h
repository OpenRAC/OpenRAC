// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <span>
#include <string>

#include "renderer/renderer.h"
#include "renderer/shader.h"

namespace openrac::lighting {
// Conservative candidate masks for up to 512 captures per 16x16 tile; transparent receivers need no depth bound.
class EnvironmentSelection {
public:
    static constexpr unsigned tile_size = 16;
    bool prepare(std::span<const std::array<float, 4>> metadata, std::string& error);
    // A missing camera deliberately selects the full-probe reference path.
    void bind(const renderer::Shader& shader, const renderer::Camera* camera = nullptr) const;
    void release();

private:
    void ensure_buffer() const;
    bool dispatch(const renderer::Camera& camera, const std::array<int, 4>& viewport) const;
    renderer::Shader m_cull;
    unsigned m_count = 0;
    mutable unsigned m_metadata_buffer = 0, m_metadata_texture = 0;
    mutable unsigned m_buffer = 0, m_texture = 0;
    mutable std::size_t m_words = 1;
    mutable renderer::Mat4 m_clip = renderer::identity();
    mutable std::array<int, 4> m_viewport{};
    mutable bool m_started = false;
};
}  // namespace openrac::lighting

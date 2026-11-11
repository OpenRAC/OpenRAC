// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Subtle multi-resolution bloom for the enhanced renderer.
#pragma once

#include <array>
#include <string>
#include <string_view>

#include "common/result.h"
#include "renderer/bloom_settings.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"

namespace openrac::enhanced {

class Bloom {
public:
    ~Bloom();

    // Applies additive bloom to completed world colour.
    void apply(renderer::FrameBuffer& output, const renderer::BloomSettings& settings = {});

    // Releases graphics resources on their owning context.
    void release();

private:
    static constexpr int kMaxLevels = 5;

    [[nodiscard]] bool prepare(int width, int height, std::string& error);
    [[nodiscard]] Result<void> draw(
        std::string_view name, renderer::FrameBuffer& output, unsigned source, bool additive
    );
    [[nodiscard]] Result<void> draw_pyramid(
        renderer::FrameBuffer& output, const renderer::BloomSettings& settings
    );

    std::array<renderer::FrameBuffer, kMaxLevels> m_levels{};
    int m_count = 0;

    renderer::Shader m_downsample;
    renderer::Shader m_upsample;
    renderer::Shader m_composite;

    unsigned m_vao = 0;
    bool m_failed = false;
};

}  // namespace openrac::enhanced

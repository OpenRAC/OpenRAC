// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Subtle multi-resolution bloom for the enhanced renderer.
#pragma once

#include <array>
#include <cstddef>
#include <string>

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

    struct Level {
        unsigned colour = 0;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] bool prepare(int width, int height, std::string& error);
    void release_level(Level& level);
    void allocate_texture(Level& level, int width, int height);
    [[nodiscard]] bool ensure_buffer(std::size_t bytes);

    void downsample(
        const Level& source,
        const Level& destination,
        bool first,
        const renderer::BloomSettings& settings
    );
    void upsample(const Level& high, const Level& low, float scatter);
    [[nodiscard]] Result<void> build_pyramid(
        const Level& source, const renderer::BloomSettings& settings
    );
    [[nodiscard]] Result<void> composite(renderer::FrameBuffer& output, float strength);

    std::array<Level, kMaxLevels> m_levels{};
    int m_count = 0;

    renderer::Shader m_downsample;
    renderer::Shader m_upsample;
    renderer::Shader m_composite;

    unsigned m_buffer = 0;
    std::size_t m_buffer_capacity = 0;
    unsigned m_vao = 0;
    bool m_failed = false;
};

}  // namespace openrac::enhanced

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <optional>

#include "enhanced/fsr3/frame_target.h"
#include "renderer/motion_blur_settings.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"

namespace openrac::enhanced::fsr3_detail {
// Tracking-focus translation is measured between displayed cameras, independently of the eye.
std::optional<renderer::Vec3> reference_displacement(
    const renderer::Camera& current, const renderer::Camera& previous
);

// Target-relative gameplay blur and normal cutscene blur leave FSR input/history unchanged.
class MotionBlur {
public:
    // Optional FSR output: reconstruction writes our colour input without a snapshot copy.
    unsigned reconstruction_output(int width, int height);
    void present_reconstruction(const renderer::RenderTarget& display);
    bool apply(
        const renderer::RenderTarget& display,
        std::array<int, 2> render_size,
        unsigned motion,
        unsigned depth,
        const renderer::Mat4& inverse_raster,
        const renderer::Mat4& previous_view,
        const std::array<float, 2>& jitter,
        const renderer::Vec3& camera_position,
        const renderer::Vec3& reference_delta,
        bool camera_relative,
        float strength,
        float milliseconds,
        const renderer::MotionBlurSettings& tuning = {},
        unsigned reconstructed_colour = 0
    );
    void release();

private:
    bool prepare(int width, int height, int render_width, int render_height, std::string& error);
    bool prepare_colour(int width, int height, std::string& error);
    FrameTarget m_velocity, m_colour;
    std::array<int, 2> m_colour_size{};
    std::array<FrameTarget, 2>
        m_seeds;  // R32UI packed 16-bit source coordinates, ping-ponged by JFA.
    renderer::Shader m_extract, m_first_jump, m_jump, m_resolve, m_copy_resolve;
    renderer::Shader m_fast_first_jump, m_fast_jump;
    unsigned m_vao = 0;
    std::array<int, 4> m_size{};
    bool m_failed = false;
};
}  // namespace openrac::enhanced::fsr3_detail

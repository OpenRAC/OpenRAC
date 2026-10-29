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

// Inputs are borrowed by a synchronous blur dispatch; no frame history is retained here.
struct BlurFrame {
    // Display output and render-resolution velocity/depth inputs.
    renderer::RenderTarget display;
    std::array<int, 2> render_size;
    unsigned motion, depth;
    // Current raster-to-world and previous displayed camera transforms.
    const renderer::Mat4& inverse_raster;
    const renderer::Mat4& previous_view;
    // Raster jitter, eye and tracking-focus translation for reference cancellation.
    const std::array<float, 2>& jitter;
    const renderer::Vec3& camera_position;
    const renderer::Vec3& reference_delta;
    // Cutscenes keep original camera motion; strength is normalized by frame duration.
    bool camera_relative;
    float strength, milliseconds;
    // Persistent controls and optional direct upscaler output texture.
    renderer::MotionBlurSettings tuning{};
    unsigned reconstructed_colour = 0;
};

// Target-relative gameplay blur and normal cutscene blur leave FSR input/history unchanged.
class MotionBlur {
public:
    // Optional FSR output: reconstruction writes our colour input without a snapshot copy.
    unsigned reconstruction_output(int width, int height);
    // Presents the complete unblurred result if a later blur pass cannot proceed.
    void present_reconstruction(const renderer::RenderTarget& display);
    // Records velocity extraction, dilation and resolve without reading the output attachment.
    bool apply(const BlurFrame& frame);
    // Releases render-thread resources and clears failure state.
    void release();

private:
    // Validated tuning is shared by the three passes of one synchronous frame.
    struct Passes {
        const BlurFrame& frame;
        float intensity;
        int max_radius, samples;
        float min_motion;
    };

    // Preserves display colour only when the upscaler did not write our input directly.
    void copy_colour(const renderer::RenderTarget& display);
    // Produces exposure-scaled velocity and receiver depth for the dilation pass.
    void extract_velocity(const Passes& passes);
    // Propagates exact source coordinates and returns the final ping-pong target index.
    int jump_flood(const Passes& passes);
    // Resolves moving samples and, for direct reconstruction, copies stationary pixels too.
    void gather(const Passes& passes, int source);
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

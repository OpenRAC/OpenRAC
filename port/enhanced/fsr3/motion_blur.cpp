// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/fsr3/motion_blur.h"

#include <algorithm>
#include <cmath>

#include "common/log.h"
#include "common/stats.h"
#include "fsr3_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/vulkan/native_compute.h"

namespace openrac::enhanced::fsr3_detail {
using namespace openrac::gl;

std::optional<renderer::Vec3> reference_displacement(
    const renderer::Camera& current, const renderer::Camera& previous
) {
    if (!current.target || !previous.target || !current.target_id
        || current.target_id != previous.target_id) {
        return std::nullopt;
    }
    renderer::Vec3 delta{};
    float distance_squared = 0;
    for (std::size_t i = 0; i < delta.size(); ++i) {
        delta[i] = (*current.target)[i] - (*previous.target)[i];
        if (!std::isfinite(delta[i])) {
            return std::nullopt;
        }
        distance_squared += delta[i] * delta[i];
    }
    // Match the velocity pass's teleport rejection; never smear a target jump across the scene.
    return distance_squared < 64.f ? std::optional{delta} : std::nullopt;
}

bool MotionBlur::prepare(int width, int height, int rw, int rh, std::string& error) {
    if (!prepare_colour(width, height, error)) {
        return false;
    }
    if (!m_extract.id()) {
        std::string copy_resolve(shaders::blur_resolve_frag);
        copy_resolve.insert(copy_resolve.find('\n') + 1, "#define COPY_UNCHANGED 1\n");
        std::string fast_jump(shaders::blur_jump_frag);
        fast_jump.insert(fast_jump.find('\n') + 1, "#define FAST_FLOOD 1\n");
        std::string fast_first(fast_jump);
        fast_first.insert(fast_first.find('\n') + 1, "#define INITIALIZE_SEEDS 1\n");
        std::string first_jump(shaders::blur_jump_frag);
        first_jump.insert(first_jump.find('\n') + 1, "#define INITIALIZE_SEEDS 1\n");
        if (!m_fast_first_jump
                 .build("blur fast initial flood", shaders::fullscreen_vert, fast_first, error)
            || !m_fast_jump.build("blur fast flood", shaders::fullscreen_vert, fast_jump, error)
            || !m_extract.build(
                "relative blur velocity",
                shaders::fullscreen_vert,
                shaders::blur_velocity_frag,
                error
            )
            || !m_first_jump
                    .build("blur initial jump flood", shaders::fullscreen_vert, first_jump, error)
            || !m_jump.build(
                "blur jump flood", shaders::fullscreen_vert, shaders::blur_jump_frag, error
            )
            || !m_resolve.build(
                "relative blur resolve", shaders::fullscreen_vert, shaders::blur_resolve_frag, error
            )
            || !m_copy_resolve.build(
                "blur and copy resolve", shaders::fullscreen_vert, copy_resolve, error
            )) {
            return false;
        }
    }
    if (m_size != std::array{width, height, rw, rh}) {
        m_size = {};
        if (!m_velocity.create(rw, rh, 0x881A, 0, error)
            || !m_seeds[0].create(rw, rh, 0x8236, 0, error)
            || !m_seeds[1].create(rw, rh, 0x8236, 0, error)) {
            return false;
        }
        m_size = {width, height, rw, rh};
    }
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
    }
    return true;
}

bool MotionBlur::prepare_colour(int width, int height, std::string& error) {
    if (m_colour_size == std::array{width, height}) {
        return true;
    }
    m_colour_size = {};
    if (!m_colour.create(width, height, GL_RGBA8, 0, error)) {
        return false;
    }
    m_colour_size = {width, height};
    return true;
}

unsigned MotionBlur::reconstruction_output(int width, int height) {
    if (m_failed) {
        return 0;
    }
    std::string error;
    if (!prepare_colour(width, height, error)) {
        return 0;
    }
    if (!vkgl::storage_texture(m_colour.texture())
        && !vkgl::allocate_storage_texture(m_colour.texture(), GL_RGBA8, width, height)) {
        return 0;
    }
    return m_colour.texture();
}

void MotionBlur::present_reconstruction(const renderer::RenderTarget& display) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_colour.framebuffer());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, display.framebuffer);
    glBlitFramebuffer(
        0,
        0,
        display.width,
        display.height,
        0,
        0,
        display.width,
        display.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, display.framebuffer);
    glViewport(0, 0, display.width, display.height);
}

bool MotionBlur::apply(
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
    const renderer::MotionBlurSettings& tuning,
    unsigned reconstructed_colour
) {
    if (m_failed || !std::isfinite(strength) || strength <= 0) {
        return false;
    }
    const float intensity =
        std::clamp(strength, 0.f, 1.f)
        * (camera_relative ? std::clamp(tuning.cutscene_multiplier, 0.f, 2.f) : 1.f);
    if (intensity <= 0) {
        return false;
    }
    const int max_radius = std::clamp(tuning.max_radius, 1, 128);
    const int samples = std::clamp(tuning.samples, 8, 64);
    const float min_motion = std::clamp(tuning.min_motion, .1f, 4.f);
    const stats::Scope timing(
        camera_relative ? "Camera motion blur" : "Target-relative motion blur", stats::kGpu
    );
    const auto [rw, rh] = render_size;
    std::string error;
    if (rw <= 0 || rh <= 0 || !prepare(display.width, display.height, rw, rh, error)) {
        log::warn("Motion blur disabled: {}", error);
        m_failed = true;
        glBindFramebuffer(GL_FRAMEBUFFER, display.framebuffer);
        glViewport(0, 0, display.width, display.height);
        return false;
    }
    glBindVertexArray(m_vao);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    const auto bind = [](unsigned unit, unsigned texture) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    };
    // The blur never reads from its output attachment: preserve the reconstructed colour first.
    if (!reconstructed_colour) {
        const stats::Scope copy("Motion blur colour copy", stats::kGpu);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, display.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_colour.framebuffer());
        glBlitFramebuffer(
            0,
            0,
            display.width,
            display.height,
            0,
            0,
            display.width,
            display.height,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );
    }
    {
        const stats::Scope extract("Motion blur velocity", stats::kGpu);
        m_extract.use();
        bind(0, motion);
        bind(1, depth);
        glUniform1i(m_extract.uniform("motion_texture"), 0);
        glUniform1f(m_extract.uniform("max_radius"), float(max_radius));
        glUniform1f(m_extract.uniform("min_motion"), min_motion);
        glUniform1i(m_extract.uniform("camera_relative"), camera_relative ? 1 : 0);
        glUniform1i(m_extract.uniform("depth_texture"), 1);
        glUniformMatrix4fv(m_extract.uniform("inverse_raster"), 1, GL_FALSE, inverse_raster.data());
        glUniformMatrix4fv(m_extract.uniform("previous_view"), 1, GL_FALSE, previous_view.data());
        glUniform3f(
            m_extract.uniform("camera_position"),
            camera_position[0],
            camera_position[1],
            camera_position[2]
        );
        glUniform3f(
            m_extract.uniform("reference_delta"),
            reference_delta[0],
            reference_delta[1],
            reference_delta[2]
        );
        glUniform2f(m_extract.uniform("jitter"), jitter[0], jitter[1]);
        glUniform2f(m_extract.uniform("display_size"), float(display.width), float(display.height));
        // Strength 1 is a 1/60 s exposure; normalize displayed-frame displacement by its duration.
        glUniform1f(
            m_extract.uniform("exposure"), intensity * 16.666667f / std::max(milliseconds, 1.f)
        );
        glBindFramebuffer(GL_FRAMEBUFFER, m_velocity.framebuffer());
        glViewport(0, 0, rw, rh);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    // Adapted from SphynxMotionBlurToolkit's jfp_simple: keep the source coordinate,
    // follow its velocity footprint, and propagate through eight neighbours at shrinking jumps.
    // Full render-resolution seeds preserve thin objects before any dilation can discard them.
    // Each R32UI packs two exact 16-bit coordinates; the initial flood generates them implicitly.
    int source = 0;
    const int radius = std::max(
        int(std::ceil(float(max_radius) * float(rw) / float(display.width))),
        int(std::ceil(float(max_radius) * float(rh) / float(display.height)))
    );
    int jump = 1;
    while (jump < radius) {
        jump *= 2;
    }
    bind(1, m_velocity.texture());
    const bool fast = renderer::render_opt::enabled(renderer::render_opt::Opt::MotionBlurFastPath);
    auto& first = fast ? m_fast_first_jump : m_first_jump;
    auto& next = fast ? m_fast_jump : m_jump;
    for (auto* program : {&first, &next}) {
        program->use();
        glUniform1i(program->uniform("seed_texture"), 0);
        glUniform1i(program->uniform("velocity_texture"), 1);
        glUniform1f(program->uniform("min_motion"), min_motion);
        glUniform1f(program->uniform("side_spread"), std::clamp(tuning.spread, 0.f, 1.f));
        glUniform2f(program->uniform("display_size"), float(display.width), float(display.height));
    }
    int passes = 0;
    for (; jump >= 1; jump /= 2) {
        const stats::Scope flood("Motion blur jump flood", stats::kGpu);
        auto& program = passes == 0 ? first : next;
        program.use();
        bind(0, m_seeds[source].texture());
        glUniform1i(program.uniform("jump_pixels"), jump);
        glBindFramebuffer(GL_FRAMEBUFFER, m_seeds[1 - source].framebuffer());
        glDrawArrays(GL_TRIANGLES, 0, 3);
        source = 1 - source;
        ++passes;
    }
    stats::gauge("motion blur JFA passes", passes);

    {
        const stats::Scope gather("Motion blur gather", stats::kGpu);
        auto& resolve = reconstructed_colour ? m_copy_resolve : m_resolve;
        resolve.use();
        bind(0, reconstructed_colour ? reconstructed_colour : m_colour.texture());
        bind(1, m_velocity.texture());
        bind(2, m_seeds[source].texture());
        glUniform1i(resolve.uniform("colour_texture"), 0);
        glUniform1i(resolve.uniform("sample_count"), samples);
        glUniform1f(resolve.uniform("min_motion"), min_motion);
        glUniform1f(
            resolve.uniform("depth_tolerance"), std::clamp(tuning.depth_tolerance, .001f, .05f)
        );
        glUniform1i(resolve.uniform("velocity_texture"), 1);
        glUniform1i(resolve.uniform("seed_texture"), 2);
        glUniform2f(resolve.uniform("jitter"), jitter[0], jitter[1]);
        glBindFramebuffer(GL_FRAMEBUFFER, display.framebuffer);
        glViewport(0, 0, display.width, display.height);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
    return true;
}

void MotionBlur::release() {
    m_velocity.release();
    for (auto& seeds : m_seeds) {
        seeds.release();
    }
    m_colour.release();
    m_extract.release();
    m_fast_first_jump.release();
    m_fast_jump.release();
    m_first_jump.release();
    m_jump.release();
    m_resolve.release();
    m_copy_resolve.release();
    m_colour_size = {};
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    m_size = {};
    m_failed = false;
}
}  // namespace openrac::enhanced::fsr3_detail

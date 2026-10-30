// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/fsr3/motion_blur.h"

#include <algorithm>
#include <cmath>

#include "common/log.h"
#include "common/stats.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"

namespace openrac::enhanced::fsr3_detail {
using namespace openrac::gl;

namespace {
// Fullscreen passes bind texture units explicitly, independent of previous scene state.
void bind(unsigned unit, unsigned texture) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
}
}  // namespace

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

bool MotionBlur::apply(const BlurFrame& frame) {
    // Failed resources and disabled exposure leave the unblurred presentation path active.
    if (m_failed || !std::isfinite(frame.strength) || frame.strength <= 0) {
        return false;
    }
    const float intensity =
        std::clamp(frame.strength, 0.f, 1.f)
        * (frame.camera_relative ? std::clamp(frame.tuning.cutscene_multiplier, 0.f, 2.f) : 1.f);
    // A zero cutscene multiplier suppresses blur without changing gameplay tuning.
    if (intensity <= 0) {
        return false;
    }
    const Passes passes{
        frame,
        intensity,
        std::clamp(frame.tuning.max_radius, 1, 128),
        std::clamp(frame.tuning.samples, 8, 64),
        std::clamp(frame.tuning.min_motion, .1f, 4.f)
    };
    const stats::Scope timing(
        frame.camera_relative ? "Camera motion blur" : "Target-relative motion blur", stats::kGpu
    );
    const auto& display = frame.display;
    const auto [rw, rh] = frame.render_size;
    std::string error;
    // Never record passes against incomplete or invalid render targets.
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
    // Direct FSR output already owns a separate input; only the reference needs a snapshot.
    if (!frame.reconstructed_colour) {
        copy_colour(display);
    }
    extract_velocity(passes);
    const int source = jump_flood(passes);
    gather(passes, source);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
    return true;
}

void MotionBlur::extract_velocity(const Passes& passes) {
    const auto& frame = passes.frame;
    const auto& display = frame.display;
    const auto [rw, rh] = frame.render_size;
    const auto& inverse_raster = frame.inverse_raster;
    const auto& previous_view = frame.previous_view;
    const auto& camera_position = frame.camera_position;
    const auto& reference_delta = frame.reference_delta;
    const auto& jitter = frame.jitter;
    const auto motion = frame.motion, depth = frame.depth;
    const auto max_radius = passes.max_radius;
    const auto min_motion = passes.min_motion;
    const auto camera_relative = frame.camera_relative;
    const auto intensity = passes.intensity, milliseconds = frame.milliseconds;

    const stats::Scope extract_scope("Motion blur velocity", stats::kGpu);
    const auto& extract = renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderVariants)
                              ? m_extract_variants[camera_relative ? 1 : 0]
                              : m_extract;
    extract.use();
    bind(0, motion);
    bind(1, depth);
    glUniform1i(extract.uniform("motion_texture"), 0);
    glUniform1f(extract.uniform("max_radius"), float(max_radius));
    glUniform1f(extract.uniform("min_motion"), min_motion);
    glUniform1i(extract.uniform("camera_relative"), camera_relative ? 1 : 0);
    glUniform1i(extract.uniform("depth_texture"), 1);
    glUniformMatrix4fv(extract.uniform("inverse_raster"), 1, GL_FALSE, inverse_raster.data());
    glUniformMatrix4fv(extract.uniform("previous_view"), 1, GL_FALSE, previous_view.data());
    glUniform3f(
        extract.uniform("camera_position"),
        camera_position[0],
        camera_position[1],
        camera_position[2]
    );
    glUniform3f(
        extract.uniform("reference_delta"),
        reference_delta[0],
        reference_delta[1],
        reference_delta[2]
    );
    glUniform2f(extract.uniform("jitter"), jitter[0], jitter[1]);
    glUniform2f(extract.uniform("display_size"), float(display.width), float(display.height));
    // Strength 1 is a 1/60 s exposure; normalize displayed-frame displacement by its duration.
    glUniform1f(extract.uniform("exposure"), intensity * 16.666667f / std::max(milliseconds, 1.f));
    glBindFramebuffer(GL_FRAMEBUFFER, m_velocity.framebuffer());
    glViewport(0, 0, rw, rh);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int MotionBlur::jump_flood(const Passes& passes) {
    const auto& frame = passes.frame;
    const auto& display = frame.display;
    const auto [rw, rh] = frame.render_size;
    const auto max_radius = passes.max_radius;
    const auto min_motion = passes.min_motion;
    const auto& tuning = frame.tuning;
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
    const bool unique =
        fast && m_unique_jump.id()
        && renderer::render_opt::enabled(renderer::render_opt::Opt::MotionBlurUniqueSeeds);
    auto& next = unique ? m_unique_jump : fast ? m_fast_jump : m_jump;
    const bool reuse =
        renderer::render_opt::enabled(renderer::render_opt::Opt::MotionBlurSetupReuse);
    const FloodSetup setup{
        {display.width, display.height},
        min_motion,
        std::clamp(tuning.spread, 0.f, 1.f),
        jump,
        fast,
        unique
    };
    const bool refresh = !m_flood_setup || *m_flood_setup != setup;
    int setups = 0;
    const auto uniforms = [&](const renderer::Shader& program) {
        glUniform1i(program.uniform("seed_texture"), 0);
        glUniform1i(program.uniform("velocity_texture"), 1);
        glUniform1f(program.uniform("min_motion"), min_motion);
        glUniform1f(program.uniform("side_spread"), setup.spread);
        glUniform2f(program.uniform("display_size"), float(display.width), float(display.height));
        ++setups;
    };
    if (!reuse) {
        for (auto* program : {&first, &next}) {
            program->use();
            uniforms(*program);
        }
    }
    int pass_count = 0;
    for (; jump >= 1; jump /= 2) {
        const stats::Scope flood("Motion blur jump flood", stats::kGpu);
        auto& program = pass_count == 0 ? first : next;
        program.use();
        if (reuse && refresh && pass_count < 2) {
            uniforms(program);
        }
        bind(0, m_seeds[source].texture());
        glUniform1i(program.uniform("jump_pixels"), jump);
        glBindFramebuffer(GL_FRAMEBUFFER, m_seeds[1 - source].framebuffer());
        glDrawArrays(GL_TRIANGLES, 0, 3);
        source = 1 - source;
        ++pass_count;
    }
    stats::gauge("motion blur JFA passes", pass_count);
    stats::gauge("motion blur JFA uniform setups", setups);
    stats::gauge("motion blur unique seeds", unique ? 1 : 0);
    m_flood_setup = setup;

    return source;
}

void MotionBlur::gather(const Passes& passes, int source) {
    const auto& frame = passes.frame;
    const auto& display = frame.display;
    const auto reconstructed_colour = frame.reconstructed_colour;
    const auto samples = passes.samples;
    const auto min_motion = passes.min_motion;
    const auto& tuning = frame.tuning;
    const auto& jitter = frame.jitter;

    const stats::Scope gather("Motion blur gather", stats::kGpu);
    auto& resolve = reconstructed_colour ? m_copy_resolve : m_resolve;
    resolve.use();
    bind(0, reconstructed_colour ? reconstructed_colour : m_colour.texture());
    bind(1, m_velocity.texture());
    bind(2, m_seeds[source].texture());
    glUniform1i(resolve.uniform("colour_texture"), 0);
    glUniform1i(resolve.uniform("sample_count"), samples);
    glUniform1i(
        resolve.uniform("adaptive_samples"),
        renderer::render_opt::enabled(renderer::render_opt::Opt::MotionBlurAdaptiveSamples) ? 1 : 0
    );
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

}  // namespace openrac::enhanced::fsr3_detail

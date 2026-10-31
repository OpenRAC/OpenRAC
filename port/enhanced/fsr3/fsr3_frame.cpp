// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Fsr3 frame resolve: velocities, particle reactivity, SDK dispatch and displayed-pose history.
#include <cmath>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/fsr3/fsr3_internal.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced {
using namespace openrac::gl;

void Fsr3::State::build_velocities() {
    const stats::Scope timing("FSR velocity setup");
    velocities.clear();

    // Map by stable identity, never by draw order; disappearing objects cannot shift another object's history.
    for (const auto& instance : instances) {
        // Blended mobys have no opaque depth and belong in the reactive colour difference.
        if (instance.blend) {
            continue;
        }

        const auto old = previous.find(instance.temporal_id);
        bool matched = !dispatch.reset && instance.temporal_id != 0 && old != nullptr;
        matched = matched && old->model == instance.model;

        // A replaced skeleton or a large translation must not reproject an unrelated pose.
        if (matched) {
            float movement = 0;

            // The fourth matrix column contains the world translation.
            for (std::size_t n = 12; n < 15; ++n) {
                const float delta = instance.matrix[n] - old->matrix[n];
                movement += delta * delta;
            }

            matched = movement < 64 && (instance.palette < 0) == (old->palette < 0);
        }

        const auto& from = matched ? *old : instance;
        scene::MotionInstance velocity;
        velocity.model = instance.model;
        velocity.current = instance.matrix;
        velocity.previous = from.matrix;
        velocity.palettes = {instance.palette, from.palette};
        velocity.alpha = instance.tint[3];
        velocity.floats_on_water = instance.floats_on_water && scene->deformation_active();
        // Simulation-driven offsets have no previous field snapshot: reject stale object history.
        velocity.reactive = !matched || velocity.floats_on_water
                            || from.floats_on_water != velocity.floats_on_water;
        velocity.exclude_glow =
            std::abs(instance.moby_light[3]) > .5f && instance.moby_ambient[3] > .5f;
        velocities.push_back(velocity);
    }
}

void Fsr3::State::save_palette() {
    const stats::Scope timing("FSR palette history", stats::kGpu);
    previous_palette.update(palette);
}

void Fsr3::State::capture_opaque(const renderer::RenderTarget& target) {
    const stats::Scope timing("FSR opaque copy", stats::kGpu);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, opaque.framebuffer());
    glBlitFramebuffer(
        0,
        0,
        target.width,
        target.height,
        0,
        0,
        target.width,
        target.height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    has_opaque = true;
}

void Fsr3::State::draw_camera_motion() {
    const stats::Scope timing("FSR camera motion", stats::kGpu);
    // Fullscreen passes use no mesh input, depth test, blending or stencil mask.
    if (!vao) {
        glGenVertexArrays(1, &vao);
    }

    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, world.width(), world.height());
    camera_motion.use();
    glUniformMatrix4fv(camera_motion.uniform("inverse_raster"), 1, GL_FALSE, inverse_raster.data());
    glUniformMatrix4fv(camera_motion.uniform("previous_view"), 1, GL_FALSE, view.previous.data());
    glUniform3f(
        camera_motion.uniform("camera_position"),
        camera.position[0],
        camera.position[1],
        camera.position[2]
    );
    glUniform2f(camera_motion.uniform("jitter"), dispatch.jitter[0], dispatch.jitter[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world.depth_texture());
    glBindFramebuffer(GL_FRAMEBUFFER, depth.framebuffer());
    glUniform1i(camera_motion.uniform("depth_only"), 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindFramebuffer(GL_FRAMEBUFFER, motion.framebuffer());
    glUniform1i(camera_motion.uniform("depth_only"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Fsr3::State::draw_motion() {
    draw_camera_motion();

    // Fresh history uses current joints at both endpoints; the reactive flag rejects any old colour.
    if (dispatch.reset || !previous_palette.texture()) {
        save_palette();
    }

    build_velocities();
    view.previous_palette = previous_palette.texture();
    // Attach world depth only after the fullscreen sampling passes have finished.
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, world.depth_texture(), 0
    );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    {
        const stats::Scope timing("FSR object motion", stats::kGpu);
        // The reference (fsr-motion-batch off) uses identical pose history, depth and coverage.
        const auto submission =
            renderer::render_opt::enabled(renderer::render_opt::Opt::FsrMotionBatch)
                ? scene::MotionSubmission::Batched
                : scene::MotionSubmission::Individual;
        scene->draw_motion(object_motion, velocities, view, *textures, submission);
    }
    glDisable(GL_DEPTH_TEST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
}

void Fsr3::State::draw_reactive() {
    const stats::Scope timing("FSR reactive mask", stats::kGpu);
    // A renderer without a particle bucket has no intervening colour changes.
    if (!has_opaque) {
        capture_opaque({world.id(), world.width(), world.height()});
    }

    glBindFramebuffer(GL_FRAMEBUFFER, reactive.framebuffer());
    glBindVertexArray(vao);
    reactive_mask.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world.colour_texture());
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, opaque.texture());
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, motion.texture());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void Fsr3::State::restore_world_depth() {
    const stats::Scope timing("FSR late world depth", stats::kGpu);
    glBindFramebuffer(GL_FRAMEBUFFER, display.framebuffer);
    glViewport(0, 0, display.width, display.height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world.depth_texture());
    late_world_depth.use();
    glUniform2f(
        late_world_depth.uniform("display_size"), float(display.width), float(display.height)
    );
    glUniform2f(late_world_depth.uniform("jitter"), dispatch.jitter[0], dispatch.jitter[1]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_GEQUAL);
    glBindVertexArray(0);
}

void Fsr3::State::resolve(renderer::RenderTarget& target) {
    const stats::Scope timing("FSR frame resolve");
    draw_motion();
    draw_reactive();
    std::string error;
    // Select the reference before dispatch: only frames that will blur borrow its output.
    const bool blur_frame = !dispatch.reset && !blur_mode_changed && blur_strength > 0
                            && (!camera_blur || blur_settings.cutscene_multiplier > 0);
    const auto delta = !blur_frame   ? std::optional<renderer::Vec3>{}
                       : camera_blur ? std::optional{renderer::Vec3{}}
                                     : fsr3_detail::reference_displacement(camera, old_camera);
    const unsigned blur_output =
        delta && renderer::render_opt::enabled(renderer::render_opt::Opt::MotionBlurFastPath)
            ? blur.reconstruction_output(display.width, display.height)
            : 0;
    const std::array<unsigned, 5> inputs{
        world.colour_texture(),
        depth.texture(),
        motion.texture(),
        reactive.texture(),
        blur_output ? blur_output : output_texture
    };
    const bool resolved = [&] {
        const stats::Scope upscaler("FSR upscaler", stats::kGpu);
        return sdk.dispatch(inputs, dispatch, error);
    }();

    // A rejected dispatch still presents a complete world and full-resolution HUD, then stops retrying.
    if (!resolved) {
        log::error("{}", error);
        world.blit_to(display.framebuffer, display.width, display.height);
        failed = true;
    }

    if (resolved && delta) {
        const bool blurred = blur.apply(
            {display,
             {world.width(), world.height()},
             motion.texture(),
             world.depth_texture(),
             inverse_raster,
             view.previous,
             dispatch.jitter,
             camera.position,
             *delta,
             camera_blur,
             blur_strength,
             dispatch.milliseconds,
             blur_settings,
             blur_output}
        );
        // Resource failures still present the complete reconstructed world and HUD.
        if (!blurred && blur_output) {
            blur.present_reconstruction(display);
        }
    }

    // Preserve displayed poses (including interpolation), without reallocating survivors.
    const auto changes = previous.update(instances, scene->deformation_active());
    stats::gauge("FSR history nodes inserted", double(changes.inserted));
    stats::gauge("FSR history nodes erased", double(changes.erased));

    save_palette();
    old_camera = camera;
    valid = resolved;
    target = display;
}
}  // namespace openrac::enhanced

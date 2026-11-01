// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// frontend/state.h's world recording: interpolated owned scene data, lighting, temporal resolve
// and native HUD. The level and graphics context are pinned until the worker handoff completes.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/log.h"
#include "common/profiler.h"
#include "common/stats.h"
#include "debug_overlay.h"
#include "frontend.h"
#include "game/common/frontend/camera_latch.h"
#include "game/common/frontend/pacing.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "openrac/game_host.h"
#include "renderer/direct.h"
#include "renderer/gl.h"
#include "scene/moby_pose.h"

namespace openrac::frontend {
namespace {

// Creates or resets temporal history for the submitted graphics settings.
bool prepare_temporal(SceneFrame& frame) {
    State& owner = frame.owner;
    const Kept& scene = frame.scene;
    const auto& graphics = frame.graphics;
    const bool temporal_wanted = enhanced_graphics_available() && graphics.enhanced_graphics
                                 && (graphics.temporal_quality != renderer::TemporalQuality::Off
                                     || (graphics.motion_blur && graphics.motion_blur_strength > 0))
                                 && enhanced::Fsr3::available() && scene.has_camera
                                 && (scene.draws & 0x1Fu) != 0;

    // SDK and pose history are absent when enhanced reconstruction is not in use.
    if (temporal_wanted && !owner.temporal) {
        owner.temporal = std::make_unique<enhanced::Fsr3>();
        // Register after GL statistics initialise, so SDK owners retire before their exit-time destruction.
        std::atexit([] {
            // A manually closed frontend has already released this owner.
            if (g) {
                // Cleanup still retires temporal history after a reported worker error.
                [[maybe_unused]] const bool finished = finish_render();
                g->temporal.reset();
            }
        });
    } else if (!temporal_wanted && owner.temporal) {
        owner.temporal->reset();
    }

    return temporal_wanted;
}

// ---- Frame configuration ----

// Recreates the target when its dimensions, samples or depth-sampling requirement changes.
void resize_target(SceneFrame& frame, int samples, bool sampleable_depth) {
    auto& target = frame.owner.frame;
    const auto& output = frame.output;

    // Minimized pictures keep their last target until a drawable size returns.
    if (output.box.width <= 0 || output.box.height <= 0) {
        return;
    }

    // Stable settings reuse the existing attachments.
    if (target.width() == output.width && target.height() == output.height
        && target.samples() == samples && (target.depth_texture() != 0) == sampleable_depth) {
        return;
    }

    std::string error;
    bool created = target.create(output.width, output.height, error, samples, sampleable_depth);

    // Some drivers advertise sample counts they cannot attach; retry without multisampling.
    if (!created && samples > 1) {
        log::warn("{}; drawing without anti-aliasing", error);
        created = target.create(
            output.width,
            output.height,
            error,
            1,
            enhanced_graphics_available() && frame.graphics.enhanced_graphics
        );
    }

    // Propagate attachment failure through the worker rather than recording against an incomplete target.
    if (!created) {
        throw std::runtime_error(error);
    }

    log::info(
        "frame {}x{} for a picture of {}x{} pixels, {} sample(s) a pixel",
        target.width(),
        target.height(),
        output.box.width,
        output.box.height,
        target.samples()
    );
}

// Sizes the render target and returns the original picture width within it.
float prepare_target(SceneFrame& frame) {
    const profiling::Zone trace("Prepare render targets");

    auto& output = frame.output;
    const auto& graphics = frame.graphics;
    frame_size(
        output.box.width, output.box.height, graphics.render_scale, output.width, output.height
    );
    // Temporal reconstruction supplies antialiasing; native rendering keeps the saved MSAA preference.
    const int samples =
        frame.temporal ? 1 : std::clamp(graphics.msaa, 1, renderer::FrameBuffer::max_samples());
    const bool sampleable_depth =
        samples == 1 && enhanced_graphics_available() && graphics.enhanced_graphics;
    resize_target(frame, samples, sampleable_depth);

    // A wider picture preserves the central television framing.
    if (output.aspect > kTvAspect && output.box.width > 0 && output.box.height > 0) {
        return std::min(
            1.0f,
            kTvAspect * static_cast<float>(output.box.height) / static_cast<float>(output.box.width)
        );
    }

    return 1.0f;
}

// Applies the submitted settings and authored shadow state.
void set_lighting(SceneFrame& frame) {
    State& owner = frame.owner;
    const Kept& scene = frame.scene;
    const auto& graphics = frame.graphics;
    auto& input = frame.input;

    // Boot pictures can arrive before a world renderer exists.
    if (owner.renderer) {
        const int anisotropy = graphics.anisotropy_samples(
            gl::enhanced_rendering_available() && graphics.enhanced_graphics
        );
        owner.renderer->textures().set_anisotropy(static_cast<float>(anisotropy));
    }

    const bool enhanced = enhanced_graphics_available() && graphics.enhanced_graphics;
    input.texture_filter = enhanced ? graphics.texture_filter : renderer::TextureFilter::Normal;
    owner.scene.set_probe_lighting(enhanced, graphics.probe_gi_multiplier);
    owner.scene.set_gameplay_rim(
        enhanced && graphics.rim_light, graphics.rim_light_strength, graphics.rim_light_falloff
    );
    owner.scene.set_gameplay_rim_lift(
        graphics.rim_shadow_lift, graphics.rim_lift_exponent, graphics.rim_lift_max_gain
    );
    owner.scene.set_probe_bias(graphics.probe_normal_bias, graphics.probe_view_bias);
    if (auto* lighting = owner.scene.level_lighting()) {
        lighting->set_sunlight_direction(enhanced ? graphics.sunlight_direction() : std::nullopt);
    }
    owner.scene.set_probe_spheres(graphics.probe_spheres);
    owner.scene.set_area_lights(graphics.ceiling_area_lights);
    owner.scene.set_medial_axis_debug(graphics.medial_axis_debug);
    owner.scene.set_environment_probe_debug(graphics.environment_probe_debug);
    input.shadow_quality = graphics.shadow_quality;
    input.sun_pixel_shadow = enhanced && graphics.enhanced_shadows
                             && owner.scene.sun_shadow_enabled() && scene.has_camera
                             && openrac_game_has_level(scene.level);
    input.hero_pixel_shadow = enhanced && graphics.enhanced_shadows
                              && owner.scene.hero_shadow_enabled()
                              && scene.hero_airborne.has_value() && scene.has_camera
                              && openrac_game_has_level(scene.level);
    input.hero_airborne = scene.hero_airborne.value_or(false);
    input.shadow_extrusion = scene.shadow_state.extrusion;

    // Disabled authored shadow state carries no usable height interval.
    if (scene.shadow_state.enabled) {
        input.shadow_heights = scene.shadow_state.heights;
    }
}

// Enables submitted world buckets while retaining native HUD drawing during loading.
void set_buckets(SceneFrame& frame) {
    State& owner = frame.owner;
    const Kept& scene = frame.scene;
    const bool has_camera = frame.has_camera;

    // Native HUD remains available even when the game submits no world.
    for (auto& bucket : owner.renderer
                            ? owner.renderer->renderers()
                            : std::span<const std::unique_ptr<renderer::BucketRenderer>>{}) {
        // Particle effects are meaningful only with a captured world camera.
        if (bucket->bucket() == renderer::Bucket::Particles) {
            // The effect quads are there only when the game drew them.
            bucket->enabled = has_camera;
        } else if (bucket->bucket() != renderer::Bucket::Hud) {
            bucket->enabled =
                has_camera && (scene.draws & (1u << static_cast<unsigned>(bucket->bucket()))) != 0;
        }
    }
}

// Returns the camera to draw the world from: `camera`, or when usable_camera or usable_view
// refuses it (non-finite, degenerate, out past every level: a frame of it is a black screen
// crossed by a few smeared triangles), the last one they accepted, and none before any was. The
// first frame of each run of refused ones is logged (a few runs a session), with the camera's
// numbers, so that a report carries them.
std::optional<renderer::Camera> guard_camera(
    State& owner, const host::GameState& state, const renderer::Camera& camera, std::uint64_t tick
) {
    const CameraPose pose{state.camera_position, state.forward, state.left, state.up};

    // A usable camera is drawn, and kept for frames that have none.
    if (usable_camera(pose) && usable_view(camera.view, camera.projection)) {
        if (owner.refused_camera_frames != 0) {
            log::info("camera usable again after {} refused frame(s)", owner.refused_camera_frames);
            owner.refused_camera_frames = 0;
        }
        owner.good_camera = camera;
        return camera;
    }

    // The first frame of a run is logged; a session logs at most a few runs.
    constexpr int kMostLogged = 4;
    if (owner.refused_camera_frames++ == 0 && owner.refused_camera_logs < kMostLogged) {
        ++owner.refused_camera_logs;
        log::warn(
            "camera: tick {} has an unusable camera (eye {} {} {}, forward {} {} {}, up {} {} {}, "
            "tangents {} {}); {}",
            tick,
            state.camera_position[0],
            state.camera_position[1],
            state.camera_position[2],
            state.forward[0],
            state.forward[1],
            state.forward[2],
            state.up[0],
            state.up[1],
            state.up[2],
            state.tan_half_fov_x,
            state.tan_half_fov_y,
            owner.good_camera ? "drawing the last good one" : "drawing no world"
        );
    }

    return owner.good_camera;
}

// Configures visible buckets, the interpolated camera and probe selection.
void set_camera(SceneFrame& frame) {
    State& owner = frame.owner;
    const Kept& scene = frame.scene;
    auto& input = frame.input;
    const float picture_scale = frame.picture_scale;
    // Turned by the late camera latch (frontend/camera_latch.h) when it is on.
    const auto state = turned_camera(
        scene.has_prev_state && frame.output.alpha < 1.0f
            ? blend_camera(scene.prev_state, scene.state, frame.output.alpha)
            : scene.state,
        {frame.output.latch_yaw, frame.output.latch_pivot}
    );
    renderer::Camera camera;
    camera.view = state.view();
    camera.target = state.camera_target;
    camera.target_id = state.camera_target_id;
    // The game's frustum: its horizontal and vertical tangents as the game set them (the frame is
    // the TV's 4:3, which they were made for), not the window's shape. A wider picture keeps the
    // vertical one and opens the horizontal one by as much as the picture is wider: the game's
    // view is its middle.
    camera.projection = state.projection(
        (state.tan_half_fov_y > 0.0f ? state.tan_half_fov_x / state.tan_half_fov_y : 4.0f / 3.0f)
            / picture_scale,
        0.05f,
        2000.0f
    );
    camera.position = state.camera_position;
    // No camera yet (loading, fades between programs): a black frame, not the level's background.
    // Nor one the guard refuses with no good one to draw instead, nor a tick that draws no world
    // (a loading screen's or a new game's captions, whose view context holds the levels' colours
    // as they come and go). With a world, the colour the game clears to (the level settings'
    // background, which a mod can change), else the extracted level's.
    const std::optional<renderer::Camera> drawn =
        scene.has_camera ? guard_camera(owner, state, camera, scene.frame) : std::nullopt;
    const bool has_camera = drawn.has_value();
    frame.has_camera = has_camera;
    const auto background = scene.state.background_or(owner.level.background);
    input.clear_colour = has_camera && (scene.draws & kWorldLayers) != 0
                             ? std::array<float, 4>{background[0], background[1], background[2], 1}
                             : std::array<float, 4>{0, 0, 0, 1};
    set_buckets(frame);
    input.camera = drawn.value_or(camera);
    owner.scene.choose_scenery(
        frame.graphics.original_lod, input.camera.position, input.camera.projection
    );
    owner.scene.update_probe_regions(input.camera);
    input.after_world_depth =
        owner.scene.compute_probe_selection()
            ? std::function<void(unsigned, int, int)>(
                  [&owner, &input](unsigned depth, int width, int height) {
                      owner.scene.select_visible_probe_regions(depth, input.camera, width, height);
                  }
              )
            : std::function<void(unsigned, int, int)>{};
}

// ---- World recording ----

// Records the submitted world and temporal resolve before native HUD drawing. Returns whether this
// frame drew anew (a repeated frame keeps the picture drawn before).
bool draw_world(SceneFrame& frame) {
    State& owner = frame.owner;
    const Kept& scene = frame.scene;
    const auto& graphics = frame.graphics;
    auto& input = frame.input;
    const int width = frame.output.width;
    const int height = frame.output.height;
    const float picture_scale = frame.picture_scale;
    const bool temporal_wanted = frame.temporal;

    // Repeated pause-menu frames preserve the already completed world; a tick drawn over the last
    // picture draws its 2D once, on its first frame (drawn again, a fade's darkening would add up).
    if (owner.renderer && !owner.repeat_frame && !(owner.overdraw && !frame.output.first)) {
        // [debug tools] The passes hidden in the overlay; the render timed (common/stats.h).
        debug_overlay::before_render(owner.renderer.get(), frame.input.frame);
        const stats::Scope render_scope("render");
        renderer::RenderTarget target{owner.frame.id(), width, height, picture_scale};
        target.depth_texture = owner.frame.depth_texture();
        target.resolve_framebuffer = owner.frame.resolve_target();
        target.resolves_itself = owner.frame.resolves_itself();
        target.wide_2d = picture_scale < 1.0f && graphics.hud == settings::Hud::Edges;

        // Loading/menu snapshots bypass history; active world frames resolve before the final HUD bucket.
        if (temporal_wanted && !input.keep_colour) {
            std::string temporal_error;
            const enhanced::TemporalFrame temporal{
                input,
                target,
                owner.frame,
                owner.scene,
                owner.renderer->textures(),
                scene.level,
                graphics.motion_blur,
                graphics.motion_blur_strength,
                scene.cutscene,
                graphics.motion_blur_settings
            };
            const bool active = owner.temporal->begin(
                temporal, graphics.temporal_quality, graphics.temporal_sharpness, temporal_error
            );

            // The temporal module reports recoverable setup failures without throwing.
            if (!active && !temporal_error.empty()) {
                log::error("{}", temporal_error);
            }
        }

        if (graphics.agx_preview && graphics.enhanced_graphics && enhanced_graphics_available()
            && scene.has_camera && !input.keep_colour && (scene.draws & 0x1Fu) != 0) {
            auto before_hud = std::move(input.before_hud);
            input.before_hud = [&owner,
                                exposure = graphics.adjusted_agx_exposure(),
                                before_hud = std::move(before_hud)] {
                owner.agx.apply(owner.frame, exposure);
                if (before_hud) {
                    before_hud();
                }
            };
        }
        owner.renderer->render(input, target);
        return true;
    }

    return false;
}
}  // namespace

void render_scene(
    State& owner,
    const Kept& snapshot,
    const settings::Settings& graphics,
    FramePresentation& output
) {
    SceneFrame frame{owner, snapshot, graphics, output, {}, {}, false, 1};
    frame.temporal = prepare_temporal(frame);
    {
        const profiling::Zone trace("Upload scene");
        upload_instances(frame);
    }
    const auto scene_done = std::chrono::steady_clock::now();
    frame.picture_scale = prepare_target(frame);
    set_lighting(frame);
    blend_shadows(frame);
    set_camera(frame);
    set_packets(frame);
    set_target_mobys(frame);
    retain_background(frame);
    const bool world = draw_world(frame);

    // A mod's views (a headset's eyes), from the same scene; nothing without a mod's.
    if (views_wanted()) {
        draw_views(frame, world);
    }

    const auto recorded = std::chrono::steady_clock::now();

    // Frame-cost diagnostics deliberately stall the GPU, matching their synchronous reference behaviour.
    if (output.measure_cost) {
        const profiling::Zone trace("Diagnostic GPU wait");

        gl::glFinish();
    }

    output.shown = owner.frame.resolved();
    output.camera = frame.input.camera;
    output.has_camera = frame.has_camera;
    output.tick = frame.input.frame;
    output.finished = std::chrono::steady_clock::now();
    output.scene_seconds = std::chrono::duration<double>(scene_done - output.started).count();
    output.record_seconds = std::chrono::duration<double>(recorded - scene_done).count();
    output.gpu_seconds = std::chrono::duration<double>(output.finished - recorded).count();
}

}  // namespace openrac::frontend

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

#include "common/breadcrumbs.h"
#include "common/env.h"
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
#include "renderer/precise_depth.h"
#include "renderer/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif
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

// Whether the frame is drawn in half floats: while HDR is shown (docs/port/HDR.md), so that the
// game's blending may add up above white. Each draw's own colour is still clamped to 0..1 as the
// 8-bit frame clamps it (vkgl::clamp_colour_writes); in SDR the frame is 8-bit, as always.
bool hdr_frame() {
#if defined(OPENRAC_VULKAN)
    // OPENRAC_HDR_FRAME: also without an HDR screen, shown clamped, to compare with the 8-bit one.
    return gl::is_vulkan() && (vkgl::hdr_frame() || env::flag(env::Var::HdrFrame));
#else
    return false;
#endif
}

// Recreates the target when its dimensions, samples, depth-sampling requirement or colour
// format changes.
void resize_target(SceneFrame& frame, int samples) {
    auto& target = frame.owner.frame;
    const auto& output = frame.output;
    const bool float_colour = hdr_frame();
    // Reserve sampleable depth while the level's probe data is still loading.
    const bool wants_sampleable_depth =
        gl::enhanced_rendering_available() && frame.graphics.enhanced_graphics;
    const bool sampleable_depth = samples == 1 && wants_sampleable_depth;

    // Minimized pictures keep their last target until a drawable size returns.
    if (output.box.width <= 0 || output.box.height <= 0) {
        return;
    }

    // Stable settings reuse the existing attachments.
    if (target.width() == output.width && target.height() == output.height
        && target.samples() == samples && (target.depth_texture() != 0) == sampleable_depth
        && target.float_colour() == float_colour) {
        return;
    }

    std::string error;
    bool created =
        target.create(output.width, output.height, error, samples, sampleable_depth, float_colour);

    // Some drivers advertise sample counts they cannot attach; retry without multisampling.
    if (!created && samples > 1) {
        log::warn("{}; drawing without anti-aliasing", error);
        created = target.create(
            output.width, output.height, error, 1, wants_sampleable_depth, float_colour
        );
    }

    // Propagate attachment failure through the worker rather than recording against an incomplete target.
    if (!created) {
        throw std::runtime_error(error);
    }

    // A transition tick can retain its picture before any world draws into the new target.
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, target.id());
    gl::glDisable(gl::GL_SCISSOR_TEST);
    gl::glColorMask(gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
    gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT);

#if defined(OPENRAC_VULKAN)
    if (float_colour) {
        vkgl::clamp_colour_writes(target.id());
    }
#endif

    log::info(
        "frame {}x{} for a picture of {}x{} pixels, {} sample(s) a pixel{}",
        target.width(),
        target.height(),
        output.box.width,
        output.box.height,
        target.samples(),
        float_colour ? ", in half floats (HDR)" : ""
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
    resize_target(frame, samples);

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
    std::vector<renderer::PointEmitter> emitters;
    const auto& orb_light = graphics.nanotech_light;
    if (enhanced && orb_light.enabled && orb_light.brightness > 0.0f) {
        auto radiance = orb_light.colour;
        for (float& channel : radiance) {
            channel *= orb_light.brightness;
        }
        emitters.reserve(scene.state.nanotech_orbs.size());
        for (const auto& position : scene.state.nanotech_orbs) {
            emitters.push_back({position, radiance, orb_light.range});
        }
    }
    owner.scene.set_point_emitters(std::move(emitters));
    input.texture_filter = enhanced ? graphics.texture_filter : renderer::TextureFilter::Normal;
    // The lossy render shortcuts follow the setting; High (and the base game) draws in full.
    renderer::render_opt::set_effects_detail(
        enhanced ? graphics.effects_detail : renderer::render_opt::EffectsDetail::High
    );
    if (auto* lighting = owner.scene.level_lighting()) {
        lighting->set_material_overrides(enhanced);
    }
    owner.scene.set_probe_lighting(
        enhanced && !graphics.sweep_probe_lighting_off, graphics.probe_gi_multiplier
    );
    owner.scene.set_sweep_translucent(
        graphics.sweep_translucent_off,
        graphics.sweep_translucent_unlit,
        graphics.sweep_translucent_full
    );
    owner.scene.set_sweep_moby_blended_off(graphics.sweep_moby_blended_off);
    owner.scene.set_sweep_one_triangle(graphics.sweep_one_triangle);
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
    input.shadow_filter = graphics.shadow_filter;
    input.volumetric_quality = graphics.volumetric_quality;
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
    static const bool& opaque_effects_off = stats::probe(
        "OPENRAC_OPAQUE_EFFECTS_OFF", "Reference: draw opaque effects in their original passes"
    );
    frame.input.opaque_effects =
        !opaque_effects_off && owner.game.starts_with("rac1-") && has_camera
        && (scene.draws & (1u << static_cast<unsigned>(renderer::Bucket::Ties))) != 0;

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
    // The world's depth starts at the profile's near (GameProfile::depth_near: finer depth far
    // away), clamped so that nothing nearer is cut away; with precise depth at the window's 0.05.
    const float profile_near =
        owner.profile != nullptr && !renderer::precise_depth() ? owner.profile->depth_near() : 0.0f;
    const float near_z = std::max(0.05f, profile_near);
    input.depth_clamp = profile_near > 0.05f;
    // The game's frustum: its horizontal and vertical tangents as the game set them (the frame is
    // the TV's 4:3, which they were made for), not the window's shape. A wider picture keeps the
    // vertical one and opens the horizontal one by as much as the picture is wider: the game's
    // view is its middle.
    camera.projection = state.projection(
        (state.tan_half_fov_y > 0.0f ? state.tan_half_fov_x / state.tan_half_fov_y : 4.0f / 3.0f)
            / picture_scale,
        near_z,
        2000.0f
    );
    camera.position = state.camera_position;
    // No camera yet (loading, fades between programs): a black frame, not the level's background.
    // Nor one the guard refuses with no good one to draw instead, nor a tick that draws no world
    // (a loading screen's or a new game's captions, whose view context holds the levels' colours
    // as they come and go). With a world, the colour the game clears to (the level settings'
    // background, which a mod can change), else the extracted level's. Mobys alone are no level:
    // the game cleared to black under them (frontend/tick_picture.h, kLevelLayers).
    const std::optional<renderer::Camera> drawn =
        scene.has_camera ? guard_camera(owner, state, camera, scene.frame) : std::nullopt;
    const bool has_camera = drawn.has_value();
    frame.has_camera = has_camera;
    const auto background = scene.state.background_or(owner.level.background);
    input.clear_colour = has_camera && (scene.draws & kLevelLayers) != 0
                             ? std::array<float, 4>{background[0], background[1], background[2], 1}
                             : std::array<float, 4>{0, 0, 0, 1};
    set_buckets(frame);
    input.camera = drawn.value_or(camera);
    // Adjust the guarded gameplay lens before the debug camera can replace it. Scaling both
    // projection axes preserves the native aspect ratio and the picture's widescreen expansion.
    // The level's world under the pause menu keeps the lens it was paused with (keep_tick). A tick
    // that drew mobys alone (the pause menu's frame objects over its picture of the frame) is no
    // gameplay view: they are placed for the native lens.
    float opened = 1.0f;
    if (has_camera && scene.gameplay_camera && (scene.draws & kLevelLayers) != 0
        && !frame.output.camera_debug.settings.enabled
        && frame.graphics.gameplay_fov != settings::kDefaultGameplayFov) {
        const float original = 1.0f / input.camera.projection[5];
        const float percent = static_cast<float>(frame.graphics.gameplay_fov) / 100.0f;
        const float scale = original / std::tan(std::atan(original) * percent);
        input.camera.projection[0] *= scale;
        input.camera.projection[5] *= scale;
        opened = 1.0f / scale;
    }
    // The game's view tests of the next ticks open by as much (openrac_game_view_opened).
    set_view_opened(opened);
    if (has_camera) {
        owner.debug_camera.apply(input, frame.output.camera_debug, scene.level);
        frame.output.freecam = frame.output.camera_debug.settings.enabled;
        frame.output.camera_level = scene.level;
    }
    const auto& visibility_camera = input.culling_camera ? *input.culling_camera : input.camera;
    owner.scene.choose_scenery(
        frame.graphics.draw_distance_scale(),
        visibility_camera.position,
        visibility_camera.projection
    );
    const bool had_probes = owner.scene.probe_data_available();
    owner.scene.update_probe_regions(input.camera);

    // Background bakes can become usable here, after initial level setup skipped modern shadows.
    if (!had_probes && owner.scene.probe_data_available()) {
        configure_level_shadows(owner, scene.level);
    }
    // This hook only prepares declared probe/light selection kernels and stable uploads.
    input.prepares_world_depth = true;
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
        if (graphics.enhanced_graphics && enhanced_graphics_available()) {
            target.depth_texture = owner.frame.depth_texture();
        }
        target.resolve_framebuffer = owner.frame.resolve_target();
        target.resolves_itself = owner.frame.resolves_itself();
        target.wide_2d = picture_scale < 1.0f && graphics.hud == settings::Hud::Edges;

        const bool enhanced_world_colour = graphics.enhanced_graphics
                                           && enhanced_graphics_available() && scene.has_camera
                                           && !input.keep_colour && (scene.draws & 0x1Fu) != 0;
        const bool bloom_colour =
            enhanced_world_colour && graphics.bloom.enabled && graphics.bloom.strength > 0.0f;
        const bool grade_colour = enhanced_world_colour && graphics.agx_preview;
        const bool finish_world_colour = bloom_colour || grade_colour;
        std::function<void()> finish_colour;
        // Bloom and optional tone mapping are colour-only and run before late world/HUD work.
        if (finish_world_colour) {
            finish_colour = [&owner,
                             bloom_colour,
                             bloom = graphics.bloom,
                             grade_colour,
                             exposure = graphics.adjusted_agx_exposure()] {
                if (bloom_colour) {
                    crash::breadcrumb("bloom");
                    owner.bloom.apply(owner.frame, bloom);
                }
                if (grade_colour) {
                    crash::breadcrumb("tone mapping");
                    owner.agx.apply(owner.frame, exposure);
                }
            };
        }
        bool colour_finished_by_temporal = false;

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
                graphics.motion_blur_settings,
                finish_colour
            };
            const bool active = owner.temporal->begin(
                temporal, graphics.temporal_quality, graphics.temporal_sharpness, temporal_error
            );
            colour_finished_by_temporal = active && finish_world_colour;

            // The temporal module reports recoverable setup failures without throwing.
            if (!active && !temporal_error.empty()) {
                log::error("{}", temporal_error);
            }
        }

        // The mods' heat haze (interface 1.12), over the world and under the HUD.
        if (!scene.haze.empty() && scene.has_camera && !input.keep_colour && !scene.world_under_menu
            && (scene.draws & 0x1Fu) != 0) {
            auto before_hud = std::move(input.before_hud);
            input.before_hud = [&owner,
                                &scene,
                                view_projection =
                                    renderer::multiply(input.camera.projection, input.camera.view),
                                before_hud = std::move(before_hud)] {
                owner.haze.apply(
                    owner.frame,
                    view_projection,
                    scene.haze,
                    static_cast<float>(scene.frame) / 60.0f
                );
                if (before_hud) {
                    before_hud();
                }
            };
        }
        if (finish_world_colour && !colour_finished_by_temporal) {
            auto before_hud = std::move(input.before_hud);
            input.before_hud = [finish_colour = std::move(finish_colour),
                                before_hud = std::move(before_hud)] {
                finish_colour();
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
    const crash::BreadcrumbScope breadcrumbs(output.source_frame);
    crash::breadcrumb("scene setup");
    SceneFrame frame{owner, snapshot, graphics, output, {}, {}, {}, false, 1};
    frame.temporal = prepare_temporal(frame);
    {
        const profiling::Zone trace("Upload scene");
        crash::breadcrumb("scene upload");
        upload_instances(frame);
    }
    const auto scene_done = std::chrono::steady_clock::now();
    crash::breadcrumb("render target setup");
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
        const profiling::Zone trace("Diagnostic GPU wait", profiling::ZoneColour::Idle);

        gl::glFinish();
    }

    output.shown = owner.frame.resolved();
    output.camera = frame.input.camera;
    output.has_camera = frame.has_camera;
    output.tick = frame.input.frame;
    output.enhanced_graphics = graphics.enhanced_graphics && enhanced_graphics_available();
    output.finished = std::chrono::steady_clock::now();
    output.scene_seconds = std::chrono::duration<double>(scene_done - output.started).count();
    output.record_seconds = std::chrono::duration<double>(recorded - scene_done).count();
    output.gpu_seconds = std::chrono::duration<double>(output.finished - recorded).count();
}

}  // namespace openrac::frontend

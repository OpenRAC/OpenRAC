// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Binds captured HUD data and offscreen menu objects without accessing live game memory.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

#include "common/log.h"
#include "frontend.h"
#include "game/common/frontend/effects_dump.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "program/program.h"
#include "renderer/direct/direct.h"
#include "renderer/gl/gl.h"
#include "scene/vendor_preview.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::frontend {
namespace {

// Offscreen target geometry decoded from the native HUD callback arguments.
struct MobyTarget {
    std::uint64_t marker;  // Address/count marker in the owned HUD packets.
    unsigned framebuffer;  // GPU framebuffer selected by the HUD renderer.
    int size;              // Square target extent in pixels.
    float centre_x;        // Horizontal projection centre in native HUD pixels.
    float centre_y;        // Vertical projection centre in native HUD pixels.
    bool clear_depth;      // Clears depth before drawing this panel.
};

// Puts `world` in the frame's snapshot (images.cpp's store_image): the menu that read the frame
// back as it opened then draws this world under it, not the one it opened on.
void refresh_frame_snapshot(const renderer::FrameBuffer& world) {
    using namespace gl;
    renderer::FrameSnapshot& snap = renderer::g_frame_snapshot;
    if (snap.texture == 0 || snap.width != world.width() || snap.height != world.height()) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, snap.texture);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, world.resolved());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, snap.width, snap.height);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

// Draws menu mobys using their captured cameras, without reading live game memory.
void draw_target_mobys(SceneRenderState& owner, const Kept& scene, const MobyTarget& target) {
    const std::uint32_t first = static_cast<std::uint32_t>(target.marker);
    // The HUD marker packs the first game address below a 16-bit instance count.
    const std::uint32_t count = static_cast<std::uint32_t>(target.marker >> 32) & 0xFFFFu;
    std::vector<scene::Instance> instances;
    const host::GameState* camera = nullptr;

    // The captured page objects occupy consecutive 256-byte moby records.
    for (const Kept::TargetMoby& moby : scene.target_mobys) {
        // Other markers draw different ranges of the captured menu objects.
        if (!moby.on_screen && moby.address >= first && moby.address < first + count * 0x100u) {
            if (moby.preview_item_class < 0) {
                instances.push_back(moby.instance);
            }
            camera = &moby.camera;
        }
    }

    // Incomplete captures cannot supply the panel projection.
    if (camera == nullptr || !owner.renderer || camera->focal_x <= 0.0f) {
        return;
    }

    gl::glViewport(0, 0, target.size, target.size);
    gl::glDisable(gl::GL_SCISSOR_TEST);

    // Each panel decides whether it starts a new depth layer.
    if (target.clear_depth) {
        gl::glDepthMask(gl::GL_TRUE);
        gl::glClearDepth(0.0);
        gl::glClear(gl::GL_DEPTH_BUFFER_BIT);
    }

    const float half = 0.5f * static_cast<float>(renderer::kOffscreenPixels);
    const float tan_x = half / camera->focal_x;
    const float tan_y = half / camera->focal_y;
    // Match the world camera's near/far range in the offscreen panel.
    const renderer::Mat4 projection =
        renderer::perspective_reversed(2.0f * std::atan(tan_y), tan_x / tan_y, 0.05f, 2000.0f);
    renderer::Mat4 offset = renderer::identity();
    // Matrix translation moves the axis from the square centre to the native HUD projection centre.
    offset[12] = (target.centre_x - half) / half;
    offset[13] = -(target.centre_y - half) / half;
    renderer::FrameInput panel;
    panel.camera.view = camera->view();
    panel.camera.projection = renderer::multiply(offset, projection);
    panel.camera.position = camera->camera_position;
    renderer::RenderState state{
        owner.renderer->textures(),
        renderer::RenderTarget{target.framebuffer, target.size, target.size},
        {}
    };
    owner.scene.draw_mobys(instances, panel, state);
    for (const Kept::TargetMoby& moby : scene.target_mobys) {
        if (moby.on_screen || moby.preview_item_class < 0 || moby.address < first
            || moby.address >= first + count * 0x100u) {
            continue;
        }
        auto inset = panel;
        const auto& adjustment = scene::vendor_preview::get(moby.preview_item_class);
        inset.camera.projection =
            scene::vendor_preview::projection(panel.camera.projection, adjustment);
        auto instance = moby.instance;
        const auto& shape = owner.level.models[instance.model];
        instance.matrix = scene::vendor_preview::orient(
            instance.matrix,
            adjustment,
            {(shape.min[0] + shape.max[0]) * .5f,
             (shape.min[1] + shape.max[1]) * .5f,
             (shape.min[2] + shape.max[2]) * .5f},
            false
        );
        owner.scene.draw_mobys({instance}, inset, state);
    }
}

// Draws a menu's mobys on the screen over the world drawn under it (frontend/menu_layers.h), with
// the camera and the frustum the menu drew them with, into `target` (bound). `clear_depth`: the
// first of the frame, which no longer test against the world's depth.
void draw_screen_mobys(
    SceneRenderState& owner,
    const Kept& scene,
    std::uint64_t marker,
    const renderer::RenderTarget& target,
    bool clear_depth
) {
    const std::uint32_t first = static_cast<std::uint32_t>(marker);
    // The HUD marker packs the first game address below a 16-bit instance count.
    const std::uint32_t count = static_cast<std::uint32_t>(marker >> 32) & 0xFFFFu;
    std::vector<scene::Instance> instances;
    const host::GameState* camera = nullptr;
    for (const Kept::TargetMoby& moby : scene.target_mobys) {
        if (moby.on_screen && moby.address >= first && moby.address < first + count * 0x100u) {
            instances.push_back(moby.instance);
            camera = &moby.camera;
        }
    }

    // The tick's capture has none of these (the list shown is the tick before's).
    if (camera == nullptr || !owner.renderer) {
        return;
    }

    if (clear_depth) {
        gl::glDepthMask(gl::GL_TRUE);
        gl::glClearDepth(0.0);
        gl::glClear(gl::GL_DEPTH_BUFFER_BIT);
    }

    // The frame's projection (scene_frame.cpp's set_camera), from the menu's camera.
    const float aspect = camera->tan_half_fov_y > 0.0f
                             ? camera->tan_half_fov_x / camera->tan_half_fov_y
                             : 4.0f / 3.0f;
    renderer::FrameInput menu;
    menu.camera.view = camera->view();
    menu.camera.projection = camera->projection(aspect / target.picture_scale_x, 0.05f, 2000.0f);
    menu.camera.position = camera->camera_position;
    renderer::RenderState state{owner.renderer->textures(), target, {}};
    owner.scene.draw_mobys(instances, menu, state);
}

}  // namespace

void set_packets(SceneFrame& frame) {
    const Kept& scene = frame.scene;
    auto& input = frame.input;
    const auto& graphics = frame.graphics;
    const bool enhanced = enhanced_graphics_available() && graphics.enhanced_graphics;
    const bool has_camera = frame.has_camera;
    const auto started = frame.output.started;
    input.gtao_settings = graphics.gtao_settings;
    // Honor the user's GTAO setting alongside probe GI and modern shadows.
    // Mesh shaders apply AO to indirect lighting before direct light is added.
    input.gtao = enhanced && graphics.gtao && has_camera && (scene.draws & 0x1Fu) != 0;
    input.frame = scene.frame;
    input.tick_rate = program::tick_rate();
    input.enhanced_water = enhanced;
    input.lily_momentum = graphics.lily_momentum;
    input.water_hands = scene.water_hands;
    input.water_impulses = scene.water_impulses;
    for (const auto& instance : scene.live) {
        if (instance.hero_shadow_anchor) {
            const renderer::Vec3 point{
                instance.matrix[12], instance.matrix[13], instance.matrix[14]
            };
            if (std::all_of(point.begin(), point.end(), [](float value) {
                    return std::isfinite(value);
                })) {
                input.water_interactor = point;
            }
            break;
        }
    }
    // Shadow direction easing follows display time rather than the number of rendered frames.
    input.seconds = std::chrono::duration<double>(started.time_since_epoch()).count();
    input.direct_before_world =
        std::span<const std::uint8_t>(scene.packets).first(scene.before_world);
    input.level_drawn = (scene.draws & kLevelLayers) != 0;
    input.packets[static_cast<std::size_t>(renderer::Bucket::Hud)] =
        std::span<const std::uint8_t>(scene.packets).subspan(scene.before_world);
    set_effects(frame);
    input.sky_shells = scene.sky_shells;
    input.sky_shell_mask = scene.sky_shell_mask;
    // A frame between ticks turns each shell part way from the tick before's (Kept's
    // prev_sky_shells): taken whole, a shell that turns each tick (the loading screen's stars and
    // galaxy) stood still between ticks and stepped at each.
    if (frame.output.alpha < 1.0f) {
        for (std::size_t i = 0; i < input.sky_shells.size(); ++i) {
            if ((scene.sky_shell_mask & scene.prev_sky_shell_mask & (1u << i)) != 0) {
                input.sky_shells[i] = blend_turning(
                    scene.prev_sky_shells[i], scene.sky_shells[i], frame.output.alpha
                );
            }
        }
    }
    input.effect_uploads = scene.effect_uploads;
    input.ui_quads = scene.ui_quads;
    input.recording = scene.recording.empty() ? nullptr : &scene.recording;
    input.splices = scene.splices;
    input.splices_before_world = std::min(scene.splices_before_world, scene.splices.size());
    input.hud_packets_offset = scene.before_world;
    effects_dump::offer(input, input.water_interactor);
}

void set_target_mobys(SceneFrame& frame) {
    SceneRenderState& owner = frame.owner;
    const Kept& scene = frame.scene;
    frame.input.target_mobys = [&owner, &scene](
                                   std::uint64_t marker,
                                   unsigned framebuffer,
                                   int size,
                                   float centre_x,
                                   float centre_y,
                                   bool clear_depth
                               ) {
        draw_target_mobys(
            owner, scene, {marker, framebuffer, size, centre_x, centre_y, clear_depth}
        );
    };
    // Over a world drawn under a menu, the menu's mobys go on the screen at their markers.
    if (scene.world_under_menu) {
        frame.input.screen_mobys =
            [&owner,
             &scene](std::uint64_t marker, const renderer::RenderTarget& target, bool clear_depth) {
                draw_screen_mobys(owner, scene, marker, target, clear_depth);
            };
    }
}

void retain_background(SceneFrame& frame) {
    SceneRenderState& owner = frame.owner;
    const Kept& scene = frame.scene;
    auto& input = frame.input;
    const bool first = frame.output.first;
    const bool has_camera = frame.has_camera;
    const int width = frame.output.width;
    const int height = frame.output.height;
    // A tick that draws no world and has no picture of its own to put under its 2D (the page
    // menu's first ticks, before its snapshot of the frame comes; the tick it closes on): the
    // console still shows the last frame there, so the 2D goes over the last world drawn, not black.
    // Only right after the world was drawn (the menu opening) or right after a menu tick with its
    // snapshot (the menu closing): other ticks without a world (a new game's captions between its
    // scenes) are drawn on black, as they come.
    constexpr std::size_t kSnapshotBytes = 0x40000;
    const bool after_world = owner.last_world_tick != 0 && scene.frame - owner.last_world_tick <= 3;

    // A level's tick without a world goes over the picture shown last (frontend/tick_picture.h):
    // the frame keeps its colour, and only the tick's first frame draws its 2D.
    if (owner.overdraw) {
        owner.hold_last_world = false;
        input.keep_colour = true;
        return;
    }

    // A newly opened page may omit the snapshot transfer for its first few ticks.
    if (first && has_camera && (scene.draws & 0x0Fu) == 0 && scene.before_world < kSnapshotBytes
        && (after_world || owner.last_tick_menu) && owner.last_world.width() == width
        && owner.last_world.height() == height) {
        owner.hold_last_world = true;
    } else if (first) {
        owner.hold_last_world = false;
    }

    // Pause-menu packet gaps reuse the retained world instead of clearing to black.
    if (owner.hold_last_world && !owner.repeat_frame && owner.last_world.width() == width
        && owner.last_world.height() == height) {
        owner.last_world.blit_to(owner.frame.id(), width, height);
        owner.frame.samples_written();
        input.keep_colour = true;
    }

    // The last frame that drew its world, as the world alone (before the HUD's 2D goes over it):
    // what the page menus read back for their background, and what a tick without a world of its
    // own is drawn over. Only a tick that drew the level (kLevelLayers): one that drew mobys alone
    // drew them over the picture held above, and kept as the world, the next tick held that again
    // and drew them over it once more. Going Commando's weapon upgrade turned the weapon so for
    // its whole length: every pose it had shown stayed on the screen, over the level's last frame.
    if (first && has_camera && (scene.draws & kLevelLayers) != 0 && width > 0 && height > 0) {
        const bool temporal = frame.temporal;
        input.before_hud = [&owner, &scene, width, height, temporal]() {
            // Size changes invalidate the retained world attachment.
            if (owner.last_world.width() != width || owner.last_world.height() != height) {
                std::string error;
                if (!owner.last_world.create_colour(width, height, error)) {
                    log::warn("world snapshot unavailable: {}", error);
                    return;
                }
            }

            // The copy ends the frame's pass. When the HUD does not test the world's depth, the
            // depth is let go first: a tiled GPU (a phone's) then neither writes it out nor
            // reads it back for the HUD's pass.
            if (!temporal && !owner.renderer->hud_reads_depth()
                && gl::glInvalidateFramebuffer != nullptr) {
                const gl::GLenum dropped[] = {gl::GL_DEPTH_ATTACHMENT, gl::GL_STENCIL_ATTACHMENT};
                gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, owner.frame.id());
                gl::glInvalidateFramebuffer(gl::GL_FRAMEBUFFER, 2, dropped);
            }
#if defined(OPENRAC_VULKAN)
            // Nothing draws on the frame after this: its samples are resolved now and let go, so
            // a tiled GPU does not write them out, and the copy reads the resolved colour.
            if (!temporal && !owner.renderer->hud_draws() && gl::is_vulkan()
                && owner.frame.samples() > 1 && !owner.frame.resolves_itself()
                && !owner.frame.samples_let_go()
                && vkgl::resolve_dropping_samples(owner.frame.id(), owner.frame.resolve_target())) {
                owner.frame.samples_dropped();
            }
#endif
            owner.frame.blit_to(owner.last_world.id(), width, height);
            owner.last_world_tick = scene.frame;
            // A world under a menu: the menu's picture of the frame (read back once as it opened)
            // is drawn from this one, so the world goes on behind it.
            if (scene.world_under_menu) {
                refresh_frame_snapshot(owner.last_world);
            }
        };
    }
}

}  // namespace openrac::frontend

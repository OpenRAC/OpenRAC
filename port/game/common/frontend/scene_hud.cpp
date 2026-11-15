// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Binds captured HUD data and offscreen menu objects without accessing live game memory.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/log.h"
#include "frontend.h"
#include "game/common/frontend/effects_dump.h"
#include "game/common/frontend/geometry.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "renderer/direct/direct.h"
#include "renderer/gl/gl.h"

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
void draw_target_mobys(State& owner, const Kept& scene, const MobyTarget& target) {
    const std::uint32_t first = static_cast<std::uint32_t>(target.marker);
    // The HUD marker packs the first game address below a 16-bit instance count.
    const std::uint32_t count = static_cast<std::uint32_t>(target.marker >> 32) & 0xFFFFu;
    std::vector<scene::Instance> instances;
    const host::GameState* camera = nullptr;

    // The captured page objects occupy consecutive 256-byte moby records.
    for (const Kept::TargetMoby& moby : scene.target_mobys) {
        // Other markers draw different ranges of the captured menu objects.
        if (!moby.on_screen && moby.address >= first && moby.address < first + count * 0x100u) {
            instances.push_back(moby.instance);
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
}

// Draws a menu's mobys on the screen over the world drawn under it (frontend/menu_layers.h), with
// the camera and the frustum the menu drew them with, into `target` (bound). `clear_depth`: the
// first of the frame, which no longer test against the world's depth.
void draw_screen_mobys(
    State& owner,
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

// Moves `point`, where the effects of the tick put it on a moby the tick drew with `now`, to the same
// place on the moby drawn with `drawn` (a frame between ticks): through the moby's own axes, so an
// effect on a moby that turns as it moves (the ship's flame in a barrel roll) stays where it is on
// it. Axes without an inverse move it by the position alone.
void follow_moby(const renderer::Mat4& now, const renderer::Mat4& drawn, float point[3]) {
    // The axes' inverse by cofactors: column c of `now` is axis c (elements c * 4 .. c * 4 + 2).
    const float a = now[0], b = now[4], c = now[8];
    const float d = now[1], e = now[5], f = now[9];
    const float g = now[2], h = now[6], i = now[10];
    const float co[9] = {
        e * i - f * h,
        c * h - b * i,
        b * f - c * e,
        f * g - d * i,
        a * i - c * g,
        c * d - a * f,
        d * h - e * g,
        b * g - a * h,
        a * e - b * d,
    };
    const float det = a * co[0] + b * co[3] + c * co[6];
    if (!(std::fabs(det) > 1e-12f)) {
        for (int k = 0; k < 3; ++k) {
            point[k] += drawn[12 + k] - now[12 + k];
        }
        return;
    }
    const float r[3] = {point[0] - now[12], point[1] - now[13], point[2] - now[14]};
    float local[3];
    for (int k = 0; k < 3; ++k) {
        local[k] = (co[k * 3] * r[0] + co[k * 3 + 1] * r[1] + co[k * 3 + 2] * r[2]) / det;
    }
    for (int k = 0; k < 3; ++k) {
        point[k] =
            drawn[k] * local[0] + drawn[4 + k] * local[1] + drawn[8 + k] * local[2] + drawn[12 + k];
    }
}

// The moves of the mobys that carry their effect quads (EffectQuad::parent), by address: this
// tick's matrix and the one this frame draws the moby with, part way from the last tick's
// (blend_matrix). Only an owner that is not drawn itself but has a drawn moby at its very place,
// its body: level 5's ship, 0x018C7400 in the challenge, whose flame func_L05_004186D8 places by
// its rotation +0xC0 and position +0x10, while its body is drawn as other mobys at its place
// (classes 4305 to 4325). Its quads move as that body does. Others' quads stay where the tick has
// them: a callback's argument is often a moby whose quads are not on it (a Lancer shot's trail
// reaching back along its path, beams to far points), and turning those with the moby tore them
// into stray lines.
std::unordered_map<std::uint32_t, std::pair<renderer::Mat4, renderer::Mat4>> owner_moves(
    const Kept& scene, float alpha
) {
    std::unordered_map<std::uint32_t, std::pair<renderer::Mat4, renderer::Mat4>> moves;
    if (alpha >= 1.0f || !scene.has_prev_state) {
        return moves;
    }
    std::unordered_set<std::uint32_t> drawn_here(scene.addresses.begin(), scene.addresses.end());
    std::unordered_set<std::uint32_t> owners;
    for (const renderer::EffectQuad& q : scene.effects) {
        if (q.parent != 0 && !drawn_here.contains(q.parent)) {
            owners.insert(q.parent);
        }
    }
    for (const host::LiveMoby& m : scene.state.mobys) {
        if (!owners.contains(m.address)) {
            continue;
        }
        // The drawn moby at its place this tick, drawn the tick before too.
        for (std::size_t n = 0; n < scene.live.size() && n < scene.addresses.size(); ++n) {
            const renderer::Mat4& now = scene.live[n].matrix;
            const float dx = now[12] - m.matrix[12], dy = now[13] - m.matrix[13],
                        dz = now[14] - m.matrix[14];
            const auto last = scene.prev_matrices.find(scene.addresses[n]);
            if (dx * dx + dy * dy + dz * dz < 1e-6f && last != scene.prev_matrices.end()) {
                moves.emplace(m.address, std::pair{now, blend_matrix(last->second, now, alpha)});
                break;
            }
        }
    }
    return moves;
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
    input.strip_vertices = scene.strip_vertices;
    input.strips = scene.strips;
    input.direct_before_world =
        std::span<const std::uint8_t>(scene.packets).first(scene.before_world);
    input.level_drawn = (scene.draws & kLevelLayers) != 0;
    input.packets[static_cast<std::size_t>(renderer::Bucket::Hud)] =
        std::span<const std::uint8_t>(scene.packets).subspan(scene.before_world);
    input.effects = scene.effects;
    // Between ticks, each quad with an id moves from its last tick's corners as the mobys move
    // (Kept::prev_effect_corners); one that jumped further than anything carried moves (a slot
    // freed and taken again in a tick) and one new this tick are drawn where the tick has them.
    // A rigid quad (EffectQuad::rigid: a sprite or flat quad) keeps this tick's shape and moves by
    // its centre: Going Commando turns the Lancer's glow to a random angle every tick (its
    // spawner, level 3's 0x0033F6B0, as retail does), and corners taken part way between two
    // angles drew it shrunken, at times to a point, on every frame between ticks: a flicker, at
    // 60 fps too, whose frames do not fall on the ticks.
    // A quad on a moby's body (EffectQuad::parent, owner_moves: the ship's engine flame in the
    // space fights) moves with the body as this frame draws it, through its axes: left at the
    // tick's place, the flame ran ahead of the ship, into its middle as it flew and out to the side
    // in a barrel roll.
    const float alpha = frame.output.alpha;
    const bool owned = std::any_of(scene.effects.begin(), scene.effects.end(), [](const auto& q) {
        return q.parent != 0;
    });
    if (alpha < 1.0f && (owned || !scene.prev_effect_corners.empty())) {
        constexpr float kFarthest = 4.0f;  // world units a tick
        frame.effects = scene.effects;
        const auto parents = owner_moves(scene, owned ? alpha : 1.0f);
        for (renderer::EffectQuad& q : frame.effects) {
            if (q.parent != 0) {
                const auto parent = parents.find(q.parent);
                if (parent != parents.end()) {
                    for (auto& corner : q.corner) {
                        follow_moby(parent->second.first, parent->second.second, corner);
                    }
                    continue;
                }
            }
            if (q.id == 0) {
                continue;
            }
            const auto last = scene.prev_effect_corners.find(q.id);
            if (last == scene.prev_effect_corners.end()) {
                continue;
            }
            const auto& from = last->second;
            float d[3];
            for (int i = 0; i < 3; ++i) {
                d[i] = (q.corner[0][i] + q.corner[3][i] - from[0][i] - from[3][i]) * 0.5f;
            }
            if (!(d[0] * d[0] + d[1] * d[1] + d[2] * d[2] < kFarthest * kFarthest)) {
                continue;
            }
            if (q.rigid) {
                for (int k = 0; k < 4; ++k) {
                    for (int i = 0; i < 3; ++i) {
                        q.corner[k][i] -= d[i] * (1.0f - alpha);
                    }
                }
                continue;
            }
            for (int k = 0; k < 4; ++k) {
                for (int i = 0; i < 4; ++i) {
                    q.corner[k][i] = from[k][i] + (q.corner[k][i] - from[k][i]) * alpha;
                }
            }
        }
        input.effects = frame.effects;
    }
    input.sky_shells = scene.sky_shells;
    input.sky_shell_mask = scene.sky_shell_mask;
    input.effect_uploads = scene.effect_uploads;
    input.ui_quads = scene.ui_quads;
    effects_dump::offer(input, input.water_interactor);
}

void set_target_mobys(SceneFrame& frame) {
    State& owner = frame.owner;
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
    State& owner = frame.owner;
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
        input.keep_colour = true;
    }

    // The last frame that drew its world, as the world alone (before the HUD's 2D goes over it):
    // what the page menus read back for their background, and what a tick without a world of its
    // own is drawn over. Only a tick that drew the level (kLevelLayers): one that drew mobys alone
    // drew them over the picture held above, and kept as the world, the next tick held that again
    // and drew them over it once more. Going Commando's weapon upgrade turned the weapon so for
    // its whole length: every pose it had shown stayed on the screen, over the level's last frame.
    if (first && has_camera && (scene.draws & kLevelLayers) != 0 && width > 0 && height > 0) {
        input.before_hud = [&owner, &scene, width, height]() {
            // Size changes invalidate the retained world attachment.
            if (owner.last_world.width() != width || owner.last_world.height() != height) {
                std::string error;
                if (!owner.last_world.create_colour(width, height, error)) {
                    log::warn("world snapshot unavailable: {}", error);
                    return;
                }
            }

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

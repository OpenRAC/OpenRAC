// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The level renderer's lighting hook: what a LevelScene asks of the lighting that is not its own,
// the enhanced renderer's probe lighting and early lighting (enhanced/viewer/scene_lighting.h).
// The scene owns the interface and calls it at the points its passes did before restructure
// package P2-39b; whoever makes a LevelScene registers one (LevelScene::set_level_lighting), so the
// scene never includes enhanced/ (CODING_CONVENTIONS.md, section 18).
//
// A scene with none registered lights as the game does: its shaders are built without the probe
// GLSL, as when enhanced rendering is unavailable, and it reports no probe data, so neither the
// probes, the early lighting nor the shadows that need them are drawn.

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "scene/level.h"

namespace openrac::scene {
class LevelScene;

// The probe and early lighting of a level, as the level renderer calls it. Created, used and
// released on the thread that holds the graphics context.
class LevelLighting {
public:
    LevelLighting() = default;
    virtual ~LevelLighting() = default;
    LevelLighting(const LevelLighting&) = delete;
    LevelLighting& operator=(const LevelLighting&) = delete;

    // ---- Shaders ----

    // Returns the scene's mesh fragment shader `source` with the lighting's GLSL put in.
    virtual std::string mesh_fragment(std::string source) const = 0;

    // Enhanced surface history uses the already submitted hero pose and joint palette.
    virtual std::string mesh_vertex(std::string source) const { return source; }

    // Optional prepass specialisation for static, dry groups. Unknown providers retain their
    // complete vertex shader; approving this source promises identical positions and normals.
    virtual std::optional<std::string> rigid_depth_vertex(std::string) const {
        return std::nullopt;
    }

    // Optional dry-caster specialisation. Providers keep their complete transform by default.
    virtual std::string dry_shadow_vertex(std::string source) const {
        return mesh_vertex(std::move(source));
    }

    virtual void prepare_wetness(
        const renderer::FrameInput&, unsigned, const std::optional<Instance>&
    ) {}

    virtual void bind_wetness(const renderer::Shader&) const {}

    // Update optional surface deformation before any depth, shadow or colour consumers.
    virtual void prepare_deformation(const renderer::FrameInput&) {}

    // Bind per-group deformation resources; false retains original mesh positions.
    virtual void bind_deformation(const renderer::Shader&, bool) const {}

    // Temporal history is retained when the optional deformation is inactive.
    virtual bool deformation_active() const { return false; }

    // Optional shared camera-medium transport, prepared once after the current shadows.
    virtual bool atmosphere_wanted(const renderer::FrameInput&) const { return false; }

    virtual void prepare_atmosphere(
        const LevelScene&,
        const renderer::FrameInput&,
        const renderer::RenderTarget&,
        const std::array<float, 4>&,
        const std::array<float, 4>&
    ) {}

    virtual void draw_atmosphere_sky(const renderer::RenderTarget&) {}

    virtual std::string atmosphere_source() const { return {}; }

    virtual void bind_atmosphere(const renderer::Shader&) const {}

    // Optional shading of the authored material on the current model.
    virtual void bind_material(const renderer::Shader&, std::uint32_t, const Material&) const {}

    // Returns the scene's mesh fragment shader `source` as the early-lit variant draws it.
    virtual std::string early_fragment(std::string source) const = 0;

    // Returns the configured primary sunlight direction, or the authored direction without an override.
    virtual renderer::Vec3 sunlight_direction(renderer::Vec3 authored) const = 0;

    // Returns whether this GPU and build can draw the early lighting at all.
    virtual bool early_supported() const = 0;

    // ---- The level ----

    // Finds and uploads the lighting of `level` (its probe bake), once it is on the GPU.
    virtual void load(const LevelData& level) = 0;

    // Frees its GL objects; the context is still current.
    virtual void release() = 0;

    // ---- State ----

    // Returns whether probe data is uploaded.
    virtual bool has_data() const = 0;

    // Why this level's bake was rejected, if loading failed.
    virtual std::string_view load_error() const { return {}; }

    // Returns whether probe lighting is on and has data.
    virtual bool active() const = 0;

    // Returns the probe view (probes.glsl's probe_mode): 0 original, 1 combined, 2 indirect,
    // 3 direct, 4 coverage.
    virtual int mode() const = 0;

    // Sets the probe view of mode().
    virtual void set_mode(int mode) = 0;

    // Turns probe lighting on or off and sets its multiplier.
    virtual void set_lighting(bool enabled, float multiplier) = 0;

    // Sets the normal and view biases of the probe lookups, in scene units.
    virtual void set_bias(float normal, float view) = 0;

    // Runtime primary-light override; null restores the verified profile/authored direction.
    virtual void set_sunlight_direction(std::optional<renderer::Vec3> /*direction*/) {}

    // Shows or hides the probe placement overlay.
    virtual void set_spheres(bool shown) = 0;

    // Shows generated ceiling-source markers when the lighting provider supports them.
    virtual void set_area_lights(bool /*shown*/) {}

    // Shows capture positions and reflection influence boxes when the provider supports them.
    virtual void set_medial_axis_debug(bool /*shown*/) {}

    virtual void set_environment_probe_debug(bool /*shown*/) {}

    // Returns whether the probe regions are chosen on the GPU from the frame's depth.
    virtual bool compute_selection() const = 0;

    // Chooses the probe regions the frame's `depth` sees, on the GPU.
    virtual void select_visible_regions(
        unsigned depth, const renderer::Camera& camera, int width, int height
    ) = 0;

    // Streams the probe regions around `camera`; `seconds` since the last call, or -1 at a jump.
    virtual void update_regions(const renderer::Camera& camera, float seconds) = 0;

    // ---- Drawing ----

    // Draws placement overlays using the snapshot player position when available.
    virtual void draw_spheres(
        const renderer::FrameInput& input,
        renderer::RenderState& state,
        std::optional<renderer::Vec3> player = std::nullopt
    ) = 0;

    // Sets the probe lighting's uniforms and textures on `shader` for `layer`.
    virtual void bind(
        const renderer::Shader& shader,
        Layer layer,
        bool all_static_receivers,
        const renderer::Camera* camera,
        bool depth_matched
    ) const = 0;

    // Draws the early lighting from the world's `depth`; `bind_lighting` sets the scene's own
    // lighting on its shader. Returns whether it drew, with `error` when it could not.
    virtual bool render_early(
        unsigned depth,
        const renderer::FrameInput& input,
        const renderer::RenderTarget& target,
        const std::function<void(const renderer::Shader&)>& bind_lighting,
        std::string& error
    ) = 0;

    // Sets the early lighting's result on `shader`.
    virtual void bind_early(
        const renderer::Shader& shader,
        const renderer::FrameInput& input,
        const renderer::RenderState& state
    ) const = 0;

    // Returns whether the early lighting drawn this frame matches `input` and `state`.
    virtual bool early_matches(
        const renderer::FrameInput& input, const renderer::RenderState& state
    ) const = 0;
};

}  // namespace openrac::scene

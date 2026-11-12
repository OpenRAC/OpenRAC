// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The viewer's renderers: a loaded level on the GPU, drawn through the port's
// renderer (renderer/renderer.h) as one BucketRenderer per layer, in the
// game's order: the sky shells, then terrain, ties, shrubs and mobys.
//
// Every mesh shares one vertex and one index buffer; the instances of a class
// are drawn with one instanced call per material, their matrices in an
// instance buffer. Live mobys are skinned in the vertex shader with the joint
// palettes the game's animation fields give (moby_pose.h), from a float
// texture each instance indexes. Textures go through the renderer's texture pool. The
// conventions are the game's: Z up, reversed depth (cleared to 0, GEQUAL).
// Lighting is a fixed sun and ambient, only so shapes read: the game's own
// lights (tie and shrub light indices, the level's directional lights) are
// not applied yet.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "renderer/debug_lines.h"
#include "renderer/framebuffer.h"
#include "renderer/gpu_draw.h"
#include "renderer/render_opt.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/world/gpu.h"
#include "scene/level.h"
#include "scene/level_lighting.h"
#include "scene/motion.h"
#include "scene/shadow_atlas.h"
#include "scene/shadow_draws.h"
#include "scene/shadow_scroll.h"
#include "scene/shadow_transition.h"
#include "scene/shrub_draw.h"
#include "scene/sun_shadow.h"

namespace openrac::scene {

// The ties and shrubs as the game holds them this tick (GameProfile::scenery_matrices), one per
// instance as loaded in each layer's order: the matrices' first three columns and translation; and
// per tie its bounding sphere and the point lights its record lists (+0x1A, four nibbles, 0xF
// ends). Empty: drawn as loaded.
struct GameScenery {
    std::array<std::vector<std::array<float, 12>>, 2> matrices;
    std::vector<std::array<float, 4>> tie_spheres;
    std::vector<std::uint16_t> tie_lights;
    // The same of the shrubs (their records' +0 and +0x1E).
    std::vector<std::array<float, 4>> shrub_spheres;
    std::vector<std::uint16_t> shrub_lights;
    // Per tie and per shrub, nonzero where the game has hidden it in its record: a tie's +0x1E bit
    // 2, a shrub's +0x16 bit 0, which Going Commando's TieProc (0x2F133C) and ShrubProc
    // (0x2E15B0) test to leave it undrawn. Empty: none hidden.
    // Going Commando's frontend also leaves out the ones its culls leave undrawn this tick: a tie
    // whose occlusion bit the frame's mask clears, and a tie or shrub past its draw distance (at
    // most the level's cap; at "Draw distance: Unlimited" the cap alone).
    std::vector<std::uint8_t> tie_hidden;
    std::vector<std::uint8_t> shrub_hidden;
    // Each terrain fragment's occlusion word, in the game's fragment order (its tfrag records'
    // +0x3B byte and +0x3A bit), for a game whose frame mask stands in for a baked grid
    // (LevelScene::set_game_pvs). Empty: the words as loaded.
    std::vector<std::uint16_t> terrain_occlusion;
    // Per terrain fragment, nonzero where it is past the terrain's far cull this tick (the
    // level's, 960 units, times Draw distance). Empty: none.
    std::vector<std::uint8_t> terrain_far;
};

class LevelScene {
public:
    // Supplies optional visual deformation to external velocity passes without owning the provider.
    std::string deformation_vertex(std::string source) const;
    // Optional surface-layer alpha coverage for the object's velocity pass.
    std::string motion_fragment(std::string source) const;
    // Reports actual resource-backed deformation, not merely a model's eligibility.
    bool deformation_active() const;
    // Authored anchors of water attachments, shared by every deformation pass.
    void floating_anchors(std::vector<std::pair<renderer::Vec3, std::uint64_t>>& anchors) const;

    // Upload a level; the GL context must be current.
    bool upload(const LevelData& level, renderer::TexturePool& textures, std::string& error);
    void release();

    void draw_layer(Layer layer, const renderer::FrameInput& input, renderer::RenderState& state);
    // The mod-only animated scenery layer at this presentation time (seconds since startup).
    std::vector<Instance> animated_instances(double seconds) const;
    bool needs_early_lighting(const renderer::FrameInput& input) const;

    // Registers the level's probe and early lighting (scene/level_lighting.h), replacing what it
    // had; without one the scene lights as the game does. Before upload.
    void set_level_lighting(std::unique_ptr<LevelLighting> provider) {
        m_lighting = std::move(provider);
    }

    // The registered lighting, or null.
    LevelLighting* level_lighting() const { return m_lighting.get(); }

    bool probe_data_available() const { return m_lighting && m_lighting->has_data(); }

    void render_early_lighting(
        unsigned depth, const renderer::FrameInput& input, renderer::RenderState& state
    );
    bool enable_hero_shadow(bool enabled, std::string& error);

    bool hero_shadow_enabled() const { return m_hero_enabled; }

    bool enable_sun_shadow(bool enabled, std::string& error);

    bool shadow_atlas_active() const { return m_shadow_atlas_active; }

    bool batches_opaque_colour() const;
    bool colour_blend_barrier(Layer layer) const;
    // Borrows view-matched lighting buffers owned by the registered provider through this frame.
    std::span<const unsigned> opaque_colour_buffers(
        const renderer::FrameInput& input, const renderer::RenderState& state
    ) const;

    bool sun_shadow_enabled() const { return m_sun_enabled; }

    struct CascadeCulling {
        std::uint64_t candidates = 0, rejected = 0, submitted = 0, unbounded = 0,
                      compacted_groups = 0;
        std::uint64_t solid_draws = 0, alpha_draws = 0;
    };

    const std::array<CascadeCulling, kSunCascades>& sun_culling() const { return m_sun_culling; }

    struct CameraCulling {
        std::uint64_t candidates = 0, rejected = 0, submitted = 0, unbounded = 0;
    };

    const std::array<CameraCulling, kLayerCount>& camera_culling() const {
        return m_camera_culling;
    }

    // Replaces a layer's instances (live mobys, each frame).
    void set_instances(Layer layer, const std::vector<Instance>& instances);

    // Draws the faces of the level's swapped texture (LevelData::variant_image: Going Commando's
    // ship paint jobs) with the game's `number`; one the level does not have leaves the one bound.
    void set_variant(int number);

    // Draw distance (settings.h) for the ties and shrubs, from the camera at `eye` seeing through
    // `projection`: those beyond their draw distances (LevelData::draw_distances) times `scale` left
    // out, every one drawn at its most detailed LOD. A layer is grouped again only when one of its
    // instances changes, so the test holds back by a few units: an instance leaves only some way
    // past its distance, by straight-line distance however the camera turns; it comes back as soon
    // as the game would draw it. Scale 0 (Unlimited): the instances as loaded, never set again
    // unless a distance was in use.
    void choose_scenery(float scale, const renderer::Vec3& eye, const renderer::Mat4& projection);

    // ShrubProc's choice for the tick (GameProfile::shrub_draws), one per shrub as loaded: at
    // "Draw distance: Original" the shrubs' layer then keeps those it draws whole (choose_scenery)
    // and add_shrub_effects the rest. Empty, or not one per shrub: the draw distances alone.
    void set_shrub_draws(std::vector<ShrubDraw> draws);

    // The fading shrubs and the billboards drawn in place of the far ones, as the live mobys' list
    // takes them (Instance::blend 4 and 5), appended to `live`.
    void add_shrub_effects(std::vector<Instance>& live) const;

    // The ties' and the shrubs' class-to-world matrices as the game holds them this tick
    // (GameProfile::scenery_matrices), one per instance as loaded in each layer's order, the
    // matrix's first three columns and its translation; empty: drawn as loaded. An instance the
    // game moved leaves its layer and is drawn with the mobys (add_moved_scenery) where the game
    // puts it; one whose matrix the game zeroed, or that its record hides, is not drawn. A layer
    // is grouped again only when the set of moved instances changes.
    // A tie that a point light reaches (its list) is lit again with it each tick, as the game does.
    void move_scenery(const GameScenery& scenery);

    // The instances move_scenery found moved, at their place now, appended to `live`, of the
    // scenery layers `layers` names (0 ties, 1 shrubs: the ones whose pass the game ran this tick);
    // `at`, when given, places each (scenery layer, instance, matrix now) for a frame drawn between
    // ticks.
    void add_moved_scenery(
        std::vector<Instance>& live,
        const std::array<bool, 2>& layers,
        const std::function<Mat4(std::size_t, std::size_t, const Mat4&)>& at = {}
    ) const;

    bool has_moved_scenery() const { return m_moved_count > 0; }

    // Draws depth-tested rigid and skinned velocities into the caller's target, without changing history.
    void draw_motion(
        const renderer::Shader& shader,
        std::span<const MotionInstance> instances,
        const MotionView& view,
        renderer::TexturePool& textures,
        MotionSubmission submission = MotionSubmission::Batched
    );

    // Draws these mobys now, as the moby layer draws its own, into what is bound (the game's
    // render to a texture: the vendor's screens); the layer's own instances are kept.
    void draw_mobys(
        const std::vector<Instance>& instances,
        const renderer::FrameInput& input,
        renderer::RenderState& state
    );

    // One joint matrix of a palette, as the vertex shader takes it: four columns (the images of
    // the axes, then the translation, in the mesh's units), column-major.
    using JointColumns = std::array<float, 16>;

    // Replaces the joint palettes the mobys' instances index (Instance::palette), each frame.
    void set_palette(const std::vector<JointColumns>& matrices);

    // The owned displayed palette, shared read-only with registered attachment providers.
    std::span<const renderer::Mat4> joint_palette() const { return m_palette_data; }

    // Draws requested visibility diagnostics using the displayed camera, never the culling camera.
    void draw_culling_debug(const renderer::FrameInput& input, renderer::RenderState& state);

    bool lighting = true;

    bool probes_active() const { return m_lighting && m_lighting->active(); }

    bool compute_probe_selection() const { return m_lighting && m_lighting->compute_selection(); }

    void select_visible_probe_regions(
        unsigned depth, const renderer::Camera& camera, int width, int height
    ) {
        // Without registered lighting there are no probe regions to choose.
        if (m_lighting) {
            m_lighting->select_visible_regions(depth, camera, width, height);
        }
    }

    void set_probe_lighting(bool enabled, float multiplier) {
        // Without registered lighting there is no probe lighting to switch.
        if (m_lighting) {
            m_lighting->set_lighting(enabled, multiplier);
        }
    }

    // The settings sweep's experiments (debug/capture.cpp): the blended passes of every layer
    // (static translucent texels, the mobys' translucent pass) left out, drawn without probe
    // light, or drawn with the full (not the cheaper) probe light.
    void set_sweep_translucent(bool off, bool unlit, bool full) {
        m_sweep_translucent_off = off;
        m_sweep_translucent_unlit = unlit;
        m_sweep_translucent_full = full;
    }

    // Another of the sweep's experiments: the mobys' glow, blended and metal passes left out (the
    // late passes other than their translucent one), to time them on a device.
    void set_sweep_moby_blended_off(bool off) { m_sweep_moby_blended_off = off; }

    // And another: every draw of the world's layers cut to its first triangle, in the prepass and
    // the colour passes alike. What is left is the cost of the draws themselves.
    void set_sweep_one_triangle(bool on) { m_sweep_one_triangle = on; }

    // Controls the GI-coloured edge light on essential gameplay objects.
    void set_gameplay_rim(bool enabled, float strength, float falloff) {
        m_gameplay_rim = {enabled ? strength : 0.0f, falloff};
    }

    // Compresses dim GI luminance for the rim only; exponent one leaves it unchanged.
    void set_gameplay_rim_lift(bool enabled, float exponent, float max_gain) {
        m_gameplay_rim_lift = {enabled ? exponent : 1.0f, max_gain};
    }

    void set_probe_bias(float normal, float view) {
        // Without registered lighting there are no probe lookups to bias.
        if (m_lighting) {
            m_lighting->set_bias(normal, view);
        }
    }

    void set_probe_spheres(bool shown) {
        // Without registered lighting there are no probes to show.
        if (m_lighting) {
            m_lighting->set_spheres(shown);
        }
    }

    // Shows generated ceiling sources through the registered lighting provider.
    void set_area_lights(bool shown) {
        // Ordinary rendering has no enhanced source diagnostics.
        if (m_lighting) {
            m_lighting->set_area_lights(shown);
        }
    }

    // Shows reflection diagnostics through the registered lighting provider.
    void set_medial_axis_debug(bool shown) {
        if (m_lighting) {
            m_lighting->set_medial_axis_debug(shown);
        }
    }

    void set_environment_probe_debug(bool shown) {
        if (m_lighting) {
            m_lighting->set_environment_probe_debug(shown);
        }
    }

    void update_probe_regions(const renderer::Camera& camera, float seconds = -1) {
        // Without registered lighting there are no probe regions to stream.
        if (m_lighting) {
            m_lighting->update_regions(camera, seconds);
        }
    }

    renderer::AoLighting ao_lighting() const {
        if (!probes_active() || m_lighting->mode() == 0) {
            return renderer::AoLighting::Legacy;
        }
        const int mode = m_lighting->mode();
        return mode == 1 || mode == 2 ? renderer::AoLighting::Indirect
                                      : renderer::AoLighting::Unoccluded;
    }

    void set_probe_view(int view) {
        // The five probe views (LevelLighting::mode), and lighting registered to show them.
        if (m_lighting && view >= 0 && view <= 4) {
            m_lighting->set_mode(view);
        }
    }

    void draw_probe_spheres(const renderer::FrameInput& input, renderer::RenderState& state) {
        // Without registered lighting there are no probes to draw.
        if (m_lighting) {
            m_lighting->draw_spheres(input, state, m_hero_centre);
        }
    }

    // Eight point lights after the 16 directional sets: colour, position/radius.
    void set_point_lights(const std::array<float, 64>& lights) { m_point_lights = lights; }

    // Immutable frame light bank for registered lighting providers.
    const std::array<float, 64>& point_lights() const { return m_point_lights; }

    void set_point_emitters(std::vector<renderer::PointEmitter> lights) {
        m_point_emitters = std::move(lights);
    }

    std::span<const renderer::PointEmitter> point_emitters() const { return m_point_emitters; }

    // Each point-lit moby's third light for the original lighting (colour, then world direction),
    // indexed by moby_light.x / 256 - 1 (mesh.vert); empty for a game whose moby program has none.
    void set_moby_point_lights(std::vector<std::array<float, 8>> lights) {
        m_moby_point_lights = std::move(lights);
    }

    // Whether the terrain's colours take the point lights (point_lights(), mesh.vert); off for a
    // game whose terrain program has none.
    void set_terrain_point_lights(bool on) { m_terrain_point_lights = on; }

    // The game's fog for this frame (GameState): FOGCOL 0..1, the depths in raw units (game
    // units x 1024) and the GS F at them (255 = none). The world layers are fogged as the GS
    // fogs them: C = FOGCOL + (C - FOGCOL) x F / 255, F linear in camera depth, clamped.
    void set_fog(
        const std::array<float, 3>& colour,
        float near_depth,
        float far_depth,
        float near_f,
        float far_f
    );

    // The light bank the mobys are lit from this frame (16 sets of 16 floats, as the game holds
    // it); until set, the level's exported sets.
    // Shared lighting for optional world materials outside the mesh renderer.
    std::string atmosphere_source() const;
    void bind_atmosphere(const renderer::Shader&) const;
    void bind_surface_lighting(
        const renderer::Shader& shader, const renderer::Camera& camera
    ) const;

    // The game's occlusion mask this tick (std::nullopt when the game has none).
    void set_game_pvs(const std::optional<Pvs::Mask>& mask) { m_pvs.game = mask; }

    void set_light_sets(const std::vector<float>& sets) {
        if (sets.size() == 16 * 16) {
            m_light_sets = sets;
        }
    }

private:
    // Returns the selected preview clip matrix, or no value when its shadow maps are unavailable.
    [[nodiscard]] std::optional<renderer::Mat4> inspection_clip(
        const renderer::FrameInput& input
    ) const;
    // One consecutive run of compatible motion instances, preserving draw order.
    struct MotionBatch {
        std::uint32_t model = 0;       // Shared geometry and material runs.
        unsigned group = 0;            // Current moby geometry VAO.
        int first = 0;                 // First nine-texel pose record.
        int count = 0;                 // Instances submitted together.
        bool floats_on_water = false;  // Shared deformation selection for this batch.
    };

    // Builds stable pose records and batches; multi-material models retain instance-major order.
    void prepare_motion_batches(std::span<const MotionInstance> instances);
    // Draws the prepared pose-buffer batches using existing geometry and alpha textures.
    void draw_motion_batches(const renderer::Shader& shader, renderer::TexturePool& textures);
    renderer::world::RecordBuffer
        m_motion_records;                       // Owned current/previous poses and opacity flags.
    renderer::world::Words m_motion_words;      // Reused CPU packing storage.
    std::vector<MotionBatch> m_motion_batches;  // Consecutive compatible instance runs.
    std::vector<unsigned> m_motion_groups;      // Model-to-VAO lookup rebuilt for the live layer.
    bool m_motion_records_ready = false;        // Includes the reference path's empty record.
    // Draws one temporal object using the existing geometry VAO and current joint palette.
    void draw_motion_instance(
        const renderer::Shader& shader,
        const MotionInstance& instance,
        renderer::TexturePool& textures
    );
    // Applies original alpha coverage to each material run of a temporal object.
    void draw_motion_materials(
        const renderer::Shader& shader,
        std::uint32_t model,
        renderer::TexturePool& textures,
        int instances = 1,
        bool floats_on_water = false
    );

    struct CasterBounds {
        renderer::Vec3 low{}, high{};
        bool valid = false;
    };

    // One instance, as the vertex shader's per-instance attributes take it.
    struct InstanceData {
        float matrix[16];
        float tint[4];
        float palette;
        float lights;
        float moby_light[4];
        float moby_ambient[4];
    };

    struct Group {
        bool hero_shadow_caster = false;
        bool gameplay_essential = false;
        bool floats_on_water = false;
        bool shadow_solid_instances = false;  // tint/glow cannot discard opaque static faces
        std::uint32_t model = 0;
        int blend = 0;  // Instance::blend of all its instances
        unsigned vao = 0;
        unsigned instances = 0;  // instance buffer
        int count = 0;
        // A static group's instances' box in the world, for leaving it out when the camera does
        // not see it (draw_layer); bounded false: always drawn (live mobys).
        bool bounded = false;
        renderer::Vec3 low{}, high{};
        std::vector<InstanceData> cpu_instances;
        std::vector<std::uint64_t> floating_ids;  // CPU only, independent of GPU instance layout.
        std::vector<CasterBounds> caster_bounds;
        std::vector<renderer::GpuCullBounds> gpu_bounds;
        renderer::GpuInstanceBatch gpu_camera, gpu_pvs;
        std::vector<CasterBounds> shadow_geometry_bounds;
        std::vector<std::uint16_t> occlusion;
        unsigned pvs_vao = 0, pvs_instances = 0;
        int pvs_count = 0;
        bool pvs_valid = false;
        Pvs::Mask pvs_mask{};
        unsigned camera_vao = 0, camera_instances = 0;
        int camera_count = 0;
        bool camera_valid = false;
        renderer::Mat4 camera_clip{};
    };

    // Checks whether the registered transport provider can prepare stable graph declarations.
    bool can_prepare_atmosphere(const renderer::FrameInput& input) const;
    // Prepares current shadows and transport once, within a caller-owned execution scope.
    void prepare_atmosphere_inputs(const renderer::FrameInput& input, renderer::RenderState& state);
    // Prepares transport before terrain colour when the level has no sky bucket.
    void prepare_atmosphere(const renderer::FrameInput& input, renderer::RenderState& state);
    // Queues authored sky and its transport overlay after preparing their shadow/atmosphere inputs.
    void record_sky(const renderer::FrameInput& input, renderer::RenderState& state);
    // Executes eligible shadows, transport and sky from one shared plan; references stay immediate.
    void render_sky(const renderer::FrameInput& input, renderer::RenderState& state);
    // Draws authored sky shells with the original material blending and camera-relative transforms.
    void draw_sky(const renderer::FrameInput& input, renderer::RenderState& state);
    // Updates the hero projection and renders its separate map when the atlas is unavailable.
    void prepare_hero_shadow(const renderer::FrameInput& input, renderer::RenderState& state);
    // Updates cascade/caster views and renders separate maps when the atlas is unavailable.
    void prepare_sun_shadow(const renderer::FrameInput& input, renderer::RenderState& state);
    // Joins an active lighting plan, or executes shadow production before atmosphere consumes it.
    void prepare_shadows(const renderer::FrameInput& input, renderer::RenderState& state);
    // Prepares stable shadow allocations and scoped caster callbacks without forcing execution.
    void prepare_shadow_passes(const renderer::FrameInput& input, renderer::RenderState& state);
    void set_shadow_quality(renderer::ShadowQuality quality);
    bool make_shadow_atlas(std::string& error);
    // Records atlas casters through the depth graph after preparing any persistent static caches.
    void draw_shadow_atlas(renderer::RenderState& state);
    // Unbinds earlier shadow sampling and refreshes the five atlas matrices and draw buckets.
    void prepare_shadow_atlas_draws();
    // Clears depth and optionally unused attachment contents before a complete atlas refresh.
    void clear_shadow_atlas(bool discard);
    // Records the unchanged sun/hero draw buckets, retaining copied static depth when requested.
    void record_shadow_atlas(
        unsigned scrolling, bool cache, bool discard, renderer::RenderState& state
    );
    // Clears and draws only the atlas views of `views` (one bit each, the hero's after the sun's):
    // one pass of a frame that keeps other cascades (shadow-far-alternate).
    void record_shadow_atlas_views(unsigned views, bool discard, renderer::RenderState& state);
    // Keeps the far cascades the last frame drew in place of this frame's while this frame's
    // receivers fit inside them (shadow-far-alternate); sets m_kept_cascades.
    void keep_far_cascades(bool level_bounds);
    // Restores the world target and releases attachment contents no lighting pass samples.
    void finish_shadow_atlas(bool discard, const renderer::RenderState& state);
    // Finishes unused attachment cleanup and publishes counters after every deferred shadow producer.
    void complete_shadow_atlas(
        const renderer::RenderState& state,
        bool discard,
        renderer::FrameStats before,
        unsigned rerendered
    );
    // Which casters a shadow map draws: all of them, or, for the static shadow cache (R-18), the
    // static layers (terrain, ties, shrubs) into the cache or the mobys over a copy of it.
    enum class ShadowCasters : std::uint8_t {
        All,
        Static,
        Dynamic,
        StaticOpaque,  // Persistent coverage: opaque, unskinned static layers only.
        DynamicAlpha,  // Mobys and all static coverage that can require alpha testing.
    };

    // A cache key decision is made before callbacks can change any persistent contents.
    struct StaticShadowUpdate {
        std::array<StaticShadowKey, kSunCascades> keys{};  // New complete cache identities.
        std::array<bool, kSunCascades> missed{};           // Rectangles that need redrawing.
        unsigned count = 0;                                // Number of changed cascades.
        unsigned last = 0;   // Last changed cascade flushes draw buckets.
        bool fresh = false;  // A new allocation needs a complete clear.
    };

    // Compares current caster views with retained cache keys without issuing GPU commands.
    StaticShadowUpdate static_shadow_update() const;
    // Allocates a cache matching the atlas, retaining the original all-caster fallback on failure.
    bool make_static_shadow_target();
    // Clears the entire fresh cache, or only changed cascade rectangles of an existing cache.
    void clear_static_shadow_updates(const StaticShadowUpdate& update);
    // Records changed static casters as one graph phase after its clear/preserve declaration.
    void record_static_shadow_updates(
        const StaticShadowUpdate& update, renderer::RenderState& state
    );
    // Copies complete cached depth into the dynamic atlas under an explicit transfer dependency.
    void copy_static_shadow_depth();
    // Updates changed static cascades and copies cached depth into the atlas; returns the update count.
    // An absent cache on allocation failure tells the caller to clear and draw all casters instead.
    unsigned draw_cached_cascades(renderer::RenderState& state);
    // Binds `framebuffer`, the atlas or the cache, for depth-only shadow drawing over all of it.
    void begin_shadow_target(unsigned framebuffer);
    // Publishes cascade and geometry rejection counters after the queued caster draws record.
    void report_sun_culling() const;
    void draw_shadow_map(
        unsigned target,
        int resolution,
        const renderer::Mat4& matrix,
        bool hero,
        bool separate_hero,
        renderer::RenderState& state,
        unsigned cascade = kSunCascades,
        int atlas_view = -1,
        bool flush_atlas = false,
        ShadowCasters casters = ShadowCasters::All,
        const std::array<int, 4>* region = nullptr
    );
    // Independent toroidal caches; any failed cascade falls back to ordinary atlas drawing.
    void prepare_scrolling_shadows(
        renderer::Vec3 direction, renderer::Vec3 low, renderer::Vec3 high
    );
    bool make_scrolling_target(unsigned cascade);
    // Records dirty physical strips as one complete-clear or preserved-depth graph phase.
    void draw_scrolling_regions(unsigned cascade, renderer::RenderState& state);
    unsigned update_scrolling_shadows(renderer::RenderState& state);
    // Combines cached opaque and dynamic depth, restoring atlas casters when composition fails.
    void combine_scrolling_shadows(unsigned mask, renderer::RenderState& state);
    void release_scrolling_shadows();
    Group make_group(std::uint32_t model);
    void point_positions() const;
    void sync_position_stream();
    void fill_group(Group& g, const std::vector<InstanceData>& list, unsigned usage);
    void build_groups(Layer layer, const std::vector<Instance>& instances, bool live);
    // build_groups' parts, also for set_instances' in-place path (level_renderer.cpp says more).
    static std::uint32_t group_key(const Instance& instance);
    static InstanceData instance_data(const Instance& instance);
    static CasterBounds instance_bounds(
        const Model& shape,
        const InstanceData& data,
        const std::array<float, 4>& sphere,
        bool sphere_off,
        bool floats_on_water = false
    );
    static renderer::GpuCullBounds gpu_cull_bounds(
        const CasterBounds& bounds, const InstanceData& data, std::uint16_t occlusion
    );
    static bool solid_shadow_instances(const std::vector<InstanceData>& instances);
    static bool moby_sphere_culling_off();
    static bool same_palette(
        const std::vector<JointColumns>& matrices,
        const std::vector<JointColumns>& sent,
        std::size_t texels
    );
    // set_instances' in-place path for the mobys (moby-pose-in-place, RENDER_PERF.md R-16):
    // remember_mobys notes where build_groups put each moby, rewrite_mobys writes the next call's
    // poses there when the mobys and their order are the same, and returns false when they are not.
    void remember_mobys(const std::vector<Instance>& instances);
    bool rewrite_mobys(const std::vector<Instance>& instances);
    renderer::GpuInstanceBatch gpu_batch(const Group& group, const renderer::GpuCullView& view);
    void submit_instances(
        renderer::GpuInstanceBatch batch, unsigned first, unsigned count, int instances
    );

    std::vector<Model> m_models;
    std::vector<Instance> m_animated_base;
    std::vector<SpinAnimation> m_spins;

    // choose_scenery: per layer (the ties, the shrubs) the instances as loaded, their draw
    // distances, their bounds' world centres and whether each is drawn now, 1 or 0 (empty while
    // the setting is off). A layer whose export has no distances is never chosen; its source is
    // kept for the instances the game moves (move_scenery).
    struct Scenery {
        std::vector<Instance> source;
        std::vector<float> distances;
        std::vector<renderer::Vec3> centres;
        std::vector<std::uint8_t> drawn;
    };

    static constexpr std::array<Layer, 2> kSceneryLayers{Layer::Ties, Layer::Shrubs};
    std::array<Scenery, kSceneryLayers.size()> m_scenery;

    // move_scenery: per layer, each instance's state (0 as loaded, 1 moved, 2 removed) and its
    // matrix now; the instances a layer leaves out for it. `checked`: whether the game's matrices
    // were found to be the loaded ones (per layer: -1 not yet, 0 no, 1 yes).
    struct MovedScenery {
        std::vector<std::uint8_t> state;
        std::vector<Mat4> matrix;
        // A tie's matrix, light set and point lights when it was last lit again
        // (relight_moved_tie); lit_set 0xFF: its colours as loaded.
        std::vector<Mat4> lit;
        std::vector<std::uint8_t> lit_set;
        std::vector<std::array<float, 32>> lit_points;
        int checked = -1;
    };

    // The ties' relighting data (LevelData::tie_relight_classes, tie_relight, tie_slots) and their
    // colours as loaded, for putting a tie back.
    std::vector<TieRelightClass> m_tie_relight_classes;
    std::vector<TieRelight> m_tie_relight;
    std::vector<std::uint32_t> m_tie_slots;
    std::vector<std::uint32_t> m_light_colours;
    // The shrubs' relighting data (LevelData::shrub_relight_classes, shrub_relight).
    std::vector<std::array<std::array<std::int16_t, 4>, 24>> m_shrub_relight_classes;
    std::vector<LevelData::ShrubRelight> m_shrub_relight;
    // Lights shrub `i` again by `matrix` and the point lights `list` names, merged from `centre`
    // (nullptr: its colours as loaded), into the light texture.
    void relight_shrub(
        std::size_t i, const Mat4* matrix, std::uint16_t list, const std::array<float, 4>& centre
    );
    // Writes `colours` into the light texture from texel `first`.
    void upload_light_colours(std::size_t first, std::span<const std::uint32_t> colours);
    // Lights tie `i` again by `matrix` and `points` (nullptr: its colours as loaded) into the
    // light texture.
    void relight_moved_tie(
        std::size_t i,
        const Mat4* matrix,
        std::span<const TiePointLight> points = {},
        const std::array<float, 4>& sphere = {}
    );
    std::array<MovedScenery, kSceneryLayers.size()> m_moved;
    std::size_t m_moved_count = 0;  // instances moved now, over both layers
    // A layer's instances as the scene draws them: its chosen ones (choose_scenery) or all of
    // them, without the ones the game moved or removed.
    std::vector<Instance> resident_scenery(std::size_t k) const;
    // set_shrub_draws: the tick's choice; per shrub as loaded its billboard's model (-1 none) and
    // its average lit colour (0x80 = 1.0), the billboard's.
    std::vector<ShrubDraw> m_shrub_draws;
    std::vector<int> m_shrub_billboards;
    std::vector<std::array<float, 4>> m_shrub_colours;
    // m_shrub_draws holds one per shrub as loaded, each with its billboard and colour.
    bool shrub_draws_usable() const;
    std::vector<std::vector<bool>> m_shadow_solid_primitives;
    // Both solid-shader eligibility modes, rebuilt with geometry/materials on level upload.
    std::vector<std::array<std::vector<ShadowDrawRange>, 2>> m_static_shadow_ranges;
    std::vector<std::vector<ShadowGeometryRange>> m_shadow_geometry;
    std::vector<std::uint8_t> m_prepass_coverage;  // Static solid/cutout material passes per model.
    std::array<std::uint64_t, 2> m_shadow_geometry_indices{};  // Tested and rejected indices.
    std::vector<std::uint8_t>
        m_static_shadow_coverage;  // Alpha-tested (1) and permanent opaque (2).

    Group m_shadow_compact;
    std::vector<InstanceData> m_shadow_survivors;
    std::array<CascadeCulling, kSunCascades> m_sun_culling{};
    std::vector<InstanceData> m_camera_survivors;
    Pvs m_pvs;

    // The view inputs whose GPU batches can be reused within one recording.
    struct CameraVisibility {
        renderer::Mat4 clip{};
        Pvs::Mask mask{};
        std::uint64_t recording = 0;
        bool culling_off = false;
    };

    // Instance updates invalidate their layer; a new recording or view rebuilds its batches.
    std::array<CameraVisibility, kLayerCount> m_camera_visibility{};
    std::vector<InstanceData> m_pvs_survivors;

    struct PvsModel {
        Pvs::Mask mask{};
        bool valid = false;
        unsigned indices = 0;
        std::size_t rejected = 0;
        std::vector<Primitive> primitives;
    };

    // Terrain filtering must preserve material runs instead of leaving holes
    // in the sorted index buffer and turning each fragment into another draw.
    std::vector<std::uint32_t> m_cpu_indices;
    std::vector<PvsModel> m_pvs_models;
    // The terrain fragments past the game's far cull this tick (GameScenery::terrain_far).
    std::vector<std::uint8_t> m_terrain_far;
    // The terrain chunk the game's records are (move_scenery); -1 every chunk.
    int m_terrain_chunk = -1;
    std::array<CameraCulling, kLayerCount> m_camera_culling{};
    // Each model's copy in material order, for the opaque pass of the static layers (upload); a
    // model of one primitive, or one only live mobys use, is its own.
    std::vector<std::uint32_t> m_opaque;
    // Retained between the opaque rounds and the original blended passes.
    std::array<std::vector<bool>, kLayerCount> m_colour_visible;
    std::vector<Material> m_materials;
    std::vector<renderer::TextureHandle> m_textures;  // by image
    // The swapped texture's image (-1 none) and the handle of each variant, by the game's number.
    int m_variant_image = -1;
    std::vector<renderer::TextureHandle> m_variants;
    renderer::TextureHandle m_white = 0;
    std::array<std::vector<Group>, kLayerCount> m_groups;
    // The groups a live layer does not draw this frame, kept for the frames it does (set_instances).
    std::array<std::vector<Group>, kLayerCount> m_idle;

    // build_groups' lists, by model, kept for their storage.
    struct InstanceGroup {
        std::uint32_t key = 0;
        std::vector<InstanceData> instances;
        std::vector<std::uint64_t> floating_ids;
        std::vector<std::array<float, 4>> spheres;  // CPU only: no extra GPU instance attributes
        std::vector<std::uint16_t> occlusion;
    };

    std::vector<InstanceGroup> m_grouping;
    // Each key's list in m_grouping during a build_groups call, for a lookup by hash.
    std::unordered_map<std::uint32_t, std::size_t> m_group_of_key;

    // Where set_instances' last regrouping put each moby (rewrite_mobys): its key, its group of
    // m_groups[Mobys] and its place there, the sphere its bounds came from, and each group's count.
    struct MobyLayout {
        std::vector<std::uint32_t> keys;
        std::vector<std::uint32_t> group;
        std::vector<std::uint32_t> slot;
        std::vector<std::array<float, 4>> spheres;
        std::vector<std::uint32_t> filled;
        bool sphere_off = false;  // OPENRAC_MOBY_SPHERE_CULL_OFF when the groups were made.
        bool valid = false;       // False until a regrouping, and after an upload or release.
    };

    MobyLayout m_moby_layout;
    std::size_t m_palette_rows = 0;  // the palette texture's height now
    unsigned m_vertices = 0;
    unsigned m_positions = 0;        // the vertices' positions alone (vertex-position-stream)
    bool m_position_stream = false;  // the vertex arrays read positions from m_positions
    unsigned m_indices = 0;
    unsigned m_palette = 0;           // RGBA32F texture, four texels per matrix
    unsigned m_lights = 0;            // RGBA8 texture, the ties' lit colours
    std::vector<float> m_light_sets;  // the level's 16 directional light sets (LevelData)
    std::array<float, 64> m_point_lights{};
    std::vector<renderer::PointEmitter> m_point_emitters;
    std::vector<std::array<float, 8>> m_moby_point_lights;
    bool m_terrain_point_lights = false;
    bool m_sweep_translucent_off = false;
    bool m_sweep_translucent_unlit = false;
    bool m_sweep_translucent_full = false;
    bool m_sweep_moby_blended_off = false;
    bool m_sweep_one_triangle = false;
    std::array<float, 2> m_gameplay_rim{0.30f, 6.46f};
    std::array<float, 2> m_gameplay_rim_lift{0.09f, 7.1f};
    std::unique_ptr<LevelLighting> m_lighting;  // The probe and early lighting; null: none.
    bool m_early_capable = false;
    void bind_hero_shadow(const renderer::Shader& shader) const;
    void bind_shadow_atlas(const renderer::Shader& shader) const;
    void bind_sun_shadow(const renderer::Shader& shader, const renderer::Camera& camera) const;
    std::array<float, 4> m_fog_colour{0, 0, 0, 0};        // w = 1: fog on
    std::array<float, 4> m_fog_params{0, 255, 255, 255};  // slope, offset, lower, upper clamp
    std::vector<JointColumns> m_palette_data;
    renderer::Shader m_mesh;
    renderer::Shader m_early_mesh, m_early_solid;
    renderer::Shader m_colour_opaque, m_early_colour_opaque, m_static_translucent,
        m_static_translucent_full, m_static_translucent_early;
    // Static solid, static alpha-tested, then opaque mobys (optional skinning).
    std::array<renderer::Shader, 3> m_colour_specialised, m_early_colour_specialised;
    // mesh.frag's opaque path alone, without discard (the static layers' opaque pass; draw_layer).
    renderer::Shader m_solid;
    // The probe coverage view's colour shaders (OPENRAC_PROBE_COVERAGE), alpha and solid.
    renderer::Shader m_coverage, m_coverage_solid;
    // The mobys' blended passes with the cheaper probe light (probe-fast-blended).
    renderer::Shader m_mesh_fast;
    renderer::Shader m_depth, m_depth_solid;
    std::array<renderer::Shader, 2> m_rigid_depth, m_rigid_surface_depth;
    bool make_rigid_depth_programs(std::string& error);
    void release_rigid_depth_programs();
    renderer::Shader m_surface_depth;        // Smooth-normal prepass with alpha testing.
    renderer::Shader m_surface_depth_solid;  // Smooth-normal prepass for opaque static geometry.
    renderer::Shader m_sky;
    std::unique_ptr<renderer::FrameBuffer> m_hero_map;
    renderer::Shader m_hero_depth, m_shadow_solid;
    renderer::Shader m_atlas_depth, m_atlas_solid;
    std::array<renderer::Shader, 2> m_dry_shadow, m_dry_atlas;
    bool make_dry_shadow_programs(bool atlas, std::string& error);
    std::unique_ptr<renderer::FrameBuffer> m_shadow_atlas;
    bool m_shadow_atlas_active = false;
    ShadowAtlasLayout m_shadow_layout;
    std::array<renderer::Mat4, 5> m_shadow_view_projection;
    // The sun cascades' static casters, in the atlas's layout, kept between frames, and what each
    // cascade's rectangle was drawn for (shadow-static-cache, R-18).
    std::unique_ptr<renderer::FrameBuffer> m_static_shadow;
    std::array<StaticShadowKey, kSunCascades> m_static_keys{};
    // Bumped whenever the static layers' instances change (an upload, set_instances of one).
    std::uint64_t m_static_generation = 1;

    struct ScrollingShadow {
        ShadowScrollGrid grid;
        ShadowScrollUpdate update;
        std::unique_ptr<renderer::FrameBuffer> statics;
        unsigned combined = 0;
        bool active = false;
    };

    std::array<ScrollingShadow, kSunCascades> m_scroll_shadows;
    bool m_scroll_requested = false;

    struct ShadowDraw {
        unsigned vao;
        int instances;
        renderer::GpuInstanceBatch batch;
        std::uint32_t first, count;
        bool solid, skinning;
        bool floats_on_water;
        int material, view;
    };

    std::array<std::vector<ShadowDraw>, 3> m_shadow_draws;
    std::vector<renderer::GpuIndexRange> m_shadow_ranges;
    renderer::Mat4 m_hero_matrix = renderer::identity();
    std::optional<renderer::Vec3> m_hero_centre;
    std::optional<Instance> m_wetness_hero;
    bool m_hero_enabled = false, m_hero_ready = false, m_hero_airborne = false;
    ShadowTransition m_hero_transition;  // Shared ease-out for projection and overlap darkening.
    std::optional<bool> m_hero_logged_airborne;
    std::array<std::unique_ptr<renderer::FrameBuffer>, kSunCascades> m_sun_maps;
    SunCascades m_sun_cascades;

    // shadow-far-alternate: the far cascades as the atlas last drew them, and what they were drawn
    // for. A frame after one that drew a cascade keeps it while its receivers fit inside it.
    struct DrawnCascades {
        SunCascades cascades;
        // The cascades the atlas holds as drawn, one bit each: the last frame's, or with
        // shadow-far-quarter also older ones kept since.
        unsigned fresh = 0;
        bool separate_hero = false;
        std::uint64_t generation = 0;
        int atlas_size = 0;
    };

    DrawnCascades m_drawn_cascades;
    unsigned m_kept_cascades = 0;  // the cascades this frame keeps from the last, one bit each
    unsigned m_far_phase = 0;      // shadow-far-quarter: which far cascade this frame draws
    renderer::Camera m_sun_camera;        // View whose splits produced the current shadow maps.
    bool m_culling_lines_failed = false;  // Retry failed diagnostic shaders after scene release.
    std::unique_ptr<renderer::DebugLines>
        m_culling_lines;  // Allocated only for inspection; the scene retains move ownership.
    bool m_sun_enabled = false, m_sun_ready = false;
};

// One layer of a LevelScene, as a bucket of the renderer.
class LayerRenderer : public renderer::BucketRenderer {
public:
    LayerRenderer(LevelScene& scene, Layer layer);

    void render(const renderer::FrameInput& input, renderer::RenderState& state) override;

    renderer::AoLighting ao_lighting() const override { return m_scene.ao_lighting(); }

    void render_depth(const renderer::FrameInput& input, renderer::RenderState& state) override;

    bool batches_opaque_colour() const override {
        return m_layer != Layer::Sky && m_scene.batches_opaque_colour();
    }

    bool opaque_colour_barrier() const override { return m_scene.colour_blend_barrier(m_layer); }

    bool prepares_colour_chain() const override { return m_layer != Layer::Sky; }

    std::span<const unsigned> opaque_colour_buffers(
        const renderer::FrameInput& input, const renderer::RenderState& state
    ) const override {
        return m_scene.opaque_colour_buffers(input, state);
    }

    void render_opaque_colour(
        unsigned phase, const renderer::FrameInput& input, renderer::RenderState& state
    ) override {
        state.opaque_colour_phase = static_cast<int>(phase);
        m_scene.draw_layer(m_layer, input, state);
        state.opaque_colour_phase = -1;
    }

    bool enhanced_lighting_available() const override { return m_scene.probe_data_available(); }

    bool needs_early_lighting(const renderer::FrameInput& input) const override {
        return m_layer == Layer::Terrain && m_scene.needs_early_lighting(input);
    }

    // Shadow callbacks retain their captures until the shared lighting plan executes.
    bool prepares_surface_chain(
        const renderer::FrameInput&, const renderer::RenderState&
    ) const override {
        return renderer::render_opt::enabled(renderer::render_opt::Opt::ShadowFrameReuse);
    }

    void render_early_lighting(
        unsigned depth, const renderer::FrameInput& input, renderer::RenderState& state
    ) override {
        m_scene.render_early_lighting(depth, input, state);
    }

    bool owns_gpu_timing() const override { return true; }

private:
    LevelScene& m_scene;
    Layer m_layer;
};

}  // namespace openrac::scene

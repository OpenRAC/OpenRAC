// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Private water material state shared by asset loading, GPU resources and simulation.
#pragma once
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <optional>
#include <vector>

#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_mesh.h"
#include "renderer/framebuffer.h"
#include "renderer/shader.h"

namespace openrac::enhanced::water_detail {
using Texel = std::array<float, 4>;
constexpr unsigned kWindowSize = 128, kWindowCells = kWindowSize * kWindowSize;
constexpr float kWindowSpacing = .25f;

enum BufferIndex : unsigned {
    kStateA,
    kStateB,
    kCoast,
    kBodies,
    kLighting,
    kRenderPixels,
    kHydraulics,
    kWindowCoast,
    kWindowHydraulics,
    kBufferCount
};

enum TextureIndex : unsigned {
    kCoastTexture,
    kBodyTexture,
    kLightTexture,
    kTextureCount
};

class Water;
extern std::atomic<bool> water_ready;
extern std::mutex water_debug_mutex;
extern WaterDebugSettings water_debug;
extern std::atomic<std::uint64_t> water_reset_epoch;
extern Water* contact_source;
bool water_disabled();

class Water final : public renderer::EffectMaterial {
public:
    Water(scene::LevelScene& scene, const std::filesystem::path& path);
    std::string fragment_source() const override;
    std::string vertex_source() const override;

    std::optional<std::string> quad_fragment_source() const override {
        return "vec3 effect_material(vec3 rgb){return rgb;}\n";
    }

    void bind_quads(const renderer::Shader&, const renderer::FrameInput&) override {}

    bool plain_strips_supported() const override { return true; }
    std::optional<renderer::EffectMesh> mesh(
        const renderer::EffectStrip& strip, std::span<const renderer::EffectVertex> vertices
    ) override;
    void upload_meshes() override;
    // Captures opaque colour and reversed depth before enhanced water composites them.
    void prepare_scene(
        const renderer::FrameInput& input, const renderer::RenderTarget& target
    ) override;
    void finish_pass() override;
    void draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) override;
    bool init(std::string& error) override;
    void update_lighting(const renderer::FrameInput& input);
    void prepare(const renderer::FrameInput& input) override;
    void bind(
        const renderer::Shader& shader,
        std::uint32_t surface,
        const renderer::FrameInput& input,
        const renderer::EffectUvMotion& motion
    ) override;
    void bind_contact(const renderer::Shader& shader) const;
    // Prepares displacement once, without restarting the liquid mesh collector.
    void prepare_deformation(const renderer::FrameInput& input);
    // Binds the same maps and timing used by the liquid vertex shader.
    void bind_deformation(const renderer::Shader& shader) const;
    void release() override;

private:
    // Geometry consumers need waves before depth; liquid lighting waits for current shadows.
    void prepare_frame(const renderer::FrameInput& input, bool lighting);
    bool m_lighting_pending = false;
    void remember_tick(
        const renderer::FrameInput& input, const renderer::Vec3& hero, std::uint64_t epoch
    );
    std::unique_ptr<WaterMesh> m_mesh;
    renderer::FrameBuffer m_background;
    bool m_background_ready = false;
    scene::LevelScene& m_scene;
    std::vector<Texel> m_bodies, m_coast, m_hydraulics;
    renderer::Shader m_step, m_light, m_window_prepare;
    std::array<unsigned, kBufferCount> m_buffers{};
    std::array<unsigned, kTextureCount> m_textures{};
    std::array<unsigned, 2> m_maps{};
    std::array<Texel, 64> m_atlas{};
    unsigned m_atlas_width = 0, m_atlas_height = 0;
    unsigned m_light_count = 0;
    unsigned m_current = 0;
    int m_sim_body = -1;
    float m_sim_height = 0, m_cached_height = 0;
    int m_cached_body = -1;
    std::array<int, 2> m_cached_origin{};
    std::array<int, 2> m_window_origin{};
    std::uint64_t m_prepared_frame = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t m_clock_frame = std::numeric_limits<std::uint64_t>::max();
    WaterDebugSettings m_debug;
    std::uint64_t m_reset_epoch = 0;
    std::uint64_t m_last_frame = std::numeric_limits<std::uint64_t>::max();
    renderer::Vec3 m_hero{};
    std::uint64_t m_impulse_frame = std::numeric_limits<std::uint64_t>::max();
    std::array<std::optional<renderer::Vec3>, 2> m_hands{};
    double m_time = 0, m_exit_time = -1;
    float m_sim_weight = 1;
    bool m_enabled = false;
    bool m_bound = false;
    unsigned m_effect_setups = 0;
};
}  // namespace openrac::enhanced::water_detail

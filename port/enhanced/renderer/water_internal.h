// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Private water material state shared by asset loading, GPU resources and simulation.
#pragma once
#include <array>
#include <atomic>
#include <complex>
#include <limits>
#include <mutex>
#include <optional>
#include <vector>

#include "enhanced/renderer/water.h"
#include "enhanced/renderer/water_mesh.h"
#include "renderer/gl/framebuffer.h"
#include "renderer/gl/shader.h"

namespace openrac::enhanced::water_detail {
using Texel = std::array<float, 4>;
constexpr unsigned kWindowSize = 128, kWindowCells = kWindowSize * kWindowSize;
constexpr float kWindowSpacing = .25f;
constexpr int kOceanSide = 64;
constexpr int kOceanCells = kOceanSide * kOceanSide;

// A periodic 32-unit field of heights and slopes, sampled by both water shaders.
struct OceanSpectrum {
    explicit OceanSpectrum(
        double depth,
        bool fine = false,
        double tile_override = 0.,
        double rms_override = -1.,
        double heading = 163.739795
    );
    void evaluate(double time);
    std::array<std::complex<double>, kOceanCells> seed{}, field{};
    std::array<double, kOceanCells> frequency{};
    std::array<Texel, kOceanCells> pixels{};
    double gain = 0;
    double slope_variance = 0;
    double tile = 32;
    double time = -1;
};

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
extern std::atomic<bool> ocean_ready;
// Publish the profile when the level's GPU resources become available.
void begin_ocean_material(const OceanMaterial& material);
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
        const renderer::FrameInput& input, const renderer::RenderTarget& target,
        unsigned pre_particle_colour
    ) override;

    bool prepares_colour_chain() const override { return true; }

    std::array<unsigned, 2> scene_textures() const override;
    void finish_pass() override;
    void draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) override;
    bool init(std::string& error) override;
    // Updates sampled water irradiance after binding the current scene probes and shadows.
    void update_lighting(const renderer::FrameInput& input);
    void prepare(const renderer::FrameInput& input) override;
    void bind(
        const renderer::Shader& shader,
        std::uint32_t surface,
        const renderer::FrameInput& input,
        const renderer::EffectUvMotion& motion
    ) override;
    void bind_contact(const renderer::Shader& shader, std::vector<WaterBufferUse>& reads) const;
    // Prepares displacement once, without restarting the liquid mesh collector.
    void prepare_deformation(const renderer::FrameInput& input);
    // Binds the same maps and timing used by the liquid vertex shader.
    void bind_deformation(const renderer::Shader& shader) const;
    void release() override;

private:
    // Borrowed frame/impulse inputs remain valid until simulate() finishes all synchronous callbacks.
    struct SimulationUpdate {
        const renderer::FrameInput& input;      // Owned by the current renderer frame.
        renderer::Vec3 hero;                    // Selected current water interactor.
        std::array<int, 2> window;              // Current local torus origin.
        std::array<float, 2> hand_impulses;     // Existing tuned hand wake strengths.
        std::span<const Texel> impulse_starts;  // Borrowed prepare_frame() stack arrays.
        std::span<const Texel> impulse_ends;    // Matching swept projectile/explosion endpoints.
        float impulse = 0;                      // Existing tuned main interactor strength.
        unsigned steps = 0;                     // Caught-up game ticks, each with four substeps.
        bool reset = false;                     // Existing discontinuity/reset decision.
    };

    // Records metadata/history preparation using unchanged shader bindings and parameters.
    void prepare_window(const SimulationUpdate& update);
    // Binds physics and interaction constants shared by every substep of this update.
    void configure_wave_steps(const SimulationUpdate& update);
    // Binds previous contact positions and swept impulse arrays without changing their sampling order.
    void configure_wave_interactions(const SimulationUpdate& update);
    // Records one unchanged solver dispatch and advances ping-pong ownership exactly once.
    void record_wave_step(const SimulationUpdate& update, unsigned step);
    // Executes the graph or reference chain, reporting transfer/recording failures to the caller.
    bool simulate(const SimulationUpdate& update);
    // Geometry consumers need waves before depth; liquid lighting waits for current shadows.
    void prepare_frame(const renderer::FrameInput& input, bool lighting);
    // Loads the opt-in ocean response and constructs the local water atlas.
    void load_ocean(const std::filesystem::path& path);
    // Binds the immutable FFT basis and coastal response for ocean effect shaders.
    void bind_ocean(const renderer::Shader& shader) const;
    void update_ocean();
    bool m_lighting_pending = false;
    void remember_tick(
        const renderer::FrameInput& input, const renderer::Vec3& hero, std::uint64_t epoch
    );
    std::unique_ptr<WaterMesh> m_mesh;
    renderer::FrameBuffer m_background;
    std::array<unsigned, 2> m_background_textures{};
    std::array<int, 2> m_background_extent{};
    scene::LevelScene& m_scene;
    std::vector<Texel> m_bodies, m_coast, m_hydraulics;
    std::vector<Texel> m_ocean_pixels;  // Coastal response followed by the inverse FFT basis row.
    std::array<std::unique_ptr<OceanSpectrum>, 2> m_ocean_spectra;
    Texel m_ocean_grid{};                  // Patch origin, cell size, and side length.
    std::array<Texel, 8> m_ocean_modes{};  // Direction, wavenumber, and angular frequency.
    std::array<Texel, 2> m_ocean_amplitudes{};
    int m_ocean_mode_count = 0;
    unsigned m_ocean_texture = 0;  // Owned ocean texture, only allocated on Pokitaru.
    std::array<unsigned, 3>
        m_ocean_detail_textures{};                  // Chop, fine slopes, horizontal displacement.
    std::array<unsigned, 5> m_ocean_fft_buffers{};  // Two seeds, scratch, and two outputs.
    renderer::Shader m_ocean_fft;
    double m_ocean_fft_time = -1;
    renderer::Vec3 m_ocean_eye{};
    renderer::Mat4 m_view_projection{};  // This pass's camera, for culling strips outside it.
    float m_ocean_horizon = 4096;
    float m_ocean_near = 32;
    float m_ocean_depth = 12;
    float m_ocean_choppiness = .85f;
    OceanMaterial m_ocean_material;  // Defaults parsed from this level's profile.
    std::array<Texel, 8> m_ocean_dry_regions{};
    int m_ocean_dry_count = 0;
    bool m_ocean_drawn = false;
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

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/renderer/local_lights.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced/renderer/water_window.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/frame/colour_graph.h"
#include "renderer/frame/surface_chain.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced::water_detail {
using namespace gl;
namespace wgl = renderer::world::gl;
std::atomic<bool> water_ready{false};
std::atomic<bool> ocean_ready{false};
std::mutex water_debug_mutex;
WaterDebugSettings water_debug;
// Profile and live values share the existing water controls mutex.
OceanMaterial ocean_profile_material;
OceanMaterial ocean_live_material;
std::atomic<std::uint64_t> water_reset_epoch{0};

void begin_ocean_material(const OceanMaterial& material) {
    const std::lock_guard lock(water_debug_mutex);
    ocean_profile_material = material;
    ocean_live_material = material;
}

bool water_disabled() {
    static const bool& off = stats::probe("OPENRAC_WATER_OFF", "Original Novalis water reference");
    return off;
}

Water* contact_source = nullptr;

// Compose water optics with shared lighting, reflection and scene-transmission helpers.
std::string Water::fragment_source() const {
    std::string source = std::string(lighting::shaders::water_window_glsl) + "\n"
                         + std::string(lighting::shaders::water_swell_glsl) + "\n"
                         + std::string(lighting::shaders::shading_brdf_glsl) + "\n"
                         + local_light_shader_source() + "\n"
                         + std::string(lighting::shaders::environment_glsl) + "\n"
                         + std::string(lighting::shaders::water_transmission_glsl) + "\n"
                         + std::string(lighting::shaders::water_glsl);
    if (!m_ocean_pixels.empty()) {
        source.insert(0, "#define OPENRAC_OCEAN 1\n");
    }
    if (m_ocean_spectra[0]) {
        source.insert(0, "#define OPENRAC_OCEAN_SPECTRUM 1\n#define OPENRAC_OCEAN_FINE 1\n");
    }
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader)) {
        source.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        return renderer::shader_constants(std::move(source), {{"water_sim_local", 1}});
    }
    return source;
}

std::string Water::vertex_source() const {
    std::string source = std::string(lighting::shaders::water_uv_glsl) + "\n"
                         + std::string(lighting::shaders::water_mesh_glsl) + "\n"
                         + std::string(lighting::shaders::water_window_glsl) + "\n"
                         + std::string(lighting::shaders::water_swell_glsl) + "\n"
                         + std::string(lighting::shaders::water_displacement_glsl) + "\n"
                         + std::string(lighting::shaders::water_vertex_glsl);
    if (!m_ocean_pixels.empty()) {
        source.insert(0, "#define OPENRAC_OCEAN 1\n");
    }
    if (m_ocean_spectra[0]) {
        source.insert(0, "#define OPENRAC_OCEAN_SPECTRUM 1\n");
    }
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader)) {
        source.insert(0, "#define OPENRAC_WATER_LOCAL 1\n");
        return renderer::shader_constants(std::move(source), {{"water_sim_local", 1}});
    }
    return source;
}

std::optional<renderer::EffectMesh> Water::mesh(
    const renderer::EffectStrip& strip, std::span<const renderer::EffectVertex> vertices
) {
    if (!m_enabled || strip.surface == 0 || strip.surface > 7 || vertices.size() < 3) {
        return std::nullopt;
    }
    if (m_ocean_spectra[0] && strip.surface == 1) {
        if (m_ocean_drawn) {
            return renderer::EffectMesh{};
        }
        m_ocean_drawn = true;
        return m_mesh->prepare_ocean(
            m_ocean_eye,
            vertices.front().z,
            m_ocean_near * m_debug.ocean_near_scale,
            m_ocean_horizon * m_debug.ocean_horizon_scale
        );
    }
    // The original horizon strip and distant quad retain their GS compositing.
    if (!m_ocean_pixels.empty()) {
        const auto height = std::abs(m_coast.front()[0]) - 512.f;
        const auto& first = vertices.front();
        constexpr float kOceanTile = 50.f;  // The NTSC-U ocean pvar's near-grid spacing.
        if (std::any_of(vertices.begin(), vertices.end(), [&](const auto& p) {
                const float x = (p.x - first.x) / kOceanTile;
                const float y = (p.y - first.y) / kOceanTile;
                return std::abs(p.z - height) > .5f || std::abs(x) > 4.01f || std::abs(y) > 4.01f
                       || std::abs(x - std::round(x)) > .002f
                       || std::abs(y - std::round(y)) > .002f;
            })) {
            return std::nullopt;
        }
    }
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::WaterStripCull)) {
        // Waves move the surface by far less than this; a strip outside the view draws nothing.
        constexpr float kWaveMargin = 4;
        renderer::Vec3 low{1e20f, 1e20f, 1e20f}, high{-1e20f, -1e20f, -1e20f};
        for (const auto& v : vertices) {
            const renderer::Vec3 p{v.x, v.y, v.z};
            for (unsigned axis = 0; axis < 3; ++axis) {
                low[axis] = std::min(low[axis], p[axis] - kWaveMargin);
                high[axis] = std::max(high[axis], p[axis] + kWaveMargin);
            }
        }
        if (!water_window_visible(m_view_projection, low, high)) {
            return renderer::EffectMesh{};
        }
    }
    return m_mesh->prepare(strip, vertices);
}

void Water::upload_meshes() {
    m_mesh->upload();
}

// Copy once per visible water pass; source and sampled images never alias.
void Water::prepare_scene(
    const renderer::FrameInput&, const renderer::RenderTarget& target, unsigned pre_particle_colour
) {
    if (!m_enabled || target.width <= 0 || target.height <= 0) {
        return;
    }
    // Simulation and lighting views have no transmission. The foam mask needs scene depth only
    // for ocean shoaling, and the shoreline mask uses it for contact coverage.
    const bool foam_mask = m_debug.view == 14;
    if ((m_debug.view >= 1 && m_debug.view <= 11) || (foam_mask && !m_ocean_spectra[0])) {
        return;
    }
    const bool needs_colour = !foam_mask && m_debug.view != 16;
    const bool copy_colour = needs_colour && pre_particle_colour == 0;
    const auto scratch = renderer::lease_surface_refraction(
        target.framebuffer, {target.width, target.height}, copy_colour
    );
    if (!scratch
        && (m_background.width() != target.width || m_background.height() != target.height)) {
        std::string error;
        if (!m_background.create(target.width, target.height, error, 1, true)) {
            log::warn("Water scene transmission unavailable: {}", error);
            glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    GLint samples = 1;
    glGetIntegerv(GL_SAMPLES, &samples);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
    const unsigned destination = scratch ? scratch.framebuffer : m_background.id();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination);
    // Copy dimensions survive preparation until the enclosing colour scope executes the blits.
    const int width = target.width;
    const int height = target.height;
    const auto copied = renderer::colour_snapshot(
        target,
        destination,
        std::max(1, samples),
        [width, height](bool depth) {
            glBlitFramebuffer(
                0,
                0,
                width,
                height,
                0,
                0,
                width,
                height,
                depth ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
            // Logical output payload. This excludes reads, MSAA samples and packed stencil.
            stats::gauge(
                depth ? "refraction depth snapshot bytes" : "refraction colour snapshot bytes",
                double(width) * height * 4
            );
        },
        copy_colour ? renderer::SnapshotAttachments::ColourDepth
                    : renderer::SnapshotAttachments::Depth
    );
    if (!copied) {
        throw std::runtime_error(copied.error().message);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    m_background_textures =
        scratch ? scratch.textures
                : std::array{m_background.colour_texture(), m_background.depth_texture()};
    if (!copy_colour) {
        m_background_textures[0] = needs_colour ? pre_particle_colour : 0;
    }
    m_background_extent = {width, height};
}

std::array<unsigned, 2> Water::scene_textures() const {
    return m_background_textures;
}

// Enhanced water already includes its background; legacy GS blending would add it twice.
void Water::draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) {
    if (mesh.draws == 0) {
        return;
    }
    glDisable(GL_BLEND);
    m_mesh->draw(shader, mesh);
    glEnable(GL_BLEND);
}

// Bind per-surface animation and per-pass lighting, including the pre-water scene snapshot.
void Water::bind(
    const renderer::Shader& shader,
    std::uint32_t surface,
    const renderer::FrameInput& input,
    const renderer::EffectUvMotion& motion
) {
    glUniform1i(shader.uniform("water_mesh"), 0);
    glUniform1i(shader.uniform("water_uv_animate"), motion.enabled ? 1 : 0);
    glUniform4f(
        shader.uniform("water_uv_motion"),
        motion.offset[0],
        motion.offset[1],
        motion.amplitude,
        float(motion.phase & 511u)
    );
    glUniform1i(shader.uniform("water_surface"), m_enabled ? int(surface) : 0);
    if (!m_bound && m_ocean_texture) {
        bind_ocean(shader);
    }
    if (!m_bound || !renderer::render_opt::enabled(renderer::render_opt::Opt::EffectSetupReuse)) {
        m_mesh->bind(shader);
        glUniform1i(shader.uniform("water_debug_view"), m_debug.view);
        glUniform1f(shader.uniform("water_debug_height_limit"), m_debug.displacement_limit);
        glUniform1f(shader.uniform("water_fine_strength"), m_debug.fine_normal_strength);
        glUniform1i(shader.uniform("water_sim_local"), 1);
        glUniform1i(shader.uniform("water_sim_body"), m_sim_body);
        glUniform1f(shader.uniform("water_sim_height"), m_sim_height);
        glUniform1f(shader.uniform("water_sim_weight"), m_sim_weight);
        glUniform4f(
            shader.uniform("water_sim_window"),
            float(m_window_origin[0]) * .25f,
            float(m_window_origin[1]) * .25f,
            .25f,
            float(kWindowSize)
        );
        ++m_effect_setups;
    }
    // All statically referenced Vulkan descriptors must be valid even on the original-
    // material branch. Bind once per effects pass, then only change the semantic surface.
    if (m_bound) {
        return;
    }
    m_bound = true;
    glUniform1i(shader.uniform("water_scene_ready"), m_background_textures[1] ? 1 : 0);
    glUniform2f(
        shader.uniform("water_scene_extent"),
        float(std::max(1, m_background_extent[0])),
        float(std::max(1, m_background_extent[1]))
    );
    glUniformMatrix4fv(shader.uniform("water_scene_view"), 1, GL_FALSE, input.camera.view.data());
    const auto& projection = input.camera.projection;
    glUniform4f(
        shader.uniform("water_scene_projection"),
        projection[10],
        projection[14],
        projection[11],
        projection[15]
    );
    glUniform1i(shader.uniform("water_scene_colour"), 26);
    glUniform1i(shader.uniform("water_scene_depth"), 27);
    // Valid descriptors are required even when no replacement mesh needed a snapshot.
    glActiveTexture(GL_TEXTURE0 + 26);
    glBindTexture(GL_TEXTURE_2D, m_background_textures[0] ? m_background_textures[0] : m_maps[1]);
    glActiveTexture(GL_TEXTURE0 + 27);
    glBindTexture(GL_TEXTURE_2D, m_background_textures[1] ? m_background_textures[1] : m_maps[1]);
    auto material = reflection_material_settings(ReflectionSurface::Water);
    if (m_ocean_spectra[0]) {
        const auto ocean = ocean_material();
        material.roughness = ocean.roughness;
        material.reflectivity = ocean.reflectivity;
        material.strength = ocean.reflection_strength;
        glUniform3f(
            shader.uniform("water_ocean_colour"), ocean.colour[0], ocean.colour[1], ocean.colour[2]
        );
        glUniform3f(
            shader.uniform("water_ocean_absorption"),
            ocean.absorption[0],
            ocean.absorption[1],
            ocean.absorption[2]
        );
    }
    glUniform3f(
        shader.uniform("water_material"),
        material.roughness,
        material.reflectivity,
        material.strength
    );
    m_scene.bind_surface_lighting(shader, input.camera);
    glUniform1i(shader.uniform("water_bodies"), 11);
    glUniform1i(shader.uniform("water_coast_map"), 12);
    glUniform1i(shader.uniform("water_wave_map"), 14);
    glUniform4fv(shader.uniform("water_atlas[0]"), 64, m_atlas[0].data());
    glUniform2f(shader.uniform("water_atlas_extent"), float(m_atlas_width), float(m_atlas_height));
    glUniform1i(shader.uniform("water_light"), 2);
    glUniform1i(shader.uniform("water_body_count"), int(m_bodies.size() / 5));
    glUniform1f(shader.uniform("water_time"), float(m_time));
    glUniform1f(shader.uniform("water_current_scale"), m_debug.current_scale);
    glUniform3f(
        shader.uniform("water_eye"),
        input.camera.position[0],
        input.camera.position[1],
        input.camera.position[2]
    );
    for (const auto& binding : {std::array<unsigned, 2>{11, kBodyTexture}, {2, kLightTexture}}) {
        glActiveTexture(GL_TEXTURE0 + binding[0]);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[binding[1]]);
    }
    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(GL_TEXTURE_2D, m_maps[1]);
    glActiveTexture(GL_TEXTURE0 + 14);
    glBindTexture(GL_TEXTURE_2D, m_maps[0]);
    glActiveTexture(GL_TEXTURE0);
}

void Water::finish_pass() {
    stats::gauge("effect water setups", m_effect_setups);
    m_mesh->finish_pass();
}

void Water::bind_ocean(const renderer::Shader& shader) const {
    // Attachment poses use this slot in the mesh pass. Probe depth uses units 30 and 31.
    constexpr int kOceanUnit = 25;
    glActiveTexture(GL_TEXTURE0 + kOceanUnit);
    glBindTexture(GL_TEXTURE_2D, m_ocean_texture);
    glUniform1i(shader.uniform("water_ocean_field"), kOceanUnit);
    glUniform4fv(shader.uniform("water_ocean_grid"), 1, m_ocean_grid.data());
    glUniform4fv(
        shader.uniform("water_ocean_modes[0]"), m_ocean_mode_count, m_ocean_modes[0].data()
    );
    glUniform4fv(shader.uniform("water_ocean_amplitudes[0]"), 2, m_ocean_amplitudes[0].data());
    glUniform1i(shader.uniform("water_ocean_mode_count"), m_ocean_mode_count);
    glUniform1i(shader.uniform("water_ocean_coastal"), m_debug.coastal_correction ? 1 : 0);
    if (m_ocean_detail_textures[0]) {
        // Early lighting uses these units in its compute pass.
        constexpr int kDetailUnit = 17, kFineUnit = 16;
        glActiveTexture(GL_TEXTURE0 + kDetailUnit);
        glBindTexture(GL_TEXTURE_2D, m_ocean_detail_textures[0]);
        glUniform1i(shader.uniform("water_ocean_detail"), kDetailUnit);
        glActiveTexture(GL_TEXTURE0 + kFineUnit);
        glBindTexture(GL_TEXTURE_2D, m_ocean_detail_textures[1]);
        glUniform1i(shader.uniform("water_ocean_fine"), kFineUnit);
        constexpr int kDisplacementUnit = 20;
        glActiveTexture(GL_TEXTURE0 + kDisplacementUnit);
        glBindTexture(GL_TEXTURE_2D, m_ocean_detail_textures[2]);
        glUniform1i(shader.uniform("water_ocean_displacement"), kDisplacementUnit);
        glUniform3f(
            shader.uniform("water_ocean_shape"),
            m_ocean_choppiness,
            float(m_ocean_spectra[0]->slope_variance),
            float(m_ocean_spectra[1]->slope_variance)
        );
        glUniform2f(
            shader.uniform("water_ocean_tiles"),
            float(m_ocean_spectra[0]->tile),
            float(m_ocean_spectra[1]->tile)
        );
        glUniform4f(
            shader.uniform("water_ocean_view"),
            m_ocean_eye[0],
            m_ocean_eye[1],
            m_ocean_near * m_debug.ocean_near_scale,
            m_ocean_horizon * m_debug.ocean_horizon_scale
        );
        const float angle = m_debug.ocean_wind_offset * .01745329252f;
        glUniform4f(
            shader.uniform("water_ocean_tuning"),
            m_debug.ocean_chop,
            m_debug.ocean_ripples,
            std::cos(angle),
            std::sin(angle)
        );
        glUniform1i(shader.uniform("water_ocean_dry_count"), m_ocean_dry_count);
        if (m_ocean_dry_count) {
            glUniform4fv(
                shader.uniform("water_ocean_dry[0]"),
                m_ocean_dry_count,
                m_ocean_dry_regions[0].data()
            );
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

void Water::prepare_deformation(const renderer::FrameInput& input) {
    prepare_frame(input, false);
}

void Water::bind_deformation(const renderer::Shader& shader) const {
    glActiveTexture(GL_TEXTURE0 + 18);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[kBodyTexture]);
    glActiveTexture(GL_TEXTURE0 + 19);
    glBindTexture(GL_TEXTURE_2D, m_maps[1]);
    glActiveTexture(GL_TEXTURE0 + 20);
    glBindTexture(GL_TEXTURE_2D, m_maps[0]);
    glUniform1i(shader.uniform("water_body_count"), int(m_bodies.size() / 5));
    glUniform4fv(shader.uniform("water_atlas[0]"), 64, m_atlas[0].data());
    glUniform2f(shader.uniform("water_atlas_extent"), float(m_atlas_width), float(m_atlas_height));
    glUniform1i(shader.uniform("water_sim_local"), 1);
    glUniform1i(shader.uniform("water_sim_body"), m_sim_body);
    glUniform1f(shader.uniform("water_sim_height"), m_sim_height);
    glUniform1f(shader.uniform("water_sim_weight"), m_sim_weight);
    glUniform4f(
        shader.uniform("water_sim_window"),
        float(m_window_origin[0]) * .25f,
        float(m_window_origin[1]) * .25f,
        .25f,
        float(kWindowSize)
    );
    glUniform1f(shader.uniform("water_time"), float(m_time));
    glUniform1f(shader.uniform("water_current_scale"), m_debug.current_scale);
    glActiveTexture(GL_TEXTURE0);
}

void Water::bind_contact(const renderer::Shader& shader, std::vector<WaterBufferUse>& reads) const {
    reads.push_back(
        {m_buffers[kCoast], m_coast.size() * sizeof(Texel), renderer::graph::Access::Read}
    );
    reads.push_back(
        {m_buffers[kBodies], m_bodies.size() * sizeof(Texel), renderer::graph::Access::Read}
    );
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_buffers[kCoast]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_buffers[kBodies]);
    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(GL_TEXTURE_2D, m_maps[1]);
    glActiveTexture(GL_TEXTURE0 + 14);
    glBindTexture(GL_TEXTURE_2D, m_maps[0]);
    glUniform1i(shader.uniform("water_body_count"), int(m_bodies.size() / 5));
    glUniform4fv(shader.uniform("water_atlas[0]"), 64, m_atlas[0].data());
    glUniform2f(shader.uniform("water_atlas_extent"), float(m_atlas_width), float(m_atlas_height));
    glUniform1i(shader.uniform("water_sim_local"), 1);
    glUniform1i(shader.uniform("water_sim_body"), m_sim_body);
    glUniform1f(shader.uniform("water_sim_height"), m_sim_height);
    glUniform1f(shader.uniform("water_sim_weight"), m_sim_weight);
    glUniform4f(
        shader.uniform("water_sim_window"),
        float(m_window_origin[0]) * .25f,
        float(m_window_origin[1]) * .25f,
        .25f,
        float(kWindowSize)
    );
    glUniform1f(shader.uniform("water_time"), float(m_time));
    glUniform1f(shader.uniform("water_current_scale"), m_debug.current_scale);
}
}  // namespace openrac::enhanced::water_detail

namespace openrac::enhanced {
using namespace water_detail;

OceanMaterial ocean_material() {
    const std::lock_guard lock(water_debug_mutex);
    return ocean_live_material;
}

void set_ocean_material(OceanMaterial material) {
    const std::lock_guard lock(water_debug_mutex);
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    for (std::size_t i = 0; i < material.colour.size(); ++i) {
        material.colour[i] = bounded(material.colour[i], 0, 1, ocean_profile_material.colour[i]);
        material.absorption[i] =
            bounded(material.absorption[i], 0, 4, ocean_profile_material.absorption[i]);
    }
    material.roughness = bounded(material.roughness, .04f, 1, ocean_profile_material.roughness);
    material.reflectivity =
        bounded(material.reflectivity, 0, 1, ocean_profile_material.reflectivity);
    material.reflection_strength =
        bounded(material.reflection_strength, 0, 4, ocean_profile_material.reflection_strength);
    ocean_live_material = material;
}

void reset_ocean_material() {
    const std::lock_guard lock(water_debug_mutex);
    ocean_live_material = ocean_profile_material;
}

WaterDebugSettings water_debug_settings() {
    std::lock_guard lock(water_debug_mutex);
    return water_debug;
}

// Keep live simulation and optical diagnostic controls within their supported ranges.
void set_water_debug_settings(WaterDebugSettings settings) {
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    settings.current_scale = bounded(settings.current_scale, 0, 2.5f, 1);
    settings.friction = bounded(settings.friction, 0, 4, .12f);
    settings.outlet_damping = bounded(settings.outlet_damping, 0, 30, 12);
    settings.interaction_strength = bounded(settings.interaction_strength, 0, 4, 1.4f);
    settings.interaction_radius = bounded(settings.interaction_radius, .1f, 2, .54772256f);
    settings.projectile_strength = bounded(settings.projectile_strength, 0, 4, 1);
    settings.projectile_radius = bounded(settings.projectile_radius, .1f, 4, 1);
    settings.explosion_strength = bounded(settings.explosion_strength, 0, 4, 1);
    settings.explosion_radius = bounded(settings.explosion_radius, .1f, 4, 1);
    settings.displacement_limit = bounded(settings.displacement_limit, 0, .5f, .25f);
    settings.fine_normal_strength = bounded(settings.fine_normal_strength, 0, 2, 1);
    settings.ocean_chop = bounded(settings.ocean_chop, 0, 4, WaterDebugSettings{}.ocean_chop);
    settings.ocean_ripples =
        bounded(settings.ocean_ripples, 0, 4, WaterDebugSettings{}.ocean_ripples);
    settings.ocean_wind_offset = bounded(settings.ocean_wind_offset, -180, 180, 0);
    settings.ocean_near_scale = bounded(settings.ocean_near_scale, .5f, 2, 1);
    settings.ocean_horizon_scale = bounded(settings.ocean_horizon_scale, .5f, 2, 1);
    settings.view = std::clamp(settings.view, 0, 17);
    std::lock_guard lock(water_debug_mutex);
    water_debug = settings;
}

void reset_water_simulation() {
    water_reset_epoch.fetch_add(1);
}

bool water_available() {
    return water_ready.load();
}

bool ocean_replaces_horizon() {
    return water_ready.load() && ocean_ready.load();
}

bool bind_water_contact(const renderer::Shader& shader, std::vector<WaterBufferUse>& reads) {
    if (!contact_source || !water_ready.load()) {
        return false;
    }
    contact_source->bind_contact(shader, reads);
    return true;
}

void prepare_water_deformation(const renderer::FrameInput& input) {
    if (contact_source) {
        contact_source->prepare_deformation(input);
    }
}

bool bind_water_deformation(const renderer::Shader& shader) {
    if (!contact_source || !water_ready.load()) {
        return false;
    }
    contact_source->bind_deformation(shader);
    return true;
}

std::unique_ptr<renderer::EffectMaterial> make_water(
    scene::LevelScene& scene, const std::filesystem::path& directory
) {
    if (!gl::enhanced_rendering_available() || !gl::is_vulkan()) {
        return {};
    }
    const auto ocean = directory / "ocean.wave";
    const auto path = std::filesystem::exists(ocean) ? ocean : directory / "water.bin";
    if (!std::filesystem::exists(path)) {
        log::info("Enhanced water needs a local water bake beside manifest.json");
        return {};
    }
    try {
        return std::make_unique<Water>(scene, path);
    } catch (const std::exception& error) {
        log::warn("Original water retained: {}", error.what());
        return {};
    }
}
}  // namespace openrac::enhanced

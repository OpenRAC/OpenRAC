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
#include <string>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"
#include "scene/level_renderer.h"

namespace openrac::enhanced::water_detail {
using namespace gl;
namespace wgl = renderer::world::gl;
std::atomic<bool> water_ready{false};
std::mutex water_debug_mutex;
WaterDebugSettings water_debug;
std::atomic<std::uint64_t> water_reset_epoch{0};

bool water_disabled() {
    static const bool& off = stats::probe("OPENRAC_WATER_OFF", "Original Novalis water reference");
    return off;
}

Water* contact_source = nullptr;

std::string Water::fragment_source() const {
    return std::string(lighting::shaders::water_window_glsl) + "\n"
           + std::string(lighting::shaders::water_swell_glsl) + "\n"
           + std::string(lighting::shaders::shading_brdf_glsl) + "\n"
           + std::string(lighting::shaders::probes_glsl) + "\n"
           + std::string(lighting::shaders::water_glsl);
}

std::string Water::vertex_source() const {
    return std::string(lighting::shaders::water_uv_glsl) + "\n"
           + std::string(lighting::shaders::water_mesh_glsl) + "\n"
           + std::string(lighting::shaders::water_window_glsl) + "\n"
           + std::string(lighting::shaders::water_swell_glsl) + "\n"
           + std::string(lighting::shaders::water_displacement_glsl) + "\n"
           + std::string(lighting::shaders::water_vertex_glsl);
}

std::optional<renderer::EffectMesh> Water::mesh(
    const renderer::EffectStrip& strip, std::span<const renderer::EffectVertex> vertices
) {
    if (!m_enabled || strip.surface == 0 || strip.surface > 7 || vertices.size() < 3) {
        return std::nullopt;
    }
    return m_mesh->prepare(strip, vertices);
}

void Water::upload_meshes() {
    m_mesh->upload();
}

void Water::draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) {
    m_mesh->draw(shader, mesh);
}

void Water::bind(
    const renderer::Shader& shader,
    std::uint32_t surface,
    const renderer::FrameInput& input,
    const renderer::EffectUvMotion& motion
) {
    m_mesh->bind(shader);
    glUniform1i(shader.uniform("water_mesh"), 0);
    glUniform1i(shader.uniform("water_uv_animate"), motion.enabled ? 1 : 0);
    glUniform4f(
        shader.uniform("water_uv_motion"),
        motion.offset[0],
        motion.offset[1],
        motion.amplitude,
        float(motion.phase & 511u)
    );
    glUniform1i(shader.uniform("water_debug_view"), m_debug.view);
    glUniform1f(shader.uniform("water_debug_height_limit"), m_debug.displacement_limit);
    glUniform1f(shader.uniform("water_fine_strength"), m_debug.fine_normal_strength);
    glUniform1i(shader.uniform("water_surface"), m_enabled ? int(surface) : 0);
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
    // All statically referenced Vulkan descriptors must be valid even on the original-
    // material branch. Bind once per effects pass, then only change the semantic surface.
    if (m_bound) {
        return;
    }
    m_bound = true;
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

void Water::bind_contact(const renderer::Shader& shader) const {
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

WaterDebugSettings water_debug_settings() {
    std::lock_guard lock(water_debug_mutex);
    return water_debug;
}

void set_water_debug_settings(WaterDebugSettings settings) {
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    settings.current_scale = bounded(settings.current_scale, 0, 2.5f, 1);
    settings.friction = bounded(settings.friction, 0, 4, .12f);
    settings.outlet_damping = bounded(settings.outlet_damping, 0, 30, 12);
    settings.interaction_strength = bounded(settings.interaction_strength, 0, 4, 1.4f);
    settings.interaction_radius = bounded(settings.interaction_radius, .1f, 2, .54772256f);
    settings.displacement_limit = bounded(settings.displacement_limit, 0, .5f, .25f);
    settings.fine_normal_strength = bounded(settings.fine_normal_strength, 0, 2, 1);
    settings.view = std::clamp(settings.view, 0, 4);
    std::lock_guard lock(water_debug_mutex);
    water_debug = settings;
}

void reset_water_simulation() {
    water_reset_epoch.fetch_add(1);
}

bool water_available() {
    return water_ready.load();
}

bool bind_water_contact(const renderer::Shader& shader) {
    if (!contact_source || !water_ready.load()) {
        return false;
    }
    contact_source->bind_contact(shader);
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
    const auto path = directory / "water.bin";
    if (!std::filesystem::exists(path)) {
        log::info("Novalis enhanced water needs a local water.bin bake beside manifest.json");
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

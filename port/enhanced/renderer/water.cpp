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
#include "enhanced/renderer/local_lights.h"
#include "enhanced/renderer/reflection_material.h"
#include "enhanced/renderer/water_internal.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
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

// Compose water optics with shared lighting, reflection and scene-transmission helpers.
std::string Water::fragment_source() const {
    std::string source = std::string(lighting::shaders::water_window_glsl) + "\n"
                         + std::string(lighting::shaders::water_swell_glsl) + "\n"
                         + std::string(lighting::shaders::shading_brdf_glsl) + "\n"
                         + local_light_shader_source() + "\n"
                         + std::string(lighting::shaders::environment_glsl) + "\n"
                         + std::string(lighting::shaders::water_transmission_glsl) + "\n"
                         + std::string(lighting::shaders::water_glsl);
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderVariants)
        && renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader)) {
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
    if (renderer::render_opt::enabled(renderer::render_opt::Opt::ShaderVariants)
        && renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader)) {
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
    return m_mesh->prepare(strip, vertices);
}

void Water::upload_meshes() {
    m_mesh->upload();
}

// Copy once per visible water pass; source and sampled images never alias.
void Water::prepare_scene(const renderer::FrameInput&, const renderer::RenderTarget& target) {
    if (!m_enabled || target.width <= 0 || target.height <= 0) {
        return;
    }
    if (m_background.width() != target.width || m_background.height() != target.height) {
        std::string error;
        if (!m_background.create(target.width, target.height, error, 1, true)) {
            log::warn("Water scene transmission unavailable: {}", error);
            glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
            return;
        }
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_background.id());
    for (const auto mask : {GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT}) {
        glBlitFramebuffer(
            0, 0, target.width, target.height, 0, 0, target.width, target.height, mask, GL_NEAREST
        );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    m_background_ready = true;
}

// Enhanced water already includes its background; legacy GS blending would add it twice.
void Water::draw_mesh(const renderer::Shader& shader, const renderer::EffectMesh& mesh) {
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
    glUniform1i(shader.uniform("water_scene_ready"), m_background_ready ? 1 : 0);
    glUniform2f(
        shader.uniform("water_scene_extent"),
        float(std::max(1, m_background.width())),
        float(std::max(1, m_background.height()))
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
    glBindTexture(GL_TEXTURE_2D, m_background_ready ? m_background.colour_texture() : m_maps[1]);
    glActiveTexture(GL_TEXTURE0 + 27);
    glBindTexture(GL_TEXTURE_2D, m_background_ready ? m_background.depth_texture() : m_maps[1]);
    const auto material = reflection_material_settings(ReflectionSurface::Water);
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

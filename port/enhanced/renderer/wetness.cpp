// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/renderer/wetness.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced/renderer/water.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"

namespace openrac::enhanced {
namespace {
using namespace gl;

struct Vertex {
    std::array<float, 4> position{};
    std::array<unsigned, 4> joints{};
    std::array<float, 4> weights{};
};

static_assert(sizeof(Vertex) == 48);
std::atomic<bool> debug{false};
std::mutex material_mutex;
WetnessMaterialSettings material;
}  // namespace

WetnessMaterialSettings wetness_material_settings() {
    const std::lock_guard lock(material_mutex);
    return material;
}

void set_wetness_material_settings(WetnessMaterialSettings settings) {
    const auto bounded = [](float value, float low, float high, float fallback) {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    const WetnessMaterialSettings defaults;
    settings.diffuse_scale = bounded(settings.diffuse_scale, 0, 1, defaults.diffuse_scale);
    settings.specular_strength = bounded(settings.specular_strength, 0, 4, defaults.specular_strength);
    settings.dry_roughness = bounded(settings.dry_roughness, .08f, 1, defaults.dry_roughness);
    settings.wet_roughness = bounded(settings.wet_roughness, .08f, 1, defaults.wet_roughness);
    settings.reflectivity = bounded(settings.reflectivity, 0, .16f, defaults.reflectivity);
    const std::lock_guard lock(material_mutex);
    material = settings;
}

bool wetness_debug_view() {
    return debug.load();
}

void set_wetness_debug_view(bool value) {
    debug.store(value);
}

HeroWetness::~HeroWetness() {
    release();
}

void HeroWetness::load(const scene::LevelData& level) {
    release();
    if (!gl::is_vulkan() || !gl::enhanced_rendering_available()) {
        return;
    }
    std::vector<Vertex> data(1);
    const auto found = level.moby_classes.find(0);
    if (found != level.moby_classes.end() && !found->second.second
        && found->second.first < level.models.size()) {
        m_model = found->second.first;
        unsigned first = ~0u, last = 0;
        for (const auto& primitive : level.models[m_model].primitives) {
            for (unsigned n = 0; n < primitive.index_count; ++n) {
                auto index = level.indices.at(primitive.first_index + n);
                first = std::min(first, index);
                last = std::max(last, index);
            }
        }
        if (first != ~0u && last < level.vertices.size() && last - first < 131072) {
            m_first = first;
            m_count = last - first + 1;
            data.resize(m_count);
            for (const auto& primitive : level.models[m_model].primitives) {
                for (unsigned n = 0; n < primitive.index_count; ++n) {
                    unsigned index = level.indices[primitive.first_index + n];
                    const auto& v = level.vertices[index];
                    auto& out = data[index - first];
                    std::copy_n(v.position, 3, out.position.begin());
                    out.position[3] = 1;
                    std::copy_n(v.joints, 4, out.joints.begin());
                    std::copy_n(v.weights, 4, out.weights.begin());
                }
            }
        }
    }
    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertices);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(data.size() * sizeof(Vertex)),
        data.data(),
        GL_STATIC_DRAW
    );
    std::vector<float> zero(data.size());
    glGenBuffers(1, &m_state);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_state);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        GLsizeiptr(zero.size() * sizeof(float)),
        zero.data(),
        GL_DYNAMIC_DRAW
    );
    glGenTextures(1, &m_dummy);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_dummy);
    std::array<float, 4> empty{};
    glTexImage2D(
        GL_TEXTURE_2D, 0, renderer::world::gl::GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, empty.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    std::string source(lighting::shaders::wetness_comp);
    source.insert(
        source.find('\n') + 1,
        std::string(lighting::shaders::water_window_glsl) + "\n"
            + std::string(lighting::shaders::water_swell_glsl) + "\n"
    );
    std::string error;
    if (!m_update.build_compute("Ratchet surface wetness", source, error)) {
        log::warn("Ratchet wetness unavailable: {}", error);
    }
}

void HeroWetness::prepare(
    const renderer::FrameInput& input, unsigned palette, const std::optional<scene::Instance>& hero
) {
    if (!input.enhanced_water) {
        m_active = false;
        m_reset = true;
        return;
    }
    if (!hero || hero->model != m_model || hero->palette < 0 || !palette || !m_count
        || !m_update.id()) {
        m_active = false;
        return;
    }
    m_active = true;
    m_palette = hero->palette;
    if (m_frame == input.frame && !m_reset && m_identity == hero->temporal_id) {
        return;
    }
    const bool reset =
        m_reset || m_frame == ~0ull || input.frame < m_frame || m_identity != hero->temporal_id;
    const float dt =
        reset ? 0.f : float(std::min<std::uint64_t>(input.frame - m_frame, 3600)) / 60.f;
    const stats::Scope timing("Ratchet wetness", stats::kGpu);
    m_update.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_vertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_state);
    // Valid fallback descriptors also cover a missing water material without any CPU contact test.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_vertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_vertices);
    glActiveTexture(GL_TEXTURE0 + 12);
    glBindTexture(GL_TEXTURE_2D, m_dummy);
    glActiveTexture(GL_TEXTURE0 + 14);
    glBindTexture(GL_TEXTURE_2D, m_dummy);
    const bool water = bind_water_contact(m_update);
    glUniform1i(m_update.uniform("contact_enabled"), water ? 1 : 0);
    glUniform1i(m_update.uniform("water_coast_map"), 12);
    glUniform1i(m_update.uniform("water_wave_map"), 14);
    glUniform1i(m_update.uniform("palette_texture"), 1);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, palette);
    glUniform1i(m_update.uniform("vertex_count"), int(m_count));
    glUniform1i(m_update.uniform("palette_base"), m_palette);
    glUniform1i(m_update.uniform("reset_history"), reset ? 1 : 0);
    glUniform1f(m_update.uniform("dry_amount"), dt / 20.f);
    glUniformMatrix4fv(m_update.uniform("hero_model"), 1, GL_FALSE, hero->matrix.data());
    glDispatchCompute((m_count + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glActiveTexture(GL_TEXTURE0);
    m_frame = input.frame;
    m_identity = hero->temporal_id;
    m_reset = false;
}

void HeroWetness::bind(const renderer::Shader& shader) const {
    glUniform1i(shader.uniform("hero_wetness_enabled"), m_active ? 1 : 0);
    glUniform1i(shader.uniform("hero_wetness_first"), int(m_first));
    glUniform1i(shader.uniform("hero_wetness_count"), int(m_count));
    glUniform1i(shader.uniform("hero_wetness_palette"), m_palette);
    glUniform1i(shader.uniform("hero_wetness_debug"), debug.load() ? 1 : 0);
    const auto settings = wetness_material_settings();
    glUniform4f(
        shader.uniform("hero_wetness_material"),
        settings.diffuse_scale,
        settings.specular_strength,
        settings.dry_roughness,
        settings.wet_roughness
    );
    glUniform1f(shader.uniform("hero_wetness_f0"), settings.reflectivity);
    if (m_state) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, m_state);
    }
}

void HeroWetness::release() {
    m_update.release();
    if (m_vertices) {
        glDeleteBuffers(1, &m_vertices);
    }
    if (m_state) {
        glDeleteBuffers(1, &m_state);
    }
    if (m_dummy) {
        glDeleteTextures(1, &m_dummy);
    }
    m_vertices = m_state = m_dummy = m_first = m_count = 0;
    m_model = ~0u;
    m_palette = -1;
    m_frame = ~0ull;
    m_identity = 0;
    m_active = false;
    m_reset = true;
}
}  // namespace openrac::enhanced

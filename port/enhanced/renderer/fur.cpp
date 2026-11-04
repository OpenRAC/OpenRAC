// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Owns the optional hero fur map and vertex selection used by coverage diagnostics.
#include "enhanced/renderer/fur.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

#include "common/log.h"
#include "renderer/gl.h"

namespace openrac::enhanced {
namespace {
// Scene samplers through 25 are reserved by existing lighting, water and attachment bindings.
constexpr unsigned kFurUnit = 26;
constexpr unsigned kDetailUnit = 27;

// Session-only diagnostics are shared by the UI and render worker.
std::atomic<FurDebug>& debug_state() {
    static std::atomic<FurDebug> debug{FurDebug::Off};
    return debug;
}

// Uploads a linear UNORM data map; no colour-space conversion or retail texture modification.
unsigned upload_map(const renderer::Rgba8Image& map, unsigned unit, bool mipmapped) {
    unsigned texture = 0;
    gl::glGenTextures(1, &texture);
    gl::glActiveTexture(gl::GL_TEXTURE0 + unit);
    gl::glBindTexture(gl::GL_TEXTURE_2D, texture);
    gl::glTexImage2D(
        gl::GL_TEXTURE_2D,
        0,
        gl::GL_RGBA8,
        map.width,
        map.height,
        0,
        gl::GL_RGBA,
        gl::GL_UNSIGNED_BYTE,
        map.pixels.data()
    );
    gl::glTexParameteri(
        gl::GL_TEXTURE_2D,
        gl::GL_TEXTURE_MIN_FILTER,
        mipmapped ? gl::GL_LINEAR_MIPMAP_LINEAR : gl::GL_LINEAR
    );
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
    if (mipmapped) {
        gl::glGenerateMipmap(gl::GL_TEXTURE_2D);
    }
    gl::glActiveTexture(gl::GL_TEXTURE0);
    return texture;
}
}  // namespace

FurDebug fur_debug_view() {
    return debug_state().load();
}

void set_fur_debug_view(FurDebug view) {
    debug_state() = view >= FurDebug::Off && view <= FurDebug::DetailNormals ? view : FurDebug::Off;
}

HeroFur::~HeroFur() {
    release();
}

void HeroFur::load(const scene::LevelData& level, const FurProfile& profile) {
    release();
    if (!gl::is_vulkan() || !gl::enhanced_rendering_available()) {
        return;
    }
    m_surface = identify_fur_surface(level, profile);
    // Which body took the detail, for checking a game's profile against its export.
    if (m_surface) {
        log::info(
            "fur detail: {} of {} body vertices ({} profile)",
            std::count(m_surface->vertices.begin(), m_surface->vertices.end(), 1u),
            m_surface->count,
            profile.game
        );
        const auto& primitive = level.models[m_surface->model].primitives[m_surface->primitive];
        m_first_index = primitive.first_index;
        m_index_count = primitive.index_count;
        // Authored meshes use different units; a coat fraction must follow the body's scale.
        for (unsigned axis = 0; axis < 3; ++axis) {
            float low = std::numeric_limits<float>::max(), high = -low;
            for (unsigned vertex = 0; vertex < m_surface->count; ++vertex) {
                const float p = level.vertices[m_surface->first + vertex].position[axis];
                low = std::min(low, p);
                high = std::max(high, p);
            }
            if (std::isfinite(high - low)) {
                m_body_extent = std::max(m_body_extent, high - low);
            }
        }
    }
    // A complete fallback descriptor also covers levels without the hero model.
    auto map =
        m_surface
            ? make_fur_map(
                  level, level.models[m_surface->model].primitives[m_surface->primitive], profile
              )
            : renderer::Rgba8Image(1, 1);
    if (!map.width || !map.height) {
        m_surface.reset();
        map = renderer::Rgba8Image(1, 1);
    }
    m_map = upload_map(map, kFurUnit, false);
    auto detail = m_surface ? make_fur_detail_map(map) : renderer::Rgba8Image(1, 1);
    if (!m_surface) {
        detail.set_texel(0, 0, renderer::rgba(128, 128, 128, 255));
    }
    m_detail = upload_map(detail, kDetailUnit, true);
    const std::vector<unsigned> empty{0};
    const auto& vertices = m_surface ? m_surface->vertices : empty;
    gl::glGenBuffers(1, &m_vertices);
    gl::glBindBuffer(gl::GL_SHADER_STORAGE_BUFFER, m_vertices);
    gl::glBufferData(
        gl::GL_SHADER_STORAGE_BUFFER,
        gl::GLsizeiptr(vertices.size() * sizeof(unsigned)),
        vertices.data(),
        gl::GL_STATIC_DRAW
    );
}

void HeroFur::prepare(const std::optional<scene::Instance>& hero, const renderer::Vec3& eye) {
    m_palette = hero && m_surface && hero->model == m_surface->model ? hero->palette : -1;
    m_eye_distance = 0;
    if (hero) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float delta = hero->matrix[12 + axis] - eye[axis];
            m_eye_distance += delta * delta;
        }
        m_eye_distance = std::sqrt(m_eye_distance);
    }
}

void HeroFur::bind(const renderer::Shader& shader) const {
    bind(shader, fur_material_settings());
}

void HeroFur::bind(const renderer::Shader& shader, const FurMaterialSettings& settings) const {
    const auto location = shader.uniform("hero_fur_debug");
    // Velocity variants consume eligibility without the colour diagnostic uniform.
    if (shader.uniform("hero_fur_palette") < 0) {
        return;
    }
    gl::glUniform1i(location, int(fur_debug_view()));
    bind_material(shader, settings);
    gl::glUniform1i(shader.uniform("hero_fur_palette"), m_palette);
    gl::glUniform1i(shader.uniform("hero_fur_first"), m_surface ? int(m_surface->first) : 0);
    gl::glUniform1i(shader.uniform("hero_fur_count"), m_surface ? int(m_surface->count) : 0);
    gl::glUniform1i(shader.uniform("hero_fur_map"), int(kFurUnit));
    gl::glUniform1i(shader.uniform("hero_fur_detail"), int(kDetailUnit));
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 10, m_vertices);
    gl::glActiveTexture(gl::GL_TEXTURE0 + kFurUnit);
    gl::glBindTexture(gl::GL_TEXTURE_2D, m_map);
    gl::glActiveTexture(gl::GL_TEXTURE0 + kDetailUnit);
    gl::glBindTexture(gl::GL_TEXTURE_2D, m_detail);
    gl::glActiveTexture(gl::GL_TEXTURE0);
    bind_shell(shader, 0);
}

void HeroFur::bind_material(
    const renderer::Shader& shader, const FurMaterialSettings& settings
) const {
    gl::glUniform1f(shader.uniform("hero_fur_environment_strength"), settings.environment_strength);
    gl::glUniform1i(
        shader.uniform("hero_fur_detail_enabled"),
        settings.enabled && (settings.strength > 0 || settings.contrast > 0) ? 1 : 0
    );
    gl::glUniform4f(
        shader.uniform("hero_fur_detail_parameters"),
        settings.strength,
        settings.contrast,
        settings.fade_distance * .5f,
        settings.fade_distance
    );
    gl::glUniform1i(
        shader.uniform("hero_fur_lighting_enabled"),
        settings.lighting_enabled && (settings.highlight > 0 || settings.scattering > 0) ? 1 : 0
    );
    gl::glUniform4f(
        shader.uniform("hero_fur_lighting_parameters"),
        settings.highlight,
        settings.softness,
        settings.directionality,
        settings.scattering
    );
}

int HeroFur::shell_layers(std::uint32_t model, std::uint32_t first, std::uint32_t count) const {
    // Unknown layouts, non-player bodies and original rendering never grow a coat.
    if (!gl::enhanced_rendering_available() || !m_surface || m_palette < 0
        || model != m_surface->model || first != m_first_index || count != m_index_count
        || m_body_extent <= 0) {
        return 0;
    }
    const auto settings = fur_material_settings();
    if (!settings.shells_enabled || settings.shell_length <= 0
        || m_eye_distance >= settings.shell_distance) {
        return 0;
    }
    return settings.shell_layers;
}

void HeroFur::bind_shell(const renderer::Shader& shader, int layer) const {
    const auto settings = fur_material_settings();
    gl::glUniform1i(shader.uniform("hero_fur_shell_enabled"), settings.shells_enabled ? 1 : 0);
    gl::glUniform1f(
        shader.uniform("hero_fur_shell"), float(layer) / float(settings.shell_layers + 1)
    );
    gl::glUniform1f(shader.uniform("hero_fur_shell_distance"), m_eye_distance);
    gl::glUniform4f(
        shader.uniform("hero_fur_shell_parameters"),
        settings.shell_length * m_body_extent,
        settings.shell_density,
        settings.shell_distance * .5f,
        settings.shell_distance
    );
}

void HeroFur::release() {
    if (m_vertices) {
        gl::glDeleteBuffers(1, &m_vertices);
    }
    m_vertices = 0;
    if (m_map) {
        gl::glDeleteTextures(1, &m_map);
    }
    m_map = 0;
    if (m_detail) {
        gl::glDeleteTextures(1, &m_detail);
    }
    m_detail = 0;
    m_palette = -1;
    m_first_index = m_index_count = 0;
    m_eye_distance = 0;
    m_surface.reset();
    m_body_extent = 0;
}
}  // namespace openrac::enhanced

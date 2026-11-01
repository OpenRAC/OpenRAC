// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Light authoring snapshots are copied before GPU upload; complete masks have no overflow path.
#include "enhanced/renderer/local_lights.h"

#include <algorithm>
#include <cmath>

#include "common/log.h"
#include "common/stats.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"

namespace openrac::enhanced {
using namespace gl;
namespace wgl = renderer::world::gl;

namespace {
// Texture units 28/29 are independent of water's transmission and material texture units.
constexpr unsigned kFirstUnit = 28;
constexpr std::size_t kAuthorLimit = 4096;
// Counts below this avoid compute overhead; GPU fixtures measure both paths independently.
constexpr unsigned kMinimumTiledLights = 16;
}  // namespace

// Keep every enhanced material on the same iterator and standalone-fixture fallback.
const std::string& local_light_shader_source() {
    const static std::string source = [] {
        std::string result = lighting::shaders::probes_glsl;
        result.insert(result.find("// Default local lights"), lighting::shaders::local_lights_glsl);
        return result;
    }();
    return source;
}

Result<void> LocalLights::set_lights(std::span<const LocalLight> lights) {
    if (lights.size() > kAuthorLimit) {
        return unexpected(Error{"at most 4096 authored local lights are supported"});
    }

    for (const auto& light : lights) {
        float axis = 0;

        for (unsigned i = 0; i < 3; ++i) {
            if (!std::isfinite(light.position[i]) || !std::isfinite(light.colour[i])
                || light.colour[i] < 0 || !std::isfinite(light.direction[i])
                || !std::isfinite(light.colour[i] * light.intensity)) {
                return unexpected(
                    Error{"local light position, colour and direction must be finite; colour must "
                          "be nonnegative"}
                );
            }

            axis += light.direction[i] * light.direction[i];
        }

        if (!std::isfinite(light.range) || light.range <= 0 || !std::isfinite(light.intensity)
            || light.intensity < 0) {
            return unexpected(
                Error{"local light range must be positive and intensity nonnegative"}
            );
        }

        if (light.spot
            && (!std::isfinite(axis) || axis < 1e-12f || !std::isfinite(light.inner_angle)
                || !std::isfinite(light.outer_angle) || light.inner_angle < 0
                || light.outer_angle < light.inner_angle || light.outer_angle >= 1.5707963f)) {
            return unexpected(
                Error{"spot light needs a nonzero direction and 0 <= inner <= outer < pi/2 radians"}
            );
        }
    }

    std::lock_guard lock(m_mutex);

    if (lights.size() + m_attached.size() > kAuthorLimit) {
        return unexpected(Error{"combined authored and attached light count exceeds 4096"});
    }

    m_pending.assign(lights.begin(), lights.end());
    ++m_revision;
    return {};
}

void LocalLights::ensure_buffers() const {
    if (m_buffers[0]) {
        return;
    }

    glGenBuffers(2, m_buffers.data());
    glGenTextures(2, m_textures.data());
    const Record empty{};

    for (unsigned i = 0; i < 2; ++i) {
        glBindBuffer(wgl::GL_TEXTURE_BUFFER, m_buffers[i]);
        glBufferData(wgl::GL_TEXTURE_BUFFER, sizeof(empty), &empty, GL_DYNAMIC_DRAW);
        glActiveTexture(GL_TEXTURE0 + kFirstUnit + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i]);
        glTexBuffer(wgl::GL_TEXTURE_BUFFER, i ? wgl::GL_R32UI : wgl::GL_RGBA32F, m_buffers[i]);
    }

    glActiveTexture(GL_TEXTURE0);
}

// Only spot directions need normalization; points retain a zero type and backlight.
LocalLights::Record LocalLights::pack_light(const LocalLight& light) {
    Record record{};

    for (unsigned a = 0; a < 3; ++a) {
        record[0][a] = light.position[a];
        record[1][a] = light.colour[a] * light.intensity;
    }

    record[0][3] = light.range;

    if (light.spot) {
        float length = 0;

        for (float x : light.direction) {
            length += x * x;
        }

        for (unsigned a = 0; a < 3; ++a) {
            record[2][a] = light.direction[a] / std::sqrt(length);
        }

        record[2][3] = std::cos(light.outer_angle);
        record[3][0] = std::cos(light.inner_angle);
        record[3][1] = 1;
    }

    return record;
}

// Freeze one authoring snapshot per game frame; unchanged authored records stay resident.
// Attachment snapshots are already validated by the profile parser and transform resolver.
Result<void> LocalLights::set_attached_lights(std::span<const LocalLight> lights) {
    std::lock_guard lock(m_mutex);

    if (lights.size() + m_pending.size() > kAuthorLimit) {
        return unexpected(Error{"combined authored and attached light count exceeds 4096"});
    }

    if (std::equal(lights.begin(), lights.end(), m_attached.begin(), m_attached.end())) {
        return {};
    }

    m_attached.assign(lights.begin(), lights.end());
    ++m_revision;
    return {};
}

// The mutex protects snapshots only; GPU work stays on the render thread.
bool LocalLights::capture_edits(bool new_frame, std::vector<LocalLight>& pending) {
    if (!new_frame) {
        return false;
    }

    std::lock_guard lock(m_mutex);

    if (m_uploaded_revision == m_revision) {
        return false;
    }

    pending = m_pending;
    pending.insert(pending.end(), m_attached.begin(), m_attached.end());
    m_uploaded_revision = m_revision;
    return true;
}

// Apply the captured lights and the current native bank once per frame.
void LocalLights::prepare(std::uint64_t frame, const std::array<float, 64>& game, bool enabled) {
    if (!is_vulkan() || !enhanced_rendering_available()) {
        return;
    }

    const bool new_frame = frame != m_frame || m_enabled != enabled;

    if (!new_frame && game == m_game) {
        return;
    }

    m_game = game;
    m_frame = frame;
    m_enabled = enabled;
    m_ready = false;
    ensure_buffers();

    if (!enabled) {
        m_count = 0;
        return;
    }

    std::vector<LocalLight> pending;
    const bool edited = capture_edits(new_frame, pending);
    std::array<Record, 8> original{};

    for (std::size_t i = 0; i < 8; ++i) {
        std::copy_n(game.data() + i * 8 + 4, 4, original[i][0].data());
        std::copy_n(game.data() + i * 8, 4, original[i][1].data());
    }

    glBindBuffer(wgl::GL_TEXTURE_BUFFER, m_buffers[0]);

    if (edited || m_records.empty()) {
        m_records.resize(8 + pending.size());
        std::copy(original.begin(), original.end(), m_records.begin());

        for (std::size_t i = 0; i < pending.size(); ++i) {
            m_records[i + 8] = pack_light(pending[i]);
        }

        glBufferData(
            wgl::GL_TEXTURE_BUFFER,
            GLsizeiptr(m_records.size() * sizeof(Record)),
            m_records.data(),
            GL_DYNAMIC_DRAW
        );
    } else if (!std::equal(original.begin(), original.end(), m_records.begin())) {
        // A moving game light changes only its eight-slot prefix, not the static authored table.
        std::copy(original.begin(), original.end(), m_records.begin());
        glBufferSubData(wgl::GL_TEXTURE_BUFFER, 0, sizeof(original), original.data());
    }

    m_count = unsigned(m_records.size());
    stats::gauge("local lights", double(m_count));
}

// Device limits and shader failures leave the complete all-light table usable.
bool LocalLights::prepare_culling(int width, int height) {
    std::string error;

    if (!m_cull.id()
        && !m_cull.build_compute(
            "Forward+ light culling", lighting::shaders::local_lights_comp, error
        )) {
        log::warn("Forward+ culling unavailable, using all local lights: {}", error);
        m_failed = true;
        return false;
    }

    const auto x = unsigned((width + 15) / 16), y = unsigned((height + 15) / 16);
    const auto words = std::size_t(x) * y * ((m_count + 31) / 32) * 2;
    GLint limit = 0;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);

    if (words > std::size_t(limit)) {
        return false;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[1]);

    if (words != m_mask_words) {
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, GLsizeiptr(words * sizeof(unsigned)), nullptr, GL_DYNAMIC_DRAW
        );
        m_mask_words = words;
    }

    return true;
}

bool LocalLights::dispatch(
    unsigned depth, const renderer::Camera& camera, int width, int height, bool force_tiles
) {
    m_ready = false;

    if (!m_enabled || (m_count < kMinimumTiledLights && !force_tiles) || reference || m_failed
        || !depth || width <= 0 || height <= 0) {
        return false;
    }

    if (!prepare_culling(width, height)) {
        return false;
    }

    const auto x = unsigned((width + 15) / 16), y = unsigned((height + 15) / 16);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, m_buffers[1]);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    m_cull.use();
    m_clip = renderer::multiply(camera.projection, camera.view);
    glUniformMatrix4fv(m_cull.uniform("world_to_clip"), 1, GL_FALSE, m_clip.data());
    glUniform2f(m_cull.uniform("scene_extent"), float(width), float(height));
    glUniform1i(m_cull.uniform("light_count"), int(m_count));
    glUniform1i(m_cull.uniform("scene_depth"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth);
    glUniform1i(m_cull.uniform("lights"), kFirstUnit);
    glActiveTexture(GL_TEXTURE0 + kFirstUnit);
    glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[0]);
    {
        const stats::Scope pass("Forward+ light culling", stats::kGpu);
        glDispatchCompute(x, y, 1);
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_width = width;
    m_height = height;
    m_ready = true;
    stats::gauge("local light masks bytes", double(m_mask_words * sizeof(unsigned)));
    return true;
}

void LocalLights::bind(
    const renderer::Shader& shader, const renderer::Camera* camera, bool depth_matched
) const {
    if (shader.uniform("forward_count") < 0) {
        return;
    }

    ensure_buffers();
    std::array<int, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    const bool tiled = m_ready && !reference && camera
                       && viewport == std::array<int, 4>{0, 0, m_width, m_height}
                       && renderer::multiply(camera->projection, camera->view) == m_clip;
    glUniform1i(shader.uniform("forward_count"), m_enabled ? int(m_count) : 0);
    glUniform1i(shader.uniform("forward_tiled"), tiled ? 1 : 0);
    GLint samples = 1;
    glGetIntegerv(GL_SAMPLES, &samples);
    // Resolved depth may omit a surface covered only by other MSAA samples.
    glUniform1i(shader.uniform("forward_transparent"), depth_matched && samples <= 1 ? 0 : 1);
    glUniform4f(
        shader.uniform("forward_view"),
        float(viewport[0]),
        float(viewport[1]),
        float(viewport[2]),
        float(viewport[3])
    );

    for (unsigned i = 0; i < 2; ++i) {
        glUniform1i(shader.uniform(i ? "forward_masks" : "forward_lights"), kFirstUnit + i);
        glActiveTexture(GL_TEXTURE0 + kFirstUnit + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i]);
    }

    glActiveTexture(GL_TEXTURE0);
}

void LocalLights::release() {
    m_cull.release();
    glDeleteBuffers(2, m_buffers.data());
    glDeleteTextures(2, m_textures.data());
    m_buffers = {};
    m_textures = {};
    m_records.clear();
    m_count = 0;
    m_mask_words = 0;
    m_ready = false;
    m_enabled = false;
    m_failed = false;
    m_frame = ~std::uint64_t{0};
    m_uploaded_revision = ~std::uint64_t{0};
    std::lock_guard lock(m_mutex);
    m_pending.clear();
    m_attached.clear();
    ++m_revision;
}
}  // namespace openrac::enhanced

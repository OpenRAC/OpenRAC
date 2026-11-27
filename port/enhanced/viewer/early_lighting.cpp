// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/viewer/early_lighting.h"

#include <cmath>
#include <cstdint>

#include "common/stats.h"
#include "enhanced/renderer/compute_graph.h"
#include "enhanced/renderer/shader_recipe_sources.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"

namespace openrac::lighting {
using namespace openrac::gl;
namespace wgl = renderer::world::gl;

namespace {
constexpr int kCloudNoiseSize = 64;
// Vulkan tracks 2D and buffer bindings separately. Slot 18 also carries early AO or water bodies
// in the buffer binding. The mesh's water maps occupy the 2D bindings on slots 19 and 20.
constexpr int kCloudNoiseUnit = 18;

// Populate a repeatable RGBA tile once. Each channel is independent periodic value noise.
unsigned make_cloud_noise() {
    const auto noise = [](int x, int y, int cells, unsigned channel) {
        const auto corner = [cells, channel](unsigned a, unsigned b) {
            std::uint32_t hash = ((a % cells) * 1597334677u) ^ ((b % cells) * 3812015801u)
                                 ^ ((channel + 1) * 2798796415u);
            hash = (hash ^ (hash >> 16)) * 2246822519u;
            return static_cast<float>((hash ^ (hash >> 13)) & 255u) / 255.f;
        };
        const int span = kCloudNoiseSize / cells;
        const unsigned a = x / span, b = y / span;
        float u = static_cast<float>(x % span) / static_cast<float>(span);
        float v = static_cast<float>(y % span) / static_cast<float>(span);
        u = u * u * (3.f - 2.f * u);
        v = v * v * (3.f - 2.f * v);
        return std::lerp(
            std::lerp(corner(a, b), corner(a + 1, b), u),
            std::lerp(corner(a, b + 1), corner(a + 1, b + 1), u),
            v
        );
    };
    std::array<std::uint8_t, kCloudNoiseSize * kCloudNoiseSize * 4> pixels{};
    for (int y = 0; y < kCloudNoiseSize; ++y) {
        for (int x = 0; x < kCloudNoiseSize; ++x) {
            for (unsigned channel = 0; channel < 4; ++channel) {
                const float value =
                    .75f * noise(x, y, 8, channel) + .25f * noise(x, y, 16, channel + 4);
                pixels[(y * kCloudNoiseSize + x) * 4 + channel] =
                    static_cast<std::uint8_t>(std::lround(255.f * value));
            }
        }
    }
    unsigned texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        kCloudNoiseSize,
        kCloudNoiseSize,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}
}  // namespace

void EarlyLighting::bind_cloud_noise(const renderer::Shader& shader, bool enabled) const {
    const auto uniform = shader.uniform("cloud_noise_texture");
    if (uniform < 0) {
        return;
    }

    glUniform1i(uniform, kCloudNoiseUnit);
    glActiveTexture(GL_TEXTURE0 + kCloudNoiseUnit);
    glBindTexture(GL_TEXTURE_2D, enabled ? m_cloud_noise : 0);
    glActiveTexture(GL_TEXTURE0);
}

bool EarlyLighting::supported() {
    static const bool& off = stats::probe(
        "OPENRAC_EARLY_LIGHTING_OFF", "Reference: full-resolution GI and cascades (reload level)"
    );
    return enhanced_rendering_available() && !off && glDispatchCompute && glMemoryBarrier;
}

void EarlyLighting::prepare_cloud_noise(const renderer::FrameInput& input) {
    if (m_cloud_noise || input.cloud_shadows.effective_strength() <= 0.f) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + kCloudNoiseUnit);
    m_cloud_noise = make_cloud_noise();
    glActiveTexture(GL_TEXTURE0);
}

bool EarlyLighting::render(
    unsigned depth,
    const renderer::FrameInput& input,
    const renderer::RenderTarget& target,
    const std::function<void(const renderer::Shader&)>& bind_lighting,
    std::string& error
) {
    m_ready = false;
    const int width = target.width, height = target.height;
    if (m_failed || !supported() || !depth || width < 2 || height < 2
        || input.camera.projection[11] != -1.0f || input.camera.projection[0] == 0
        || input.camera.projection[5] == 0) {
        return false;
    }
    const bool normal =
        input.early_lighting_quality == renderer::EarlyLightingQuality::Normal
        && renderer::render_opt::enabled(renderer::render_opt::Opt::EarlyLightingQuarter);
    const int s = normal ? 4 : 2;
    const std::size_t cells = std::size_t((width + s - 1) / s) * ((height + s - 1) / s);
    const bool cloud_guides = input.cloud_shadows.effective_strength() > 0.f;
    const std::size_t guide_words = cells * (cloud_guides ? 3 : 2);
    GLint limit = 0;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    if (guide_words > std::size_t(limit)) {
        return false;
    }
    // Shadow filtering Fast: four-tap shadows, one cascade at a time.
    const bool fast = input.shadow_filter == renderer::ShadowFilter::Fast;
    renderer::Shader& compute = m_compute[(fast ? 1 : 0) + (s == 4 ? 2 : 0)];
    if (!compute.id()) {
        const auto source = enhanced::recipe_sources::early_lighting(
            s,
            fast,
            renderer::render_opt::enabled(renderer::render_opt::Opt::EarlyProbeSetup),
            shader_sources::runtime_options()
        );
        if (!compute.build_compute(
                fast ? "early GI and cascades (fast shadows)" : "early GI and cascades",
                source,
                error
            )) {
            m_failed = true;
            return false;
        }
    }
    if (!m_buffers[0]) {
        glGenBuffers(2, m_buffers.data());
        glGenTextures(2, m_textures.data());
    }
    for (unsigned i = 0; i < 2; ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[i]);
        if (width != m_width || height != m_height || s != m_scale
            || (i == 1 && cloud_guides != m_cloud_guides)) {
            glBufferData(
                GL_SHADER_STORAGE_BUFFER, (i ? guide_words : cells) * 4, nullptr, GL_DYNAMIC_DRAW
            );
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9 + i, m_buffers[i]);
        glActiveTexture(GL_TEXTURE0 + 16 + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i]);
        glTexBuffer(wgl::GL_TEXTURE_BUFFER, i ? wgl::GL_R32UI : GL_RGBA8, m_buffers[i]);
    }
    m_width = width;
    m_height = height;
    m_scale = s;
    m_cloud_guides = cloud_guides;
    prepare_cloud_noise(input);
    compute.use();
    bind_lighting(compute);
    glUniform1i(compute.uniform("early_cloud_guides"), cloud_guides ? 1 : 0);
    bind_cloud_noise(compute, cloud_guides);
    glBindTexture(GL_TEXTURE_2D, depth);
    glUniform1i(compute.uniform("early_depth"), 0);
    glUniform1i(compute.uniform("surface_normals"), 19);
    glUniform1i(compute.uniform("surface_normals_enabled"), target.normal_texture != 0 ? 1 : 0);
    glActiveTexture(GL_TEXTURE0 + 19);
    glBindTexture(GL_TEXTURE_2D, target.normal_texture);
    glActiveTexture(GL_TEXTURE0);
    glUniformMatrix4fv(
        compute.uniform("early_projection"), 1, GL_FALSE, input.camera.projection.data()
    );
    glUniformMatrix4fv(compute.uniform("early_view"), 1, GL_FALSE, input.camera.view.data());
    const std::array<enhanced::ComputeOutput, 2> outputs{
        {{m_buffers[0], cells * 4, true}, {m_buffers[1], guide_words * 4, true}}
    };
    // Input capture follows lighting binding so probes, AO, tile masks and shadow maps match the dispatch.
    if (!enhanced::run_compute_graph(
            "early GI and cascades",
            outputs,
            [&] {
                const stats::Scope pass("early GI and cascades", stats::kGpu);
                glDispatchCompute(
                    unsigned((width + 8 * s - 1) / (8 * s)),
                    unsigned((height + 8 * s - 1) / (8 * s)),
                    1
                );
            },
            error,
            enhanced::ComputeInputs::All,
            enhanced::SampledInputExports::Continuation
        )) {
        return false;
    }
    m_clip = renderer::multiply(input.camera.projection, input.camera.view);
    m_frame = input.frame;
    m_target = target.framebuffer;
    m_ready = true;
    glBindTexture(GL_TEXTURE_2D, 0);
    stats::gauge("early lighting bytes", double((cells + guide_words) * 4));
    stats::gauge("early lighting texels", double(cells));
    return true;
}

void EarlyLighting::bind(
    const renderer::Shader& shader,
    const renderer::FrameInput& input,
    const renderer::RenderState& state
) const {
    bind_cloud_noise(shader, input.cloud_shadows.effective_strength() > 0.f);
    const bool active = matches(input, state);
    glUniform1i(shader.uniform("early_lighting_enabled"), active ? 1 : 0);
    if (!active) {
        return;
    }
    glUniform2f(shader.uniform("early_extent"), float(m_width), float(m_height));
    glUniform1i(shader.uniform("early_scale"), m_scale);
    glUniform1i(shader.uniform("early_cloud_guides"), m_cloud_guides ? 1 : 0);
    glUniformMatrix4fv(
        shader.uniform("early_projection"), 1, GL_FALSE, input.camera.projection.data()
    );
    glUniform1i(shader.uniform("early_values"), 16);
    glUniform1i(shader.uniform("early_guides"), 17);
    for (unsigned i = 0; i < 2; ++i) {
        glActiveTexture(GL_TEXTURE0 + 16 + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i]);
    }
    glActiveTexture(GL_TEXTURE0);
}

bool EarlyLighting::matches(
    const renderer::FrameInput& input, const renderer::RenderState& state
) const {
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT, viewport);
    return state.early_lighting && m_ready && m_frame == input.frame
           && m_target == state.target.framebuffer && viewport[0] == 0 && viewport[1] == 0
           && viewport[2] == m_width && viewport[3] == m_height && m_width == state.target.width
           && m_height == state.target.height
           && m_clip == renderer::multiply(input.camera.projection, input.camera.view);
}

std::span<const unsigned> EarlyLighting::buffers(
    const renderer::FrameInput& input, const renderer::RenderState& state
) const {
    // Stale caches cannot supply dependencies for another view or target.
    if (!matches(input, state)) {
        return {};
    }

    return m_buffers;
}

void EarlyLighting::release() {
    for (auto& compute : m_compute) {
        compute.release();
    }
    if (m_buffers[0]) {
        glDeleteBuffers(2, m_buffers.data());
    }
    if (m_textures[0]) {
        glDeleteTextures(2, m_textures.data());
    }
    if (m_cloud_noise) {
        glDeleteTextures(1, &m_cloud_noise);
        m_cloud_noise = 0;
    }
    m_buffers = {};
    m_textures = {};
    m_width = m_height = 0;
    m_ready = false;
    m_failed = false;
}
}  // namespace openrac::lighting

// SPDX-License-Identifier: GPL-3.0-or-later
#include "enhanced/viewer/early_lighting.h"

#include "common/stats.h"
#include "enhanced/renderer/compute_graph.h"
#include "enhanced/viewer/probe_lighting.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/render_opt.h"
#include "renderer/world/gl_world.h"
#include "renderer_shaders.h"

namespace openrac::lighting {
using namespace openrac::gl;
namespace wgl = renderer::world::gl;

bool EarlyLighting::supported() {
    static const bool& off = stats::probe(
        "OPENRAC_EARLY_LIGHTING_OFF", "Reference: full-resolution GI and cascades (reload level)"
    );
    return enhanced_rendering_available() && !off && glDispatchCompute && glMemoryBarrier;
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
    const int s =
        renderer::render_opt::enabled(renderer::render_opt::Opt::EarlyLightingQuarter) ? 4 : 2;
    const std::size_t cells = std::size_t((width + s - 1) / s) * ((height + s - 1) / s);
    GLint limit = 0;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    if (cells * 2 > std::size_t(limit)) {
        return false;
    }
    // Shadow filtering Fast: four-tap shadows, one cascade at a time.
    const bool fast = input.shadow_filter == renderer::ShadowFilter::Fast;
    renderer::Shader& compute = m_compute[(fast ? 1 : 0) + (s == 4 ? 2 : 0)];
    if (!compute.id()) {
        std::string source = shaders::early_lighting_comp;
        source = probe_shader_source(std::move(source));
        if (renderer::render_opt::enabled(renderer::render_opt::Opt::EarlyProbeSetup)) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_PROBE_RECEIVER_SETUP 1\n");
        }
        if (fast) {
            source.insert(source.find('\n') + 1, "#define OPENRAC_CHEAP_SHADOWS 1\n");
        }
        source.insert(source.find('\n') + 1, renderer::shaders::surface_normal_glsl);
        source.insert(
            source.find('\n') + 1, "#define OPENRAC_PROBE_PIXEL early_pixel\nivec2 early_pixel;\n"
        );
        source.insert(
            source.find('\n') + 1, "#define OPENRAC_EARLY_SCALE " + std::to_string(s) + "\n"
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
        if (width != m_width || height != m_height || s != m_scale) {
            glBufferData(GL_SHADER_STORAGE_BUFFER, cells * (i ? 8 : 4), nullptr, GL_DYNAMIC_DRAW);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9 + i, m_buffers[i]);
        glActiveTexture(GL_TEXTURE0 + 16 + i);
        glBindTexture(wgl::GL_TEXTURE_BUFFER, m_textures[i]);
        glTexBuffer(wgl::GL_TEXTURE_BUFFER, i ? wgl::GL_R32UI : GL_RGBA8, m_buffers[i]);
    }
    compute.use();
    bind_lighting(compute);
    glActiveTexture(GL_TEXTURE0);
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
        {{m_buffers[0], cells * 4}, {m_buffers[1], cells * 8}}
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
            error
        )) {
        return false;
    }
    m_width = width;
    m_height = height;
    m_scale = s;
    m_clip = renderer::multiply(input.camera.projection, input.camera.view);
    m_frame = input.frame;
    m_target = target.framebuffer;
    m_ready = true;
    glBindTexture(GL_TEXTURE_2D, 0);
    stats::gauge("early lighting bytes", double(cells * 12));
    stats::gauge("early lighting texels", double(cells));
    return true;
}

void EarlyLighting::bind(
    const renderer::Shader& shader,
    const renderer::FrameInput& input,
    const renderer::RenderState& state
) const {
    const bool active = matches(input, state);
    glUniform1i(shader.uniform("early_lighting_enabled"), active ? 1 : 0);
    // Below effects detail High, early-lit leaves reuse their front shadow on the back.
    glUniform1i(
        shader.uniform("foliage_back_shadow"),
        renderer::render_opt::effects_detail() == renderer::render_opt::EffectsDetail::High ? 1 : 0
    );
    if (!active) {
        return;
    }
    glUniform2f(shader.uniform("early_extent"), float(m_width), float(m_height));
    glUniform1i(shader.uniform("early_scale"), m_scale);
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
    m_buffers = {};
    m_textures = {};
    m_width = m_height = 0;
    m_ready = false;
    m_failed = false;
}
}  // namespace openrac::lighting

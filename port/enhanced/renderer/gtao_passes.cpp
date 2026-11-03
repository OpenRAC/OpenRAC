// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// GTAO pass recording; graph declarations and depth hierarchy scheduling keep shader work unchanged.
#include <algorithm>

#include "common/stats.h"
#include "enhanced/renderer/gtao.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#include "renderer/world/gl_world.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::renderer {
using namespace gl;
#if !defined(OPENRAC_VULKAN)
void Gtao::render_prepass(unsigned, const std::function<void()>& record) {
    record();
}
#endif

bool Gtao::execute(
    const FrameInput& input, const RenderTarget& target, bool apply_to_colour, bool evaluate_only
) {
    const bool hi_z = input.gtao_settings.hi_z && m_hi_z.prepare(target.width, target.height);
    const unsigned mip_count = hi_z ? static_cast<unsigned>(m_hi_z.levels().size()) : 0;
#if defined(OPENRAC_VULKAN)
    // Vulkan plans all GTAO resource dependencies.
    if (vkgl::active()) {
        return execute_graph(input, target, apply_to_colour, evaluate_only, hi_z);
    }
#endif
    // Reference barriers also order disjoint levels of the shared hierarchy allocation.
    for (unsigned level = 0; level < mip_count; ++level) {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        m_hi_z.build_level(m_sampled_depth, level);
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    evaluate(input, target, hi_z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // Early lighting consumes packed horizons directly and supplies its own filtering.
    if (!evaluate_only) {
        filter(input, target, apply_to_colour);
    }

    return true;
}

void Gtao::evaluate(const FrameInput& input, const RenderTarget& target, bool hi_z) {
    const auto& options = input.gtao_settings;
    const Mat4& p = input.camera.projection;
    const int scale = options.half_resolution ? 2 : 1;
    const stats::Scope horizons("GTAO horizons", stats::kGpu);
    const unsigned variant = m_program + (options.hi_z ? kProgramVariants : 0u);
    Shader& evaluate = options.quality == GtaoQuality::Performance ? m_evaluate_performance[variant]
                                                                   : m_evaluate[variant];
    evaluate.use();
    m_hi_z.bind(evaluate, hi_z);
    glUniform1i(evaluate.uniform("surface_normals"), 2);
    glUniform1i(evaluate.uniform("surface_normals_enabled"), target.normal_texture != 0 ? 1 : 0);
    glUniform1i(evaluate.uniform("tex_T0"), 0);
    glUniformMatrix4fv(evaluate.uniform("projection"), 1, GL_FALSE, p.data());
    glUniform1f(evaluate.uniform("radius"), std::clamp(options.radius, 0.05f, 8.0f));
    glUniform1i(evaluate.uniform("evaluation_scale"), scale);
    glUniform1i(evaluate.uniform("slice_count"), std::clamp(options.slices, 1, 12));
    glUniform1i(evaluate.uniform("step_count"), std::clamp(options.steps, 4, 24));
    glDispatchCompute(unsigned((m_ao_width + 7) / 8), unsigned((m_ao_height + 7) / 8), 1);
}

void Gtao::filter(const FrameInput& input, const RenderTarget& target, bool apply_to_colour) {
    const auto& options = input.gtao_settings;
    const Mat4& p = input.camera.projection;
    const int scale = options.half_resolution ? 2 : 1;
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(world::gl::GL_TEXTURE_BUFFER, m_ao_texture);
    glBindFramebuffer(GL_FRAMEBUFFER, apply_to_colour ? target.framebuffer : m_visibility.id());
    glViewport(0, 0, target.width, target.height);
    {
        const stats::Scope filter("GTAO filter", stats::kGpu);
        auto& composite = m_composite[m_program];
        composite.use();
        glUniform1i(composite.uniform("surface_normals"), 2);
        glUniform1i(
            composite.uniform("surface_normals_enabled"), target.normal_texture != 0 ? 1 : 0
        );
        glUniformMatrix4fv(composite.uniform("projection"), 1, GL_FALSE, p.data());
        glUniform1i(composite.uniform("evaluation_scale"), scale);
        glUniform1f(composite.uniform("strength"), std::clamp(options.strength, 0.0f, 3.0f));

        // Legacy colour compositing multiplies existing illumination; visibility targets overwrite it.
        if (apply_to_colour) {
            glEnable(GL_BLEND);
            // Legacy prelit paths do not expose separate indirect illumination.
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
        }

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

}  // namespace openrac::renderer

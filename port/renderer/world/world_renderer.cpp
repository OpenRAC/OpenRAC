// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/world/world_renderer.h"

#include "renderer/world/gl_world.h"

namespace openrac::renderer::world {

using namespace openrac::renderer::world::gl;

void WorldRenderer::set_gs_draw(const Shader& shader, const GsDraw& draw) {
    apply(draw);
    glUniform1i(shader.uniform("u_alpha_test"), alpha_test_mode(draw));
    glUniform1f(shader.uniform("u_aref"), static_cast<float>(draw.aref));
    glUniform1i(shader.uniform("u_output"), output_mode(draw));
}

void WorldRenderer::count_draw(RenderState& state, std::size_t indices) {
    state.stats.draw_calls += 1;
    state.stats.triangles += indices / 3;
}

}  // namespace openrac::renderer::world

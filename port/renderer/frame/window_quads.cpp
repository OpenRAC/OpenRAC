// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements window_quads.h using the shared graph planner and the active graphics backend.
#include "renderer/frame/window_quads.h"

#include <algorithm>
#include <vector>

#include "renderer/gl/gl.h"
#include "renderer/graph/command_buffer.h"
#include "renderer/renderer.h"

namespace openrac::renderer {
namespace {

// The shader generates the four corners from successive vertex IDs.
constexpr graph::Draw kQuad{4, 1, 0, 0};

}  // namespace

void draw_window_quads(
    std::string_view name,
    std::array<int, 2> size,
    std::span<const RasterTexture> textures,
    const std::function<void()>& setup,
    const std::function<void(std::size_t)>& uniforms
) {
    using namespace gl;

    // An empty overlay has no work or resource dependencies.
    if (textures.empty()) {
        return;
    }

    std::vector<RasterTexture> sampled;

    // Each imported image can be shared by any number of quads in the pass.
    for (const RasterTexture& texture : textures) {
        // Zero requests the backend's unbound sampler and has no allocation to import.
        if (texture.name != 0
            && std::ranges::find(sampled, texture.name, &RasterTexture::name) == sampled.end()) {
            sampled.push_back(texture);
        }
    }

    // Both backends record these commands synchronously with the pass's declared inputs.
    const auto record = [&] {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, size[0], size[1]);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
        setup();
        glActiveTexture(GL_TEXTURE0);
        unsigned bound_texture = textures.front().name;
        glBindTexture(GL_TEXTURE_2D, bound_texture);

        // Untextured quads use the backend fallback even when a preceding quad uses an image.
        for (std::size_t index = 0; index < textures.size(); ++index) {
            uniforms(index);

            // Adjacent controls can share the same icon atlas or leave the sampler unbound.
            if (bound_texture != textures[index].name) {
                bound_texture = textures[index].name;
                glBindTexture(GL_TEXTURE_2D, bound_texture);
            }

            glDrawArrays(
                GL_TRIANGLE_STRIP,
                static_cast<GLint>(kQuad.first_vertex),
                static_cast<GLsizei>(kQuad.vertices)
            );
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    };

    raster_pass(name, {0, size[0], size[1]}, {{}, sampled}, record);
}

}  // namespace openrac::renderer

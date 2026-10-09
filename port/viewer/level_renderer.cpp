// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "viewer/level_renderer.h"

#include <map>

#include "renderer/gl.h"
#include "viewer_shaders.h"

namespace openrac::viewer {

using namespace openrac::gl;
using renderer::Bucket;

namespace {

struct InstanceData {
    float matrix[16];
    float tint[4];
};

Bucket bucket_of(Layer layer) {
    switch (layer) {
        case Layer::Sky:
            return Bucket::Sky;
        case Layer::Terrain:
            return Bucket::Terrain;
        case Layer::Ties:
            return Bucket::Ties;
        case Layer::Shrubs:
            return Bucket::Shrubs;
        default:
            return Bucket::Mobys;
    }
}

const void* offset(std::size_t bytes) {
    return reinterpret_cast<const void*>(bytes);
}

}  // namespace

bool LevelScene::upload(
    const LevelData& level, renderer::TexturePool& textures, std::string& error
) {
    if (!m_mesh.build("mesh", shaders::mesh_vert, shaders::mesh_frag, error)
        || !m_sky.build("sky", shaders::sky_vert, shaders::sky_frag, error)) {
        return false;
    }
    m_models = level.models;
    m_materials = level.materials;
    renderer::Rgba8Image white(1, 1);
    white.set_texel(0, 0, 0xFFFFFFFFu);
    m_white = textures.add(std::move(white), renderer::AlphaScale::Full, "white");
    for (std::size_t i = 0; i < level.images.size(); ++i) {
        m_textures.push_back(
            level.images[i].width > 0
                ? textures
                      .add(level.images[i], renderer::AlphaScale::Full, level.image_paths[i], true)
                : m_white
        );
    }

    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(level.vertices.size() * sizeof(Vertex)),
        level.vertices.data(),
        GL_STATIC_DRAW
    );
    // Buffers have no type of their own: the indices are filled through the
    // array target, before any vertex array exists to hold the element binding.
    glGenBuffers(1, &m_indices);
    glBindBuffer(GL_ARRAY_BUFFER, m_indices);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(level.indices.size() * sizeof(std::uint32_t)),
        level.indices.data(),
        GL_STATIC_DRAW
    );

    for (std::size_t layer = 0; layer < kLayerCount; ++layer) {
        // Instances grouped by class model, in first-seen order.
        std::map<std::uint32_t, std::vector<InstanceData>> by_model;
        std::vector<std::uint32_t> order;
        for (const Instance& instance : level.instances[layer]) {
            auto [it, fresh] = by_model.try_emplace(instance.model);
            if (fresh) {
                order.push_back(instance.model);
            }
            InstanceData data{};
            std::copy(instance.matrix.begin(), instance.matrix.end(), data.matrix);
            std::copy(instance.tint.begin(), instance.tint.end(), data.tint);
            it->second.push_back(data);
        }
        for (std::uint32_t model : order) {
            const auto& list = by_model[model];
            Group g;
            g.model = model;
            g.count = static_cast<int>(list.size());
            glGenVertexArrays(1, &g.vao);
            glBindVertexArray(g.vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);
            constexpr auto stride = static_cast<GLsizei>(sizeof(Vertex));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, position))
            );
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, normal))
            );
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, uv)));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(
                3, 4, GL_FLOAT, GL_FALSE, stride, offset(offsetof(Vertex, colour))
            );
            glGenBuffers(1, &g.instances);
            glBindBuffer(GL_ARRAY_BUFFER, g.instances);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(list.size() * sizeof(InstanceData)),
                list.data(),
                GL_STATIC_DRAW
            );
            constexpr auto instance_stride = static_cast<GLsizei>(sizeof(InstanceData));
            for (GLuint column = 0; column < 4; ++column) {
                glEnableVertexAttribArray(4 + column);
                glVertexAttribPointer(
                    4 + column,
                    4,
                    GL_FLOAT,
                    GL_FALSE,
                    instance_stride,
                    offset(column * 4 * sizeof(float))
                );
                glVertexAttribDivisor(4 + column, 1);
            }
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(
                8, 4, GL_FLOAT, GL_FALSE, instance_stride, offset(offsetof(InstanceData, tint))
            );
            glVertexAttribDivisor(8, 1);
            m_groups[layer].push_back(g);
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void LevelScene::release() {
    for (auto& groups : m_groups) {
        for (Group& g : groups) {
            glDeleteVertexArrays(1, &g.vao);
            glDeleteBuffers(1, &g.instances);
        }
        groups.clear();
    }
    if (m_vertices != 0) {
        glDeleteBuffers(1, &m_vertices);
        m_vertices = 0;
    }
    if (m_indices != 0) {
        glDeleteBuffers(1, &m_indices);
        m_indices = 0;
    }
    m_mesh.release();
    m_sky.release();
}

void LevelScene::draw_sky(const renderer::FrameInput& input, renderer::RenderState& state) {
    // The shells sit around the camera and never move with it: the view's
    // rotation only. Each is blended over the last, without depth.
    renderer::Mat4 rotation = input.camera.view;
    rotation[12] = rotation[13] = rotation[14] = 0.0f;
    const renderer::Mat4 view_projection = renderer::multiply(input.camera.projection, rotation);
    m_sky.use();
    glUniformMatrix4fv(m_sky.uniform("view_projection"), 1, GL_FALSE, view_projection.data());
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    glActiveTexture(GL_TEXTURE0);
    for (const Group& g : m_groups[static_cast<std::size_t>(Layer::Sky)]) {
        glBindVertexArray(g.vao);
        for (const Primitive& p : m_models[g.model].primitives) {
            const Material& m = m_materials[static_cast<std::size_t>(p.material)];
            const renderer::TextureHandle texture =
                m.image >= 0 ? m_textures[static_cast<std::size_t>(m.image)] : m_white;
            glBindTexture(GL_TEXTURE_2D, state.textures.gl_texture(texture));
            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(p.index_count),
                GL_UNSIGNED_INT,
                offset(p.first_index * sizeof(std::uint32_t)),
                g.count
            );
            ++state.stats.draw_calls;
            state.stats.triangles += p.index_count / 3;
        }
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void LevelScene::draw_layer(
    Layer layer, const renderer::FrameInput& input, renderer::RenderState& state
) {
    if (layer == Layer::Sky) {
        draw_sky(input, state);
        return;
    }
    const auto& groups = m_groups[static_cast<std::size_t>(layer)];
    if (groups.empty()) {
        return;
    }
    const renderer::Mat4 view_projection =
        renderer::multiply(input.camera.projection, input.camera.view);
    m_mesh.use();
    glUniformMatrix4fv(m_mesh.uniform("view_projection"), 1, GL_FALSE, view_projection.data());
    glUniform1i(m_mesh.uniform("lighting"), lighting ? 1 : 0);
    const int cutout = m_mesh.uniform("cutout");
    const int colour = m_mesh.uniform("material_colour");
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);  // reversed depth
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);  // the editor's materials are double-sided
    glActiveTexture(GL_TEXTURE0);
    for (const Group& g : groups) {
        glBindVertexArray(g.vao);
        for (const Primitive& p : m_models[g.model].primitives) {
            const Material& m = m_materials[static_cast<std::size_t>(p.material)];
            const renderer::TextureHandle texture =
                m.image >= 0 ? m_textures[static_cast<std::size_t>(m.image)] : m_white;
            glBindTexture(GL_TEXTURE_2D, state.textures.gl_texture(texture));
            glUniform1i(cutout, m.cutout ? 1 : 0);
            glUniform4f(colour, m.colour[0], m.colour[1], m.colour[2], m.colour[3]);
            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(p.index_count),
                GL_UNSIGNED_INT,
                offset(p.first_index * sizeof(std::uint32_t)),
                g.count
            );
            ++state.stats.draw_calls;
            state.stats.triangles +=
                static_cast<std::uint64_t>(p.index_count / 3) * static_cast<std::uint64_t>(g.count);
        }
    }
    glBindVertexArray(0);
}

LayerRenderer::LayerRenderer(LevelScene& scene, Layer layer)
    : BucketRenderer(layer_name(layer), bucket_of(layer)),
      m_scene(scene),
      m_layer(layer) {}

void LayerRenderer::render(const renderer::FrameInput& input, renderer::RenderState& state) {
    m_scene.draw_layer(m_layer, input, state);
}

}  // namespace openrac::viewer

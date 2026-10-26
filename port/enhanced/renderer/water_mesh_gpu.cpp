// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// GPU ownership and draws for the baked water templates; no dense CPU stream is constructed.
#include <cstddef>

#include "common/profiler.h"
#include "enhanced/renderer/water_mesh.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl.h"
#include "renderer/world/gl_world.h"

namespace openrac::enhanced {
namespace {
// Unit 13 is between the coast and wave maps and outside the probe atlas bindings.
constexpr unsigned kSourceUnit = 13;
}  // namespace

WaterMesh::~WaterMesh() {
    release();
}

bool WaterMesh::init(std::string& error) {
    using namespace gl;
    std::string source(lighting::shaders::water_uv_comp);
    source.insert(source.find('\n') + 1, lighting::shaders::water_uv_glsl);
    if (!m_animation.build_compute("water authored UV animation", source, error)) {
        return false;
    }
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vertices);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        GLsizeiptr(m_weights.size() * sizeof(m_weights[0])),
        m_weights.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(m_weights[0]), nullptr);
    glBindVertexArray(0);
    glGenBuffers(1, &m_source_buffer);
    glBindBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, m_source_buffer);
    const Source empty{};
    glBufferData(renderer::world::gl::GL_TEXTURE_BUFFER, sizeof(empty), &empty, GL_STREAM_DRAW);
    glGenTextures(1, &m_source_texture);
    glActiveTexture(GL_TEXTURE0 + kSourceUnit);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_source_texture);
    glTexBuffer(
        renderer::world::gl::GL_TEXTURE_BUFFER, renderer::world::gl::GL_RGBA32UI, m_source_buffer
    );
    glActiveTexture(GL_TEXTURE0);
    return true;
}

void WaterMesh::upload() {
    const profiling::Zone trace("Upload water coarse instances");
    // Keep the valid dummy allocation when this pass contains no water.
    if (m_sources.empty()) {
        return;
    }
    gl::glBindBuffer(renderer::world::gl::GL_TEXTURE_BUFFER, m_source_buffer);
    gl::glBufferData(
        renderer::world::gl::GL_TEXTURE_BUFFER,
        gl::GLsizeiptr(m_sources.size() * sizeof(Source)),
        m_sources.data(),
        gl::GL_STREAM_DRAW
    );
    m_animation.use();
    gl::glUniform1i(m_animation.uniform("triangle_count"), static_cast<int>(m_sources.size()));
    gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, 0, m_source_buffer);
    gl::glDispatchCompute(static_cast<unsigned>((m_sources.size() + 63) / 64), 1, 1);
    gl::glMemoryBarrier(gl::GL_SHADER_STORAGE_BARRIER_BIT | gl::GL_TEXTURE_FETCH_BARRIER_BIT);
}

void WaterMesh::bind(const renderer::Shader& shader) const {
    using namespace gl;
    glUniform1i(shader.uniform("water_coarse"), kSourceUnit);
    glActiveTexture(GL_TEXTURE0 + kSourceUnit);
    glBindTexture(renderer::world::gl::GL_TEXTURE_BUFFER, m_source_texture);
    glActiveTexture(GL_TEXTURE0);
}

void WaterMesh::draw(const renderer::Shader& shader, const renderer::EffectMesh& mesh) const {
    const profiling::Zone trace("Draw baked water mesh");
    using namespace gl;
    glBindVertexArray(m_vao);
    glUniform1i(shader.uniform("water_mesh"), 1);
    for (const auto& batch : m_plans.at(mesh.handle).batches) {
        glUniform1i(shader.uniform("water_first_triangle"), batch.first);
        glDrawArraysInstanced(GL_TRIANGLES, batch.shape.first, batch.shape.count, batch.count);
    }
    glUniform1i(shader.uniform("water_mesh"), 0);
}

void WaterMesh::release() {
    using namespace gl;
    m_animation.release();
    // A CPU-only load or a repeated release has no context-owned objects to destroy.
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vertices);
        glDeleteBuffers(1, &m_source_buffer);
        glDeleteTextures(1, &m_source_texture);
    }
    m_vao = m_vertices = m_source_buffer = m_source_texture = 0;
    begin_pass();
}
}  // namespace openrac::enhanced

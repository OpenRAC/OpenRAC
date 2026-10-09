// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/world/gpu.h"

#include <bit>
#include <utility>

#include "renderer/world/gl_world.h"

namespace openrac::renderer::world {

using namespace openrac::renderer::world::gl;

// ---- GpuBuffer ----

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
    : m_buffer(std::exchange(other.m_buffer, 0)), m_size(std::exchange(other.m_size, 0)) {}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this != &other) {
        m_buffer = std::exchange(other.m_buffer, 0);
        m_size = std::exchange(other.m_size, 0);
    }
    return *this;
}

void GpuBuffer::upload(unsigned target, const void* data, std::size_t bytes, bool dynamic) {
    if (m_buffer == 0) {
        glGenBuffers(1, &m_buffer);
    }
    glBindBuffer(target, m_buffer);
    if (bytes > m_size || !dynamic) {
        // Never a zero-sized store: a buffer texture needs at least a texel.
        const std::size_t size = bytes == 0 ? 16 : bytes;
        glBufferData(
            target,
            static_cast<GLsizeiptr>(size),
            bytes == 0 ? nullptr : data,
            dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW
        );
        m_size = size;
    } else if (bytes > 0) {
        glBufferSubData(target, 0, static_cast<GLsizeiptr>(bytes), data);
    }
}

void GpuBuffer::release() {
    if (m_buffer != 0) {
        glDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
        m_size = 0;
    }
}

// ---- RecordBuffer ----

void RecordBuffer::upload(std::span<const std::uint32_t> words, Format format, bool dynamic) {
    std::vector<std::uint32_t> padded;
    std::span<const std::uint32_t> data = words;
    const std::size_t per_texel = format == Format::Rgba32ui ? 4 : 1;
    if (words.empty() || words.size() % per_texel != 0) {
        padded.assign(words.begin(), words.end());
        padded.resize((words.size() / per_texel + 1) * per_texel, 0);
        data = padded;
    }
    m_buffer.upload(GL_TEXTURE_BUFFER, data, dynamic);
    if (m_texture == 0 || m_format != format) {
        if (m_texture == 0) {
            glGenTextures(1, &m_texture);
        }
        m_format = format;
    }
    glBindTexture(GL_TEXTURE_BUFFER, m_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, format == Format::Rgba32ui ? GL_RGBA32UI : GL_R32UI, m_buffer.id());
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void RecordBuffer::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_BUFFER, m_texture);
}

void RecordBuffer::release() {
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    m_buffer.release();
}

// ---- Words ----

Words& Words::f(float v) {
    m_words.push_back(std::bit_cast<std::uint32_t>(v));
    return *this;
}

Words& Words::u(std::uint32_t v) {
    m_words.push_back(v);
    return *this;
}

Words& Words::f(std::initializer_list<float> v) {
    for (const float x : v) {
        f(x);
    }
    return *this;
}

Words& Words::u(std::initializer_list<std::uint32_t> v) {
    m_words.insert(m_words.end(), v.begin(), v.end());
    return *this;
}

Words& Words::mat(const Mat4& m) {
    for (const float x : m) {
        f(x);
    }
    return *this;
}

Words& Words::align4() {
    while (m_words.size() % 4 != 0) {
        m_words.push_back(0);
    }
    return *this;
}

// ---- Mesh ----

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(std::exchange(other.m_vao, 0)),
      m_vertices(std::move(other.m_vertices)),
      m_indices(std::move(other.m_indices)),
      m_instances(std::move(other.m_instances)),
      m_index_count(std::exchange(other.m_index_count, 0)),
      m_vertex_count(std::exchange(other.m_vertex_count, 0)),
      m_stride(other.m_stride),
      m_dynamic(other.m_dynamic) {}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        m_vao = std::exchange(other.m_vao, 0);
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_instances = std::move(other.m_instances);
        m_index_count = std::exchange(other.m_index_count, 0);
        m_vertex_count = std::exchange(other.m_vertex_count, 0);
        m_stride = other.m_stride;
        m_dynamic = other.m_dynamic;
    }
    return *this;
}

void Mesh::create(
    std::span<const std::uint8_t> vertices,
    std::size_t stride,
    std::span<const Attribute> attributes,
    std::span<const std::uint32_t> indices,
    bool dynamic
) {
    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
    }
    m_stride = stride;
    m_dynamic = dynamic;
    glBindVertexArray(m_vao);
    m_vertices.upload(GL_ARRAY_BUFFER, vertices.data(), vertices.size(), dynamic);
    for (const Attribute& a : attributes) {
        glEnableVertexAttribArray(a.location);
        const auto* offset = reinterpret_cast<const void*>(a.offset);
        if (a.integer) {
            glVertexAttribIPointer(a.location, a.components, a.type, static_cast<GLsizei>(stride), offset);
        } else {
            glVertexAttribPointer(
                a.location, a.components, a.type, GL_FALSE, static_cast<GLsizei>(stride), offset
            );
        }
    }
    if (!indices.empty()) {
        m_indices.upload(GL_ELEMENT_ARRAY_BUFFER, indices, dynamic);
    }
    glBindVertexArray(0);
    m_vertex_count = stride == 0 ? 0 : vertices.size() / stride;
    m_index_count = indices.size();
}

void Mesh::update(std::span<const std::uint8_t> vertices, std::span<const std::uint32_t> indices) {
    glBindVertexArray(m_vao);
    m_vertices.upload(GL_ARRAY_BUFFER, vertices.data(), vertices.size(), true);
    if (!indices.empty()) {
        m_indices.upload(GL_ELEMENT_ARRAY_BUFFER, indices, true);
    }
    glBindVertexArray(0);
    m_vertex_count = m_stride == 0 ? 0 : vertices.size() / m_stride;
    m_index_count = indices.size();
}

void Mesh::set_instance_ids(unsigned location, std::span<const std::uint32_t> ids) {
    glBindVertexArray(m_vao);
    m_instances.upload(GL_ARRAY_BUFFER, ids, true);
    glEnableVertexAttribArray(location);
    glVertexAttribIPointer(location, 1, GL_UNSIGNED_INT, 4, nullptr);
    glVertexAttribDivisor(location, 1);
    glBindVertexArray(0);
}

void Mesh::bind() const {
    glBindVertexArray(m_vao);
}

void Mesh::draw(unsigned mode) const {
    if (m_index_count > 0) {
        glDrawElements(mode, static_cast<GLsizei>(m_index_count), GL_UNSIGNED_INT, nullptr);
    } else if (m_vertex_count > 0) {
        glDrawArrays(mode, 0, static_cast<GLsizei>(m_vertex_count));
    }
}

void Mesh::draw_instanced(int instances, unsigned mode) const {
    if (instances <= 0) {
        return;
    }
    if (m_index_count > 0) {
        glDrawElementsInstanced(
            mode, static_cast<GLsizei>(m_index_count), GL_UNSIGNED_INT, nullptr, instances
        );
    } else if (m_vertex_count > 0) {
        glDrawArraysInstanced(mode, 0, static_cast<GLsizei>(m_vertex_count), instances);
    }
}

void Mesh::release() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_vertices.release();
    m_indices.release();
    m_instances.release();
    m_index_count = 0;
    m_vertex_count = 0;
}

// ---- Shaders ----

std::string compose(std::string_view stage, std::initializer_list<std::string_view> parts) {
    const std::size_t eol = stage.find('\n');
    std::string out;
    if (eol == std::string_view::npos) {
        out.assign(stage);
        out += '\n';
    } else {
        out.assign(stage.substr(0, eol + 1));
    }
    for (const std::string_view part : parts) {
        out += part;
        out += '\n';
    }
    // Keep the stage's own line numbers recognisable in compiler logs.
    out += "#line 2\n";
    if (eol != std::string_view::npos) {
        out += stage.substr(eol + 1);
    }
    return out;
}

bool build_world_shader(
    Shader& shader,
    std::string_view name,
    std::string_view vertex,
    std::string_view fragment,
    std::initializer_list<std::string_view> parts,
    std::string& error
) {
    if (!shader.build(name, compose(vertex, parts), compose(fragment, parts), error)) {
        return false;
    }
    const GLuint view = glGetUniformBlockIndex(shader.id(), "WorldView");
    if (view != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader.id(), view, kWorldViewBinding);
    }
    const GLuint lights = glGetUniformBlockIndex(shader.id(), "WorldLights");
    if (lights != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader.id(), lights, kWorldLightsBinding);
    }
    return true;
}

// ---- Samplers ----

unsigned SamplerCache::get(const SamplerState& state) {
    for (const Entry& e : m_entries) {
        if (e.state == state) {
            return e.sampler;
        }
    }
    GLuint s = 0;
    glGenSamplers(1, &s);
    const auto wrap = [](bool clamp) {
        return static_cast<GLint>(clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    };
    glSamplerParameteri(s, GL_TEXTURE_WRAP_S, wrap(state.clamp_s));
    glSamplerParameteri(s, GL_TEXTURE_WRAP_T, wrap(state.clamp_t));
    GLenum mag = state.nearest ? GL_NEAREST : GL_LINEAR;
    GLenum min = mag;
    if (state.mipmapped) {
        // LINEAR_MIPMAP_NEAREST: bilinear inside the level the shader picks.
        min = state.nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
    }
    glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(mag));
    glSamplerParameteri(s, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(min));
    m_entries.push_back({state, s});
    return s;
}

void SamplerCache::release() {
    for (const Entry& e : m_entries) {
        glDeleteSamplers(1, &e.sampler);
    }
    m_entries.clear();
}

}  // namespace openrac::renderer::world

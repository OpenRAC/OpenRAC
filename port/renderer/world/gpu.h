// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The GL objects the world renderers build on: buffers, vertex arrays,
// record buffers (a buffer read by the shaders through a buffer texture, the
// OpenGL 4.1 stand-in for the storage buffers ReRAC's WGSL reads), sampler
// objects for the game's wrap and filter states, and the composition of the
// world shaders from their shared parts.
//
// Shader sources live in renderer/shaders/world (embedded by CMake into
// world_shaders.h). A stage file starts with "#version 410 core"; the shared
// parts (world_common.glsl, display_blend.glsl, world_lights.glsl) have no
// version line and are inserted after it. Uniform blocks are bound by name to
// fixed binding points (GLSL 4.10 has no binding qualifier), samplers by
// renderer/shader.h's tex_T<n> rule.

#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/shader.h"
#include "renderer/world/draw_data.h"

namespace openrac::renderer::world {

// Uniform block binding points.
inline constexpr unsigned kWorldViewBinding = 0;
inline constexpr unsigned kWorldLightsBinding = 1;

// A GL buffer object.
class GpuBuffer {
public:
    GpuBuffer() = default;
    ~GpuBuffer() = default;
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;
    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    // Replace the contents (grows the buffer as needed).
    void upload(unsigned target, const void* data, std::size_t bytes, bool dynamic);

    template <typename T>
    void upload(unsigned target, std::span<const T> data, bool dynamic) {
        upload(target, data.data(), data.size_bytes(), dynamic);
    }

    unsigned id() const { return m_buffer; }

    std::size_t size() const { return m_size; }

    void release();

private:
    unsigned m_buffer = 0;
    std::size_t m_size = 0;
};

// Records the shaders index with texelFetch: a buffer of 32-bit words seen
// as RGBA32UI texels (four words a texel) or R32UI (one word a texel).
class RecordBuffer {
public:
    enum class Format {
        Rgba32ui,
        R32ui,
    };

    RecordBuffer() = default;
    RecordBuffer(RecordBuffer&&) noexcept = default;
    RecordBuffer& operator=(RecordBuffer&&) noexcept = default;
    RecordBuffer(const RecordBuffer&) = delete;
    RecordBuffer& operator=(const RecordBuffer&) = delete;

    // Upload the words (padded to whole texels; never empty on the GPU).
    void upload(std::span<const std::uint32_t> words, Format format, bool dynamic);
    void bind(int unit) const;
    void release();

private:
    GpuBuffer m_buffer;
    unsigned m_texture = 0;
    Format m_format = Format::Rgba32ui;
};

// Words of a record: floats by their bits, in order.
class Words {
public:
    Words& f(float v);
    Words& u(std::uint32_t v);
    Words& f(std::initializer_list<float> v);
    Words& u(std::initializer_list<std::uint32_t> v);
    Words& mat(const Mat4& m);
    // Pad with zeros to a multiple of four words (a whole texel).
    Words& align4();

    std::vector<std::uint32_t>& data() { return m_words; }

    const std::vector<std::uint32_t>& data() const { return m_words; }

    std::size_t size() const { return m_words.size(); }

private:
    std::vector<std::uint32_t> m_words;
};

// A vertex array with its vertex and index buffers.
class Mesh {
public:
    Mesh() = default;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    struct Attribute {
        unsigned location;
        int components;
        unsigned type;  // GL_FLOAT, GL_UNSIGNED_INT, ...
        bool integer;   // read as an integer (glVertexAttribIPointer)
        std::size_t offset;
    };

    // Create the arrays from interleaved vertices of `stride` bytes.
    void create(
        std::span<const std::uint8_t> vertices,
        std::size_t stride,
        std::span<const Attribute> attributes,
        std::span<const std::uint32_t> indices,
        bool dynamic = false
    );
    // Replace the contents, same layout.
    void update(std::span<const std::uint8_t> vertices, std::span<const std::uint32_t> indices);

    // A per-instance attribute stream (divisor 1) of one uint each.
    void set_instance_ids(unsigned location, std::span<const std::uint32_t> ids);

    void bind() const;
    void draw(unsigned mode = 0x0004) const;  // GL_TRIANGLES by default
    void draw_instanced(int instances, unsigned mode = 0x0004) const;

    std::size_t index_count() const { return m_index_count; }

    std::size_t vertex_count() const { return m_vertex_count; }

    bool empty() const { return m_vao == 0 || (m_index_count == 0 && m_vertex_count == 0); }

    void release();

private:
    unsigned m_vao = 0;
    GpuBuffer m_vertices;
    GpuBuffer m_indices;
    GpuBuffer m_instances;
    std::size_t m_index_count = 0;
    std::size_t m_vertex_count = 0;
    std::size_t m_stride = 0;
    bool m_dynamic = false;
};

// Vertex bytes from plain structs.
template <typename T>
std::span<const std::uint8_t> bytes_of(const std::vector<T>& v) {
    return {reinterpret_cast<const std::uint8_t*>(v.data()), v.size() * sizeof(T)};
}

// A shader stage with the shared parts inserted after its version line.
std::string compose(std::string_view stage, std::initializer_list<std::string_view> parts);

// Build a world program: the stages composed with `parts`, the WorldView
// and WorldLights blocks bound to their binding points.
bool build_world_shader(
    Shader& shader,
    std::string_view name,
    std::string_view vertex,
    std::string_view fragment,
    std::initializer_list<std::string_view> parts,
    std::string& error
);

// Sampler objects for the states the game uses.
class SamplerCache {
public:
    unsigned get(const SamplerState& state);
    void release();

private:
    struct Entry {
        SamplerState state;
        unsigned sampler;
    };

    std::vector<Entry> m_entries;
};

}  // namespace openrac::renderer::world

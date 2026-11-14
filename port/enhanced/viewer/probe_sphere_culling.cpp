// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Diagnostic sphere compaction: immutable uploads and ordered device-written vertices/commands.

#include "enhanced/viewer/probe_sphere_culling.h"

#include <limits>
#include <stdexcept>

#include "enhanced_viewer_shaders.h"
#include "renderer/gl/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution.h"
#endif

namespace openrac::lighting {
namespace {
using namespace openrac::gl;

// Dedicated slots follow the tiled selector's binding eight.
constexpr unsigned kSourceBinding = 9;
constexpr unsigned kOutputBinding = 10;
constexpr unsigned kCommandBinding = 11;
constexpr unsigned kWorkgroupSize = 128;
constexpr unsigned kCommandWords = 5;  // VkDrawIndexedIndirectCommand.
constexpr unsigned kRegionCapacity = 1024;
constexpr GLbitfield kDrawBarrier =
    GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_COMMAND_BARRIER_BIT;
static_assert(sizeof(ProbeSphere) == 9 * sizeof(unsigned));
}  // namespace

ProbeSphereCulling::~ProbeSphereCulling() {
    release();
}

bool ProbeSphereCulling::prepare(std::span<const ProbeSphere> spheres, std::string& error) {
    GLint bindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);

    // The native indirect path and its three storage bindings are Vulkan-only.
    if (!enhanced_rendering_available() || bindings <= static_cast<GLint>(kCommandBinding)
        || spheres.empty()
        || spheres.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        error = "GPU sphere compaction is unavailable";
        return false;
    }

    if (!m_shader.build_compute("probe sphere compaction", shaders::probe_spheres_comp, error)) {
        return false;
    }

    glGenBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
    const GLsizeiptr sizes[] =
        {static_cast<GLsizeiptr>(spheres.size_bytes()),
         static_cast<GLsizeiptr>(spheres.size_bytes()),
         static_cast<GLsizeiptr>((kCommandWords + kRegionCapacity) * sizeof(unsigned))};

    for (unsigned n = 0; n < m_buffers.size(); ++n) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[n]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, sizes[n], n == 0 ? spheres.data() : nullptr, GL_STATIC_DRAW
        );
    }

    m_count = static_cast<unsigned>(spheres.size());
    // A failed allocation must leave an actionable fallback message.
    if (glGetError() != GL_NO_ERROR) {
        error = "cannot allocate resident probe sphere buffers";
        return false;
    }

    return true;
}

void ProbeSphereCulling::dispatch(
    const renderer::Camera& camera,
    float range,
    const std::array<ProbeRegionSlot, kDenseProbeSlots>& slots,
    bool gpu_selection
) {
    const unsigned bindings[] = {kSourceBinding, kOutputBinding, kCommandBinding};

    for (unsigned n = 0; n < m_buffers.size(); ++n) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[n], m_buffers[n]);
    }

    m_shader.use();
    const auto clip = renderer::multiply(camera.projection, camera.view);
    glUniformMatrix4fv(m_shader.uniform("world_to_clip"), 1, GL_FALSE, clip.data());
    glUniform3f(
        m_shader.uniform("camera_position"),
        camera.position[0],
        camera.position[1],
        camera.position[2]
    );
    glUniform1f(m_shader.uniform("draw_range"), range);
    glUniform1i(m_shader.uniform("source_count"), static_cast<int>(m_count));
    glUniform1i(m_shader.uniform("index_count"), static_cast<int>(kProbeSphereVertices));

    for (unsigned slot = 0; slot < slots.size(); ++slot) {
        const auto suffix = "[" + std::to_string(slot) + "]";
        glUniform1i(m_shader.uniform(("cpu_regions" + suffix).c_str()), slots[slot].region);
        glUniform1f(m_shader.uniform(("cpu_weights" + suffix).c_str()), slots[slot].weight);
    }

    const auto compact = [&] {
        glUniform1i(m_shader.uniform("reset_command"), 1);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glUniform1i(m_shader.uniform("reset_command"), 0);
        glDispatchCompute((m_count + kWorkgroupSize - 1) / kWorkgroupSize, 1, 1);
    };
#if defined(OPENRAC_VULKAN)
    if (is_vulkan()) {
        namespace graph = renderer::graph;
        vkgl::GraphExecution execution;
        const auto source =
            execution.read_buffer("probe sphere source", m_buffers[0], graph::Stage::ComputeShader);
        const auto output = execution.import_buffer("probe sphere vertices", m_buffers[1]);
        const auto arguments = execution.import_buffer("probe sphere arguments", m_buffers[2]);
        if (!source || !output || !arguments) {
            throw std::runtime_error("cannot import probe sphere compute buffers");
        }
        const auto vertex_bytes = std::uint64_t{m_count} * sizeof(ProbeSphere);
        constexpr auto argument_bytes = (kCommandWords + kRegionCapacity) * sizeof(unsigned);
        auto pass = execution.make_pass("probe sphere compaction");
        pass.buffers.push_back(*source);
        pass.buffers.push_back(
            {*output,
             {0, vertex_bytes},
             {graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer}}
        );
        pass.buffers.push_back(
            {*arguments,
             {0, argument_bytes},
             {graph::Stage::ComputeShader, graph::Access::ReadWrite, graph::Layout::Buffer}}
        );
        // The CPU-selected branch never samples a mask; unit zero may hold an unrelated buffer view.
        const auto inputs = execution.capture_compute_reads(
            pass, gpu_selection ? vkgl::ComputeInputs::All : vkgl::ComputeInputs::UniformBlocks
        );
        if (!inputs) {
            throw std::runtime_error(inputs.error().message);
        }
        auto consumers = execution.make_pass("probe sphere draw inputs");
        graph::shader_read_export(
            consumers,
            pass,
            graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader
        );
        for (auto& use : consumers.buffers) {
            if (use.buffer == *output) {
                use.state.stages = use.state.stages | graph::Stage::VertexInput;
            }
            if (use.buffer == *arguments) {
                use.state.stages = use.state.stages | graph::Stage::DrawIndirect;
            }
        }
        execution.add_pass(std::move(pass));
        execution.add_pass(std::move(consumers));
        const auto result = execution.run([&](unsigned) { compact(); });
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        return;
    }
#endif
    // Compatibility barriers retain ordering outside Vulkan's declared pass graph.
    (void)gpu_selection;
    glMemoryBarrier(kDrawBarrier);
    compact();
    glMemoryBarrier(kDrawBarrier);
}

void ProbeSphereCulling::release() {
    m_shader.release();

    // A default or already released owner has no live GL names.
    if (m_buffers[0]) {
        glDeleteBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
    }

    m_buffers = {};
    m_count = 0;
}

}  // namespace openrac::lighting

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Graph submissions for synthetic GPU fixtures that set up their own bindings.
#include "tests/graph_fixture.h"

#include <stdexcept>
#include <utility>

#include "renderer/gl/gl.h"

#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution_internal.h"
#include "renderer/vulkan/graph/graph_immediate_work.h"
#include "renderer/vulkan/graph/graph_pipeline.h"
#include "renderer/vulkan/graph/surface_batch.h"

namespace openrac::vkgl::detail {
namespace graph = renderer::graph;

class GraphFixture {
public:
    static void draw(const std::function<void()>& record);
    static void dispatch(const std::function<void()>& record);
};

namespace {
template <class T>
T checked(Result<T> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return std::move(*result);
}

void checked(Result<void> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}
}  // namespace

void GraphFixture::draw(const std::function<void()>& record) {
    const auto* program = g->programs.find(g->state.program);
    if (!program || !program->linked) {
        return;
    }
    prepare_frame();
    const auto target = target_of(g->state.draw_framebuffer);
    if ((!target.colour && !target.depth) || target.width <= 0 || target.height <= 0
        || g->state.viewport[2] <= 0 || g->state.viewport[3] <= 0) {
        return;
    }
    GraphExecution execution;
    graph::Pass pass{"fixture raster"};
    pass.execution = graph::Stage::VertexShader | graph::Stage::FragmentShader;
    checked(execution.capture_graphics_reads(pass));
    GraphImmediateWork::capture_attachment(execution, pass, target.colour, graph::Aspect::Colour);
    GraphImmediateWork::capture_attachment(execution, pass, target.depth, graph::Aspect::Depth);
    if (target.depth && has_stencil(target.depth->format) && g->state.stencil_test) {
        GraphImmediateWork::capture_attachment(
            execution, pass, target.depth, graph::Aspect::Stencil
        );
    }
    auto& recording = *execution.m_recording;
    const auto input = [&](std::string_view name, BufferRef region, graph::Stage stage) {
        const auto handle = checked(recording.import_buffer(name, region, true));
        pass.buffers.push_back(
            {handle,
             {region.offset, region.size},
             {stage, graph::Access::Read, graph::Layout::Buffer}}
        );
    };
    const auto& array = vertex_array();
    for (const auto& attribute : program->compiled.attributes) {
        if (attribute.location < 0 || attribute.location >= kAttributes) {
            continue;
        }
        const auto& bound = array.attributes[attribute.location];
        auto* buffer = bound.enabled ? g->buffers.find(bound.buffer) : nullptr;
        if (!buffer || !buffer->size || bound.format == VK_FORMAT_UNDEFINED) {
            continue;
        }
        input(
            "raster vertices",
            resolve(*buffer, bound.divisor == 0 || buffer->storage),
            graph::Stage::VertexInput
        );
    }
    if (auto* elements = g->buffers.find(array.elements); elements && elements->size) {
        input("raster indices", resolve(*elements), graph::Stage::VertexInput);
    }
    execution.add_pass(std::move(pass));
    checked(execution.run([&](unsigned) { record(); }));
}

void GraphFixture::dispatch(const std::function<void()>& record) {
    const auto* program = g->programs.find(g->state.program);
    if (!program || !program->linked || !program->compute_pipeline) {
        return;
    }
    prepare_frame();
    end_rendering(PassEnd::Compute);
    GraphExecution execution;
    graph::Pass pass{"fixture compute"};
    pass.execution = graph::Stage::ComputeShader;
    checked(execution.capture_compute_reads(pass));
    for (const auto& block : program->compiled.storage) {
        if (block.binding < 0 || block.binding >= static_cast<int>(kUniformBindings)) {
            continue;
        }
        const auto name = g->state.storage_bindings[block.binding];
        const auto* buffer = g->buffers.find(name);
        if (!buffer || !buffer->size) {
            continue;
        }
        const auto handle = checked(execution.import_buffer("compute storage", name));
        // Fixture kernels may update only some of the bound storage.
        std::erase_if(pass.buffers, [&](const graph::BufferUse& use) {
            return use.buffer.index == handle.index;
        });
        pass.buffers.push_back(
            {handle,
             {0, buffer->size},
             {graph::Stage::ComputeShader,
              block.read_only ? graph::Access::Read : graph::Access::ReadWrite,
              graph::Layout::Buffer}}
        );
    }
    execution.add_pass(std::move(pass));
    checked(execution.run([&](unsigned) { record(); }));
}

}  // namespace openrac::vkgl::detail
#endif

namespace openrac::test {
void fixture_draw(const std::function<void()>& draw) {
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan() && !vkgl::detail::graph_pipeline_active()) {
        vkgl::detail::GraphFixture::draw(draw);
        return;
    }
#endif
    draw();
}

void fixture_dispatch(const std::function<void()>& dispatch) {
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan() && !vkgl::detail::graph_pipeline_active()
        && !vkgl::detail::collecting_surface_chain()) {
        vkgl::detail::GraphFixture::dispatch(dispatch);
        return;
    }
#endif
    dispatch();
}
}  // namespace openrac::test

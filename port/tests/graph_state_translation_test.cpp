// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Checks compiled build, scratch and ray-query dependencies without requiring a Vulkan device.
#include <bit>
#include <cstdio>
#include <exception>

#include "renderer/graph/frame_graph.h"
#include "renderer/vulkan/graph/graph_state_translation.h"
#include "tests/check.h"

namespace {
namespace graph = openrac::renderer::graph;
namespace native = openrac::vkgl::detail;

// Checks native visibility for one compiled geometry or traversal dependency.
void check_dependency(
    const graph::Barrier& transition, const graph::BufferDesc& desc, bool geometry
) {
    const auto barrier = native::graph_buffer_barrier(
        transition, std::bit_cast<VkBuffer>(desc.key.identity), 0, desc.kind
    );
    if (geometry) {
        CHECK(barrier.srcAccessMask == VK_ACCESS_HOST_WRITE_BIT);
        CHECK(barrier.dstAccessMask == VK_ACCESS_SHADER_READ_BIT);
    } else {
        CHECK(barrier.srcAccessMask == VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
        CHECK(barrier.dstAccessMask == VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
    }
}

// The same compute stage must distinguish ordinary shader data from opaque AS storage.
void test_build_to_query_dependencies() {
    constexpr std::uint64_t kBytes = 64;
    graph::Graph builder;
    const auto geometry = builder.import_buffer(
        "geometry",
        {{1, 0}, kBytes, {graph::Stage::Host, graph::Access::Write, graph::Layout::Buffer}, true}
    );
    const auto structure = builder.import_buffer(
        "structure",
        {{2, 0},
         kBytes,
         {graph::Stage::None, graph::Access::None, graph::Layout::Buffer},
         false,
         graph::BufferKind::AccelerationStructure}
    );
    CHECK(geometry && structure);
    builder.add_pass(
        {"build",
         {},
         {{*geometry,
           {0, kBytes},
           {graph::Stage::AccelerationBuild, graph::Access::Read, graph::Layout::Buffer}},
          {*structure,
           {0, kBytes},
           {graph::Stage::AccelerationBuild, graph::Access::Write, graph::Layout::Buffer}}}}
    );
    builder.add_pass(
        {"query",
         {},
         {{*structure,
           {0, kBytes},
           {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}}}
    );
    const auto plan = builder.compile();
    CHECK(plan);
    CHECK(plan->passes[0].barriers.size() == 1);
    CHECK(plan->passes[1].barriers.size() == 1);
    for (const auto& pass : plan->passes) {
        for (const auto& transition : pass.barriers) {
            const auto& desc =
                std::get<graph::BufferDesc>(plan->resources[transition.resource].description);
            check_dependency(transition, desc, transition.resource == geometry->index);
        }
    }
    const graph::State sampled{
        graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer
    };
    CHECK((native::access_of(sampled) & VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR) == 0);
    CHECK((native::access_of(sampled) & VK_ACCESS_SHADER_READ_BIT) != 0);
}

// Reusing scratch must wait for both its internal reads and writes, not geometry reads.
void test_scratch_and_alias_roles() {
    const graph::State scratch{
        graph::Stage::AccelerationBuild, graph::Access::ReadWrite, graph::Layout::Buffer
    };
    CHECK(
        native::access_of(scratch, graph::BufferKind::AccelerationScratch)
        == (VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
            | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR)
    );
    graph::Graph builder;
    graph::BufferDesc desc{
        {1, 0},
        1,
        {graph::Stage::AccelerationBuild, graph::Access::Write, graph::Layout::Buffer},
        true,
        graph::BufferKind::AccelerationStructure
    };
    CHECK(builder.import_buffer("AS", desc));
    desc.kind = graph::BufferKind::Data;
    CHECK(!builder.import_buffer("conflicting data alias", desc));
}

void test_buffer_read_waits_for_writers() {
    constexpr auto writers =
        graph::Stage::Host | graph::Stage::Transfer | graph::Stage::ComputeShader;
    constexpr auto prior = writers | graph::Stage::VertexInput | graph::Stage::DrawIndirect;
    constexpr auto layout = graph::Layout::Buffer;
    graph::Graph builder;
    const auto buffer =
        builder
            .import_buffer(
                "owned buffer", {{1, 0}, 64, {prior, graph::Access::ReadWrite, layout}, true}
            )
            .value();
    builder.add_pass(
        {"read owned buffer",
         {},
         {{buffer, {0, 64}, {graph::Stage::VertexShader, graph::Access::Read, layout}}}}
    );
    const auto plan = builder.compile().value();
    auto barrier = plan.passes.front().barriers.at(0);
    CHECK(native::graph_buffer_source_stages(barrier) == native::stages_of(writers));
    const auto handle = std::bit_cast<VkBuffer>(std::uint64_t{1});
    CHECK(
        native::graph_buffer_barrier(barrier, handle).srcAccessMask
        == (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT)
    );
    barrier.after = {graph::Stage::ComputeShader, graph::Access::Write, layout};
    CHECK(native::graph_buffer_source_stages(barrier) == native::stages_of(prior));
    CHECK(
        native::graph_buffer_barrier(barrier, handle).srcAccessMask
        & VK_ACCESS_INDIRECT_COMMAND_READ_BIT
    );
    barrier.before.stages = graph::Stage::All;
    barrier.after.access = graph::Access::Read;
    CHECK(native::graph_buffer_source_stages(barrier) == VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}
}  // namespace

int main() {
    try {
        test_build_to_query_dependencies();
        test_scratch_and_alias_roles();
        test_buffer_read_waits_for_writers();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "graph state translation: %s\n", error.what());
        return 1;
    }
}

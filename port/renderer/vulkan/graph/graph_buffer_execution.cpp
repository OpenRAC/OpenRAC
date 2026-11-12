// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shares graph buffer barrier lowering with independent-device executors such as the baker.
#include "renderer/vulkan/graph/graph_buffer_execution.h"

#include <bit>

#include "renderer/graph/breadcrumbs.h"
#include "renderer/graph/recording_scope.h"
#include "renderer/vulkan/commands/native_command_encoder.h"
#include "renderer/vulkan/graph/graph_state_translation.h"

namespace openrac::vkgl {

// Independent devices share state translation, but have no renderer counters or frame slots.
void record_buffer_graph(
    VkCommandBuffer command,
    const renderer::graph::Plan& plan,
    const std::function<void(unsigned)>& record
) {
    namespace graph = renderer::graph;
    for (unsigned index = 0; index < plan.passes.size(); ++index) {
        const auto& pass = plan.passes[index];
        graph::BreadcrumbScope operation{"graph buffer pass", pass.name};
        for (const auto& transition : pass.barriers) {
            const auto& buffer =
                std::get<graph::BufferDesc>(plan.resources.at(transition.resource).description);
            const auto barrier = detail::graph_buffer_barrier(
                transition, std::bit_cast<VkBuffer>(buffer.key.identity), 0, buffer.kind
            );
            NativeCommandEncoder{command}.pipeline_barrier(
                detail::graph_buffer_source_stages(transition),
                detail::stages_of(transition.after.stages),
                0,
                0,
                nullptr,
                1,
                &barrier,
                0,
                nullptr
            );
        }
        // Exports carry dependencies without authorizing native work.
        if (pass.kind == graph::PassKind::Work) {
            const graph::RecordingScope work(pass);
            record(index);
        }
        operation.complete();
    }
}

}  // namespace openrac::vkgl

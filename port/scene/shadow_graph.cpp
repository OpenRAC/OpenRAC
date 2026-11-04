// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shadow graph phases, importing current attachment and sampler states.
#include "scene/shadow_graph.h"

#include <exception>
#include <string>
#include <utility>

#include "renderer/gl.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph_execution.h"
#endif

namespace openrac::scene {
namespace {
// OpenGL recording keeps the same error boundary as Vulkan graph callbacks.
Result<void> compatibility_pass(const std::function<void()>& record) {
    // A missing callback cannot satisfy the declared shadow phase.
    if (!record) {
        return unexpected(Error{"shadow pass requires a recording callback"});
    }

    // OpenGL recording failures stay within the scene module.
    try {
        record();
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"shadow recording callback threw an unknown exception"});
    }

    return {};
}
#if defined(OPENRAC_VULKAN)
namespace graph = renderer::graph;
// Early lighting and fallback material shaders can sample any completed shadow map.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// Returns whether the active backend uses Vulkan shadow declarations.
bool graph_enabled() {
    return gl::is_vulkan();
}

// Declares depth testing against clear or retained contents, leaving physical clears to the callback.
graph::Pass depth_write(std::string name, graph::ImageHandle depth, bool clear) {
    return {
        std::move(name),
        {{depth,
          {graph::Aspect::Depth},
          {graph::Stage::DepthTests, graph::Access::ReadWrite, graph::Layout::DepthAttachment},
          clear ? graph::Load::Clear : graph::Load::Preserve}}
    };
}

// Keeps cached copies transfer-ready and all lighting maps readable by every shader stage.
graph::State consumer_state(ShadowDepthConsumer consumer) {
    // Static caches feed the depth copy before any lighting sampler consumes their contents.
    if (consumer == ShadowDepthConsumer::Copy) {
        return {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource};
    }

    return {kShaders, graph::Access::Read, graph::Layout::Sampled};
}

// Declares complete source/destination image accesses for the unchanged depth copy command.
graph::Pass depth_copy(graph::ImageHandle source, graph::ImageHandle destination) {
    return {
        "static shadow depth copy",
        {{source,
          {graph::Aspect::Depth},
          {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}},
         {destination,
          {graph::Aspect::Depth},
          {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}}
    };
}
#endif
}  // namespace

Result<void> shadow_depth_pass(
    std::string_view name,
    unsigned framebuffer,
    ShadowDepthContents contents,
    const std::function<void()>& record,
    ShadowDepthConsumer consumer
) {
#if defined(OPENRAC_VULKAN)
    // Vulkan declares attachment dependencies; OpenGL retains its implicit transitions.
    if (graph_enabled()) {
        const bool clear = contents == ShadowDepthContents::Clear;
        vkgl::GraphExecution execution;
        const auto depth = execution.import_attachment(
            std::string(name),
            framebuffer,
            graph::Aspect::Depth,
            clear ? vkgl::InitialContents::Discard : vkgl::InitialContents::Preserve
        );

        // Missing or undefined preserved depth cannot satisfy this phase before any callback.
        if (!depth) {
            return unexpected(depth.error());
        }

        execution.add_pass(depth_write(std::string(name), *depth, clear));
        execution.add_pass(
            {std::string(name) + " consumers",
             {{*depth, {graph::Aspect::Depth}, consumer_state(consumer)}},
             {},
             graph::PassKind::Export}
        );
        // Copy the callback; its explicit captures survive through an enclosing shared plan.
        return execution.run_prepared([record](unsigned) { record(); });
    }
#else
    // Non-Vulkan builds need neither native imports nor graph state.
    (void)name;
    (void)framebuffer;
    (void)contents;
    (void)consumer;
#endif
    return compatibility_pass(record);
}

Result<void> shadow_depth_copy(
    unsigned source, unsigned destination, const std::function<void()>& record
) {
#if defined(OPENRAC_VULKAN)
    // The copy stays a legacy GL-shaped command, bounded by explicit transfer dependencies.
    if (graph_enabled()) {
        vkgl::GraphExecution execution;
        const auto from =
            execution.import_attachment("static shadow copy source", source, graph::Aspect::Depth);
        const auto to = execution.import_attachment(
            "shadow atlas copy target",
            destination,
            graph::Aspect::Depth,
            vkgl::InitialContents::Discard
        );

        // Both complete depth allocations must exist before recording a transfer.
        if (!from || !to) {
            return unexpected(!from ? from.error() : to.error());
        }

        execution.add_pass(depth_copy(*from, *to));
        execution.add_pass(
            {"shadow atlas attachment",
             {{*to,
               {graph::Aspect::Depth},
               {graph::Stage::DepthTests, graph::Access::Read, graph::Layout::DepthAttachment}}},
             {},
             graph::PassKind::Export}
        );
        // The copied transfer callback records on the owning thread after its shared dependencies.
        return execution.run_prepared([record](unsigned) { record(); });
    }
#else
    // Source and destination names are consumed only by the original callback on compatibility builds.
    (void)source;
    (void)destination;
#endif
    return compatibility_pass(record);
}

Result<void> shadow_depth_ready(unsigned framebuffer, const std::function<void()>& record) {
#if defined(OPENRAC_VULKAN)
    // Final cleanup follows shadow writers and composition without declaring another depth writer.
    if (graph_enabled()) {
        vkgl::GraphExecution execution;
        const auto depth = execution.import_attachment(
            "completed shadow atlas", framebuffer, graph::Aspect::Depth
        );
        // A completion callback cannot advertise an unavailable atlas to lighting.
        if (!depth) {
            return unexpected(depth.error());
        }
        execution.add_pass(
            {"shadow atlas completion",
             {{*depth,
               {graph::Aspect::Depth},
               {kShaders, graph::Access::Read, graph::Layout::Sampled}}}}
        );
        // The copied callback owns its bookkeeping snapshots until the enclosing plan finishes.
        return execution.run_prepared([record](unsigned) { record(); });
    }
#else
    (void)framebuffer;
#endif
    return compatibility_pass(record);
}
}  // namespace openrac::scene

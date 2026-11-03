// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Declares GTAO's image and exact buffer-range dependencies; callbacks retain the existing shader dispatches.
#include <stdexcept>
#include <utility>

#include "common/log.h"
#include "enhanced/renderer/gtao.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph_execution.h"
#include "renderer/vulkan/surface_batch.h"
#include "renderer/vulkan/vkgl.h"

namespace openrac::renderer {

namespace {
namespace graph = renderer::graph;
// All shader stages may consume the exported raw AO and prepass images after this subgraph.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;
constexpr graph::State kRead{
    graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer
};
constexpr graph::State kWrite{
    graph::Stage::ComputeShader, graph::Access::Write, graph::Layout::Buffer
};

// Typed imports shared by the optional hierarchy, horizon shader, filter and boundary exports.
struct Inputs {
    graph::ImageHandle depth;                      // Borrowed, resolved prepass depth.
    std::optional<graph::ImageHandle> normals;     // Borrowed smooth normals, when available.
    graph::BufferHandle ao;                        // Packed RGBA8 horizon/guide output.
    std::optional<graph::BufferHandle> hierarchy;  // All min/max mip ranges in one allocation.
    std::optional<graph::ImageHandle> output;      // Optional full-resolution filter destination.
    std::uint64_t ao_bytes = 0;                    // Exact written horizon extent.
};

// Renderer names and output intent, resolved to actual allocations before any callbacks.
struct Resources {
    unsigned depth = 0;          // Single-sample source depth.
    unsigned normals = 0;        // Optional source normals.
    unsigned ao = 0;             // Horizon output storage.
    unsigned hierarchy = 0;      // Optional hierarchy storage.
    unsigned destination = 0;    // Filter framebuffer, including the default framebuffer.
    std::uint64_t ao_bytes = 0;  // Written horizon bytes.
    bool filter = false;         // Whether a separate filter pass is needed.
    bool blend = false;          // Whether the filter preserves and multiplies existing colour.
};

// Resolves optional resources while preserving the precise import error.
Result<Inputs> import_inputs(vkgl::GraphExecution& execution, const Resources& names) {
    const auto depth = execution.import_image("GTAO depth", names.depth);
    const auto ao =
        execution.import_buffer("GTAO horizons", names.ao, vkgl::InitialContents::Discard);

    // Missing required imports must fail before any pass callback runs.
    if (!depth || !ao) {
        return unexpected(!depth ? depth.error() : ao.error());
    }

    Inputs imports{*depth, {}, *ao, {}, {}, names.ao_bytes};

    // Standalone AO tests can omit prepass normals; the shader retains its existing fallback.
    if (names.normals) {
        const auto normals = execution.import_image("prepass normals", names.normals);

        if (!normals) {
            return unexpected(normals.error());
        }

        imports.normals = *normals;
    }

    // No-Hi-Z graphs neither import nor synchronise hierarchy storage.
    if (names.hierarchy) {
        const auto hierarchy = execution.import_buffer(
            "GTAO depth hierarchy", names.hierarchy, vkgl::InitialContents::Discard
        );

        if (!hierarchy) {
            return unexpected(hierarchy.error());
        }

        imports.hierarchy = *hierarchy;
    }

    // Fused early lighting reads raw AO, so it does not need a separate filter destination.
    if (names.filter) {
        const auto output = execution.import_attachment(
            "GTAO filter target",
            names.destination,
            graph::Aspect::Colour,
            names.blend ? vkgl::InitialContents::Preserve : vkgl::InitialContents::Discard
        );

        if (!output) {
            return unexpected(output.error());
        }

        imports.output = *output;
    }

    return imports;
}

// Creates image reads for whichever shader stage consumes the prepass data.
std::vector<graph::ImageUse> reads(const Inputs& inputs, graph::Stage stage) {
    const graph::State state{stage, graph::Access::Read, graph::Layout::Sampled};
    std::vector<graph::ImageUse> result{{inputs.depth, {graph::Aspect::Depth}, state}};

    // Only declare sampled resources actually present in this evaluation.
    if (inputs.normals) {
        result.push_back({*inputs.normals, {}, state});
    }

    return result;
}

// Writes every hierarchy cell once and orders each reduction against its preceding mip's exact bytes.
void declare_hierarchy(
    vkgl::GraphExecution& execution, const Inputs& inputs, std::span<const GtaoDepthLevel> levels
) {
    // Every destination range is disjoint from the preceding level's source range.
    for (unsigned level = 0; level < levels.size(); ++level) {
        const auto& destination = levels[level];
        graph::Pass pass{"GTAO depth mip " + std::to_string(level)};
        pass.buffers.push_back(
            {*inputs.hierarchy, {destination.offset, destination.bytes}, kWrite}
        );

        // The first reduction reads full-resolution depth; later reductions read stored bounds.
        if (level == 0) {
            pass.images = reads(inputs, graph::Stage::ComputeShader);
        } else {
            const auto& source = levels[level - 1];
            pass.buffers.push_back({*inputs.hierarchy, {source.offset, source.bytes}, kRead});
        }

        execution.add_pass(std::move(pass));
    }
}

// Declares horizon writes, optional filtering, and the state required by fused early lighting.
void declare_ao(
    vkgl::GraphExecution& execution, const Inputs& inputs, bool blend, std::uint64_t depth_bytes
) {
    graph::Pass evaluate{
        "GTAO horizons",
        reads(inputs, graph::Stage::ComputeShader),
        {{inputs.ao, {0, inputs.ao_bytes}, kWrite}}
    };

    // Distant horizon taps can read any stored level, all of which have been produced above.
    if (inputs.hierarchy) {
        evaluate.buffers.push_back({*inputs.hierarchy, {0, depth_bytes}, kRead});
    }

    execution.add_pass(std::move(evaluate));

    // Standalone filtering reads the horizon buffer without modifying it.
    if (inputs.output) {
        graph::Pass filter{
            "GTAO filter",
            reads(inputs, graph::Stage::FragmentShader),
            {{inputs.ao,
              {0, inputs.ao_bytes},
              {graph::Stage::FragmentShader, graph::Access::Read, graph::Layout::Buffer}}}
        };
        filter.images.push_back(
            {*inputs.output,
             {},
             {graph::Stage::ColourOutput,
              blend ? graph::Access::ReadWrite : graph::Access::Write,
              graph::Layout::ColourAttachment},
             blend ? graph::Load::Preserve : graph::Load::Discard}
        );
        execution.add_pass(std::move(filter));
    }

    graph::Pass boundary{
        "GTAO consumers",
        reads(inputs, kShaders),
        {{inputs.ao, {0, inputs.ao_bytes}, {kShaders, graph::Access::Read, graph::Layout::Buffer}}},
        graph::PassKind::Export
    };
    execution.add_pass(std::move(boundary));
}

// Declares cleared prepass attachments and their immediate reuse/sampling consumers.
void declare_prepass(
    vkgl::GraphExecution& execution,
    graph::ImageHandle depth,
    graph::ImageHandle normal,
    bool multisample
) {
    execution.add_pass(
        {"depth/normal prepass",
         {{depth,
           {graph::Aspect::Depth},
           {graph::Stage::DepthTests, graph::Access::Write, graph::Layout::DepthAttachment},
           graph::Load::Clear},
          {normal,
           {},
           {graph::Stage::ColourOutput, graph::Access::Write, graph::Layout::ColourAttachment},
           graph::Load::Clear}}}
    );
    // Multisample normals feed a colour resolve; single-sample normals feed shaders directly.
    execution.add_pass(
        {"prepass consumers",
         {{depth,
           {graph::Aspect::Depth},
           {graph::Stage::Transfer, graph::Access::Read, graph::Layout::TransferSource}},
          {normal,
           {},
           {multisample ? graph::Stage::Transfer
                        : graph::Stage::ComputeShader | graph::Stage::FragmentShader,
            graph::Access::Read,
            multisample ? graph::Layout::TransferSource : graph::Layout::Sampled}}},
         {},
         graph::PassKind::Export}
    );
}
}  // namespace

void Gtao::render_prepass(unsigned framebuffer, const std::function<void()>& record) {
    // OpenGL uses the same scene draw callback without Vulkan declarations.
    if (!vkgl::active()) {
        record();
        return;
    }

    namespace graph = renderer::graph;
    vkgl::GraphExecution execution;
    const auto depth = execution.import_attachment(
        "prepass depth", framebuffer, graph::Aspect::Depth, vkgl::InitialContents::Discard
    );
    const auto normal = execution.import_attachment(
        "prepass normals", framebuffer, graph::Aspect::Colour, vkgl::InitialContents::Discard
    );

    // A missing attachment cannot satisfy the declared clear/write contract.
    if (!depth || !normal) {
        log::error("prepass graph: {}", !depth ? depth.error().message : normal.error().message);
        record();
        return;
    }

    gl::GLint samples = 1;
    gl::glGetIntegerv(gl::GL_SAMPLES, &samples);
    declare_prepass(execution, *depth, *normal, samples > 1);
    // A surface chain owns this callback until its enclosing preparation/execution call returns.
    if (vkgl::detail::collecting_surface_chain()) {
        const auto result = execution.run_prepared([record](unsigned) { record(); });
        // Do not publish a prepass whose deferred declaration was rejected.
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        return;
    }
    // The callback runs synchronously while this render state remains alive.
    bool recorded = false;
    const auto result = execution.run([&](unsigned) {
        recorded = true;
        record();
    });

    if (!result) {
        log::error("prepass graph: {}", result.error().message);
        // Validation precedes callbacks, so the ordinary prepass can still run exactly once.
        if (!recorded) {
            record();
        }
    }
}

bool Gtao::execute_graph(
    const FrameInput& input,
    const RenderTarget& target,
    bool apply_to_colour,
    bool evaluate_only,
    bool hi_z
) {
    vkgl::GraphExecution execution;
    const Resources names{
        m_sampled_depth,
        target.normal_texture,
        m_ao_buffer,
        hi_z ? m_hi_z.buffer() : 0,
        apply_to_colour ? target.framebuffer : m_visibility.id(),
        static_cast<std::uint64_t>(m_ao_width) * static_cast<unsigned>(m_ao_height) * 4,
        !evaluate_only,
        apply_to_colour
    };
    const auto imported = import_inputs(execution, names);

    if (!imported) {
        log::error("GTAO graph: {}", imported.error().message);
        return false;
    }

    const Inputs& imports = *imported;
    std::uint64_t depth_bytes = 0;

    // The graph contains no hierarchy producers when full-resolution sampling is selected.
    if (hi_z) {
        const auto& last = m_hi_z.levels().back();
        depth_bytes = last.offset + last.bytes;
        declare_hierarchy(execution, imports, m_hi_z.levels());
    }

    declare_ao(execution, imports, apply_to_colour, depth_bytes);
    const unsigned mip_count = hi_z ? static_cast<unsigned>(m_hi_z.levels().size()) : 0;
    // The executor invokes this callback synchronously on the render thread, after each entry barrier.
    const auto result = execution.run([&](unsigned pass) {
        // Only hierarchy passes precede the horizon dispatch; the optional filter follows it.
        if (pass < mip_count) {
            m_hi_z.build_level(m_sampled_depth, pass);
        } else if (pass == mip_count) {
            evaluate(input, target, hi_z);
        } else {
            filter(input, target, apply_to_colour);
        }
    });

    // Keep execution errors visible instead of silently drawing an incomplete AO result.
    if (!result) {
        log::error("GTAO graph: {}", result.error().message);
        return false;
    }

    return true;
}
}  // namespace openrac::renderer
#endif

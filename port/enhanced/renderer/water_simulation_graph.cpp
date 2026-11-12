// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One fixed-order water graph: window/history preparation, alternating substeps, transfer and shader exports.
#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include "enhanced/renderer/water_graph.h"
#include "enhanced/renderer/water_internal.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"
#if defined(OPENRAC_VULKAN)
#include "renderer/vulkan/graph/graph_execution.h"
#include "renderer/vulkan/vkgl.h"
#endif

namespace openrac::enhanced::water_detail {
namespace {
#if defined(OPENRAC_VULKAN)
namespace graph = renderer::graph;
// All drawing stages consume displacement, including depth and shadow vertex deformation.
constexpr graph::Stage kShaders =
    graph::Stage::VertexShader | graph::Stage::FragmentShader | graph::Stage::ComputeShader;

// One simulation owns these borrowed allocations until every synchronous callback finishes.
struct Imports {
    std::array<graph::BufferHandle, kBufferCount> buffers{};  // Binding-indexed canonical storage.
    graph::ImageHandle map;                // Complete sampled displacement image.
    std::span<const std::uint64_t> bytes;  // Owned by the caller through this call.
};

// Resolves stable buffer allocations with explicit incoming-content policies before recording any work.
Result<Imports> import_resources(
    vkgl::GraphExecution& execution,
    const SimulationResources& resources,
    const SimulationPlan& plan
) {
    constexpr std::array<const char*, kBufferCount> kNames{
        "water state A",
        "water state B",
        "water coast",
        "water bodies",
        "water lighting",
        "water wave pixels",
        "water hydraulics",
        "water window coast",
        "water window hydraulics"
    };
    Imports result;
    result.bytes = resources.bytes;

    // The lighting binding is not part of simulation; its zero extent deliberately leaves it unimported.
    for (unsigned index = 0; index < kBufferCount; ++index) {
        // Unused bindings contribute no allocation or dependency to simulation.
        if (!resources.bytes[index]) {
            continue;
        }

        const bool overwritten =
            index == kRenderPixels
            || (plan.prepare_window
                && (index == kWindowHydraulics || (index == kWindowCoast && plan.reset)));
        const auto imported = execution.import_buffer(
            kNames[index],
            resources.buffers[index],
            overwritten ? vkgl::InitialContents::Discard : vkgl::InitialContents::Preserve
        );

        // Validation finishes before callbacks can change retained simulation state.
        if (!imported) {
            return unexpected(imported.error());
        }

        result.buffers[index] = *imported;
    }

    const auto map = execution.import_image(
        "water wave map", resources.wave_map, vkgl::InitialContents::Discard
    );

    // A complete single-level output image is required before any solver dispatch.
    if (!map) {
        return unexpected(map.error());
    }

    result.map = *map;
    return result;
}

// Declares one complete storage range using the shader's existing binding index.
graph::BufferUse storage(const Imports& imports, unsigned index, graph::Access access) {
    return {
        imports.buffers[index],
        {0, imports.bytes[index]},
        {graph::Stage::ComputeShader, access, graph::Layout::Buffer}
    };
}

// Window updates can write only newly exposed history slots, preserving the rest of both state buffers.
graph::Pass window_pass(const Imports& imports, bool reset) {
    graph::Pass pass{"water window preparation"};

    // Metadata preparation samples the immutable authored fields.
    for (unsigned index : {kCoast, kBodies, kHydraulics}) {
        pass.buffers.push_back(storage(imports, index, graph::Access::Read));
    }

    // Both histories preserve retained torus slots while newly exposed slots are seeded.
    for (unsigned index : {kStateA, kStateB}) {
        pass.buffers.push_back(storage(imports, index, graph::Access::ReadWrite));
    }

    pass.buffers.push_back(
        storage(imports, kWindowCoast, reset ? graph::Access::Write : graph::Access::ReadWrite)
    );
    pass.buffers.push_back(storage(imports, kWindowHydraulics, graph::Access::Write));
    return pass;
}

// Each substep reads the current state and writes the alternate state and the complete wave-pixel buffer.
graph::Pass step_pass(const Imports& imports, unsigned current, unsigned step) {
    graph::Pass pass{"water substep " + std::to_string(step)};

    // Solver geometry and hydraulic fields stay read-only across all substeps.
    for (unsigned index : {kCoast, kBodies, kHydraulics, kWindowCoast, kWindowHydraulics}) {
        pass.buffers.push_back(storage(imports, index, graph::Access::Read));
    }

    pass.buffers.push_back(storage(imports, current, graph::Access::Read));
    pass.buffers.push_back(storage(imports, 1 - current, graph::Access::Write));
    pass.buffers.push_back(storage(imports, kRenderPixels, graph::Access::Write));
    return pass;
}

// Transfers exactly the produced pixels and exports the image to all deformation and shading consumers.
Result<void> declare_copy(vkgl::GraphExecution& execution, const Imports& imports) {
    const graph::BufferUse pixels{
        imports.buffers[kRenderPixels],
        {0, imports.bytes[kRenderPixels]},
        {graph::Stage::Transfer, graph::Access::Read, graph::Layout::Buffer}
    };
    graph::CommandBuffer commands;
    commands.copy_buffer_to_image(pixels.buffer, pixels.range, imports.map);
    const auto added = execution.add_pass(
        {"water wave-map copy",
         {{imports.map,
           {},
           {graph::Stage::Transfer, graph::Access::Write, graph::Layout::TransferDestination}}},
         {pixels}},
        std::move(commands)
    );
    if (!added) {
        return added;
    }
    graph::Pass consumers{
        "water simulation consumers",
        {{imports.map, {}, {kShaders, graph::Access::Read, graph::Layout::Sampled}}},
        {},
        graph::PassKind::Export
    };

    // The next update reads retained history and metadata before writing new values.
    for (unsigned index : {kStateA, kStateB, kWindowCoast, kWindowHydraulics}) {
        consumers.buffers.push_back(storage(imports, index, graph::Access::Read));
    }

    execution.add_pass(std::move(consumers));
    return {};
}

// Compiles the whole chain before callbacks mutate ping-pong ownership or metadata cache keys.
Result<void> graph_simulation(
    const SimulationResources& resources,
    const SimulationPlan& plan,
    const std::function<void(SimulationPhase, unsigned)>& record
) {
    vkgl::GraphExecution execution;
    const auto imports = import_resources(execution, resources, plan);

    // No dispatch may run with an incomplete set of imported resources.
    if (!imports) {
        return unexpected(imports.error());
    }

    // A warm unchanged window needs neither metadata work nor its associated barriers.
    if (plan.prepare_window) {
        execution.add_pass(window_pass(*imports, plan.reset));
    }

    // Preserve the original substep order, including all alternating read/write hazards.
    for (unsigned step = 0; step < plan.substeps; ++step) {
        execution.add_pass(step_pass(*imports, (plan.current + step) % 2, step));
    }

    const auto copied = declare_copy(execution, *imports);
    if (!copied) {
        return copied;
    }
    const unsigned first_step = plan.prepare_window ? 1 : 0;
    // The executor invokes each callback synchronously on the render thread, after its entry barriers.
    return execution.run([&](unsigned index) {
        // Only the optional first work pass prepares history and metadata.
        if (index < first_step) {
            record(SimulationPhase::Window, 0);
        } else {
            record(SimulationPhase::Step, index - first_step);
        }
    });
}
#endif

// Orders OpenGL metadata and substeps before compatibility consumers.
Result<void> compatibility_simulation(
    const SimulationResources& resources,
    const SimulationPlan& plan,
    const std::function<void(SimulationPhase, unsigned)>& record
) {
    using namespace gl;

    // A warm unchanged window needs neither metadata work nor its associated barriers.
    if (plan.prepare_window) {
        record(SimulationPhase::Window, 0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    const bool compact =
        renderer::render_opt::enabled(renderer::render_opt::Opt::WaterStepBarriers);

    // Preserve the original substep order, including all alternating read/write hazards.
    for (unsigned step = 0; step < plan.substeps; ++step) {
        // OpenGL reuses each trailing dependency for the next substep.
        if (step == 0 || !compact) {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        record(SimulationPhase::Step, step);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }
    (void)resources;
    return {};
}
}  // namespace

Result<void> water_simulation_passes(
    const SimulationResources& resources,
    const SimulationPlan& plan,
    const std::function<void(SimulationPhase, unsigned)>& record
) {
    // One game update has at most four caught-up ticks, with four existing substeps per tick.
    constexpr unsigned kMaxSubsteps = 16;

    if (!record || resources.buffers.size() != kBufferCount
        || resources.bytes.size() != kBufferCount || !resources.side || plan.current > 1
        || !plan.substeps || plan.substeps > kMaxSubsteps) {
        return unexpected(Error{"invalid water simulation graph inputs"});
    }

    // Driver and callback failures remain inside the water graph interface.
    try {
#if defined(OPENRAC_VULKAN)
        // Vulkan uses declared simulation dependencies; OpenGL keeps its native ordering.
        if (gl::is_vulkan()) {
            return graph_simulation(resources, plan, record);
        }
#endif
        return compatibility_simulation(resources, plan, record);
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"water simulation callback threw an unknown exception"});
    }
}
}  // namespace openrac::enhanced::water_detail

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Synchronous water graph phases, preserving shader kernels and the reference dispatch order.
#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

#include "renderer/graph/frame_graph.h"

namespace openrac::enhanced {
// Explicit whole-buffer storage access; reflected texture and uniform reads are captured separately.
struct WaterBufferUse {
    unsigned buffer = 0;      // Stable renderer-owned storage allocation.
    std::uint64_t bytes = 0;  // Complete accessed extent from offset zero.
    renderer::graph::Access access = renderer::graph::Access::Read;  // Actual kernel access.
};

// Records one water lighting, pose or authored-UV kernel and exports shader-visible results.
[[nodiscard]] Result<void> water_compute_pass(
    std::string_view name,
    std::span<const WaterBufferUse> buffers,
    const std::function<void()>& record
);

namespace water_detail {
// Stable binding-indexed buffers and byte extents, borrowed only through synchronous execution.
struct SimulationResources {
    std::span<const unsigned> buffers;     // Water's existing binding-indexed allocations.
    std::span<const std::uint64_t> bytes;  // Matching byte extents; unused bindings are zero.
    unsigned wave_map = 0;                 // Complete RGBA32F displacement/velocity/marker image.
    unsigned side = 0;                     // Square local simulation extent in texels.
};

// Describes only dependencies; physics parameters and sampling stay with the existing kernel recorder.
struct SimulationPlan {
    unsigned current = 0;         // Initial ping-pong source index, zero or one.
    unsigned substeps = 0;        // Existing ordered simulation substep count.
    bool prepare_window = false;  // Whether metadata and newly exposed history slots need updating.
    bool reset = false;           // A reset does not read previous local metadata.
};
// Metadata preparation precedes every wave substep; the graph owns the final transfer command.
enum class SimulationPhase : std::uint8_t {
    Window,
    Step
};
// Records the complete window/substep/copy chain synchronously on the render thread.
// Callbacks may update loose uniforms and bindings, but must not replace imported allocations.
[[nodiscard]] Result<void> water_simulation_passes(
    const SimulationResources& resources,
    const SimulationPlan& plan,
    const std::function<void(SimulationPhase, unsigned)>& record
);
}  // namespace water_detail
}  // namespace openrac::enhanced

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Scoped preparation and execution of the surface-lighting chain, including multisample resolves.
#pragma once
#include <array>
#include <functional>

#include "common/result.h"

namespace openrac::renderer {
// Complete surface copies retain colour averaging and sample-zero depth/stencil resolve behavior.
enum class SurfaceCopy {
    DepthStencil,
    Colour
};

struct SurfaceDepthScratch {
    unsigned framebuffer = 0;
    unsigned texture = 0;

    explicit operator bool() const { return framebuffer != 0; }
};

struct SurfaceRefractionScratch {
    unsigned framebuffer = 0;
    std::array<unsigned, 2> textures{};

    explicit operator bool() const { return framebuffer != 0; }
};

// Borrows a single-sample depth/stencil target through the current surface scope.
// The caller must declare a complete write before sampling it.
[[nodiscard]] SurfaceDepthScratch lease_surface_depth(unsigned source, std::array<int, 2> size);
[[nodiscard]] SurfaceRefractionScratch lease_surface_refraction(
    unsigned source, std::array<int, 2> size
);

// Returns whether Vulkan surface and lighting graphs can compose this frame's prepared work.
[[nodiscard]] bool surface_chain_enabled();
// Returns whether the graphics owner is collecting an enclosing shared plan.
[[nodiscard]] bool collecting_surface_chain();
// Prepares stable resources and dispatch bindings, compiles once, then records in declared order.
// The callback and its deferred raster captures must remain alive until this call returns.
[[nodiscard]] Result<void> run_surface_chain(const std::function<void()>& prepare);
// Finishes pending work and runs synchronous provider setup outside collection, then resumes it.
// Borrowed simulation/copy callbacks must complete before this call returns.
[[nodiscard]] Result<void> run_surface_setup(const std::function<void()>& prepare);
// Queues a full surface copy/resolve; false leaves native copying to the caller.
// Source and destination cover `size` pixels; invalid allocations or extents reject the chain.
[[nodiscard]] Result<bool> queue_surface_copy(
    unsigned source, unsigned destination, std::array<int, 2> size, SurfaceCopy aspect
);

namespace backend {
[[nodiscard]] SurfaceDepthScratch lease_surface_depth(unsigned source, std::array<int, 2> size);
[[nodiscard]] SurfaceRefractionScratch lease_surface_refraction(
    unsigned source, std::array<int, 2> size
);
// Queries the graphics owner's active collector without exposing native state.
[[nodiscard]] bool collecting_surface_chain();
// Implements scoped Vulkan collection without exposing native state in the core renderer.
[[nodiscard]] Result<void> run_surface_chain(const std::function<void()>& prepare);
// Preserves the active collector around synchronous setup and propagates pending-section failures.
[[nodiscard]] Result<void> run_surface_setup(const std::function<void()>& prepare);
// Adds the stable, complete surface copy or resolve to the active chain.
[[nodiscard]] Result<bool> queue_surface_copy(
    unsigned source, unsigned destination, std::array<int, 2> size, SurfaceCopy aspect
);
}  // namespace backend
}  // namespace openrac::renderer

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Private scoped chain hooks used by existing graph and GL-shaped recording entry points.
#pragma once
#include <functional>
#include <memory>

#include "common/result.h"

namespace openrac::vkgl {
class GraphExecution;

namespace detail {
class SurfaceBatch;
struct PreparedDispatch;
// Distinguishes mutable compute setup from commands whose bindings and captures are already stable.
enum class SurfaceWork {
    Compute,
    Prepared,
    Raster,
    Ordered
};
// Returns true only while a scoped chain collects its resource and binding declarations.
bool collecting_surface_chain();
// Executes pending declarations before a synchronous native provider returns its dispatch result.
[[nodiscard]] Result<void> flush_surface_chain();
// Adds borrowed graph declarations and owned execution packets to the current scoped chain.
[[nodiscard]] Result<void> collect_surface_graph(
    GraphExecution& source, const std::function<void(unsigned)>& record, SurfaceWork kind
);
// Captures immutable dispatch bindings; false leaves the ordinary immediate path active.
bool collect_surface_dispatch(PreparedDispatch& packet);
// Rejects unrepresented graphics/submission work before it can bypass a deferred producer.
void require_surface_execution();
}  // namespace detail
}  // namespace openrac::vkgl

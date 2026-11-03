// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The colour graph's refraction snapshot: native copies/resolves followed by sampled background
// exports.
#include <exception>

#include "renderer/colour_graph.h"
#include "renderer/colour_graph_backend.h"
#include "renderer/renderer.h"

namespace openrac::renderer {
Result<void> colour_snapshot(
    const RenderTarget& source,
    unsigned destination,
    int samples,
    const std::function<void(bool)>& record
) {
    // These are complete same-size snapshots, never partial/scaled copies or missing callbacks.
    if (!record || source.width <= 0 || source.height <= 0 || samples < 1) {
        return unexpected(Error{"invalid refraction snapshot inputs"});
    }

    // Both graph and reference callbacks report errors through the same renderer boundary.
    try {
#if defined(OPENRAC_VULKAN)
        // Reference snapshots retain the original backend-managed dependencies.
        if (colour_graph_enabled()) {
            return backend::record_colour_snapshot(
                source.framebuffer, destination, samples, record
            );
        }
#else
        (void)destination;
#endif
        record(false);
        record(true);
        return {};
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"refraction snapshot callback threw an unknown exception"});
    }
}
}  // namespace openrac::renderer

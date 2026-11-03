// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Colour graph entry points: preserved main attachments and descriptor-matched shader input
// visibility.
#include "renderer/colour_graph.h"

#include <exception>

#include "renderer/colour_graph_backend.h"
#include "renderer/gl.h"
#include "renderer/renderer.h"

namespace openrac::renderer {

bool colour_graph_enabled() {
#if defined(OPENRAC_VULKAN)
    return gl::is_vulkan();
#else
    return false;
#endif
}

Result<void> colour_pass(
    std::string_view name,
    const RenderTarget& target,
    const std::function<void()>& record,
    ColourAttachments attachments,
    ColourInputs inputs
) {
    // A missing callback cannot satisfy the attachment declaration on either backend.
    if (!record) {
        return unexpected(Error{"colour pass requires a callback"});
    }

    // Callback exceptions remain inside the renderer's Result boundary on both paths.
    try {
#if defined(OPENRAC_VULKAN)
        // Compatibility draws retain the existing implicit attachment handling.
        if (colour_graph_enabled()) {
            return backend::record_colour(name, target, record, attachments, inputs);
        }
#else
        (void)name;
        (void)target;
        (void)attachments;
        (void)inputs;
#endif
        record();
        return {};
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"colour callback threw an unknown exception"});
    }
}

Result<void> graphics_shader_reads(std::string_view name) {
    // Import and recording failures stay inside this public renderer boundary.
    try {
#if defined(OPENRAC_VULKAN)
        // Capture once at shared shader setup, never on each mesh or primitive draw.
        if (gl::is_vulkan()) {
            return backend::prepare_colour_reads(name);
        }
#else
        (void)name;
#endif
        return {};
    } catch (const std::exception& failure) {
        return unexpected(Error{failure.what()});
    } catch (...) {
        return unexpected(Error{"colour input preparation threw an unknown exception"});
    }
}
}  // namespace openrac::renderer

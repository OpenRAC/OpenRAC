// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements surface_chain.h's renderer-owned facade; compatibility rendering retains synchronous
// work.
#include "renderer/surface_chain.h"

#include <exception>

#include "renderer/gl.h"
#include "renderer/renderer.h"

namespace openrac::renderer {
bool surface_chain_enabled() {
#if defined(OPENRAC_VULKAN)
    return gl::is_vulkan();
#else
    return false;
#endif
}

bool collecting_surface_chain() {
#if defined(OPENRAC_VULKAN)
    return gl::is_vulkan() && backend::collecting_surface_chain();
#else
    return false;
#endif
}

SurfaceDepthScratch lease_surface_depth(unsigned source, std::array<int, 2> size) {
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan()) {
        return backend::lease_surface_depth(source, size);
    }
#else
    (void)source;
    (void)size;
#endif
    return {};
}

SurfaceRefractionScratch lease_surface_refraction(unsigned source, std::array<int, 2> size) {
#if defined(OPENRAC_VULKAN)
    if (gl::is_vulkan()) {
        return backend::lease_surface_refraction(source, size);
    }
#else
    (void)source;
    (void)size;
#endif
    return {};
}

Result<void> run_surface_chain(const std::function<void()>& prepare) {
    // An absent preparation callback cannot define the shared plan.
    if (!prepare) {
        return unexpected(Error{"surface chain requires preparation"});
    }
    // Preparation and recording failures stay inside this synchronous renderer boundary.
    try {
#if defined(OPENRAC_VULKAN)
        // OpenGL executes its compatibility callbacks synchronously.
        if (surface_chain_enabled()) {
            return backend::run_surface_chain(prepare);
        }
#endif
        prepare();
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown surface chain failure"});
    }
}

Result<bool> queue_surface_copy(
    unsigned source, unsigned destination, std::array<int, 2> size, SurfaceCopy aspect
) {
#if defined(OPENRAC_VULKAN)
    // Only Vulkan has an active collection context; other backends retain their original blit.
    if (gl::is_vulkan()) {
        return backend::queue_surface_copy(source, destination, size, aspect);
    }
#else
    (void)source;
    (void)destination;
    (void)size;
    (void)aspect;
#endif
    return false;
}

Result<void> run_surface_setup(const std::function<void()>& prepare) {
    // An absent setup callback cannot establish a valid preparation boundary.
    if (!prepare) {
        return unexpected(Error{"surface setup requires a callback"});
    }

    // Synchronous provider failures follow the same boundary as deferred preparation.
    try {
#if defined(OPENRAC_VULKAN)
        // Other backends have no deferred collector to suspend.
        if (gl::is_vulkan()) {
            return backend::run_surface_setup(prepare);
        }
#endif
        prepare();
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    } catch (...) {
        return unexpected(Error{"unknown surface setup failure"});
    }
}

bool Renderer::can_prepare_surface_chain(const FrameInput& input, const RenderState& state) const {
    // Unknown hooks use separate scoped graphs.
    if (!surface_chain_enabled() || !m_ambient_occlusion
        || !m_ambient_occlusion->prepares_surface_chain()
        || (input.after_world_depth && !input.prepares_world_depth)) {
        return false;
    }
    // Every early provider must prepare stable shadow and lighting declarations before execution.
    for (const auto& renderer : m_renderers) {
        // One unsupported provider prevents retaining the complete chain.
        if (renderer->enabled && renderer->needs_early_lighting(input)
            && !renderer->prepares_surface_chain(input, state)) {
            return false;
        }
    }
    return true;
}
}  // namespace openrac::renderer

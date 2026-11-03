// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Implements prepared_raster.h without changing shader arithmetic or borrowing setup stack references.
#include "renderer/vulkan/prepared_raster.h"

#include <stdexcept>
#include <utility>

namespace openrac::vkgl::detail {
PreparedRaster::PreparedRaster() : m_state(g->state) {
    const auto* program = g->programs.find(m_state.program);

    // Object-motion callbacks may instead install their own graphics program during execution.
    if (program && !program->compute_pipeline) {
        m_program_serial = program->serial;
        m_uniforms = program->uniforms;
        m_units = program->units;
        m_blocks = program->block_bindings;
    }

    m_targets[0] = m_state.draw_framebuffer;
    // The same framebuffer must not be exchanged twice.
    m_targets[1] = m_state.read_framebuffer == m_targets[0] ? 0 : m_state.read_framebuffer;

    // Later passes may attach and detach borrowed world depth on the same name.
    for (std::size_t n = 0; n < m_targets.size(); ++n) {
        const auto* target = g->framebuffers.find(m_targets[n]);

        // Default framebuffers have no context-owned attachment description.
        if (target) {
            m_attachments[n] = *target;
        }
    }
}

void PreparedRaster::exchange_program() {
    // Higher-level callbacks can install a program after collection.
    if (!m_program_serial) {
        return;
    }

    auto* program = g->programs.find(m_state.program);

    // Relinking invalidates the retained uniform layout.
    if (!program || program->serial != m_program_serial) {
        throw std::runtime_error("prepared raster program changed before execution");
    }

    program->uniforms.swap(m_uniforms);
    program->units.swap(m_units);
    program->block_bindings.swap(m_blocks);
    program->uniforms_dirty = true;
    program->last_set_frame = 0;
}

void PreparedRaster::exchange_targets() {
    // Validate both owners before exchanging either attachment description.
    for (std::size_t n = 0; n < m_targets.size(); ++n) {
        // A released named target cannot be replaced with a different allocation.
        if (m_attachments[n] && !g->framebuffers.find(m_targets[n])) {
            throw std::runtime_error("prepared raster framebuffer was released");
        }
    }

    end_rendering();

    // Every named target remains owned by its temporal pass through this scope.
    for (std::size_t n = 0; n < m_targets.size(); ++n) {
        // Default targets have no mutable named attachment description.
        if (m_attachments[n]) {
            auto* target = g->framebuffers.find(m_targets[n]);
            std::swap(*target, *m_attachments[n]);
        }
    }
}

void PreparedRaster::record(const std::function<void()>& draw) {
    g->state = m_state;
    g->bound_pipeline = VK_NULL_HANDLE;
    g->recorded.forget();
    exchange_targets();

    // Callback failures must not publish replay-time uniforms or attachment roles to later frames.
    try {
        exchange_program();
    } catch (...) {
        exchange_targets();
        throw;
    }

    // Draw failures restore the captured program as well as attachment roles.
    try {
        draw();
    } catch (...) {
        exchange_targets();
        exchange_program();
        throw;
    }

    exchange_targets();
    exchange_program();
}
}  // namespace openrac::vkgl::detail

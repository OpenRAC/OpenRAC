// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/subsystems.h"

#include "common/log.h"

namespace openrac::renderer {

void PlaceholderRenderer::render(const FrameInput& input, RenderState& state) {
    (void)state;
    const auto packets = input.packets[static_cast<std::size_t>(bucket())];
    if (!packets.empty() && !m_reported) {
        log::info(
            "renderer '{}' is a placeholder: {} bytes of {} packets not drawn",
            name(),
            packets.size(),
            bucket_name(bucket())
        );
        m_reported = true;
    }
}

}  // namespace openrac::renderer

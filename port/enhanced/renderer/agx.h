// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional SDR tone mapping with unchanged compute and raster curve evaluation.
#pragma once

#include <string>

#include "renderer/framebuffer.h"
#include "renderer/shader.h"

namespace openrac::enhanced {
// Optional SDR colour-treatment experiment, owned and used only by the render worker.
class AgxPreview {
public:
    // Releases preview-owned graphics resources on their owning context.
    ~AgxPreview();
    // Applies bounded EV exposure and tone mapping; logs and disables the preview on failure.
    void apply(renderer::FrameBuffer& output, float exposure);
    // Releases cached resources and allows a later initialization attempt.
    void release();

private:
    // Prepares the independent raster snapshot and shader before graph imports.
    [[nodiscard]] bool prepare(const renderer::FrameBuffer& output, std::string& error);

    renderer::FrameBuffer m_copy;  // Independent sampled source for the raster fallback.
    renderer::Shader m_shader;     // Original fullscreen curve.
    unsigned m_vao = 0;            // Attribute-free fullscreen vertex input.
    bool m_failed = false;  // Stops retries after an unsuccessful initialization or recording.
};
}  // namespace openrac::enhanced

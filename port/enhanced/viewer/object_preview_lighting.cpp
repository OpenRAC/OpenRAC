// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Production Forward+ table, attenuation, spot cones and GGX in the private object view.
#include "enhanced/viewer/object_preview_lighting.h"

#include "enhanced/viewer/object_preview_shader_source.h"
#include "enhanced_viewer_shaders.h"
#include "renderer/gl/gl.h"

namespace openrac::enhanced {
Result<void> ObjectPreviewLighting::set_lights(std::span<const LocalLight> lights) {
    auto result = m_lights.set_lights(lights);

    if (result) {
        m_count = lights.size();
    }

    return result;
}

std::string ObjectPreviewLighting::source() const {
    return object_preview_lighting_source();
}

void ObjectPreviewLighting::prepare(std::uint64_t frame) {
    m_lights.reference = reference;
    m_lights.prepare(frame, {}, true);
}

void ObjectPreviewLighting::cull(
    unsigned depth, const renderer::Camera& camera, int width, int height
) {
    m_lights.dispatch(depth, camera, width, height);
}

void ObjectPreviewLighting::bind(
    const renderer::Shader& shader, const renderer::Camera& camera, bool opaque
) const {
    m_lights.bind(shader, &camera, opaque);
    gl::glUniform1i(shader.uniform("preview_enhanced"), enabled);
}

void ObjectPreviewLighting::release() {
    m_lights.release();
}
}  // namespace openrac::enhanced

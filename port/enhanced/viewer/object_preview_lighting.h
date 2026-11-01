// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Optional Forward+ shading registered with the general object viewer.
#pragma once
#include "enhanced/renderer/local_lights.h"
#include "scene/object_preview.h"

namespace openrac::enhanced {
class ObjectPreviewLighting final : public scene::PreviewLighting {
public:
    bool enabled = true, reference = false;
    Result<void> set_lights(std::span<const LocalLight> lights);
    std::string source() const override;
    void prepare(std::uint64_t frame) override;

    bool wants_depth() const override { return enabled && m_count >= 8; }

    void cull(unsigned depth, const renderer::Camera& camera, int width, int height) override;
    void bind(const renderer::Shader&, const renderer::Camera&, bool opaque) const override;
    void release() override;

private:
    LocalLights m_lights;
    std::size_t m_count = 0;
};
}  // namespace openrac::enhanced

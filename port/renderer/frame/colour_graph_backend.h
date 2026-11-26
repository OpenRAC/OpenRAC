// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Renderer-owned contract implemented by the Vulkan backend; no native graphics types cross this
// interface.
#pragma once

#include <functional>
#include <string_view>

#include "common/result.h"

namespace openrac::renderer {
struct RenderTarget;
enum class ColourAttachments;
enum class SnapshotAttachments;
struct ColourInputs;
struct RasterInputs;

namespace backend {
// Records preserved colour and optional world depth; captures survive an enclosing shared scope.
[[nodiscard]] Result<void> record_colour(
    std::string_view name,
    const RenderTarget& target,
    const std::function<void()>& record,
    ColourAttachments attachments,
    ColourInputs inputs
);
// Orders the bound graphics shader's shared read-only descriptors before subsequent draws.
[[nodiscard]] Result<void> prepare_colour_reads(std::string_view name);
[[nodiscard]] Result<void> prepare_raster_reads(std::string_view name, RasterInputs inputs);
// Declares the active depth attachment and, when requested, the normal target.
[[nodiscard]] Result<void> record_surface_prepass(
    unsigned framebuffer, bool normals, bool multisample, const std::function<void()>& record
);
// Records complete colour/depth snapshot copies or resolves; callbacks receive false for colour,
// true for depth.
[[nodiscard]] Result<void> record_colour_snapshot(
    unsigned source, unsigned destination, int samples, const std::function<void(bool)>& record,
    SnapshotAttachments attachments
);
}  // namespace backend
}  // namespace openrac::renderer

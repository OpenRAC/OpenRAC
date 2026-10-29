// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Geometry for every loaded reflection capture, independent of the active per-pixel blend.
#pragma once
#include "enhanced/lighting/environment.h"
#include "enhanced/viewer/debug_lines.h"

namespace openrac::lighting {
// White position markers and cyan outer influence/parallax boxes from the actual asset metadata.
std::vector<DebugLineVertex> environment_probe_debug_vertices(
    std::span<const EnvironmentProbe> probes
);
}  // namespace openrac::lighting

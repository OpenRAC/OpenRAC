// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Optional bake-only sidecar adapter for inferred interior sources.

#pragma once

#include <filesystem>
#include <string>

#include "common/result.h"

namespace openrac::scene {
struct LevelData;
}

namespace openrac::lighting {
struct BakeScene;

// Applies probe-interiors.json when present and returns a JSON diagnostic report, or an empty string.
// Invalid sidecars return an error. The adapter never changes runtime lighting or source files.
[[nodiscard]] Result<std::string> apply_interior_sources(
    const scene::LevelData& level, BakeScene& scene
);
}  // namespace openrac::lighting

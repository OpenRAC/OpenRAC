// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Declarative water source decoding from retained sectioned images and per-level bake profiles.
#pragma once

#include <filesystem>

#include "common/json.h"
#include "common/result.h"

namespace openrac::lighting::water {
// Decodes bounded source tables into a native water request; reports malformed profiles or sources.
[[nodiscard]] Result<json::Value> decode_sources(const std::filesystem::path& level);
}  // namespace openrac::lighting::water

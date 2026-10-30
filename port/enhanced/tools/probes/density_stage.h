// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <filesystem>
#include <ostream>

#include "enhanced/lighting/probe_density.h"

namespace openrac::lighting {
// Check the final top-three selection as well as individual volumes. Restore fine references
// wherever changed selection violates the error bound, before reflections or registration run.
void verify_probe_density_field(
    std::span<const std::filesystem::path> paths,
    std::span<const bake_detail::ProbeDensityReceiver> receivers,
    const bake_detail::ProbeDensitySettings& settings,
    std::ostream& output
);
}  // namespace openrac::lighting

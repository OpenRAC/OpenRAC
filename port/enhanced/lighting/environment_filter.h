// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Seam-aware GGX reflection filtering and the matching split-sum BRDF integral.
#pragma once
#include "enhanced/lighting/environment.h"

namespace openrac::lighting {
// Fills retained GGX levels from six full-resolution HDR faces, omitting the sharpest source mip.
void filter_environment(
    EnvironmentAsset& asset, unsigned probe, std::span<const Float3> faces, unsigned samples
);
// Store the shared GGX/height-correlated Smith lookup below the face atlas.
void integrate_environment_brdf(EnvironmentAsset& asset, unsigned samples);
}  // namespace openrac::lighting

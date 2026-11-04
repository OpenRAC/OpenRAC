// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Headless HDR format detection for the local bake device, without a rendering context.
#pragma once
#include "enhanced/lighting/probe_texture.h"

namespace openrac::lighting {
// Mirrors the renderer's adapter preference; unavailable HDR support retains R11G11B10F.
ProbeTextureFormat detect_probe_texture_format();
}  // namespace openrac::lighting

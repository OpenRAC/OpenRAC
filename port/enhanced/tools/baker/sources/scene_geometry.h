// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Static render geometry conversion used by the probe scene exporter.

#pragma once

namespace openrac::scene {
struct LevelData;
}

namespace openrac::lighting {

// Transport scene and conversion statistics owned by the caller.
struct BakeSceneExport;

// Appends eligible render triangles in layer order; rejects malformed primitives and positions.
void append_probe_geometry(const scene::LevelData& level, BakeSceneExport& out);

}  // namespace openrac::lighting

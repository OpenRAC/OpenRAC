// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include "enhanced/lighting/room_distance_field.h"

namespace openrac::lighting::bake_detail {
struct RoomBackbone {
    std::vector<std::vector<unsigned>> skeleton;
    std::vector<unsigned> centres;
    std::vector<unsigned> entrances;
};

// One hub per open area; checked entrance routes and narrow passage/dead-end centres.
RoomBackbone room_backbone(
    RoomDistanceField& field, const Scene& scene, const BakeSettings& settings
);
// Recover continuously supported floor paths missing from the coarse source lattice.
void repair_backbone_floor_gaps(
    RoomDistanceField& field, const Scene& scene, const BakeSettings& settings
);
}  // namespace openrac::lighting::bake_detail

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#pragma once
#include <cstdint>
#include <map>
#include <vector>

#include "renderer/math.h"

namespace openrac::scene {
struct SpecialActorFrame {
    renderer::Vec3 position{};
    std::vector<renderer::Mat4> pose;
};

struct SpecialActor {
    std::uint32_t model = 0;
    float scale = 1;
    std::vector<SpecialActorFrame> frames;
};

struct SpecialClip {
    float fps = 25, duration = 0, yaw = 0;
    renderer::Mat4 upright = renderer::identity();
    renderer::Vec3 origin{};
    std::vector<SpecialActor> actors;
};
}  // namespace openrac::scene

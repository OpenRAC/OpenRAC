// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#pragma once

namespace openrac::renderer {

struct BloomSettings {
    bool enabled = true;
    float strength = 0.780f;
    float threshold = 0.78f;
    float knee = 0.27f;
    float scatter = 0.90f;
};

}  // namespace openrac::renderer

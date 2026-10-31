// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared bordered-tile addressing for both offline HDR encoders.
#pragma once
#include "enhanced/lighting/probes.h"

namespace openrac::lighting::compression {
// A probe tile includes its one-texel octahedral gutter; one GPU block always occupies 16 bytes.
using Tile = std::array<Float3, 64>;
using Block = std::array<std::uint8_t, 16>;

// Maps each gutter texel to the interior texel it must match after compression.
inline unsigned gutter_source(unsigned p) {
    unsigned x = p % 8, y = p / 8;

    if ((x == 0 || x == 7) && (y == 0 || y == 7)) {
        x = x == 0 ? 6 : 1;
        y = y == 0 ? 6 : 1;
    } else if (x == 0 || x == 7) {
        x = x == 0 ? 1 : 6;
        y = 7 - y;
    } else if (y == 0 || y == 7) {
        x = 7 - x;
        y = y == 0 ? 1 : 6;
    }

    return y * 8 + x;
}

// Squared encoded-colour distance selects the shared interpolation index.
inline float squared_colour_error(Float3 a, Float3 b) {
    float value = 0;

    for (unsigned c = 0; c < 3; ++c) {
        const float delta = a[c] - b[c];
        value += delta * delta;
    }

    return value;
}

}  // namespace openrac::lighting::compression

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/idct.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The 8x8 inverse DCT of ISO/IEC 13818-2 Annex A: separable, in float with
// the exact cosine basis, rounded to the nearest integer. It meets IEEE
// 1180-1990 (the test checks it with the standard's generator, ranges and
// limits) and is deterministic: the same coefficients always give the same
// samples.

#pragma once

#include <array>
#include <cstdint>

namespace openrac::media {

// Transforms `block` (coefficients in raster order, F[v * 8 + u]) in place
// into samples f[y * 8 + x], rounded to nearest (halves up), not clamped.
void inverse_dct(std::array<std::int32_t, 64>& block);

}  // namespace openrac::media

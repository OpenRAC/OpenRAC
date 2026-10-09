// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/texture.rs
// and crates/rc-formats/src/hud.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Decoding of PSMT8 images through their CSM1 palette.

#include "assets/world/psmt8.h"

#include <array>

namespace openrac::assets {

Rgba8Pixels decode_psmt8(const Psmt8Image& image, GsAlpha alpha) {
    const std::size_t count = std::size_t{image.width} * image.height;
    image.indices.check(0, count, "indexed image pixels");
    image.palette.check(0, 0x400, "indexed image palette");
    std::array<std::array<u8, 4>, 256> colours{};
    for (u32 i = 0; i < 256; ++i) {
        const std::size_t at = std::size_t{csm1_palette_slot(i)} * 4;
        for (std::size_t k = 0; k < 4; ++k) {
            colours[i][k] = image.palette.data()[at + k];
        }
        if (alpha == GsAlpha::Scaled) {
            colours[i][3] = scale_gs_alpha(colours[i][3]);
        }
    }
    Rgba8Pixels out{image.width, image.height, {}};
    out.rgba.reserve(count * 4);
    for (std::size_t p = 0; p < count; ++p) {
        const auto& c = colours[image.indices.data()[p]];
        out.rgba.insert(out.rgba.end(), c.begin(), c.end());
    }
    return out;
}

}  // namespace openrac::assets

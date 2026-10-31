// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Shared device-format validation before direct uploads of compressed probe assets.
#pragma once
#include <stdexcept>

#include "enhanced/lighting/probe_texture.h"
#include "renderer/gl.h"

namespace openrac::lighting {
// Returns the stored GPU format, or throws within the owning loader when a local rebake is needed.
inline gl::GLenum probe_texture_internal(ProbeTextureFormat format) {
    if (format == ProbeTextureFormat::Bc6h
        && gl::has_extension("GL_ARB_texture_compression_bptc")) {
        return gl::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
    }

    if (format == ProbeTextureFormat::Astc8x8
        && gl::has_extension("GL_KHR_texture_compression_astc_hdr")) {
        return gl::GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
    }

    if (format != ProbeTextureFormat::R11G11B10) {
        throw std::runtime_error(
            "probe texture compression is unsupported on this GPU; rebake lighting"
        );
    }

    return gl::GL_R11F_G11F_B10F;
}
}  // namespace openrac::lighting

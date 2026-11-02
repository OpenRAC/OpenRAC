// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The swapchain's format and colour space (vkgl_present.cpp's make_swapchain), chosen from what the
// surface offers. Arithmetic on what it is given only, so that tests/surface_format_test.cpp can
// pin it: an SDR choice is the one the backend always made, an HDR one falls back to the other HDR
// kind and then to SDR.

#pragma once

#include <span>
#include <volk.h>

#include "common/hdr.h"

namespace openrac::vkgl {

struct SurfaceChoice {
    VkSurfaceFormatKHR format{};
    hdr::Output output = hdr::Output::Sdr;
};

// Whether the encode pass can write `format` (and the surface's images allow storage): the HDR
// formats are only chosen when it can.
using StorageCheck = bool (*)(void* user, VkFormat format);

namespace surface_format_detail {

inline bool is_sdr(const VkSurfaceFormatKHR& f) {
    return f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
           && (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM
               || f.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32);
}

// The one format of each HDR kind the encode pass writes (GLSL's rgb10_a2 and rgba16f).
inline bool is(const VkSurfaceFormatKHR& f, hdr::Output output) {
    using hdr::Output;
    switch (output) {
        case Output::Hdr10:
            return f.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT
                   && f.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case Output::ScRgb:
            return f.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT
                   && f.format == VK_FORMAT_R16G16B16A16_SFLOAT;
        case Output::Sdr:
        case Output::Count:
            break;
    }
    return false;
}

}  // namespace surface_format_detail

// The format for `wanted` among `formats` (not empty). SDR: the first linear 8- or 10-bit format
// in the sRGB colour space, else the first offered, the picture being in the display's own (gamma)
// space already, as on the GS. HDR: that kind, else the other HDR kind, where `storage` says the
// encode pass can write it; else SDR.
inline SurfaceChoice choose_surface_format(
    std::span<const VkSurfaceFormatKHR> formats,
    hdr::Output wanted,
    StorageCheck storage,
    void* user
) {
    using hdr::Output;
    namespace d = surface_format_detail;
    if (wanted != Output::Sdr) {
        const Output other = wanted == Output::Hdr10 ? Output::ScRgb : Output::Hdr10;
        for (const Output kind : {wanted, other}) {
            for (const VkSurfaceFormatKHR& f : formats) {
                if (d::is(f, kind) && storage != nullptr && storage(user, f.format)) {
                    return {f, kind};
                }
            }
        }
    }
    for (const VkSurfaceFormatKHR& f : formats) {
        if (d::is_sdr(f)) {
            return {f, Output::Sdr};
        }
    }
    // Nothing known: the first SDR colour space offered, else the first format at all.
    for (const VkSurfaceFormatKHR& f : formats) {
        if (f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return {f, Output::Sdr};
        }
    }
    return {formats.front(), Output::Sdr};
}

// Whether the surface offers either HDR kind at all (for the settings' "not offered" line).
inline bool offers_hdr(std::span<const VkSurfaceFormatKHR> formats) {
    namespace d = surface_format_detail;
    for (const VkSurfaceFormatKHR& f : formats) {
        if (d::is(f, hdr::Output::Hdr10) || d::is(f, hdr::Output::ScRgb)) {
            return true;
        }
    }
    return false;
}

}  // namespace openrac::vkgl

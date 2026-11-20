// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's GL (renderer/gl/gl.h, renderer/world/gl_world.h) carried out with Metal, as
// renderer/vulkan/vkgl.h carries it out with Vulkan: chosen with OPENRAC_GL_API=metal.
//
// The GL model kept: rows are counted from the bottom as GL counts them. A texture's row 0 is GL's
// row 0, the shaders flip y (shader_translate.h) so that a draw lands on the rows GL would put it
// on, and the default framebuffer is a texture of the backend's that present() copies to the
// screen upside down, the right way up there. GL's objects are Metal objects held by reference:
// the command buffers keep what they use alive, so deleting a GL object never waits for the GPU.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/hdr.h"

struct SDL_Window;

namespace openrac::mtlgl {

struct Config {
    // The window to present to (made with SDL_WINDOW_METAL), or null: drawing without a screen,
    // for the tests (glReadPixels reads the default framebuffer).
    SDL_Window* window = nullptr;
    int width = 0;  // the default framebuffer without a window
    int height = 0;
    bool vsync = true;
};

// Opens the device and the context. False, with why in `error`, when Metal cannot be used: the
// caller falls back to another API.
bool open(const Config& config, std::string& error);
void close();
bool active();

// renderer/gl/gl.h's loader: the GL functions by name, null for one this does not carry out.
using Proc = void (*)();
Proc get_proc(const char* name);

// Ends the frame: the default framebuffer to the screen, the frame's commands submitted.
void present();
void set_swap_interval(int interval);

// The GPU's name, for the log.
std::string device_name();

// ---- HDR output (common/hdr.h, docs/port/HDR.md) ----
//
// macOS's EDR: the layer takes half floats in linear extended sRGB, 1.0 being the system's SDR
// white, and present() writes the default framebuffer's picture into it with the Vulkan backend's
// encode pass (renderer/vulkan/shaders/hdr_encode.comp, translated to MSL), so both renderers send
// the same values. In SDR the layer and the present are as they always were.

// How the window's CAMetalLayer is set up to show an output.
struct LayerSetup {
    bool extended_range = false;  // wantsExtendedDynamicRangeContent
    bool half_float = false;      // MTLPixelFormatRGBA16Float; else MTLPixelFormatBGRA8Unorm
    bool linear_srgb = false;     // kCGColorSpaceExtendedLinearSRGB; else the layer's own
    bool shader_writes = false;   // framebufferOnly off, so the encode pass writes the drawable

    bool operator==(const LayerSetup&) const = default;
};

// What the screen is sent when `wanted` is asked for on a display that offers EDR (`edr`: its
// headroom over SDR white is above 1) or not: scRGB for any HDR, since EDR is linear; else SDR.
hdr::Output shown_output(hdr::Output wanted, bool edr);

// The layer for `shown`.
LayerSetup layer_setup(hdr::Output shown);

// Shows `wanted` at `levels` from the next present on (see shown_output). The layer changes at
// once, so HDR may be turned on and off at any time. Falls back to SDR when the encode pass cannot
// be built.
void set_hdr(hdr::Output wanted, const hdr::Levels& levels, bool edr);

// What the screen is sent now, and whether the display offered EDR at the last set_hdr.
hdr::Output hdr_output();
bool hdr_offered();

// The window's layer as it is set up now; the defaults without a window.
LayerSetup current_layer();

// For the tests: the encode pass run on the default framebuffer into a `width` x `height` RGBA16F
// texture standing in for the layer's drawable, at `levels`, read back as stored: four half floats
// a pixel, the top row first (vkgl::test_hdr_encode's scRGB form). False without a context.
bool test_hdr_encode(
    const hdr::Levels& levels, int width, int height, std::vector<std::uint8_t>& out
);

}  // namespace openrac::mtlgl

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's GL (renderer/gl.h, renderer/world/gl_world.h) carried out with Metal, as
// renderer/vulkan/vkgl.h carries it out with Vulkan: chosen with OPENRAC_GL_API=metal.
//
// The GL model kept: rows are counted from the bottom as GL counts them. A texture's row 0 is GL's
// row 0, the shaders flip y (shader_translate.h) so that a draw lands on the rows GL would put it
// on, and the default framebuffer is a texture of the backend's that present() copies to the
// screen upside down, the right way up there. GL's objects are Metal objects held by reference:
// the command buffers keep what they use alive, so deleting a GL object never waits for the GPU.

#pragma once

#include <string>

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

// renderer/gl.h's loader: the GL functions by name, null for one this does not carry out.
using Proc = void (*)();
Proc get_proc(const char* name);

// Ends the frame: the default framebuffer to the screen, the frame's commands submitted.
void present();
void set_swap_interval(int interval);

// The GPU's name, for the log.
std::string device_name();

}  // namespace openrac::mtlgl

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Records Dear ImGui draw lists as RenderGraph passes for the debug overlay and the Android drawer.
#pragma once

#include <string>

struct ImDrawData;

namespace openrac::renderer::ui::imgui_gl {

// Before the first frame (the context is current): the program, the buffers, the font's texture.
bool init(std::string& error);

// The font's texture again, after the overlay rebuilt ImGui's font atlas (a new scale).
void reload_font();

// Draws the frame's lists into the default framebuffer with the graphics context current.
void render(const ImDrawData* data);

// Releases the graphics objects before the context is destroyed.
void shutdown();

}  // namespace openrac::renderer::ui::imgui_gl

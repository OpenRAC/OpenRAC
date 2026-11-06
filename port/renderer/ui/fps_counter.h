// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frame-rate counter (the player's `show_fps` setting): how many frames a second the window
// shows and the whole time between presentations, in the picture's top left corner. Counted over the frames
// actually shown, so it tells a frame cap, a display's refresh and a slow frame apart from the
// game's own tick rate, which never changes.
// F2 also offers separate CPU work and measured GPU-pass times; see docs/port/SETTINGS.md.
#pragma once

#include <chrono>
#include <cstdint>

namespace openrac::renderer::ui::fps_counter {

// Player-facing display modes, passed by the host without depending on its settings module.
enum class Mode {
    Off,
    Basic,
    Timings,
    Count
};

// Records a completed presentation; `swap_seconds` is the duration of its swap call.
// Called even when hidden; CPU time includes all intervening work and waits except swap.
// Detailed mode consumes completed profiler frames without waiting for GPU results.
void frame_drawn(std::chrono::steady_clock::time_point now, double swap_seconds, Mode mode);

// Draws the counter over a framebuffer of one sample a pixel, width x height, its glyphs in `ink`
// over a box of `paper` (RGBA, red in the low byte: the host's game accent and theme). The
// context is current.
void draw(unsigned framebuffer, int width, int height, std::uint32_t ink, std::uint32_t paper);

// Frees its GL objects (the context is still current).
void release();

}  // namespace openrac::renderer::ui::fps_counter

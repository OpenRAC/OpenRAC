// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The host's side of game_host.h: what the game's C and the library replacements call in the
// program that runs them (a crash, a warning, each frame's vertical blank, the display list, the
// images it uploads and reads back, what its renderers drew, the movies, the pad). Every game's
// program has these, with a window or without: without one (no OPENRAC_FRONTEND) a frame is only
// paced and what the window would draw is dropped.
//
// The state these share with the rest of the program (the frame count, the display list sent, the
// window's presence) is main.cpp's, through program/program.h. The two long bodies, the vertical
// blank's and the movie player's, are program/vsync.cpp's and program/movie.cpp's (program::vsync,
// program::play_movie); the entry points here call them.
//
// The definitions sit inside openrac::program so that they name the port's modules as the rest of
// the program does. A function with C linkage is one function whatever namespace declares it, so
// each is the one game_host.h declares, with the same symbol.
//
// Each entry point runs its body through the C-boundary wrapper (common/c_boundary.h, conventions
// section 13.4): an exception stops there, is written to the log under the entry point's name, and
// the game gets the value game_host.h gives for no window (0, -1 for openrac_game_ui_quad, 1 for
// openrac_game_picture_wider), 0 from the pad (none connected), and 0 from openrac_game_vsync,
// for which game_host.h says none. The game's C goes on as it would without the window's answer.
// openrac_game_crash ends the program even then.

#include "openrac/game_host.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include "common/c_boundary.h"
#include "common/crash.h"
#include "common/log.h"
#include "mods.h"
#include "openrac/guest.h"
#include "openrac/memory.h"
#include "program/program.h"

#ifdef OPENRAC_FRONTEND
#include "debug/overlay.h"
#include "frontend.h"
#include "options_menu.h"
#include "renderer/direct/direct.h"
#include "renderer/textures/texture.h"
#include "scene/moby_pose.h"
#include "settings.h"
#endif

#ifndef OPENRAC_FRONTEND
// A headless game has no enhanced simulation to replace the original water.
int openrac_game_enhanced_water(void) {
    return 0;
}
#endif

namespace openrac::program {
namespace {

#ifdef OPENRAC_FRONTEND
// [verbose log] The first pad's buttons each time they change, and its sticks when one moves far.
// Compiled with the window only: without one, no pad is read.
void log_pad(int port, const uint16_t* buttons, const uint8_t analog[4]) {
    static uint16_t last = 0xFFFF;
    static uint8_t last_analog[4] = {0x80, 0x80, 0x80, 0x80};

    if (port != 0 || !log::verbose_on()) {
        return;
    }

    bool moved = false;

    for (int i = 0; i < 4; ++i) {
        moved = moved || std::abs(int(analog[i]) - int(last_analog[i])) >= 0x30;
    }

    if (*buttons == last && !moved) {
        return;
    }

    last = *buttons;
    std::memcpy(last_analog, analog, 4);
    log::verbose(
        "frame {}: pad 0 buttons {:#06x} (active low), sticks {} {} {} {}",
        g_frame,
        *buttons,
        analog[0],
        analog[1],
        analog[2],
        analog[3]
    );
}
#endif

// The pad's report as the pad library builds it (lib/pad.c's scePad2Read: buttons, the sticks,
// twelve pressures), through the mods' callbacks, back into buttons and sticks: for the port's own
// readers of the pad (the movie player), so a mod's keys skip a movie as the pad's Start does.
const unsigned kPressureBit[12] = {5, 7, 4, 6, 12, 13, 14, 15, 10, 11, 8, 9};

void report_from(uint16_t buttons, const uint8_t analog[4], uint8_t out[18]) {
    out[0] = static_cast<uint8_t>(buttons);
    out[1] = static_cast<uint8_t>(buttons >> 8);
    std::memcpy(out + 2, analog, 4);

    for (int n = 0; n < 12; ++n) {
        out[6 + n] = (buttons >> kPressureBit[n]) & 1 ? 0 : 0xFF;
    }
}

}  // namespace

extern "C" {

void openrac_game_crash(const char* reason) {
    // Reported to the log, the wrapper's default, never back to this function, so that a throw
    // while the report is written cannot recurse; the program ends whatever happened.
    c_boundary::call("openrac_game_crash", [&] {
        log::error("{}", reason);
        crash::report(reason, crash::Kind::Trap);
    });
    std::_Exit(3);
}

void openrac_game_warn(const char* message) {
    c_boundary::call("openrac_game_warn", [&] { log::warn("{}", message); });
}

void openrac_game_moby_colour(gaddr object, unsigned colour) {
    c_boundary::call("openrac_game_moby_colour", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            frontend::moby_colour(object, colour, openrac_game_loaded_overlay());
        }
#else
        (void)object;
        (void)colour;
#endif
    });
}

// hostgen namespaces external calls from translated game C with game_.
extern "C" void game_openrac_game_moby_colour(unsigned object, unsigned colour) {
    c_boundary::call("game_openrac_game_moby_colour", [&] {
        openrac_game_moby_colour(object, colour);
    });
}

// The options a game keeps itself are the settings file's (settings::game_option). A program
// without a window has no settings file (settings.cpp is the window's): it remembers none, and
// the game gets `fallback`.
int openrac_game_option(const char* key, int fallback) {
    return c_boundary::call("openrac_game_option", fallback, [&] {
#ifdef OPENRAC_FRONTEND
        return settings::game_option(key, fallback);
#else
        (void)key;
        return fallback;
#endif
    });
}

void openrac_game_set_option(const char* key, int value) {
    c_boundary::call("openrac_game_set_option", [&] {
#ifdef OPENRAC_FRONTEND
        settings::set_game_option(key, value);
#else
        (void)key;
        (void)value;
#endif
    });
}

void openrac_game_draw(unsigned layers) {
    c_boundary::call("openrac_game_draw", [&] {
        g_draws |= layers;
        g_reports_draws = true;
#ifdef OPENRAC_FRONTEND
        // The world's terrain: the camera it is drawn with is the frame's world camera.
        if (g_window && (layers & OPENRAC_DRAW_TERRAIN) != 0) {
            const auto* ram = runtime::Memory::get().base();
            frontend::world_drawn(
                std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size())
            );
        }
#endif
    });
}

void openrac_game_world_view(void) {
    c_boundary::call("openrac_game_world_view", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            const auto* ram = runtime::Memory::get().base();
            frontend::world_drawn(
                std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size())
            );
        }
#endif
    });
}

void openrac_game_world_under_menu(void) {
    c_boundary::call("openrac_game_world_under_menu", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            frontend::world_under_menu();
        }
#endif
    });
}

int openrac_game_vsync(void) {
    return c_boundary::call("openrac_game_vsync", 0, [&]() -> int { return vsync(); });
}

void openrac_game_tick_boundary(void) {
    c_boundary::call("openrac_game_tick_boundary", [&] {
        mods::tick_boundary(static_cast<std::uint32_t>(g_frame));
    });
}

void openrac_game_dma_send(gaddr channel, gaddr tag) {
    c_boundary::call("openrac_game_dma_send", [&] {
        // A frame sent to VIF1 with no vertical blank waited for since the last one: on the console
        // the loop is paced by the DMA and the interrupts (the title loop never calls sceGsSyncV), so
        // the frame ends here, with the vertical blank's handlers, as if the hardware had done it.
        if (channel == 0x10009000u) {
            if (!g_vsync_since_kick) {
                openrac_game_vsync();
                openrac_game_run_vsync_handlers();
                openrac_game_tick_boundary();
            }

            // The frame's list. What goes to VIF0 (Going Commando's VU0 programs, which it sends in
            // the middle of a frame) is not one.
            g_chain = tag;
            g_vsync_since_kick = false;
        }

        log::debug("frame {}: DMA chain at {:#010x} to channel {:#010x}", g_frame, tag, channel);
    });
}

void openrac_game_set_display(const openrac_game_display* d) {
    c_boundary::call("openrac_game_set_display", [&] {
        static openrac_game_display last{};

        if (std::memcmp(&last, d, sizeof(last)) != 0) {
            last = *d;
            log::info(
                "display: {}x{} at page {} (width {}), format {:#x}, at {},{}",
                d->width,
                d->height,
                d->frame_base,
                d->frame_width,
                d->psm,
                d->x,
                d->y
            );
            // The draw buffer follows the display buffer: a 512-line display (PAL) puts it at block
            // 0x1000, 448 lines tall, the 448-line one of a 60 Hz mode at 0xE00, 416 lines tall. Missed,
            // the game's copy of the draw buffer into the display was drawn from an empty texture over
            // the picture (the PAL game in its 60 Hz mode showed its menus over black), and the frame
            // the pause menu reads back came out 32 lines short (a black band at the bottom).
#ifdef OPENRAC_FRONTEND
            // Display changes replace globals read by the pending snapshot, so drain it first.
            if (d->frame_width > 0 && d->height > 0 && frontend::finish_render()) {
                const std::uint32_t named = frontend::draw_buffer_block();
                renderer::set_draw_buffer_block(
                    named != 0 ? named
                               : static_cast<std::uint32_t>(d->frame_base) * 32
                                     + static_cast<std::uint32_t>(d->height * d->frame_width),
                    d->height >= 512 ? 448 : 416
                );
            }
#endif
        }
    });
}

// The video mode the game asks the console for (sceGsResetGraph, SetGsCrt): 2 is NTSC, 60 fields a
// second, 3 is PAL, 50. The game's tick is one field: a PAL game that changes to its 60 Hz mode
// counts its time in sixtieths from then on (rac1/pal: func_002157C0 sets the time base when a
// level starts). The port keeps the PAL game in that mode and ticks every game 60 times a second.
void openrac_game_set_video_mode(int interlace, int mode, int field_mode) {
    c_boundary::call("openrac_game_set_video_mode", [&] {
        // Logged only: the port paces every game at 60 Hz whatever the mode (tick_rate()).
        log::info(
            "video mode {:#x} (interlace {}, field mode {}){}",
            mode,
            interlace,
            field_mode,
            mode == 3 ? " (PAL 50 Hz: the port keeps 60)" : ""
        );
    });
}

void openrac_game_load_image(const openrac_game_image* image) {
    c_boundary::call("openrac_game_load_image", [&] {
        log::debug(
            "texture upload {}x{} format {:#x} to block {}",
            image->width,
            image->height,
            image->psm,
            image->base
        );
#ifdef OPENRAC_FRONTEND
        if (g_window && image->width > 0 && image->height > 0) {
            const std::size_t bytes = renderer::image_bytes(
                static_cast<std::uint8_t>(image->psm), image->width, image->height
            );
            frontend::upload_image(
                static_cast<std::uint32_t>(image->base),
                static_cast<std::uint32_t>(image->width_units),
                static_cast<std::uint8_t>(image->psm),
                static_cast<std::uint32_t>(image->x),
                static_cast<std::uint32_t>(image->y),
                static_cast<std::uint32_t>(image->width),
                static_cast<std::uint32_t>(image->height),
                std::span<const std::uint8_t>(
                    static_cast<const std::uint8_t*>(G(image->pixels)), bytes
                )
            );
        }
#endif
    });
}

void openrac_game_store_image(const openrac_game_image* image) {
    c_boundary::call("openrac_game_store_image", [&] {
        if (image->width <= 0 || image->height <= 0) {
            return;
        }

        const std::size_t bytes =
            static_cast<std::size_t>(image->width) * static_cast<std::size_t>(image->height) * 4;
        auto* out = static_cast<std::uint8_t*>(G(image->pixels));
#ifdef OPENRAC_FRONTEND
        if (g_window
            && frontend::store_image(
                static_cast<std::uint32_t>(image->base),
                static_cast<std::uint32_t>(image->width_units),
                static_cast<std::uint8_t>(image->psm),
                image->x,
                image->y,
                image->width,
                image->height,
                std::span<std::uint8_t>(out, bytes)
            )) {
            return;
        }
#endif
        // Nothing to answer with: black, as an empty frame buffer reads.
        if (image->psm == 0) {
            std::memset(out, 0, bytes);
        }
    });
}

int openrac_game_play_movie(uint32_t lsn, uint32_t bytes, int channel, int start_skips) {
    return c_boundary::call("openrac_game_play_movie", 0, [&]() -> int {
        return play_movie(lsn, bytes, channel, start_skips);
    });
}

int openrac_game_mobys_drawn(gaddr first, int count) {
    return c_boundary::call("openrac_game_mobys_drawn", 0, [&]() -> int {
#ifdef OPENRAC_FRONTEND
        if (g_window && count > 0) {
            const auto* ram = runtime::Memory::get().base();
            frontend::mobys_drawn(
                std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size()), first, count
            );
            // The console's arena keeps the retail limit (its marker held a byte); an expanded one
            // a halfword.
            const bool expanded = runtime::Memory::get().ram_size() > GUEST_RAM_SIZE;
            return count <= (expanded ? 0xFFFF : 0xFF) ? 1 : 0;
        }
#else
        (void)first;
        (void)count;
#endif
        return 0;
    });
}

void openrac_game_effect_quad(const openrac_game_quad* quad) {
    c_boundary::call("openrac_game_effect_quad", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            const auto* ram = runtime::Memory::get().base();
            frontend::effect_quad(
                std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size()), *quad
            );
        }
#else
        (void)quad;
#endif
    });
}

void openrac_game_water_explosion(const float position[3], float radius, float strength) {
    c_boundary::call("openrac_game_water_explosion", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window)
            frontend::water_explosion(position, radius, strength);
#else
        (void)position; (void)radius; (void)strength;
#endif
    });
}

void openrac_game_sky_shell(int shell, const float* matrix) {
    c_boundary::call("openrac_game_sky_shell", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            frontend::sky_shell(shell, matrix);
        }
#else
        (void)shell;
        (void)matrix;
#endif
    });
}

int openrac_game_hud_texts(openrac_game_hud_text* out, int max) {
    return c_boundary::call("openrac_game_hud_texts", 0, [&]() -> int {
        const std::vector<openrac_hud_text> lines = mods::hud_texts();
        const int n = static_cast<int>(lines.size());
        for (int i = 0; out != nullptr && i < n && i < max; ++i) {
            const openrac_hud_text& l = lines[static_cast<std::size_t>(i)];
            openrac_game_hud_text& o = out[i];
            std::memcpy(o.text, l.text, sizeof o.text);
            o.text[sizeof o.text - 1] = '\0';
            o.x = l.x;
            o.y = l.y;
            o.scale = l.scale;
            o.rgba = static_cast<std::uint32_t>(l.rgba[0])
                     | static_cast<std::uint32_t>(l.rgba[1]) << 8
                     | static_cast<std::uint32_t>(l.rgba[2]) << 16
                     | static_cast<std::uint32_t>(l.rgba[3]) << 24;
            o.flags = l.flags;
        }
        return n;
    });
}

int openrac_game_hud_side(void) {
    return c_boundary::call("openrac_game_hud_side", 0, [&]() -> int {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::hud_side();
        }
#endif
        return 0;
    });
}

uint32_t openrac_game_effect_quad_marker(void) {
    return c_boundary::call("openrac_game_effect_quad_marker", 0u, [&]() -> std::uint32_t {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::effect_quad_marker();
        }
#endif
        return 0u;
    });
}

float openrac_game_active_distance(void) {
    return c_boundary::call("openrac_game_active_distance", 1.0f, [&]() -> float {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return settings::get().active_distance_scale();
        }
#endif
        return 1.0f;
    });
}

float openrac_game_draw_distance(void) {
    return c_boundary::call("openrac_game_draw_distance", 1.0f, [&]() -> float {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return settings::get().draw_distance_scale();
        }
#endif
        return 1.0f;
    });
}

float openrac_game_view_opened(void) {
    return c_boundary::call("openrac_game_view_opened", 1.0f, [&]() -> float {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::view_opened();
        }
#endif
        return 1.0f;
    });
}

float openrac_game_picture_wider(void) {
    return c_boundary::call("openrac_game_picture_wider", 1.0f, [&]() -> float {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::picture_wider();
        }
#endif
        return 1.0f;
    });
}

int openrac_game_ui_quad(const openrac_game_ui_rect* quad) {
    return c_boundary::call("openrac_game_ui_quad", -1, [&]() -> int {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::ui_quad(*quad);
        }
#else
        (void)quad;
#endif
        return -1;
    });
}

void openrac_game_particle(
    const uint8_t* record, uint32_t pixels, uint32_t clut, int log2_side, uint32_t id
) {
    c_boundary::call("openrac_game_particle", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            const auto* ram = runtime::Memory::get().base();
            frontend::particle(
                std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size()),
                record,
                pixels,
                clut,
                log2_side,
                id
            );
        }
#else
        (void)record;
        (void)pixels;
        (void)clut;
        (void)log2_side;
        (void)id;
#endif
    });
}

void openrac_game_sky_sprite(const uint8_t* record, uint64_t tex0) {
    c_boundary::call("openrac_game_sky_sprite", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            frontend::sky_sprite(record, tex0);
        }
#else
        (void)record;
        (void)tex0;
#endif
    });
}

void openrac_game_moby_chain(gaddr moby, gaddr marks) {
    c_boundary::call("openrac_game_moby_chain", [&] {
#ifdef OPENRAC_FRONTEND
        const auto* ram = runtime::Memory::get().base();
        const auto pose = scene::moby_pose_matrices(
            std::span<const std::uint8_t>(ram, runtime::Memory::get().ram_size()), moby
        );
        const auto* mark = static_cast<const std::uint8_t*>(G(marks));
        const std::size_t count = std::min<std::size_t>(mark[0x7F], pose.size());

        for (std::size_t j = 0; j < count; ++j) {
            if (mark[j] != 0) {
                std::memcpy(G(0x70000000u + static_cast<gaddr>(j) * 0x40), pose[j].data(), 0x40);
            }
        }
#else
        (void)moby;
        (void)marks;
#endif
    });
}

void openrac_game_moby_local_pose(gaddr moby) {
    c_boundary::call("openrac_game_moby_local_pose", [&] {
#ifdef OPENRAC_FRONTEND
        const auto mods = scene::extra_joint_modifiers(moby);
        if (mods.empty()) {
            return;
        }
        const gaddr cls = *static_cast<const gaddr*>(G(moby + 0x24));
        if (cls == 0) {
            return;
        }
        const unsigned joints = *static_cast<const std::uint8_t*>(G(cls + 0x08));
        // As moby_animation.cpp's evaluate_keys applies a mode 0 modifier: q * rotation, the scale
        // multiplied, the translation added.
        for (const auto& m : mods) {
            if (m.joint >= joints || m.joint >= 0x7F || m.mode != 0) {
                continue;
            }
            auto* rec = static_cast<float*>(G(0x70000000u + static_cast<gaddr>(m.joint) * 0x40));
            const float q[4] = {rec[0], rec[1], rec[2], rec[3]};
            const auto& r = m.quat;
            rec[0] = q[3] * r[0] + r[3] * q[0] + (q[1] * r[2] - q[2] * r[1]);
            rec[1] = q[3] * r[1] + r[3] * q[1] + (q[2] * r[0] - q[0] * r[2]);
            rec[2] = q[3] * r[2] + r[3] * q[2] + (q[0] * r[1] - q[1] * r[0]);
            rec[3] = q[3] * r[3] - (q[0] * r[0] + q[1] * r[1] + q[2] * r[2]);
            for (unsigned k = 0; k < 3; ++k) {
                rec[4 + k] *= m.scale[k];
                rec[8 + k] += m.trans[k];
            }
        }
#else
        (void)moby;
#endif
    });
}

gaddr openrac_game_text(int id) {
    return c_boundary::call("openrac_game_text", 0, [&]() -> gaddr {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::text(id);
        }
#endif
        (void)id;
        return 0;
    });
}

namespace {

// TEXT into the game's memory at OUT, cut to SIZE bytes with its end.
void put_option_text(const std::string& text, char* out, int size) {
    if (out != nullptr && size > 0) {
        std::snprintf(out, static_cast<std::size_t>(size), "%s", text.c_str());
    }
}

}  // namespace

int openrac_game_option_rows(void) {
    return c_boundary::call("openrac_game_option_rows", 0, [&]() -> int {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            return frontend::options_menu::row_count();
        }
#endif
        return 0;
    });
}

void openrac_game_option_row_text(int row, char* label, char* value, int size) {
    c_boundary::call("openrac_game_option_row_text", [&] {
        put_option_text("", label, size);
        put_option_text("", value, size);
#ifdef OPENRAC_FRONTEND
        if (g_window && row >= 0 && row < frontend::options_menu::row_count()) {
            put_option_text(
                frontend::options_menu::shown(frontend::options_menu::label(row)), label, size
            );
            put_option_text(
                frontend::options_menu::shown(frontend::options_menu::value(row)), value, size
            );
        }
#endif
        (void)row;
    });
}

void openrac_game_option_step(int row, int by) {
    c_boundary::call("openrac_game_option_step", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window && row >= 0 && row < frontend::options_menu::row_count()) {
            frontend::debug_overlay::change_setting(row, by);
        }
#endif
        (void)row;
        (void)by;
    });
}

void openrac_game_option_word(const char* english, char* out, int size) {
    c_boundary::call("openrac_game_option_word", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            put_option_text(frontend::options_menu::shown(english), out, size);
            return;
        }
#endif
        put_option_text(english, out, size);
    });
}

int openrac_game_pad_through_mods(int port, uint16_t* buttons, uint8_t analog[4]) {
    return c_boundary::call("openrac_game_pad_through_mods", 0, [&]() -> int {
        const int connected = openrac_game_pad(port, buttons, analog);

        if (openrac_game_pad_report_hook != nullptr) {
            uint8_t report[18];
            report_from(*buttons, analog, report);
            openrac_game_pad_report_hook(port, report, 18);
            openrac_game_pad_after_mods(port, report, 18);
            *buttons = static_cast<uint16_t>(report[0] | (report[1] << 8));
            std::memcpy(analog, report + 2, 4);
        }

        return connected;
    });
}

void openrac_game_pad_after_mods(int port, uint8_t* report, int size) {
    c_boundary::call("openrac_game_pad_after_mods", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window && size >= 2) {
            uint16_t buttons = static_cast<uint16_t>(report[0] | (report[1] << 8));
            const uint16_t before = buttons;
            frontend::skip_after_mods(port, buttons);
            const uint16_t skipped = buttons;
            frontend::menus_after_mods(port, buttons);
            report[0] = static_cast<uint8_t>(buttons);
            report[1] = static_cast<uint8_t>(buttons >> 8);

            // What the skip pressed is pressed hard, as the pad library gives a pressed button;
            // what a menu took is let go.
            for (int n = 0; n < 12 && 6 + n < size; ++n) {
                if (((before & ~skipped) >> kPressureBit[n]) & 1) {
                    report[6 + n] = 0xFF;
                }
                if (((buttons & ~skipped) >> kPressureBit[n]) & 1) {
                    report[6 + n] = 0;
                }
            }
        }
#endif
        (void)port;
        (void)report;
        (void)size;
    });
}

void openrac_game_set_motors(int port, int small, int large) {
    c_boundary::call("openrac_game_set_motors", [&] {
#ifdef OPENRAC_FRONTEND
        if (g_window) {
            frontend::set_motors(
                port, small != 0, static_cast<std::uint8_t>(std::clamp(large, 0, 255))
            );
        }
#endif
        (void)port;
        (void)small;
        (void)large;
    });
}

int openrac_game_pad(int port, uint16_t* buttons, uint8_t analog[4]) {
    return c_boundary::call("openrac_game_pad", 0, [&]() -> int {
#ifdef OPENRAC_FRONTEND
        if (g_window && frontend::pad(port, buttons, analog)) {
            log_pad(port, buttons, analog);
            return 1;
        }
#endif
        (void)port;
        *buttons = 0xFFFF;  // nothing pressed (active low)
        std::memset(analog, 0x80, 4);
        return 0;
    });
}

}  // extern "C"

}  // namespace openrac::program

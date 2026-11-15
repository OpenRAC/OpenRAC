// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The window's own side of the frontend, a slice of frontend/state.h: the player's display settings
// applied to the window (apply_settings), and the window's events and keys, which the game never
// sees (window_key).
//
// The window's opening (open), its icon and the Linux desktop entries are in frontend.cpp: they
// read the icon's bytes through an extern "C" declaration, which a new C++ file may only hold when
// it is named for a C contract (CODING_CONVENTIONS.md, section 14).

#include <cmath>
#include <format>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "common/log.h"
#include "debug_overlay.h"
#include "frontend.h"
#include "game/common/frontend/state.h"
#include "renderer/gl/gl.h"
#include "settings.h"

namespace openrac::frontend {

namespace {

// A window the player sizes: never on Android, where the game covers the screen whatever the
// settings say (Window::set_display_mode).
bool windowed() {
#if defined(__ANDROID__)
    return false;
#else
    return g->display_mode == settings::DisplayMode::Windowed;
#endif
}

// The window's own events, which the game never sees: the window moved to another display or its
// display changed mode (the refresh rate is read again), it was sized (the size is kept for the
// next start), it lost or got the keyboard (the player's "mute in the background").
void window_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
        case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            pacing.interval_stale = true;
            if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED) {
                apply_hdr();
            }
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            // Remembered in memory; written with the next setting saved and when the window closes.
            if (windowed() && event.window.data1 >= 320 && event.window.data2 >= 240) {
                settings::get().window_width = event.window.data1;
                settings::get().window_height = event.window.data2;
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            g->muted = event.type == SDL_EVENT_WINDOW_FOCUS_LOST;
            break;
        // The display's HDR turned on or off, or another display: HDR output follows (On).
        case SDL_EVENT_WINDOW_HDR_STATE_CHANGED:
            apply_hdr();
            break;
        default:
            break;
    }
    const bool muted = g->muted && settings::get().mute_unfocused;
    settings::set_muted(muted);
}

}  // namespace

// The window as the player's settings want it: its display mode and display, its
// shape, and each swap waiting for a refresh or not. Called when they may have changed; all of
// them take effect at once (the frame's size, its samples and the textures' filter follow the
// settings by themselves, in draw_kept).
void apply_settings() {
    const settings::Settings& s = settings::get();
    if (s.display_mode != g->display_mode || s.monitor != g->monitor) {
        g->window->set_display_mode(window_mode(s.display_mode), s.monitor);
        g->display_mode = s.display_mode;
        g->monitor = s.monitor;
        pacing.interval_stale = true;
        // A change of mode may leave the driver's swap interval at its default: asked again below.
        g->swap_asked = wanted_swap() == platform::SwapInterval::Immediate
                            ? platform::SwapInterval::VSync
                            : platform::SwapInterval::Immediate;
    }
    if (s.aspect != g->aspect || s.aspect_custom_x != g->aspect_custom_x
        || s.aspect_custom_y != g->aspect_custom_y) {
        g->aspect = s.aspect;
        g->aspect_custom_x = s.aspect_custom_x;
        g->aspect_custom_y = s.aspect_custom_y;
        // A window takes the picture's new shape, at the height it has.
        int w = 0;
        int h = 0;
        if (windowed() && s.aspect != settings::Aspect::Window
            && SDL_GetWindowSize(g->window->sdl(), &w, &h) && h > 0) {
            SDL_SetWindowSize(
                g->window->sdl(),
                static_cast<int>(std::lround(static_cast<float>(h) * picture_aspect(w, h))),
                h
            );
        }
    }
    if (wanted_swap() != g->swap_asked) {
        g->swap_asked = wanted_swap();
        g->swap = g->window->set_swap_interval(g->swap_asked);
        pacing.last_swap = {};
    }
    g->window->set_low_latency(s.low_latency_frames);
    apply_hdr();
}

void apply_hdr() {
    const settings::Settings& s = settings::get();
    const hdr::Display display = g->window->hdr_display();
    g->hdr_display = display;
    // macOS's EDR is linear (scRGB); HDR10 elsewhere, where it costs half the bandwidth.
#if defined(__APPLE__)
    constexpr bool prefer_scrgb = true;
#else
    constexpr bool prefer_scrgb = false;
#endif
    const hdr::Output wanted = hdr::wanted_output(s.hdr, display, prefer_scrgb);
    hdr::Levels levels = hdr::resolve_levels(s.hdr_paper_white, s.hdr_peak, display);
    levels.look =
        hdr::look_from_percent(s.hdr_highlights, s.hdr_sdr_hue, s.hdr_sdr_chroma, s.hdr_blowout);
    // Said once: HDR asked for where it cannot be had.
    static bool said = false;
    if (s.hdr != hdr::Mode::Off && !said) {
        if (g->window->gl_api() != platform::GlApi::Vulkan) {
            log::info("HDR output needs the Vulkan renderer; the picture stays SDR");
            said = true;
        } else if (!g->hdr_at_start) {
            log::info("HDR output was off at start; it takes effect at the next start");
            said = true;
        }
    }
    g->window->set_hdr(wanted, levels);
    // In the log when it changes (settings are put into effect again on every change).
    static std::string last;
    if (std::string status = hdr_status(); s.hdr != hdr::Mode::Off && status != last) {
        log::info("HDR: {}", status);
        last = std::move(status);
    }
}

std::string hdr_status() {
    const hdr::Display& d = g->hdr_display;
    std::string screen;
    if (!d.known) {
        screen = "the system does not say whether the display shows HDR";
    } else if (d.relative) {
        screen = std::format("the display gives {:.1f}x SDR white for HDR", d.headroom);
    } else if (d.hdr_enabled) {
        screen = d.peak_nits > 0.0f
                     ? std::format("the display shows HDR10, peak {:.0f} nits", d.peak_nits)
                     : std::string("the display shows HDR10");
    } else {
        screen = "the display shows no HDR";
    }
    const hdr::Output shown = g->window->hdr_output();
    std::string sent = shown == hdr::Output::Sdr ? std::string("Sending SDR")
                                                 : std::format("Sending {}", hdr::name(shown));
    if (settings::get().hdr != hdr::Mode::Off) {
        if (g->window->gl_api() != platform::GlApi::Vulkan) {
            sent += " (HDR needs the Vulkan renderer)";
        } else if (!g->hdr_at_start) {
            sent += " (HDR from the next start)";
        } else if (shown == hdr::Output::Sdr && !g->window->hdr_offered()) {
            sent += " (the screen offers no HDR format)";
        }
    }
    return sent + "; " + screen + ".";
}

// The window's own keys: F11 and Alt+Return change between fullscreen and the window, F9 and F10
// turn the port's volume down and up a tenth, F2 shows or hides the frame counter,
// and F4 toggles enhanced graphics. True when the event was one of them.
bool window_key(const SDL_Event& event) {
    window_event(event);
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return false;
    }
    if (event.key.key == SDLK_F2) {  // the frame counter, as the Video options' row
        settings::step_frame_counter(1);
        settings::save();
        return true;
    }
    if (event.key.key == SDLK_F4 && gl::enhanced_rendering_available()) {
        if (!enhanced_graphics_available()) {
            debug_overlay::notify({
                .id = "enhanced",
                .title = "Enhanced lighting is not available here",
                .text = "This level has no compatible probe data.",
                .tone = debug_overlay::Tone::Warning,
                .seconds = 6.0,
            });
            return true;
        }
        auto& graphics = settings::get();
        graphics.enhanced_graphics = !graphics.enhanced_graphics;
        settings::save();
        log::info("enhanced graphics {}", graphics.enhanced_graphics ? "on" : "off");
        return true;
    }
    if (event.key.key == SDLK_F9 || event.key.key == SDLK_F10) {
        settings::set_volume(settings::get().volume + (event.key.key == SDLK_F10 ? 10 : -10));
        settings::save();
        log::info("volume {}", settings::get().volume);
        return true;
    }
    const bool alt_return = (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
                            && (event.key.mod & SDL_KMOD_ALT) != 0;
    if (event.key.key != SDLK_F11 && !alt_return) {
        return false;
    }
    // Between the window and borderless full screen.
    settings::Settings& s = settings::get();
    s.display_mode = s.display_mode == settings::DisplayMode::Windowed ? settings::DisplayMode::Borderless
                                                                       : settings::DisplayMode::Windowed;
    settings::save();
    apply_settings();
    return true;
}

}  // namespace openrac::frontend

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// [debug tools] OPENRAC_INPUT_LATENCY (input_latency.h): a virtual gamepad presses a button at
// random moments from a thread of its own; the window thread notes when the game's pad read first
// sees the press and when the frames of that tick are shown. One press is measured at a time.

#include "game/common/frontend/input_latency.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "common/env.h"
#include "common/joining_thread.h"
#include "common/log.h"

namespace openrac::frontend::input_latency {
namespace {

using Clock = std::chrono::steady_clock;

// One press being measured, shared by the pressing thread and the window thread.
struct Press {
    Clock::time_point at{};             // when the virtual button went down
    bool sampled = false;               // the port's input has it (Input::update or refresh_pads)
    std::optional<std::uint64_t> read;  // the window frame whose pad read first saw it
    int shown = 0;                      // frames of that tick or later shown since
    bool open = false;                  // being measured
};

struct Probe {
    std::mutex mutex;
    std::condition_variable wake;
    Press press;
    SDL_Joystick* joystick = nullptr;
    SDL_GamepadButton button = SDL_GAMEPAD_BUTTON_LEFT_STICK;
    bool stop = false;  // set when the probe goes; the pressing thread then returns
    JoiningThread presser;

    ~Probe() {
        {
            const std::scoped_lock lock(mutex);
            stop = true;
        }
        wake.notify_all();
    }
};

double ms_since(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

// The pressing thread: waits for the virtual pad to be taken (a couple of seconds), then presses
// for 120 ms at a random 250-650 ms after each release, so presses fall at every phase of a tick.
void press_loop(Probe& probe) {
    std::mt19937 random(12345);
    std::uniform_int_distribution<int> gap(250, 650);
    std::unique_lock lock(probe.mutex);
    // Waits `time` unless the probe goes first; true when it went.
    const auto stopped = [&](auto time) {
        return probe.wake.wait_for(lock, time, [&] { return probe.stop; });
    };
    if (stopped(std::chrono::seconds(3))) {
        return;
    }
    while (!stopped(std::chrono::milliseconds(gap(random)))) {
        // An unfinished press (a load that read no pad) is reported and dropped.
        if (probe.press.open) {
            log::info(
                "input latency: press not seen (read {}, shown {})",
                probe.press.read ? "yes" : "no",
                probe.press.shown
            );
        }
        probe.press = Press{Clock::now(), false, std::nullopt, 0, true};
        SDL_SetJoystickVirtualButton(probe.joystick, probe.button, true);
        const bool done = stopped(std::chrono::milliseconds(120));
        SDL_SetJoystickVirtualButton(probe.joystick, probe.button, false);
        if (done) {
            return;
        }
    }
}

// The probe, made on the first pad read when the variable is set; nullptr otherwise.
Probe* probe() {
    static Probe* const made = []() -> Probe* {
        const std::optional<std::string_view> value = env::value(env::Var::InputLatency);
        if (!value || value->empty() || *value == "0") {
            return nullptr;
        }
        static Probe p;
        // A button by SDL's name (a, b, x, y, back, start, leftstick, dpdown, ...); 1: L3.
        if (*value != "1") {
            const SDL_GamepadButton named =
                SDL_GetGamepadButtonFromString(std::string(*value).c_str());
            if (named != SDL_GAMEPAD_BUTTON_INVALID) {
                p.button = named;
            }
        }
        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.name = "OpenRAC input latency probe";
        const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
        p.joystick = id != 0 ? SDL_OpenJoystick(id) : nullptr;
        if (p.joystick == nullptr) {
            log::warn("OPENRAC_INPUT_LATENCY: no virtual gamepad ({})", SDL_GetError());
            return nullptr;
        }
        log::info(
            "input latency: virtual gamepad pressing {}", SDL_GetGamepadStringForButton(p.button)
        );
        p.presser = JoiningThread([] { press_loop(p); });
        return &p;
    }();
    return made;
}

}  // namespace

void sampled(std::uint16_t buttons) {
    Probe* p = probe();
    if (p == nullptr) {
        return;
    }
    const auto now = Clock::now();
    const std::scoped_lock lock(p->mutex);
    if (!p->press.open || p->press.sampled || buttons == 0xFFFF) {
        return;
    }
    p->press.sampled = true;
    log::info("input latency: sampled {:.2f} ms after the press", ms_since(p->press.at, now));
}

void pad_read(std::uint16_t buttons, std::uint64_t index) {
    Probe* p = probe();
    if (p == nullptr) {
        return;
    }
    const auto now = Clock::now();
    const std::scoped_lock lock(p->mutex);
    if (!p->press.open || p->press.read || buttons == 0xFFFF) {
        return;
    }
    p->press.read = index;
    log::info(
        "input latency: pad read {:.2f} ms after the press (frame {})",
        ms_since(p->press.at, now),
        index
    );
}

void shown(std::uint64_t tick) {
    Probe* p = probe();
    if (p == nullptr) {
        return;
    }
    const auto now = Clock::now();
    const std::scoped_lock lock(p->mutex);
    if (!p->press.open || !p->press.read || tick < *p->press.read) {
        return;
    }
    ++p->press.shown;
    log::info(
        "input latency: frame {} shown {:.2f} ms after the press ({} of its tick or later)",
        tick,
        ms_since(p->press.at, now),
        p->press.shown
    );
    if (p->press.shown >= 2) {
        p->press.open = false;
    }
}

}  // namespace openrac::frontend::input_latency

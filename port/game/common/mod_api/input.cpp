// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "input": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "button_labels.h"
#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

const openrac_input* api_input(void) {
    static openrac_input none = [] {
        openrac_input i{};
        i.size = sizeof(openrac_input);
        return i;
    }();
    const openrac_input* in =
        g_host != nullptr && g_host->input != nullptr ? g_host->input() : nullptr;
    return in != nullptr ? in : &none;
}

int api_take_input(openrac_mod* mod, uint32_t devices) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    devices &= OPENRAC_INPUT_KEYBOARD | OPENRAC_INPUT_MOUSE;
    if (devices != 0 && g_host != nullptr && g_host->take_input != nullptr) {
        g_host->take_input(devices);
    }
    if ((devices & OPENRAC_INPUT_KEYBOARD) != 0) {
        log::info(
            "mod {} takes the keyboard: the port's own keys no longer press pad 0's buttons",
            mod->id
        );
    }
    return OPENRAC_HOOK_OK;
}

void api_capture_mouse(openrac_mod* mod, int capture) {
    (void)mod;
    if (g_host != nullptr && g_host->capture_mouse != nullptr) {
        g_host->capture_mouse(capture != 0);
    }
}

// Interface 1.31: a button's label for the device in use (button_labels.h).
int api_button_label(openrac_mod*, std::uint32_t button, char* out, std::uint32_t size) {
    return button_labels::label_into(button, out, size);
}

// Interface 1.37: a mod's motor values for a pad (Host::motors), played with the game's own.
int api_set_motors(openrac_mod* mod, int port, int small, int large) {
    // The game's two pad ports (on_pad's), libvib's small motor off or on, its large 0 to 255.
    constexpr int kPorts = 2;
    if (mod == nullptr || port < 0 || port >= kPorts || (small != 0 && small != 1) || large < 0
        || large > 255 || g_host == nullptr || g_host->motors == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    {
        // From now on the mod drives motors: the menu shows the player's vibration settings.
        const std::lock_guard<std::mutex> lock(g_motor_mods_lock);
        if (std::find(g_motor_mods.begin(), g_motor_mods.end(), mod) == g_motor_mods.end()) {
            g_motor_mods.push_back(mod);
        }
    }
    g_host->motors(mod, port, small != 0, static_cast<std::uint8_t>(large));
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail

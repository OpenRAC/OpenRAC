// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/bindings.h"

#include <array>
#include <cstring>

#include "common/log.h"
#include "openrac/keys.h"

namespace openrac::platform {
namespace {

// The pad's own names, the port's: the SDK's table (keys.h) names keys and the mouse only, and a
// mod reading a keybind setting cannot use a pad input, so these are not added to it.
struct PadName {
    const char* name;
    SDL_GamepadButton button;  // INVALID for a trigger
    SDL_GamepadAxis axis;      // INVALID for a button
};

constexpr SDL_GamepadButton kNoButton = SDL_GAMEPAD_BUTTON_INVALID;
constexpr SDL_GamepadAxis kNoAxis = SDL_GAMEPAD_AXIS_INVALID;

// By position, as the port's own table is: PadSouth is Cross on a DualShock and A on an Xbox pad.
constexpr PadName kPadNames[] = {
    {"PadSouth", SDL_GAMEPAD_BUTTON_SOUTH, kNoAxis},
    {"PadEast", SDL_GAMEPAD_BUTTON_EAST, kNoAxis},
    {"PadWest", SDL_GAMEPAD_BUTTON_WEST, kNoAxis},
    {"PadNorth", SDL_GAMEPAD_BUTTON_NORTH, kNoAxis},
    {"PadBack", SDL_GAMEPAD_BUTTON_BACK, kNoAxis},
    {"PadGuide", SDL_GAMEPAD_BUTTON_GUIDE, kNoAxis},
    {"PadStart", SDL_GAMEPAD_BUTTON_START, kNoAxis},
    {"PadLeftStick", SDL_GAMEPAD_BUTTON_LEFT_STICK, kNoAxis},
    {"PadRightStick", SDL_GAMEPAD_BUTTON_RIGHT_STICK, kNoAxis},
    {"PadLeftShoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, kNoAxis},
    {"PadRightShoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, kNoAxis},
    {"PadDpadUp", SDL_GAMEPAD_BUTTON_DPAD_UP, kNoAxis},
    {"PadDpadDown", SDL_GAMEPAD_BUTTON_DPAD_DOWN, kNoAxis},
    {"PadDpadLeft", SDL_GAMEPAD_BUTTON_DPAD_LEFT, kNoAxis},
    {"PadDpadRight", SDL_GAMEPAD_BUTTON_DPAD_RIGHT, kNoAxis},
    {"PadMisc1", SDL_GAMEPAD_BUTTON_MISC1, kNoAxis},
    {"PadRightPaddle1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, kNoAxis},
    {"PadLeftPaddle1", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, kNoAxis},
    {"PadRightPaddle2", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, kNoAxis},
    {"PadLeftPaddle2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, kNoAxis},
    {"PadTouchpad", SDL_GAMEPAD_BUTTON_TOUCHPAD, kNoAxis},
    {"PadLeftTrigger", kNoButton, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {"PadRightTrigger", kNoButton, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
};

// ---- The port's defaults ----------------------------------------------------------------------
//
// The tables the port has always had, which a button with no binding of that kind keeps. Keys are
// by position, whatever the layout; pad buttons by position too, so the bottom face button is
// Cross on any controller.

struct DefaultKey {
    SDL_Scancode key;
    Button button;
};

constexpr DefaultKey kDefaultKeys[] = {
    {SDL_SCANCODE_UP, Button::Up},
    {SDL_SCANCODE_DOWN, Button::Down},
    {SDL_SCANCODE_LEFT, Button::Left},
    {SDL_SCANCODE_RIGHT, Button::Right},
    {SDL_SCANCODE_SPACE, Button::Cross},
    {SDL_SCANCODE_F, Button::Square},
    {SDL_SCANCODE_E, Button::Circle},
    {SDL_SCANCODE_R, Button::Triangle},
    {SDL_SCANCODE_LSHIFT, Button::L1},
    {SDL_SCANCODE_Q, Button::R1},
    {SDL_SCANCODE_1, Button::L2},
    {SDL_SCANCODE_3, Button::R2},
    {SDL_SCANCODE_C, Button::L3},
    {SDL_SCANCODE_V, Button::R3},
    {SDL_SCANCODE_RETURN, Button::Start},
    {SDL_SCANCODE_BACKSPACE, Button::Select},
};

struct DefaultPad {
    SDL_GamepadButton pad_button;
    Button button;
};

constexpr DefaultPad kDefaultPads[] = {
    {SDL_GAMEPAD_BUTTON_SOUTH, Button::Cross},
    {SDL_GAMEPAD_BUTTON_EAST, Button::Circle},
    {SDL_GAMEPAD_BUTTON_WEST, Button::Square},
    {SDL_GAMEPAD_BUTTON_NORTH, Button::Triangle},
    {SDL_GAMEPAD_BUTTON_BACK, Button::Select},
    {SDL_GAMEPAD_BUTTON_START, Button::Start},
    {SDL_GAMEPAD_BUTTON_LEFT_STICK, Button::L3},
    {SDL_GAMEPAD_BUTTON_RIGHT_STICK, Button::R3},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Button::L1},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Button::R1},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, Button::Up},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Button::Down},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Button::Left},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Button::Right},
};

// L2 and R2 are the triggers by default; nothing else is an axis.
SDL_GamepadAxis default_trigger(Button button) {
    if (button == Button::L2) {
        return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    }
    if (button == Button::R2) {
        return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    }
    return kNoAxis;
}

// A settings key per button, in the order the launcher shows them.
struct ButtonKey {
    const char* key;
    Button button;
};

constexpr ButtonKey kButtonKeys[] = {
    {"bind_cross", Button::Cross},
    {"bind_circle", Button::Circle},
    {"bind_square", Button::Square},
    {"bind_triangle", Button::Triangle},
    {"bind_l1", Button::L1},
    {"bind_r1", Button::R1},
    {"bind_l2", Button::L2},
    {"bind_r2", Button::R2},
    {"bind_l3", Button::L3},
    {"bind_r3", Button::R3},
    {"bind_start", Button::Start},
    {"bind_select", Button::Select},
    {"bind_up", Button::Up},
    {"bind_down", Button::Down},
    {"bind_left", Button::Left},
    {"bind_right", Button::Right},
};

constexpr int kButtonCount = static_cast<int>(std::size(kButtonKeys));

// At most this many of a kind on one button: two is what the launcher offers (a key and a spare).
constexpr int kMostOfAKind = 2;

// One button's bindings, with room for the port's defaults when the player set none.
struct Slot {
    std::array<SDL_Scancode, kMostOfAKind + 1> keys{};
    std::array<SDL_GamepadButton, kMostOfAKind + 1> pad_buttons{};
    SDL_GamepadAxis trigger = kNoAxis;
    Bound view{};
    bool player_keys = false;
    bool player_pad = false;
};

std::array<Slot, kButtonCount> g_slots;
bool g_started = false;

int slot_of(Button button) {
    for (int i = 0; i < kButtonCount; ++i) {
        if (kButtonKeys[i].button == button) {
            return i;
        }
    }
    return -1;
}

void refresh(Slot& slot) {
    slot.view.keys = slot.keys.data();
    slot.view.key_count = 0;
    for (SDL_Scancode code : slot.keys) {
        if (code == SDL_SCANCODE_UNKNOWN) {
            break;
        }
        ++slot.view.key_count;
    }
    slot.view.pad_buttons = slot.pad_buttons.data();
    slot.view.pad_button_count = 0;
    for (SDL_GamepadButton pad : slot.pad_buttons) {
        if (pad == kNoButton) {
            break;
        }
        ++slot.view.pad_button_count;
    }
    slot.view.trigger = slot.trigger;
}

// The port's own tables, which a button with no binding of that kind keeps (input.cpp).
void start_from_defaults() {
    for (int i = 0; i < kButtonCount; ++i) {
        Slot& slot = g_slots[static_cast<std::size_t>(i)];
        slot = Slot{};
        slot.keys.fill(SDL_SCANCODE_UNKNOWN);
        slot.pad_buttons.fill(kNoButton);
        int keys = 0;
        for (const DefaultKey& d : kDefaultKeys) {
            if (d.button == kButtonKeys[i].button && keys < kMostOfAKind) {
                slot.keys[static_cast<std::size_t>(keys++)] = d.key;
            }
        }
        int pads = 0;
        for (const DefaultPad& d : kDefaultPads) {
            if (d.button == kButtonKeys[i].button && pads < kMostOfAKind) {
                slot.pad_buttons[static_cast<std::size_t>(pads++)] = d.pad_button;
            }
        }
        slot.trigger = default_trigger(kButtonKeys[i].button);
        refresh(slot);
    }
    g_started = true;
}

}  // namespace

Binding parse_binding(std::string_view name) {
    if (name.empty()) {
        return {};
    }
    for (const PadName& pad : kPadNames) {
        if (name == pad.name) {
            return pad.button != kNoButton
                       ? Binding{Binding::Kind::PadButton, static_cast<int>(pad.button)}
                       : Binding{Binding::Kind::PadTrigger, static_cast<int>(pad.axis)};
        }
    }
    // The SDK's table names every key, and a name's value there is its HID usage, which is what an
    // SDL scancode is (input.h).
    const int key = openrac_bind_from_name_n(name.data(), name.size());
    if (key > 0 && key < static_cast<int>(SDL_SCANCODE_COUNT)) {
        return {Binding::Kind::Key, key};
    }
    return {};
}

const char* binding_name(const Binding& binding) {
    switch (binding.kind) {
        case Binding::Kind::Key:
            return openrac_bind_to_name(binding.value);
        case Binding::Kind::PadButton:
            for (const PadName& pad : kPadNames) {
                if (pad.button != kNoButton && static_cast<int>(pad.button) == binding.value) {
                    return pad.name;
                }
            }
            return "";
        case Binding::Kind::PadTrigger:
            for (const PadName& pad : kPadNames) {
                if (pad.axis != kNoAxis && static_cast<int>(pad.axis) == binding.value) {
                    return pad.name;
                }
            }
            return "";
        case Binding::Kind::None:
            break;
    }
    return "";
}

bool button_of_key(std::string_view key, Button& button) {
    for (const ButtonKey& b : kButtonKeys) {
        if (key == b.key) {
            button = b.button;
            return true;
        }
    }
    return false;
}

const char* const* binding_keys() {
    static const auto keys = [] {
        std::array<const char*, kButtonCount + 1> all{};
        for (int i = 0; i < kButtonCount; ++i) {
            all[static_cast<std::size_t>(i)] = kButtonKeys[i].key;
        }
        all[kButtonCount] = nullptr;
        return all;
    }();
    return keys.data();
}

void set_bindings(const std::map<std::string, std::string, std::less<>>& bindings) {
    start_from_defaults();
    for (const auto& [key, value] : bindings) {
        Button button{};
        if (!button_of_key(key, button)) {
            continue;
        }
        const int index = slot_of(button);
        if (index < 0) {
            continue;
        }
        Slot& slot = g_slots[static_cast<std::size_t>(index)];
        int keys = 0;
        int pads = 0;
        SDL_GamepadAxis trigger = kNoAxis;
        std::array<SDL_Scancode, kMostOfAKind + 1> read_keys{};
        std::array<SDL_GamepadButton, kMostOfAKind + 1> read_pads{};
        read_keys.fill(SDL_SCANCODE_UNKNOWN);
        read_pads.fill(kNoButton);
        for (std::size_t at = 0; at <= value.size();) {
            const std::size_t comma = value.find(',', at);
            const std::size_t end = comma == std::string::npos ? value.size() : comma;
            std::string_view name(value.data() + at, end - at);
            while (!name.empty() && name.front() == ' ') {
                name.remove_prefix(1);
            }
            while (!name.empty() && name.back() == ' ') {
                name.remove_suffix(1);
            }
            if (!name.empty()) {
                const Binding binding = parse_binding(name);
                switch (binding.kind) {
                    case Binding::Kind::Key:
                        if (keys < kMostOfAKind) {
                            read_keys[static_cast<std::size_t>(keys++)] =
                                static_cast<SDL_Scancode>(binding.value);
                        }
                        break;
                    case Binding::Kind::PadButton:
                        if (pads < kMostOfAKind) {
                            read_pads[static_cast<std::size_t>(pads++)] =
                                static_cast<SDL_GamepadButton>(binding.value);
                        }
                        break;
                    case Binding::Kind::PadTrigger:
                        trigger = static_cast<SDL_GamepadAxis>(binding.value);
                        break;
                    case Binding::Kind::None:
                        log::warn(
                            "{} = {}: {} is not a key or pad input the port knows; the default is "
                            "used",
                            key,
                            value,
                            std::string(name)
                        );
                        break;
                }
            }
            if (comma == std::string::npos) {
                break;
            }
            at = comma + 1;
        }
        // Nothing of a kind: that kind keeps the port's default, so a cleared binding can never
        // leave a player unable to pause.
        if (keys > 0) {
            slot.keys = read_keys;
            slot.player_keys = true;
        }
        if (pads > 0 || trigger != kNoAxis) {
            slot.pad_buttons = read_pads;
            slot.trigger = trigger;
            slot.player_pad = true;
        }
        refresh(slot);
    }
}

const Bound& bound(Button button) {
    if (!g_started) {
        start_from_defaults();
    }
    const int index = slot_of(button);
    static const Bound none{nullptr, 0, nullptr, 0, kNoAxis};
    return index >= 0 ? g_slots[static_cast<std::size_t>(index)].view : none;
}

}  // namespace openrac::platform

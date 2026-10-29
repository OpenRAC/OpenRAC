// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/bindings.h"

#include <array>
#include <bit>
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

// The slot of a button in Bindings::slots: its bit's position.
int slot_of(Button button) {
    return std::countr_zero(static_cast<unsigned>(button));
}

// One button's value from the settings file, read into `slot`: a kind the value names replaces
// that kind, and a kind it does not name is left as it was.
void read_value(std::string_view key, std::string_view value, Bindings::Slot& slot) {
    std::array<SDL_Scancode, kMostOfAKind> keys{};
    std::array<Binding, kMostOfAKind> pad{};
    int key_count = 0;
    int pad_count = 0;
    for (std::size_t at = 0; at <= value.size();) {
        const std::size_t comma = value.find(',', at);
        const std::size_t end = comma == std::string_view::npos ? value.size() : comma;
        std::string_view name = value.substr(at, end - at);
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
                    if (key_count < kMostOfAKind) {
                        keys[static_cast<std::size_t>(key_count++)] =
                            static_cast<SDL_Scancode>(binding.value);
                    }
                    break;
                case Binding::Kind::PadButton:
                case Binding::Kind::PadTrigger:
                    if (pad_count < kMostOfAKind) {
                        pad[static_cast<std::size_t>(pad_count++)] = binding;
                    }
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
        if (comma == std::string_view::npos) {
            break;
        }
        at = comma + 1;
    }
    // Nothing of a kind: that kind keeps what it had, so a cleared binding can never leave a
    // player unable to pause.
    if (key_count > 0) {
        slot.keys = keys;
        slot.key_count = key_count;
    }
    if (pad_count > 0) {
        slot.pad = pad;
        slot.pad_count = pad_count;
    }
}

bool key(std::span<const bool> keys, SDL_Scancode code) {
    const auto index = static_cast<std::size_t>(code);
    return index < keys.size() && keys[index];
}

// -1, 0 or +1 from two opposite keys.
int key_axis(std::span<const bool> keys, SDL_Scancode negative, SDL_Scancode positive) {
    return (key(keys, positive) ? 1 : 0) - (key(keys, negative) ? 1 : 0);
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

const Bindings::Slot& Bindings::of(Button button) const {
    return slots[static_cast<std::size_t>(slot_of(button))];
}

Bindings default_bindings() {
    Bindings bindings;
    for (const DefaultKey& d : kDefaultKeys) {
        Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(slot_of(d.button))];
        if (slot.key_count < kMostOfAKind) {
            slot.keys[static_cast<std::size_t>(slot.key_count++)] = d.key;
        }
    }
    for (const DefaultPad& d : kDefaultPads) {
        Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(slot_of(d.button))];
        if (slot.pad_count < kMostOfAKind) {
            slot.pad[static_cast<std::size_t>(slot.pad_count++)] =
                Binding{Binding::Kind::PadButton, static_cast<int>(d.pad_button)};
        }
    }
    for (const Button button : {Button::L2, Button::R2}) {
        Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(slot_of(button))];
        slot.pad[static_cast<std::size_t>(slot.pad_count++)] =
            Binding{Binding::Kind::PadTrigger, static_cast<int>(default_trigger(button))};
    }
    return bindings;
}

Bindings compile_bindings(const std::map<std::string, std::string, std::less<>>& bindings) {
    Bindings out = default_bindings();
    for (const auto& [key, value] : bindings) {
        Button button{};
        if (button_of_key(key, button)) {
            read_value(key, value, out.slots[static_cast<std::size_t>(slot_of(button))]);
        }
    }
    return out;
}

void apply_keys(const Bindings& bindings, std::span<const bool> keys, Pad& pad) {
    for (const ButtonKey& b : kButtonKeys) {
        const Bindings::Slot& slot = bindings.of(b.button);
        for (int k = 0; k < slot.key_count; ++k) {
            if (key(keys, slot.keys[static_cast<std::size_t>(k)])) {
                pad.set(b.button, true);
                break;
            }
        }
    }
    // Escape pauses, as Start does, whatever Start is bound to: it used to end the program at
    // once, a level's progress with it, and a player must always be able to reach the menu.
    if (key(keys, SDL_SCANCODE_ESCAPE)) {
        pad.set(Button::Start, true);
    }
    const int lx = key_axis(keys, SDL_SCANCODE_A, SDL_SCANCODE_D);
    const int ly = key_axis(keys, SDL_SCANCODE_W, SDL_SCANCODE_S);
    const int rx = key_axis(keys, SDL_SCANCODE_J, SDL_SCANCODE_L);
    const int ry = key_axis(keys, SDL_SCANCODE_I, SDL_SCANCODE_K);
    if (lx != 0) {
        pad.left_x = key_stick_byte(lx);
    }
    if (ly != 0) {
        pad.left_y = key_stick_byte(ly);
    }
    if (rx != 0) {
        pad.right_x = key_stick_byte(rx);
    }
    if (ry != 0) {
        pad.right_y = key_stick_byte(ry);
    }
}

void apply_pad(
    const Bindings& bindings, const PadInputs& inputs, std::array<StickGate, 2>& gates, Pad& pad
) {
    for (const ButtonKey& b : kButtonKeys) {
        const Bindings::Slot& slot = bindings.of(b.button);
        for (int p = 0; p < slot.pad_count; ++p) {
            const Binding& binding = slot.pad[static_cast<std::size_t>(p)];
            if (binding.kind == Binding::Kind::PadButton) {
                if (inputs.buttons[static_cast<std::size_t>(binding.value)]) {
                    pad.set(b.button, true);
                }
            } else if (binding.kind == Binding::Kind::PadTrigger) {
                // A trigger bound to a button presses it above about an eighth of its travel, and
                // its pressure byte follows the trigger all the way.
                const std::uint8_t travel =
                    trigger_byte(inputs.axes[static_cast<std::size_t>(binding.value)]);
                if (travel >= kTriggerThreshold) {
                    pad.set(b.button, true, travel);
                }
            }
        }
    }
    // Squared as the DualShock 2's (pad.h): a round stick pushed to a diagonal runs.
    stick_bytes(
        inputs.axes[SDL_GAMEPAD_AXIS_LEFTX],
        inputs.axes[SDL_GAMEPAD_AXIS_LEFTY],
        gates[0],
        pad.left_x,
        pad.left_y
    );
    stick_bytes(
        inputs.axes[SDL_GAMEPAD_AXIS_RIGHTX],
        inputs.axes[SDL_GAMEPAD_AXIS_RIGHTY],
        gates[1],
        pad.right_x,
        pad.right_y
    );
}

}  // namespace openrac::platform

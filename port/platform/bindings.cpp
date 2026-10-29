// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/bindings.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstring>
#include <utility>

#include "common/log.h"
#include "openrac/keys.h"

namespace openrac::platform {
namespace {

// The pad's own names, the port's: the SDK's table (keys.h) names keys and the mouse only, and a
// mod reading a keybind setting cannot use a pad input, so these are not added to it.
struct PadName {
    const char* name;
    SDL_GamepadButton button;  // INVALID for an axis
    SDL_GamepadAxis axis;      // INVALID for a button
    int sign;                  // a stick's direction: -1 left or up, +1 right or down; 0 a trigger
};

constexpr SDL_GamepadButton kNoButton = SDL_GAMEPAD_BUTTON_INVALID;
constexpr SDL_GamepadAxis kNoAxis = SDL_GAMEPAD_AXIS_INVALID;

// By position, as the port's own table is: PadSouth is Cross on a DualShock and A on an Xbox pad.
constexpr PadName kPadNames[] = {
    {"PadSouth", SDL_GAMEPAD_BUTTON_SOUTH, kNoAxis, 0},
    {"PadEast", SDL_GAMEPAD_BUTTON_EAST, kNoAxis, 0},
    {"PadWest", SDL_GAMEPAD_BUTTON_WEST, kNoAxis, 0},
    {"PadNorth", SDL_GAMEPAD_BUTTON_NORTH, kNoAxis, 0},
    {"PadBack", SDL_GAMEPAD_BUTTON_BACK, kNoAxis, 0},
    {"PadGuide", SDL_GAMEPAD_BUTTON_GUIDE, kNoAxis, 0},
    {"PadStart", SDL_GAMEPAD_BUTTON_START, kNoAxis, 0},
    {"PadLeftStick", SDL_GAMEPAD_BUTTON_LEFT_STICK, kNoAxis, 0},
    {"PadRightStick", SDL_GAMEPAD_BUTTON_RIGHT_STICK, kNoAxis, 0},
    {"PadLeftShoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, kNoAxis, 0},
    {"PadRightShoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, kNoAxis, 0},
    {"PadDpadUp", SDL_GAMEPAD_BUTTON_DPAD_UP, kNoAxis, 0},
    {"PadDpadDown", SDL_GAMEPAD_BUTTON_DPAD_DOWN, kNoAxis, 0},
    {"PadDpadLeft", SDL_GAMEPAD_BUTTON_DPAD_LEFT, kNoAxis, 0},
    {"PadDpadRight", SDL_GAMEPAD_BUTTON_DPAD_RIGHT, kNoAxis, 0},
    {"PadMisc1", SDL_GAMEPAD_BUTTON_MISC1, kNoAxis, 0},
    {"PadRightPaddle1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, kNoAxis, 0},
    {"PadLeftPaddle1", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, kNoAxis, 0},
    {"PadRightPaddle2", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, kNoAxis, 0},
    {"PadLeftPaddle2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, kNoAxis, 0},
    {"PadTouchpad", SDL_GAMEPAD_BUTTON_TOUCHPAD, kNoAxis, 0},
    {"PadMisc2", SDL_GAMEPAD_BUTTON_MISC2, kNoAxis, 0},
    {"PadMisc3", SDL_GAMEPAD_BUTTON_MISC3, kNoAxis, 0},
    {"PadMisc4", SDL_GAMEPAD_BUTTON_MISC4, kNoAxis, 0},
    {"PadMisc5", SDL_GAMEPAD_BUTTON_MISC5, kNoAxis, 0},
    {"PadMisc6", SDL_GAMEPAD_BUTTON_MISC6, kNoAxis, 0},
    {"PadLeftTrigger", kNoButton, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0},
    {"PadRightTrigger", kNoButton, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0},
    {"PadLeftStickUp", kNoButton, SDL_GAMEPAD_AXIS_LEFTY, -1},
    {"PadLeftStickDown", kNoButton, SDL_GAMEPAD_AXIS_LEFTY, 1},
    {"PadLeftStickLeft", kNoButton, SDL_GAMEPAD_AXIS_LEFTX, -1},
    {"PadLeftStickRight", kNoButton, SDL_GAMEPAD_AXIS_LEFTX, 1},
    {"PadRightStickUp", kNoButton, SDL_GAMEPAD_AXIS_RIGHTY, -1},
    {"PadRightStickDown", kNoButton, SDL_GAMEPAD_AXIS_RIGHTY, 1},
    {"PadRightStickLeft", kNoButton, SDL_GAMEPAD_AXIS_RIGHTX, -1},
    {"PadRightStickRight", kNoButton, SDL_GAMEPAD_AXIS_RIGHTX, 1},
};

// The binding a row of kPadNames names.
Binding binding_of(const PadName& pad) {
    if (pad.button != kNoButton) {
        return {Binding::Kind::PadButton, static_cast<int>(pad.button), 0};
    }
    if (pad.sign == 0) {
        return {Binding::Kind::PadTrigger, static_cast<int>(pad.axis), 0};
    }
    return {Binding::Kind::PadAxis, static_cast<int>(pad.axis), pad.sign};
}

// ---- The port's defaults ----------------------------------------------------------------------
//
// The tables the port has always had, which a target with no binding of that kind keeps. Keys are
// by position, whatever the layout; pad buttons by position too, so the bottom face button is
// Cross on any controller. Each stick's direction is the gamepad's own stick's.

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

constexpr int button_target(Button button) {
    return std::countr_zero(static_cast<unsigned>(button));
}

constexpr int stick_target(Target target) {
    return static_cast<int>(target);
}

struct DefaultPad {
    const char* name;  // in kPadNames
    int target;
};

constexpr DefaultPad kDefaultPads[] = {
    {"PadSouth", button_target(Button::Cross)},
    {"PadEast", button_target(Button::Circle)},
    {"PadWest", button_target(Button::Square)},
    {"PadNorth", button_target(Button::Triangle)},
    {"PadBack", button_target(Button::Select)},
    {"PadStart", button_target(Button::Start)},
    {"PadLeftStick", button_target(Button::L3)},
    {"PadRightStick", button_target(Button::R3)},
    {"PadLeftShoulder", button_target(Button::L1)},
    {"PadRightShoulder", button_target(Button::R1)},
    {"PadDpadUp", button_target(Button::Up)},
    {"PadDpadDown", button_target(Button::Down)},
    {"PadDpadLeft", button_target(Button::Left)},
    {"PadDpadRight", button_target(Button::Right)},
    {"PadLeftTrigger", button_target(Button::L2)},
    {"PadRightTrigger", button_target(Button::R2)},
    {"PadLeftStickUp", stick_target(Target::LeftStickUp)},
    {"PadLeftStickDown", stick_target(Target::LeftStickDown)},
    {"PadLeftStickLeft", stick_target(Target::LeftStickLeft)},
    {"PadLeftStickRight", stick_target(Target::LeftStickRight)},
    {"PadRightStickUp", stick_target(Target::RightStickUp)},
    {"PadRightStickDown", stick_target(Target::RightStickDown)},
    {"PadRightStickLeft", stick_target(Target::RightStickLeft)},
    {"PadRightStickRight", stick_target(Target::RightStickRight)},
};

// A settings key per target, in the order the launcher shows them.
struct TargetKey {
    const char* key;
    int target;
};

constexpr TargetKey kTargetKeys[] = {
    {"bind_cross", button_target(Button::Cross)},
    {"bind_circle", button_target(Button::Circle)},
    {"bind_square", button_target(Button::Square)},
    {"bind_triangle", button_target(Button::Triangle)},
    {"bind_l1", button_target(Button::L1)},
    {"bind_r1", button_target(Button::R1)},
    {"bind_l2", button_target(Button::L2)},
    {"bind_r2", button_target(Button::R2)},
    {"bind_l3", button_target(Button::L3)},
    {"bind_r3", button_target(Button::R3)},
    {"bind_start", button_target(Button::Start)},
    {"bind_select", button_target(Button::Select)},
    {"bind_up", button_target(Button::Up)},
    {"bind_down", button_target(Button::Down)},
    {"bind_left", button_target(Button::Left)},
    {"bind_right", button_target(Button::Right)},
    {"bind_lstick_up", stick_target(Target::LeftStickUp)},
    {"bind_lstick_down", stick_target(Target::LeftStickDown)},
    {"bind_lstick_left", stick_target(Target::LeftStickLeft)},
    {"bind_lstick_right", stick_target(Target::LeftStickRight)},
    {"bind_rstick_up", stick_target(Target::RightStickUp)},
    {"bind_rstick_down", stick_target(Target::RightStickDown)},
    {"bind_rstick_left", stick_target(Target::RightStickLeft)},
    {"bind_rstick_right", stick_target(Target::RightStickRight)},
};

static_assert(std::size(kTargetKeys) == kTargetCount);

std::string_view trimmed(std::string_view s) {
    while (!s.empty() && s.front() == ' ') {
        s.remove_prefix(1);
    }
    while (!s.empty() && s.back() == ' ') {
        s.remove_suffix(1);
    }
    return s;
}

bool is_pad(const Binding& b) {
    return b.kind == Binding::Kind::PadButton || b.kind == Binding::Kind::PadTrigger
           || b.kind == Binding::Kind::PadAxis;
}

// One name of a value read, a pad chord ("A+B") or a single name: Kind::None, reported, for one
// that is not a key or pad input the port knows, or a chord that is not of two pad inputs.
PadChord read_name(std::string_view key, std::string_view value, std::string_view name) {
    const std::size_t plus = name.find('+');
    if (plus == std::string_view::npos) {
        PadChord one{parse_binding(name), {}};
        if (one.input.kind == Binding::Kind::None) {
            log::warn(
                "{} = {}: {} is not a key or pad input the port knows; the default is used",
                key,
                value,
                std::string(name)
            );
        }
        return one;
    }
    const Binding with = parse_binding(trimmed(name.substr(0, plus)));
    const Binding input = parse_binding(trimmed(name.substr(plus + 1)));
    if (!is_pad(with) || !is_pad(input) || with == input) {
        log::warn(
            "{} = {}: {} is not a chord of two pad inputs; it is left out",
            key,
            value,
            std::string(name)
        );
        return {};
    }
    return {input, with};
}

bool key(std::span<const bool> keys, SDL_Scancode code) {
    const auto index = static_cast<std::size_t>(code);
    return index < keys.size() && keys[index];
}

// -1, 0 or +1 from two opposite keys.
int key_axis(std::span<const bool> keys, SDL_Scancode negative, SDL_Scancode positive) {
    return (key(keys, positive) ? 1 : 0) - (key(keys, negative) ? 1 : 0);
}

// One gamepad as apply_pad reads it: its sticks, tuned and shaped (left x, left y, right x, right
// y, -1..1, which is SDL's order of the axes, so a stick's direction finds its axis by its number),
// and how far a trigger goes before it holds a button.
struct Reading {
    std::array<float, 4> sticks{};
    std::uint8_t threshold = 0x20;
};

static_assert(SDL_GAMEPAD_AXIS_LEFTX == 0 && SDL_GAMEPAD_AXIS_LEFTY == 1);
static_assert(SDL_GAMEPAD_AXIS_RIGHTX == 2 && SDL_GAMEPAD_AXIS_RIGHTY == 3);

// What one gamepad gives for a pad input, 0 to 1: a button 0 or 1, a trigger its travel, a stick's
// direction how far the stick is pushed that way.
float amount(const Binding& b, const PadInputs& inputs, const Reading& reading) {
    switch (b.kind) {
        case Binding::Kind::PadButton:
            return inputs.buttons[static_cast<std::size_t>(b.value)] ? 1.0f : 0.0f;
        case Binding::Kind::PadTrigger:
            return std::clamp(
                static_cast<float>(inputs.axes[static_cast<std::size_t>(b.value)]) / 32767.0f,
                0.0f,
                1.0f
            );
        case Binding::Kind::PadAxis:
            return std::max(
                0.0f, static_cast<float>(b.sign) * reading.sticks[static_cast<std::size_t>(b.value)]
            );
        case Binding::Kind::Key:
        case Binding::Kind::None:
            break;
    }
    return 0.0f;
}

// Says whether a pad input counts as held: a button pressed, a trigger past an eighth of its
// travel, a stick past half way.
bool held(const Binding& b, const PadInputs& inputs, const Reading& reading) {
    switch (b.kind) {
        case Binding::Kind::PadButton:
            return inputs.buttons[static_cast<std::size_t>(b.value)];
        case Binding::Kind::PadTrigger:
            return trigger_byte(inputs.axes[static_cast<std::size_t>(b.value)])
                   >= reading.threshold;
        case Binding::Kind::PadAxis:
            return amount(b, inputs, reading) >= kStickPress;
        case Binding::Kind::Key:
        case Binding::Kind::None:
            break;
    }
    return false;
}

// The pressure a held pad input gives a button: a button's is full, a trigger's or a stick's
// follows how far it is pushed.
int pressure_of(const Binding& b, const PadInputs& inputs, const Reading& reading) {
    if (b.kind == Binding::Kind::PadButton) {
        return 0xFF;
    }
    return trigger_byte(
        static_cast<std::int16_t>(std::lround(amount(b, inputs, reading) * 32767.0f))
    );
}

// A stick's axis, -1..1, as the console's byte, rounded as stick_bytes rounds it.
std::uint8_t axis_byte(float v) {
    return stick_byte(
        static_cast<std::int16_t>(std::clamp(std::lround(v * 32768.0f), -32768L, 32767L))
    );
}

// One pad_* key's place in PadTuning: an int in a range, or a switch.
struct TuningKey {
    const char* key;
    int low;
    int high;
    int StickTuning::* stick_int;  // with `stick`, one of a stick's numbers
    bool StickTuning::* stick_bool;
    int stick;  // 0 left, 1 right, -1 not a stick's
};

constexpr TuningKey kStickKeys[] = {
    {"pad_left_deadzone", 0, 50, &StickTuning::deadzone, nullptr, 0},
    {"pad_left_outer", 50, 100, &StickTuning::outer, nullptr, 0},
    {"pad_left_sensitivity", 25, 300, &StickTuning::sensitivity, nullptr, 0},
    {"pad_left_invert_x", 0, 1, nullptr, &StickTuning::invert_x, 0},
    {"pad_left_invert_y", 0, 1, nullptr, &StickTuning::invert_y, 0},
    {"pad_right_deadzone", 0, 50, &StickTuning::deadzone, nullptr, 1},
    {"pad_right_outer", 50, 100, &StickTuning::outer, nullptr, 1},
    {"pad_right_sensitivity", 25, 300, &StickTuning::sensitivity, nullptr, 1},
    {"pad_right_invert_x", 0, 1, nullptr, &StickTuning::invert_x, 1},
    {"pad_right_invert_y", 0, 1, nullptr, &StickTuning::invert_y, 1},
};

// A whole number from `low` to `high`, or nothing.
bool read_int(std::string_view value, int low, int high, int& out) {
    value = trimmed(value);
    int number = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
    if (error != std::errc{} || end != value.data() + value.size() || number < low
        || number > high) {
        return false;
    }
    out = number;
    return true;
}

// A switch: 1, 0, on, off, true, false, yes, no (as the settings file reads them).
bool read_switch(std::string_view value, bool& out) {
    value = trimmed(value);
    if (value == "1" || value == "on" || value == "true" || value == "yes") {
        out = true;
        return true;
    }
    if (value == "0" || value == "off" || value == "false" || value == "no") {
        out = false;
        return true;
    }
    return false;
}

// The trigger threshold in the file: a percentage of the travel.
int threshold_percent(std::uint8_t threshold) {
    return (static_cast<int>(threshold) * 100 + 127) / 255;
}

}  // namespace

bool read_tuning(std::string_view key, std::string_view value, PadTuning& tuning) {
    bool known = true;
    bool good = false;
    if (key == "pad_trigger_threshold") {
        int percent = 0;
        good = read_int(value, 1, 100, percent);
        if (good) {
            tuning.trigger_threshold = static_cast<std::uint8_t>((percent * 255 + 50) / 100);
        }
    } else if (key == "pad_face_buttons") {
        const std::string_view v = trimmed(value);
        good = v == "position" || v == "labels";
        if (good) {
            tuning.face_labels = v == "labels";
        }
    } else if (key == "pad_background") {
        good = read_switch(value, tuning.background);
    } else {
        known = false;
        for (const TuningKey& t : kStickKeys) {
            if (key != t.key) {
                continue;
            }
            known = true;
            StickTuning& stick = tuning.sticks[static_cast<std::size_t>(t.stick)];
            good = t.stick_int != nullptr ? read_int(value, t.low, t.high, stick.*t.stick_int)
                                          : read_switch(value, stick.*t.stick_bool);
        }
    }
    if (!known) {
        log::warn("{} = {}: not a setting of the controllers'; it is left out", key, value);
    } else if (!good) {
        log::warn("{} = {}: not a value it takes; the one it had is kept", key, value);
    }
    return known && good;
}

std::map<std::string, std::string, std::less<>> tuning_values(const PadTuning& tuning) {
    std::map<std::string, std::string, std::less<>> out;
    for (const TuningKey& t : kStickKeys) {
        const StickTuning& stick = tuning.sticks[static_cast<std::size_t>(t.stick)];
        out[t.key] = t.stick_int != nullptr ? std::to_string(stick.*t.stick_int)
                                            : (stick.*t.stick_bool ? "1" : "0");
    }
    out["pad_trigger_threshold"] = std::to_string(threshold_percent(tuning.trigger_threshold));
    out["pad_face_buttons"] = tuning.face_labels ? "labels" : "position";
    out["pad_background"] = tuning.background ? "1" : "0";
    return out;
}

void tune_stick(float& x, float& y, const StickTuning& tuning) {
    if (tuning.invert_x) {
        x = -x;
    }
    if (tuning.invert_y) {
        y = -y;
    }
    if (tuning.deadzone != 0 || tuning.outer != 100) {
        // Radial: the stick's direction stays, its length is measured from the dead zone's edge
        // to the outer one.
        const float length = std::sqrt(x * x + y * y);
        const float inner = static_cast<float>(tuning.deadzone) / 100.0f;
        const float outer = std::max(static_cast<float>(tuning.outer) / 100.0f, inner + 0.01f);
        const float scale = length <= inner ? 0.0f : (length - inner) / (outer - inner) / length;
        x = std::clamp(x * scale, -1.0f, 1.0f);
        y = std::clamp(y * scale, -1.0f, 1.0f);
    }
    if (tuning.sensitivity != 100) {
        const float scale = static_cast<float>(tuning.sensitivity) / 100.0f;
        x = std::clamp(x * scale, -1.0f, 1.0f);
        y = std::clamp(y * scale, -1.0f, 1.0f);
    }
}

void swap_face_labels(PadInputs& inputs) {
    std::swap(inputs.buttons[SDL_GAMEPAD_BUTTON_SOUTH], inputs.buttons[SDL_GAMEPAD_BUTTON_EAST]);
    std::swap(inputs.buttons[SDL_GAMEPAD_BUTTON_WEST], inputs.buttons[SDL_GAMEPAD_BUTTON_NORTH]);
}

void latch_held(const PadInputs& inputs, std::uint8_t threshold, Latch& latch) {
    for (std::size_t b = 0; b < inputs.buttons.size(); ++b) {
        latch.buttons[b] = latch.buttons[b] || inputs.buttons[b];
    }
    for (const auto axis : {SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}) {
        latch.triggers[axis] = latch.triggers[axis] || trigger_byte(inputs.axes[axis]) >= threshold;
    }
}

void hold_back(PadInputs& inputs, std::uint8_t threshold, Latch& latch) {
    for (std::size_t b = 0; b < inputs.buttons.size(); ++b) {
        latch.buttons[b] = latch.buttons[b] && inputs.buttons[b];
        inputs.buttons[b] = inputs.buttons[b] && !latch.buttons[b];
    }
    for (const auto axis : {SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}) {
        latch.triggers[axis] = latch.triggers[axis] && trigger_byte(inputs.axes[axis]) >= threshold;
        if (latch.triggers[axis]) {
            inputs.axes[axis] = 0;
        }
    }
}

void latch_held_keys(std::span<const bool> keys, std::span<bool> latch) {
    for (std::size_t k = 0; k < latch.size(); ++k) {
        latch[k] = latch[k] || (k < keys.size() && keys[k]);
    }
}

void hold_back_keys(std::span<bool> keys, std::span<bool> latch) {
    for (std::size_t k = 0; k < latch.size(); ++k) {
        const bool held = k < keys.size() && keys[k];
        latch[k] = latch[k] && held;
        if (held && latch[k]) {
            keys[k] = false;
        }
    }
}

bool differ(const Bindings& a, const Bindings& b) {
    if (!(a.tuning == b.tuning)) {
        return true;
    }
    for (std::size_t t = 0; t < a.slots.size(); ++t) {
        const Bindings::Slot& x = a.slots[t];
        const Bindings::Slot& y = b.slots[t];
        if (x.key_count != y.key_count || x.pad_count != y.pad_count
            || !std::equal(x.keys.begin(), x.keys.begin() + x.key_count, y.keys.begin())
            || !std::equal(x.pad.begin(), x.pad.begin() + x.pad_count, y.pad.begin())) {
            return true;
        }
    }
    return false;
}

int target_of(Button button) {
    return button_target(button);
}

Binding parse_binding(std::string_view name) {
    if (name.empty()) {
        return {};
    }
    for (const PadName& pad : kPadNames) {
        if (name == pad.name) {
            return binding_of(pad);
        }
    }
    // The SDK's table names every key, and a name's value there is its HID usage, which is what an
    // SDL scancode is (input.h).
    const int key = openrac_bind_from_name_n(name.data(), name.size());
    if (key > 0 && key < static_cast<int>(SDL_SCANCODE_COUNT)) {
        return {Binding::Kind::Key, key, 0};
    }
    return {};
}

const char* binding_name(const Binding& binding) {
    if (binding.kind == Binding::Kind::Key) {
        const char* name = openrac_bind_to_name(binding.value);
        return name != nullptr ? name : "";
    }
    for (const PadName& pad : kPadNames) {
        if (binding_of(pad) == binding) {
            return pad.name;
        }
    }
    return "";
}

int target_of_key(std::string_view key) {
    for (const TargetKey& t : kTargetKeys) {
        if (key == t.key) {
            return t.target;
        }
    }
    return -1;
}

bool button_of_key(std::string_view key, Button& button) {
    const int target = target_of_key(key);
    if (target < 0 || !is_button(target)) {
        return false;
    }
    button = static_cast<Button>(1u << target);
    return true;
}

const char* const* binding_keys() {
    static const auto keys = [] {
        std::array<const char*, kTargetCount + 1> all{};
        for (int i = 0; i < kTargetCount; ++i) {
            all[static_cast<std::size_t>(i)] = kTargetKeys[i].key;
        }
        all[kTargetCount] = nullptr;
        return all;
    }();
    return keys.data();
}

const char* key_of_target(int target) {
    for (const TargetKey& t : kTargetKeys) {
        if (t.target == target) {
            return t.key;
        }
    }
    return "";
}

Bindings default_bindings() {
    Bindings bindings;
    for (const DefaultKey& d : kDefaultKeys) {
        Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(button_target(d.button))];
        slot.keys[static_cast<std::size_t>(slot.key_count++)] = d.key;
    }
    for (const DefaultPad& d : kDefaultPads) {
        Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(d.target)];
        slot.pad[static_cast<std::size_t>(slot.pad_count++)] = PadChord{parse_binding(d.name), {}};
    }
    return bindings;
}

void read_binding(std::string_view key, std::string_view value, int target, Bindings::Slot& slot) {
    if (trimmed(value) == kUnbound) {
        slot = Bindings::Slot{};
        return;
    }
    std::array<SDL_Scancode, kMostOfAKind> keys{};
    std::array<PadChord, kMostOfAKind> pad{};
    int key_count = 0;
    int pad_count = 0;
    for (std::size_t at = 0; at <= value.size();) {
        const std::size_t comma = value.find(',', at);
        const std::size_t end = comma == std::string_view::npos ? value.size() : comma;
        const std::string_view name = trimmed(value.substr(at, end - at));
        const PadChord read = name.empty() ? PadChord{} : read_name(key, value, name);
        if (read.input.kind == Binding::Kind::Key && !is_button(target)) {
            log::warn(
                "{} = {}: a stick's direction takes pad inputs only; {} is left out",
                key,
                value,
                std::string(name)
            );
        } else if (read.input.kind == Binding::Kind::Key) {
            const auto code = static_cast<SDL_Scancode>(read.input.value);
            const auto end_keys = keys.begin() + key_count;
            if (std::find(keys.begin(), end_keys, code) == end_keys && key_count < kMostOfAKind) {
                keys[static_cast<std::size_t>(key_count++)] = code;
            }
        } else if (read.input.kind != Binding::Kind::None) {
            const auto end_pad = pad.begin() + pad_count;
            if (std::find(pad.begin(), end_pad, read) == end_pad && pad_count < kMostOfAKind) {
                pad[static_cast<std::size_t>(pad_count++)] = read;
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

std::string binding_value(const Bindings::Slot& slot) {
    std::string out;
    const auto add = [&out](const std::string& name) {
        if (!out.empty()) {
            out += ',';
        }
        out += name;
    };
    for (int k = 0; k < slot.key_count; ++k) {
        const int code = static_cast<int>(slot.keys[static_cast<std::size_t>(k)]);
        add(binding_name({Binding::Kind::Key, code, 0}));
    }
    for (int p = 0; p < slot.pad_count; ++p) {
        const PadChord& chord = slot.pad[static_cast<std::size_t>(p)];
        std::string name = binding_name(chord.input);
        if (chord.with.kind != Binding::Kind::None) {
            name = std::string(binding_name(chord.with)) + "+" + name;
        }
        add(name);
    }
    return out.empty() ? std::string(kUnbound) : out;
}

Bindings compile_bindings(const std::map<std::string, std::string, std::less<>>& bindings) {
    Bindings out = default_bindings();
    for (const auto& [key, value] : bindings) {
        const int target = target_of_key(key);
        if (target >= 0) {
            read_binding(key, value, target, out.slots[static_cast<std::size_t>(target)]);
        } else if (key.starts_with(kTuningPrefix)) {
            read_tuning(key, value, out.tuning);
        }
    }
    return out;
}

void apply_keys(const Bindings& bindings, std::span<const bool> keys, Pad& pad) {
    for (int target = 0; target < 16; ++target) {
        const Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(target)];
        for (int k = 0; k < slot.key_count; ++k) {
            if (key(keys, slot.keys[static_cast<std::size_t>(k)])) {
                pad.set(static_cast<Button>(1u << target), true);
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
    // The gamepad's own sticks, squared as the DualShock 2's (pad.h): a round stick pushed to a
    // diagonal runs. Everything below reads them from here.
    // Each is tuned first (the player's dead zone, sensitivity and inversion), then shaped.
    Reading reading;
    reading.threshold = bindings.tuning.trigger_threshold;
    for (std::size_t s = 0; s < 2; ++s) {
        float& x = reading.sticks[2 * s];
        float& y = reading.sticks[2 * s + 1];
        x = static_cast<float>(inputs.axes[2 * s]) / 32768.0f;
        y = static_cast<float>(inputs.axes[2 * s + 1]) / 32768.0f;
        tune_stick(x, y, bindings.tuning.sticks[s]);
        shape_stick(x, y, gates[s]);
    }

    // A chord held takes its input away from that input's own bindings this frame: with L1+Cross
    // bound to Select, L1 and Cross pressed together are Select and L1, not Cross as well.
    std::array<Binding, static_cast<std::size_t>(kTargetCount * kMostOfAKind)> taken{};
    std::size_t taken_count = 0;
    for (const Bindings::Slot& slot : bindings.slots) {
        for (int p = 0; p < slot.pad_count; ++p) {
            const PadChord& chord = slot.pad[static_cast<std::size_t>(p)];
            if (chord.with.kind != Binding::Kind::None && held(chord.with, inputs, reading)
                && held(chord.input, inputs, reading)) {
                taken[taken_count++] = chord.input;
            }
        }
    }
    const auto taken_end = taken.begin() + static_cast<std::ptrdiff_t>(taken_count);

    // How far each target is pushed (0 to 1); a button held is pressed now.
    std::array<float, kTargetCount> push{};
    for (int target = 0; target < kTargetCount; ++target) {
        const Bindings::Slot& slot = bindings.slots[static_cast<std::size_t>(target)];
        float most = 0.0f;
        int pressure = -1;
        for (int p = 0; p < slot.pad_count; ++p) {
            const PadChord& chord = slot.pad[static_cast<std::size_t>(p)];
            const bool chord_off =
                chord.with.kind != Binding::Kind::None && !held(chord.with, inputs, reading);
            const bool lone_taken =
                chord.with.kind == Binding::Kind::None
                && std::find(taken.begin(), taken_end, chord.input) != taken_end;
            if (chord_off || lone_taken) {
                continue;
            }
            most = std::max(most, amount(chord.input, inputs, reading));
            if (held(chord.input, inputs, reading)) {
                pressure = std::max(pressure, pressure_of(chord.input, inputs, reading));
            }
        }
        push[static_cast<std::size_t>(target)] = most;
        if (is_button(target) && pressure >= 0) {
            pad.set(static_cast<Button>(1u << target), true, static_cast<std::uint8_t>(pressure));
        }
    }

    // Each stick's axis: how far it is pushed one way less how far the other.
    const auto axis = [&push](Target negative, Target positive) {
        return axis_byte(
            push[static_cast<std::size_t>(positive)] - push[static_cast<std::size_t>(negative)]
        );
    };
    pad.left_x = axis(Target::LeftStickLeft, Target::LeftStickRight);
    pad.left_y = axis(Target::LeftStickUp, Target::LeftStickDown);
    pad.right_x = axis(Target::RightStickLeft, Target::RightStickRight);
    pad.right_y = axis(Target::RightStickUp, Target::RightStickDown);
}

}  // namespace openrac::platform

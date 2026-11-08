// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Turns the game's DualShock 2 motor values into rumble for a modern controller.
//
// The game sets two motors through its pad library: a small one (on or off) and a large one
// (speed 0-255). Classic plays them as they are. Enhanced (the default) adds a short spin-up and
// fade to each motor, a stronger start so very short pulses are felt, a softer small motor, and a
// bit of the large motor in the high one. Optional: trigger motors, strength 0-200%.
//
// No SDL here, so tests/rumble_test.cpp tests it directly.

#pragma once

#include <cstdint>

namespace openrac::platform {

// How the game's motor commands are played.
enum class RumbleStyle : std::uint8_t {
    Enhanced,  // envelopes, kick and mix (above)
    Classic,   // the values as they are: large to the low motor, small on/off to the high one
};

// The player's rumble settings (the controls files' pad_rumble* keys, bindings.h).
struct RumbleTuning {
    int strength = 100;  // percent, 0 (off) to 200
    RumbleStyle style = RumbleStyle::Enhanced;
    bool triggers = false;  // also play it in the triggers' motors, where the pad has them
    bool phone = true;      // a phone or tablet without a gamepad vibrates itself (Android)

    bool operator==(const RumbleTuning&) const = default;
};

// What the game asks of a pad's two motors now.
struct MotorCommand {
    bool small = false;
    std::uint8_t large = 0;

    bool operator==(const MotorCommand&) const = default;
};

// What a gamepad plays, each 0..1.
struct RumbleOutput {
    float low = 0.0f;
    float high = 0.0f;
    float left_trigger = 0.0f;
    float right_trigger = 0.0f;

    bool silent() const {
        return low <= 0.0f && high <= 0.0f && left_trigger <= 0.0f && right_trigger <= 0.0f;
    }
};

// One pad's motors over time. step() is called once a frame with the game's command and the time
// since the last call.
class RumbleShaper {
public:
    RumbleOutput step(const MotorCommand& command, float seconds, const RumbleTuning& tuning);

    // Both motors at rest at once (a pad handed to another player, the game stopped).
    void reset();

    // Whether either motor is still turning (the envelope's tail included).
    bool moving() const { return m_low > 0.0f || m_high > 0.0f; }

    // The shaping's constants, in seconds and strengths (0..1). Public for the tests.
    static constexpr float kLowRise = 0.030f;   // large motor: spin-up time constant
    static constexpr float kLowFall = 0.080f;   // and coast-down
    static constexpr float kHighRise = 0.008f;  // small motor
    static constexpr float kHighFall = 0.035f;
    static constexpr float kKickTime = 0.040f;   // how long a start from rest is driven harder
    static constexpr float kKickGain = 1.6f;     // by how much
    static constexpr float kSmallLevel = 0.62f;  // the small motor next to a full large one
    static constexpr float kTexture = 0.18f;     // of the large motor's rumble in the high motor
    static constexpr float kTriggerLevel = 0.45f;
    // Below this an envelope's tail is cut to rest.
    static constexpr float kRest = 0.01f;

private:
    float m_low = 0.0f;  // the motors' envelopes, 0..1, before the player's strength
    float m_high = 0.0f;
    float m_low_kick = 0.0f;  // the kick's time left
    float m_high_kick = 0.0f;
};

// A 0..1 strength as SDL's 0..0xFFFF.
std::uint16_t rumble_word(float strength);

// Whether two outputs differ enough to be worth sending to the pad again (about 1% of a motor).
bool rumble_changed(const RumbleOutput& a, const RumbleOutput& b);

}  // namespace openrac::platform

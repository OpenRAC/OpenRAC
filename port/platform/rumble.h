// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The DualShock 2's two motors, as the game drives them, turned into what a modern gamepad plays.
//
// The game decides when and how hard the pad shakes, as on the console: its own code sets the
// motors through the pad library (libvib's sceVibSetActParam, libpad's scePadSetActDirect), and the
// port's replacement of that library hands the two values here (Pad::small_motor, large_motor). The
// DualShock 2 has
//
//   - a small motor, on or off: a light, fast buzz;
//   - a large motor with a speed 0..255: a heavy, slow rumble that takes a moment to spin up and
//     coasts on a little after it is let go.
//
// SDL's gamepads have a low- and a high-frequency motor with a strength each (some have motors in
// the triggers too). Playing the game's values straight onto them (Style::Classic) works, but
// feels cheap on today's pads: the small motor at full strength is a harsh rattle, a short pulse
// of the large motor ends in a hard cut, and a pulse of a frame or two never gets a small, light
// motor going at all. RumbleShaper (Style::Enhanced, the default) plays the same commands with
//
//   - an envelope for each motor: a quick spin-up and a short coast-down, as the DualShock 2's
//     weighted motors have, so a burst ends in a tail rather than a click;
//   - a kick on the way up: a motor that starts from rest is driven harder for its first few
//     milliseconds, so a one-frame pulse is felt;
//   - the small motor at the strength a DualShock 2's small motor has next to its large one, with
//     a little of the large motor's rumble in the high-frequency motor, which gives the heavy
//     rumble its texture on pads whose low motor alone feels dull;
//   - optionally the triggers (Xbox's impulse triggers): the buzz in both;
//   - the player's strength (0 off, up to 200%).
//
// Nothing here touches SDL, so the shaping is tested on its own (tests/rumble_test.cpp).

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

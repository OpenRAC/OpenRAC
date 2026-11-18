// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game's motor commands as a gamepad plays them (platform/rumble.h), and the rumble settings
// of the controls files (platform/bindings.h).

#include <cmath>

#include "platform/bindings.h"
#include "platform/rumble.h"
#include "tests/check.h"

namespace {

using openrac::platform::MotorCommand;
using openrac::platform::PadTuning;
using openrac::platform::RumbleOutput;
using openrac::platform::RumbleShaper;
using openrac::platform::RumbleStyle;
using openrac::platform::RumbleTuning;

constexpr float kFrame = 1.0f / 60.0f;

bool near(float a, float b, float within = 0.01f) {
    return std::fabs(a - b) <= within;
}

// Steps `shaper` with `command` for `frames` frames; the last output.
RumbleOutput run(RumbleShaper& shaper, MotorCommand command, int frames, const RumbleTuning& t) {
    RumbleOutput out;
    for (int i = 0; i < frames; ++i) {
        out = shaper.step(command, kFrame, t);
    }
    return out;
}

void classic_plays_the_values_as_they_are() {
    RumbleTuning t;
    t.style = RumbleStyle::Classic;
    RumbleShaper s;
    RumbleOutput out = s.step({true, 128}, kFrame, t);
    CHECK(near(out.low, 128.0f / 255.0f));
    CHECK(near(out.high, 1.0f));
    CHECK(out.left_trigger == 0.0f);
    out = s.step({}, kFrame, t);
    CHECK(out.silent());
    // Strength scales and clamps.
    t.strength = 50;
    out = s.step({false, 255}, kFrame, t);
    CHECK(near(out.low, 0.5f));
    CHECK(out.high == 0.0f);
    t.strength = 200;
    out = s.step({false, 200}, kFrame, t);
    CHECK(out.low == 1.0f);
}

void enhanced_kicks_a_motor_from_rest() {
    const RumbleTuning t;
    RumbleShaper s;
    // A one-frame pulse of a weak large motor is played harder than asked, so it is felt.
    const RumbleOutput first = s.step({false, 100}, kFrame, t);
    CHECK(first.low > 100.0f / 255.0f);
    CHECK(near(first.low, 100.0f / 255.0f * RumbleShaper::kKickGain));
    // Held on, the kick is over within kKickTime and the motor settles on its command.
    const RumbleOutput held = run(s, {false, 100}, 30, t);
    CHECK(near(held.low, 100.0f / 255.0f, 0.02f));
}

void enhanced_coasts_down_and_stops() {
    const RumbleTuning t;
    RumbleShaper s;
    run(s, {true, 255}, 30, t);
    // Let go: a tail, not a cut...
    const RumbleOutput tail = s.step({}, kFrame, t);
    CHECK(tail.low > 0.5f);
    CHECK(tail.low < 1.0f);
    // ...which ends at rest within half a second.
    const RumbleOutput later = run(s, {}, 30, t);
    CHECK(later.silent());
    CHECK(!s.moving());
}

void enhanced_mixes_the_motors() {
    const RumbleTuning t;
    RumbleShaper s;
    // The small motor at a DualShock 2's small motor's level, not the pad's full buzz.
    RumbleOutput out = run(s, {true, 0}, 30, t);
    CHECK(near(out.high, RumbleShaper::kSmallLevel, 0.02f));
    CHECK(out.low == 0.0f);
    s.reset();
    // The large motor alone gives the high motor some texture.
    out = run(s, {false, 255}, 30, t);
    CHECK(near(out.low, 1.0f, 0.02f));
    CHECK(near(out.high, RumbleShaper::kTexture, 0.02f));
    // No triggers unless asked for.
    CHECK(out.left_trigger == 0.0f && out.right_trigger == 0.0f);
}

void triggers_and_strength() {
    RumbleTuning t;
    t.triggers = true;
    RumbleShaper s;
    RumbleOutput out = run(s, {true, 0}, 30, t);
    CHECK(out.left_trigger > 0.3f);
    CHECK(out.left_trigger == out.right_trigger);
    t.strength = 0;
    out = s.step({true, 255}, kFrame, t);
    CHECK(out.silent());
}

void a_long_frame_is_one_short_step() {
    const RumbleTuning t;
    RumbleShaper s;
    run(s, {false, 255}, 30, t);
    // A two-second frame (a load) is a tenth of a second of coast-down, not a jump to rest.
    const RumbleOutput out = s.step({}, 2.0f, t);
    CHECK(out.low > 0.1f);
}

void outputs_are_sent_when_they_change() {
    RumbleOutput a;
    RumbleOutput b;
    CHECK(!openrac::platform::rumble_changed(a, b));
    b.low = 0.005f;
    CHECK(openrac::platform::rumble_changed(a, b));  // silent to moving
    a.low = 0.5f;
    b.low = 0.505f;
    CHECK(!openrac::platform::rumble_changed(a, b));
    b.high = 0.2f;
    CHECK(openrac::platform::rumble_changed(a, b));
    CHECK(openrac::platform::rumble_word(1.0f) == 0xFFFF);
    CHECK(openrac::platform::rumble_word(0.0f) == 0);
    CHECK(openrac::platform::rumble_word(2.0f) == 0xFFFF);
}

void settings_keys() {
    PadTuning tuning;
    CHECK(openrac::platform::read_tuning("pad_rumble", "150", tuning));
    CHECK(tuning.rumble.strength == 150);
    CHECK(!openrac::platform::read_tuning("pad_rumble", "250", tuning));
    CHECK(tuning.rumble.strength == 150);
    CHECK(openrac::platform::read_tuning("pad_rumble_style", "classic", tuning));
    CHECK(tuning.rumble.style == RumbleStyle::Classic);
    CHECK(!openrac::platform::read_tuning("pad_rumble_style", "loud", tuning));
    CHECK(openrac::platform::read_tuning("pad_rumble_triggers", "on", tuning));
    CHECK(tuning.rumble.triggers);
    CHECK(openrac::platform::read_tuning("pad_rumble_phone", "0", tuning));
    CHECK(!tuning.rumble.phone);
    const auto values = openrac::platform::tuning_values(tuning);
    CHECK(values.at("pad_rumble") == "150");
    CHECK(values.at("pad_rumble_style") == "classic");
    CHECK(values.at("pad_rumble_triggers") == "1");
    CHECK(values.at("pad_rumble_phone") == "0");
    const auto defaults = openrac::platform::tuning_values(PadTuning{});
    CHECK(defaults.at("pad_rumble") == "100");
    CHECK(defaults.at("pad_rumble_style") == "enhanced");
}

}  // namespace

int main() {
    classic_plays_the_values_as_they_are();
    enhanced_kicks_a_motor_from_rest();
    enhanced_coasts_down_and_stops();
    enhanced_mixes_the_motors();
    triggers_and_strength();
    a_long_frame_is_one_short_step();
    outputs_are_sent_when_they_change();
    settings_keys();
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Gyro aiming (platform/gyro.h): a turn of the controller turns the camera by as much times the
// sensitivity; the resting offset is learnt while it lies still, not while it turns slowly; the
// spaces pick the right twist; the button and the dead zone gate it.
#include <cmath>
#include <cstdint>

#include "platform/gyro.h"
#include "tests/check.h"

namespace {

using openrac::platform::GyroAim;
using openrac::platform::GyroButtonAction;
using openrac::platform::GyroOptions;
using openrac::platform::GyroSpace;
using openrac::platform::Vec3f;

constexpr std::uint64_t kStepNs = 4'000'000;  // 250 samples a second, a DualShock 4's rate
constexpr float kStep = 0.004f;

bool near(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) <= eps;
}

GyroOptions on() {
    GyroOptions o;
    o.enabled = true;
    o.sensitivity_x = 1.0f;
    o.sensitivity_y = 1.0f;
    o.auto_calibrate = false;
    return o;
}

// `n` samples of `rate` from `ns` on.
std::uint64_t feed(GyroAim& g, Vec3f rate, int n, std::uint64_t ns) {
    for (int i = 0; i < n; ++i) {
        ns += kStepNs;
        g.gyro(rate, ns);
    }
    return ns;
}

void test_turn_is_the_controllers() {
    GyroAim g;
    GyroOptions o = on();
    o.space = GyroSpace::Yaw;
    g.set_options(o);
    // A quarter turn left in a second: the camera turns a quarter turn left.
    const float rate = static_cast<float>(M_PI) / 2.0f;
    feed(g, {0.0f, rate, 0.0f}, 251, 0);  // the first sample only starts the clock
    float yaw = 0.0f;
    float pitch = 0.0f;
    g.take(yaw, pitch);
    CHECK(near(yaw, rate, 0.01f));
    CHECK(near(pitch, 0.0f));
    // Taken: nothing more until it moves.
    g.take(yaw, pitch);
    CHECK(yaw == 0.0f);

    // Sensitivity 2 and inverted up and down.
    o.sensitivity_y = 2.0f;
    o.invert_y = true;
    g.set_options(o);
    feed(g, {0.5f, 0.0f, 0.0f}, 250, 10'000'000'000ull);
    g.take(yaw, pitch);
    CHECK(near(pitch, -2.0f * 0.5f * 249.0f * kStep, 0.01f));
}

void test_gap_is_not_motion() {
    GyroAim g;
    g.set_options(on());
    std::uint64_t ns = feed(g, {0.0f, 1.0f, 0.0f}, 2, 0);
    float yaw = 0.0f;
    float pitch = 0.0f;
    g.take(yaw, pitch);
    // A second without samples (a lost link), then one: no turn for the gap.
    g.gyro({0.0f, 1.0f, 0.0f}, ns + 1'000'000'000ull);
    g.take(yaw, pitch);
    CHECK(yaw == 0.0f);
}

void test_calibrates_when_still() {
    GyroAim g;
    GyroOptions o = on();
    o.auto_calibrate = true;
    o.space = GyroSpace::Yaw;
    g.set_options(o);
    // Lying on the desk with a drift of 0.03 rad/s on y, and a little noise.
    std::uint64_t ns = 0;
    for (int i = 0; i < 500; ++i) {
        const float noise = (i % 2 == 0 ? 1.0f : -1.0f) * 0.004f;
        ns += kStepNs;
        g.gyro({noise, 0.03f + noise, -noise}, ns);
    }
    CHECK(g.calibrated());
    CHECK(near(g.offset().y, 0.03f, 0.002f));
    float yaw = 0.0f;
    float pitch = 0.0f;
    g.take(yaw, pitch);
    // Still drifting: now it barely turns.
    feed(g, {0.0f, 0.03f, 0.0f}, 250, ns);
    g.take(yaw, pitch);
    CHECK(std::fabs(yaw) < 0.002f);
}

void test_shake_or_turn_is_not_still() {
    GyroAim g;
    GyroOptions o = on();
    o.auto_calibrate = true;
    g.set_options(o);
    // A slow but steady turn (0.2 rad/s, over kMaxOffset) is not learnt as drift.
    feed(g, {0.0f, 0.2f, 0.0f}, 1000, 0);
    CHECK(!g.calibrated());
    // Nor is a hand's shake.
    GyroAim h;
    h.set_options(o);
    std::uint64_t ns = 0;
    for (int i = 0; i < 1000; ++i) {
        ns += kStepNs;
        h.gyro({0.0f, (i % 10 < 5) ? 0.05f : -0.05f, 0.0f}, ns);
    }
    CHECK(!h.calibrated());
}

void test_spaces() {
    float yaw = 0.0f;
    float pitch = 0.0f;
    // Roll: leaning the controller left (counter-clockwise about z) turns left.
    GyroAim roll;
    GyroOptions o = on();
    o.space = GyroSpace::Roll;
    roll.set_options(o);
    feed(roll, {0.0f, 0.0f, 1.0f}, 101, 0);
    roll.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));

    // Player space, the controller stood on its end (up is its z): its z twist is the turn.
    GyroAim player;
    o.space = GyroSpace::Player;
    player.set_options(o);
    player.accel({0.0f, 0.0f, 9.8f});
    feed(player, {0.0f, 0.0f, 1.0f}, 101, 0);
    player.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));
    // Held flat, the same twist is a roll, not a turn.
    GyroAim flat;
    flat.set_options(o);
    flat.accel({0.0f, 9.8f, 0.0f});
    feed(flat, {0.0f, 0.0f, 1.0f}, 101, 0);
    flat.take(yaw, pitch);
    CHECK(near(yaw, 0.0f));
    // Held at 45 degrees, the relaxed turn still gives (nearly) the whole of it.
    GyroAim tilted;
    tilted.set_options(o);
    tilted.accel({0.0f, 6.93f, 6.93f});
    feed(tilted, {0.0f, 0.7071f, 0.7071f}, 101, 0);
    tilted.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));
    // No accelerometer: as Yaw.
    GyroAim none;
    none.set_options(o);
    feed(none, {0.0f, 1.0f, 0.0f}, 101, 0);
    none.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));
}

void test_button() {
    float yaw = 0.0f;
    float pitch = 0.0f;
    GyroOptions o = on();
    o.button = 15;  // any button
    o.button_action = GyroButtonAction::Enable;
    GyroAim g;
    g.set_options(o);
    g.button(false);
    feed(g, {0.0f, 1.0f, 0.0f}, 101, 0);
    g.take(yaw, pitch);
    CHECK(yaw == 0.0f);
    g.button(true);
    feed(g, {0.0f, 1.0f, 0.0f}, 101, 1'000'000'000ull);
    g.take(yaw, pitch);
    CHECK(yaw > 0.3f);

    // Pause: off while held.
    o.button_action = GyroButtonAction::Pause;
    g.set_options(o);
    g.button(true);
    feed(g, {0.0f, 1.0f, 0.0f}, 101, 2'000'000'000ull);
    g.take(yaw, pitch);
    CHECK(yaw == 0.0f);

    // Toggle: each press flips it.
    o.button_action = GyroButtonAction::Toggle;
    GyroAim t;
    t.set_options(o);
    t.button(false);
    CHECK(!t.aiming());
    t.button(true);
    CHECK(t.aiming());
    t.button(true);
    CHECK(t.aiming());
    t.button(false);
    t.button(true);
    CHECK(!t.aiming());

    // Off in the options: nothing.
    GyroAim off;
    off.set_options(GyroOptions{});
    feed(off, {0.0f, 1.0f, 0.0f}, 101, 0);
    off.take(yaw, pitch);
    CHECK(yaw == 0.0f);
}

void test_dead_zone_and_smoothing() {
    float yaw = 0.0f;
    float pitch = 0.0f;
    GyroOptions o = on();
    o.space = GyroSpace::Yaw;
    o.dead_zone = 0.05f;
    GyroAim g;
    g.set_options(o);
    feed(g, {0.0f, 0.04f, 0.0f}, 101, 0);
    g.take(yaw, pitch);
    CHECK(yaw == 0.0f);
    feed(g, {0.0f, 1.05f, 0.0f}, 101, 1'000'000'000ull);
    g.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));

    // Smoothing: a fast turn goes straight through, a slow one comes out the same in the end.
    o.dead_zone = 0.0f;
    o.smoothing = 0.2f;
    GyroAim s;
    s.set_options(o);
    feed(s, {0.0f, 1.0f, 0.0f}, 101, 0);
    s.take(yaw, pitch);
    CHECK(near(yaw, 0.4f, 0.01f));
    feed(s, {0.0f, 0.05f, 0.0f}, 501, 1'000'000'000ull);
    s.take(yaw, pitch);
    CHECK(near(yaw, 0.1f, 0.01f));
}

void test_parse() {
    CHECK(openrac::platform::gyro_space_from("roll") == GyroSpace::Roll);
    CHECK(openrac::platform::gyro_space_from("yaw") == GyroSpace::Yaw);
    CHECK(openrac::platform::gyro_space_from("local") == GyroSpace::Local);
    CHECK(openrac::platform::gyro_space_from("") == GyroSpace::Player);
    CHECK(openrac::platform::gyro_button_action_from("pause") == GyroButtonAction::Pause);
    CHECK(openrac::platform::gyro_button_action_from("x") == GyroButtonAction::Enable);
}

}  // namespace

int main() {
    test_turn_is_the_controllers();
    test_gap_is_not_motion();
    test_calibrates_when_still();
    test_shake_or_turn_is_not_still();
    test_spaces();
    test_button();
    test_dead_zone_and_smoothing();
    test_parse();
    return openrac::test::result();
}

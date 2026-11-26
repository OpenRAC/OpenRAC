// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Gyro aiming: a controller's (or a phone's) gyroscope turned into a turn of the camera, in
// radians. It reads the sensor itself, not a mouse that a driver makes of it, so it has no screen
// edge, no cursor and no mouse acceleration, and it works beside the gamepad's sticks.
//
// Each sample goes through: the gyro's resting offset (learnt while the controller lies still),
// the space (which twist turns the camera), a dead zone, a soft smoothing of the smallest motion
// (hand shake), then the sensitivity. The camera turns by what comes out: at sensitivity 1, as far
// as the controller turned. SDL's axes (SDL_sensor.h): x right, y up, z toward the player; a turn
// counter-clockwise about an axis is positive.
//
// This is only input. The game's camera code then reads it as it reads the mouse
// (docs/port/KEYBOARD_MOUSE.md, "Gyro").

#pragma once

#include <cstdint>

namespace openrac::platform {

// Which of the controller's turns move the camera left and right.
enum class GyroSpace {
    Player,  // the turn about the way up, however the controller is held (needs its accelerometer)
    Yaw,     // the turn about the controller's own up axis (held flat)
    Roll,    // leaning the controller left and right, like a wheel
    Local    // both of the above added together
};

// What the gyro button does.
enum class GyroButtonAction {
    Enable,  // the gyro aims only while it is held
    Pause,   // the gyro aims except while it is held (lift it to re-centre)
    Toggle   // each press turns the gyro on or off
};

struct GyroOptions {
    bool enabled = false;
    GyroSpace space = GyroSpace::Player;
    float sensitivity_x = 2.0f;  // camera radians per controller radian, left and right
    float sensitivity_y = 2.0f;  // and up and down
    bool invert_x = false;
    bool invert_y = false;
    float dead_zone = 0.0f;  // radians a second below which the gyro is ignored
    float smoothing = 0.0f;  // radians a second below which motion is smoothed (0: none)
    bool auto_calibrate = true;
    int button = -1;  // SDL_GamepadButton, -1 none
    GyroButtonAction button_action = GyroButtonAction::Enable;
};

// One parse of a setting's text, "" or unknown giving the default.
GyroSpace gyro_space_from(const char* text);
GyroButtonAction gyro_button_action_from(const char* text);

struct Vec3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};

// The gyro's filter, with no SDL in it (tests/gyro_test.cpp).
class GyroAim {
public:
    // How still the controller must be, and for how long, to learn its offset: the spread of each
    // axis under kStillSpread for kStillSeconds, the mean under kMaxOffset (a gyro's drift is
    // small; a slow steady turn is not drift).
    static constexpr float kStillSpread = 0.02f;  // radians a second (about 1.1 degrees)
    static constexpr float kStillSeconds = 1.5f;
    static constexpr float kMaxOffset = 0.1f;            // radians a second
    static constexpr float kStillAccelSpread = 0.4f;     // m/s^2
    static constexpr float kSmoothingSeconds = 0.0625f;  // the smoothed part's time constant
    static constexpr float kGravitySeconds = 0.25f;      // the accelerometer's low pass
    // Player space: how much of the twist off the way up still counts (GyroWiki's 1.41).
    static constexpr float kRelax = 1.41f;
    // A gap between samples longer than this is not motion (a sleep, a lost link).
    static constexpr float kMaxStep = 0.05f;

    void set_options(const GyroOptions& options) { m_options = options; }

    const GyroOptions& options() const { return m_options; }

    // A gyro sample (radians a second, SDL's axes) at `ns` (the sensor's own clock).
    void gyro(Vec3f rate, std::uint64_t ns);
    // An accelerometer sample (m/s^2, SDL's axes): the way up, for the player space.
    void accel(Vec3f a);

    // Whether the gyro aims now: on in the options and by the button. `held` is the button now.
    void button(bool held);

    bool aiming() const { return m_options.enabled && m_button_on; }

    // The turn gathered since the last take: yaw > 0 left, pitch > 0 up, radians.
    void take(float& yaw, float& pitch) {
        yaw = m_yaw;
        pitch = m_pitch;
        m_yaw = m_pitch = 0.0f;
    }

    void peek(float& yaw, float& pitch) const {
        yaw = m_yaw;
        pitch = m_pitch;
    }

    // A new controller, or none: all learnt state starts over.
    void reset();

    bool calibrated() const { return m_has_offset; }

    Vec3f offset() const { return m_offset; }

    bool has_gravity() const { return m_has_gravity; }

private:
    void learn_offset(Vec3f rate, Vec3f accel, float dt);

    GyroOptions m_options;
    bool m_button_on = true;
    bool m_button_was = false;
    bool m_toggled = false;

    std::uint64_t m_last_ns = 0;
    bool m_has_last = false;

    Vec3f m_offset;
    bool m_has_offset = false;
    // The still window: its sums, spreads and length.
    Vec3f m_sum, m_min, m_max, m_accel_min, m_accel_max;
    int m_count = 0;
    float m_still = 0.0f;

    Vec3f m_gravity;
    bool m_has_gravity = false;
    Vec3f m_last_accel;

    float m_smooth_yaw = 0.0f;
    float m_smooth_pitch = 0.0f;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};

}  // namespace openrac::platform

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Gyro aiming as platform/input.h reads it from SDL, against SDL's virtual gamepads: player 1's
// gamepad's gyro read only while gyro aiming is on, its button, a Joy-Con's own gyro, a gamepad
// with none, and controllers coming and going.
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <SDL3/SDL.h>

#include "platform/input.h"
#include "tests/check.h"

namespace {

using openrac::platform::GyroButtonAction;
using openrac::platform::GyroOptions;
using openrac::platform::GyroSpace;
using openrac::platform::Input;

// A gamepad with the sensors given (none: no gyro).
SDL_JoystickID attach(const char* name, SDL_SensorType gyro, SDL_SensorType accel) {
    static SDL_VirtualJoystickSensorDesc sensors[2];
    int count = 0;
    if (gyro != SDL_SENSOR_INVALID) {
        sensors[count++] = {gyro, 250.0f};
    }
    if (accel != SDL_SENSOR_INVALID) {
        sensors[count++] = {accel, 250.0f};
    }
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.nsensors = static_cast<Uint16>(count);
    desc.sensors = count > 0 ? sensors : nullptr;
    desc.name = name;
    return SDL_AttachVirtualJoystick(&desc);
}

void pump(Input& input) {
    SDL_UpdateJoysticks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input.handle_event(event);
    }
}

std::uint64_t g_sensor_ns = 1'000'000'000;

// `n` gyro samples of `rate` (rad/s) 4 ms apart, then the tick's read.
void turn(Input& input, SDL_JoystickID id, SDL_SensorType type, float rate_y, int n) {
    SDL_Joystick* j = SDL_GetJoystickFromID(id);
    for (int i = 0; i < n; ++i) {
        g_sensor_ns += 4'000'000;
        const float data[3] = {0.0f, rate_y, 0.0f};
        SDL_SendJoystickVirtualSensorData(j, type, g_sensor_ns, data, 3);
        if (i % 16 == 15) {
            pump(input);
        }
    }
    pump(input);
    input.update();
}

GyroOptions on() {
    GyroOptions o;
    o.enabled = true;
    o.space = GyroSpace::Yaw;
    o.sensitivity_x = 1.0f;
    o.sensitivity_y = 1.0f;
    o.auto_calibrate = false;
    return o;
}

bool near(float a, float b, float eps) {
    return std::fabs(a - b) <= eps;
}

void test_reads_player_ones_gyro() {
    Input input;
    const SDL_JoystickID pad = attach("DualSense", SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL);
    pump(input);
    input.update();
    // Off: no gyro read, no turn.
    CHECK(!input.gyro_available());
    turn(input, pad, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(input.gyro_yaw() == 0.0f);

    // On: 0.4 s at 1 rad/s left (the first sample only starts the clock).
    input.set_gyro_options(on());
    pump(input);
    CHECK(input.gyro_available());
    turn(input, pad, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(near(input.gyro_yaw(), 0.4f, 0.02f));
    // Read once: the next tick has nothing new.
    input.update();
    CHECK(input.gyro_yaw() == 0.0f);

    // Off again: nothing, even as samples would come.
    GyroOptions off = on();
    off.enabled = false;
    input.set_gyro_options(off);
    CHECK(!input.gyro_available());
    turn(input, pad, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(input.gyro_yaw() == 0.0f);

    SDL_DetachVirtualJoystick(pad);
    pump(input);
    input.update();
}

void test_button() {
    Input input;
    const SDL_JoystickID pad = attach("DualShock 4", SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL);
    pump(input);
    GyroOptions o = on();
    o.button = SDL_GAMEPAD_BUTTON_TOUCHPAD;
    o.button_action = GyroButtonAction::Enable;
    input.set_gyro_options(o);
    pump(input);
    input.update();
    turn(input, pad, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(input.gyro_yaw() == 0.0f);  // not held
    SDL_SetJoystickVirtualButton(SDL_GetJoystickFromID(pad), SDL_GAMEPAD_BUTTON_TOUCHPAD, true);
    pump(input);
    input.update();
    turn(input, pad, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(near(input.gyro_yaw(), 0.4f, 0.02f));
    SDL_SetJoystickVirtualButton(SDL_GetJoystickFromID(pad), SDL_GAMEPAD_BUTTON_TOUCHPAD, false);
    SDL_DetachVirtualJoystick(pad);
    pump(input);
    input.update();
}

void test_joycon_and_none() {
    Input input;
    input.set_gyro_options(on());
    // No gamepad: nothing read, and no crash on a tick.
    input.update();
    CHECK(!input.gyro_available());

    // A gamepad without a gyro (an Xbox pad): still none.
    const SDL_JoystickID xbox = attach("Xbox", SDL_SENSOR_INVALID, SDL_SENSOR_INVALID);
    pump(input);
    input.update();
    CHECK(!input.gyro_available());
    SDL_DetachVirtualJoystick(xbox);
    pump(input);
    input.update();

    // A right Joy-Con alone: its own gyro.
    const SDL_JoystickID joycon = attach("Joy-Con (R)", SDL_SENSOR_GYRO_R, SDL_SENSOR_ACCEL_R);
    pump(input);
    input.update();
    CHECK(input.gyro_available());
    turn(input, joycon, SDL_SENSOR_GYRO_R, -1.0f, 101);
    CHECK(near(input.gyro_yaw(), -0.4f, 0.02f));

    // Unplugged mid-turn: no gyro, no turn, no crash.
    SDL_Joystick* j = SDL_GetJoystickFromID(joycon);
    const float data[3] = {0.0f, 1.0f, 0.0f};
    SDL_SendJoystickVirtualSensorData(j, SDL_SENSOR_GYRO_R, g_sensor_ns + 4'000'000, data, 3);
    SDL_DetachVirtualJoystick(joycon);
    pump(input);
    input.update();
    CHECK(!input.gyro_available());
    CHECK(input.gyro_yaw() == 0.0f);

    // Plugged in again: read again.
    const SDL_JoystickID again = attach("DualSense", SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL);
    pump(input);
    input.update();
    CHECK(input.gyro_available());
    turn(input, again, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(near(input.gyro_yaw(), 0.4f, 0.02f));
    SDL_DetachVirtualJoystick(again);
    pump(input);
    input.update();
}

// Two players: only player 1's gamepad aims, even when only player 2's has a gyro.
void test_player_two() {
    Input input;
    input.set_gyro_options(on());
    const SDL_JoystickID one = attach("Xbox", SDL_SENSOR_INVALID, SDL_SENSOR_INVALID);
    pump(input);
    const SDL_JoystickID two = attach("DualSense", SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL);
    pump(input);
    input.update();
    CHECK(!input.gyro_available());
    turn(input, two, SDL_SENSOR_GYRO, 1.0f, 101);
    CHECK(input.gyro_yaw() == 0.0f);
    SDL_DetachVirtualJoystick(one);
    SDL_DetachVirtualJoystick(two);
    pump(input);
    input.update();
}

}  // namespace

int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    test_reads_player_ones_gyro();
    test_button();
    test_joycon_and_none();
    test_player_two();
    SDL_Quit();
    return openrac::test::result();
}

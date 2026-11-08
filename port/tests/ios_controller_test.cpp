// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "ios/controller_input.h"
#include "tests/check.h"

int main() {
    openrac::ios::ControllerInput input;
    constexpr uint32_t accept = 1u << 4;
    constexpr uint32_t right = 1u << 3;
    // Opening the launcher with Cross held must not start a game.
    CHECK(input.update(accept, true).down == 0);
    CHECK(input.update(accept, true).down == 0);
    input.update(0, true);
    CHECK(input.update(accept, true).down == accept);
    CHECK(input.update(accept, true).down == 0);
    CHECK(input.update(0, true).up == accept);
    // D-pad, stick and multiple controllers are merged before edge detection.
    CHECK(input.update(right, true).down == right);
    CHECK(input.update(right, true).up == 0);
    CHECK(input.update(0, true).up == right);  // Includes controller disconnection.
    // A system dialog/background transition releases the JavaScript repeat immediately.
    input.update(right, true);
    CHECK(input.update(right | accept, false).up == right);
    CHECK(input.update(right | accept, true).down == 0);
    CHECK(input.update(right | accept, true).down == 0);
    input.update(0, true);
    CHECK(input.update(accept, true).down == accept);
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The built-in keyboard and mouse's logic (game/common/keyboard_mouse/kbm.hpp) alone: binds read
// from settings, conflicts, the keys' stick, the mouse's stick, the Quick Select's direction, the report the game reads and
// the weapon wheel; and the manifest's keys and defaults against the action table.
//
// Expected values are worked out by hand from the behaviour docs/port/KEYBOARD_MOUSE.md and openrac-mod.json
// describe, and from the game's own stick and Quick Select reading (as kbm.hpp quotes it).
//
// The manifest is the one beside kbm.hpp (KBM_MANIFEST, set by cmake/Window.cmake).
#include <cmath>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "game/common/keyboard_mouse/kbm.hpp"

namespace {

int g_failures = 0;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #c);                    \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (0)

bool near(float a, float b, float within = 0.02f) {
    return std::fabs(a - b) <= within;
}

using kbm::Action;

void parsing() {
    kbm::Binds b;
    CHECK(kbm::parse_binds("KeyW", b) && b[0] == 26 && b[1] == 0);
    CHECK(kbm::parse_binds("KeyW,ArrowUp", b) && b[0] == 26 && b[1] == 82);
    CHECK(kbm::parse_binds(" KeyW , ArrowUp ", b) && b[0] == 26 && b[1] == 82);
    CHECK(kbm::parse_binds("", b) && b[0] == 0 && b[1] == 0);
    CHECK(
        kbm::parse_binds("MouseRight,WheelDown", b) && b[0] == OPENRAC_BIND_MOUSE_RIGHT
        && b[1] == OPENRAC_BIND_WHEEL_DOWN
    );
    CHECK(!kbm::parse_binds("KeyW,KeyW", b) && b[0] == 0);
    CHECK(!kbm::parse_binds("KeyW,KeyA,KeyS", b));
    CHECK(!kbm::parse_binds("Spacebar", b));
    CHECK(!kbm::parse_binds("keyw", b));  // names are as KeyboardEvent.code has them

    // A setting the mod cannot read is its default; the rest as written.
    std::map<std::string, std::string> settings =
        {{"jump", "KeyJ"}, {"fire", "NotAKey"}, {"walk", ""}};
    const kbm::Bindings bindings = kbm::read_bindings([&](const char* key) {
        const auto it = settings.find(key);
        return it == settings.end() ? std::string("?") : it->second;
    });
    CHECK(bindings.of(Action::Jump)[0] == 13);                       // J
    CHECK(bindings.of(Action::Fire)[0] == OPENRAC_BIND_MOUSE_LEFT);  // its default
    CHECK(bindings.of(Action::Walk)[0] == 0);                        // unbound on purpose
    CHECK(bindings.of(Action::MoveForward)[0] == 26);                // "?": the default, W
}

void conflicts() {
    // The defaults conflict nowhere.
    const kbm::Bindings defaults = kbm::read_bindings([](const char*) { return std::string("?"); });
    CHECK(kbm::conflicts(defaults).empty());
    // The same key on two play actions is a conflict; on a play and a menu action it is not.
    kbm::Bindings b = defaults;
    b.binds[static_cast<std::size_t>(Action::CameraReset)] = {9, 0};  // F, the wrench's too
    const auto found = kbm::conflicts(b);
    CHECK(
        found.size() == 1 && found[0].a == Action::Wrench && found[0].b == Action::CameraReset
        && found[0].bind == 9
    );
    b = defaults;
    b.binds[static_cast<std::size_t>(Action::MenuBack)] = {41, 0};  // Escape, also Pause (play)
    CHECK(kbm::conflicts(b).empty());

    // Every default's name is one the table knows.
    for (const kbm::ActionInfo& a : kbm::kActions) {
        kbm::Binds parsed;
        CHECK(kbm::parse_binds(a.fallback, parsed) && parsed[0] != 0);
    }
}

void key_stick() {
    kbm::MoveTuning t;
    t.rise_ticks = 4.0f;
    kbm::KeyStick s;
    // W held: up (y < 0), a quarter more each tick, full after four.
    s.step(true, false, false, false, false, t);
    CHECK(near(s.y, -0.25f) && s.x == 0.0f);

    for (int i = 0; i < 5; ++i) {
        s.step(true, false, false, false, false, t);
    }

    CHECK(near(s.y, -1.0f));

    // W and D: the diagonal on the circle.
    for (int i = 0; i < 8; ++i) {
        s.step(true, false, false, true, false, t);
    }

    CHECK(near(s.x, 0.7071f) && near(s.y, -0.7071f));
    CHECK(near(std::sqrt(s.x * s.x + s.y * s.y), 1.0f));

    // Walking: half way.
    for (int i = 0; i < 8; ++i) {
        s.step(true, false, false, false, true, t);
    }

    CHECK(near(s.y, -0.5f) && near(s.x, 0.0f));
    // Let go: back twice as fast as it rose (from 0.5, at 0.5 a tick: at once).
    s.step(false, false, false, false, false, t);
    CHECK(s.x == 0.0f && s.y == 0.0f);
    // No ramp: full at once. Opposite keys cancel.
    t.rise_ticks = 0.0f;
    s.step(false, true, false, false, false, t);
    CHECK(s.y == 1.0f);
    s.step(true, true, false, false, false, t);
    CHECK(s.y == 0.0f);
}

void mouse_stick() {
    kbm::LookTuning t;
    kbm::MouseLook m;
    // A tick's motion at sensitivity 1: eased over two ticks, then proportional.
    m.step(12.0f, 0.0f, false, t);
    CHECK(near(m.x, 0.25f));
    m.step(12.0f, 0.0f, false, t);
    CHECK(near(m.x, 0.5f));
    m.step(100.0f, 0.0f, false, t);
    CHECK(m.x == 1.0f);  // clamped
    m.step(0.0f, 0.0f, false, t);
    m.step(0.0f, 0.0f, false, t);
    CHECK(m.x == 0.0f);  // the mouse at rest: the camera stops
    m.step(-24.0f, 0.0f, false, t);
    m.step(-24.0f, 0.0f, false, t);
    CHECK(near(m.x, -1.0f));
    // Sensitivity doubles it.
    t.sensitivity = 2.0f;
    m.reset();
    m.step(6.0f, 0.0f, false, t);
    m.step(6.0f, 0.0f, false, t);
    CHECK(near(m.x, 0.5f));
    // No curve: twice the motion is twice the stick.
    t.sensitivity = 1.0f;
    kbm::MouseLook slow, fast;
    slow.step(2.0f, 0.0f, false, t);
    slow.step(2.0f, 0.0f, false, t);
    fast.step(4.0f, 0.0f, false, t);
    fast.step(4.0f, 0.0f, false, t);
    CHECK(near(fast.x, 2.0f * slow.x));
    // The tilt holds where the mouse left it; up (dy < 0) is a stick pushed up.
    m.reset();
    m.step(0.0f, -80.0f, false, t);
    CHECK(near(m.y, -0.5f));
    m.step(0.0f, 0.0f, false, t);
    CHECK(near(m.y, -0.5f));
    m.step(0.0f, -400.0f, false, t);
    CHECK(m.y == -1.0f);
    // Inverted.
    t.invert_y = true;
    m.reset();
    m.step(0.0f, -80.0f, false, t);
    CHECK(near(m.y, 0.5f));
    t.invert_y = false;
    // Levelling out when it rests.
    t.pitch_holds = false;
    m.reset();
    m.step(0.0f, 80.0f, false, t);

    for (int i = 0; i < 60; ++i) {
        m.step(0.0f, 0.0f, false, t);
    }

    CHECK(m.y == 0.0f);
    // First person: the tilt is a rate too.
    m.reset();
    m.step(0.0f, 12.0f, true, t);
    CHECK(near(m.y, 0.5f));
    m.step(0.0f, 0.0f, true, t);
    CHECK(m.y == 0.0f);
}

// The game's FollowCamera_ReadStickInput and Cam_SteerHeadingFromStick from the stick's y to the
// eased tilt target w: the option's invert, the resting fallback, the dead zone.
float game_follow_target(float stick_y, bool game_inverts, bool* fallback) {
    float b = game_inverts ? -stick_y : stick_y;
    *fallback = b == 0.0f;

    if (b < -0.3f) {
        b = (b + 0.3f) * 1.4285715f;
    } else if (0.3f < b) {
        b = (b - 0.3f) * 1.4285715f;
    } else {
        b = 0.0f;
    }

    return b;
}

// The view's direction from the follow camera's orbit (Ratchet to the eye): the other way.
kbm::Vec3 view_of(kbm::Vec3 orbit) {
    return kbm::scaled(orbit, -1.0f);
}

void direct_look() {
    using kbm::Vec3;
    kbm::LookTuning t;
    const Vec3 up = kbm::kWorldUp;

    // In proportion to the motion, at once: no easing over ticks, no ramp, the same each tick.
    const float one = kbm::DirectLook::yaw(10.0f, 0.0f, t);
    CHECK(near(one, -10.0f * kbm::kRadiansPerPixel, 1e-6f));
    CHECK(near(kbm::DirectLook::yaw(20.0f, 0.0f, t), 2.0f * one, 1e-6f));
    CHECK(
        kbm::DirectLook::yaw(0.0f, 0.0f, t) == 0.0f && kbm::DirectLook::pitch(0.0f, 0.0f, t) == 0.0f
    );
    CHECK(near(kbm::DirectLook::yaw(-10.0f, 0.0f, t), -one, 1e-6f));
    t.sensitivity = 2.0f;
    CHECK(near(kbm::DirectLook::yaw(10.0f, 0.0f, t), 2.0f * one, 1e-6f));
    t.sensitivity = 1.0f;
    // No curve: a fast flick turns exactly its share, and the other axis does not change it.
    CHECK(near(kbm::DirectLook::yaw(40.0f, 0.0f, t), 4.0f * one, 1e-6f));
    CHECK(near(kbm::DirectLook::yaw(10.0f, 300.0f, t), one, 1e-6f));
    // A negative sensitivity or ratio (a broken settings file) turns nothing.
    t.sensitivity = -1.0f;
    CHECK(kbm::DirectLook::yaw(10.0f, 0.0f, t) == 0.0f);
    t.sensitivity = 1.0f;
    // Each sensitivity scales its own axis only.
    const float tilt = kbm::DirectLook::pitch(0.0f, 10.0f, t);
    t.sensitivity_y = 0.5f;
    CHECK(near(kbm::DirectLook::pitch(0.0f, 10.0f, t), 0.5f * tilt, 1e-6f));
    CHECK(near(kbm::DirectLook::yaw(10.0f, 0.0f, t), one, 1e-6f));
    t.sensitivity_y = 1.0f;
    t.sensitivity = 2.0f;
    CHECK(near(kbm::DirectLook::pitch(0.0f, 10.0f, t), tilt, 1e-6f));
    t.sensitivity = 1.0f;
    t.sensitivity_y = -1.0f;
    CHECK(kbm::DirectLook::pitch(0.0f, 10.0f, t) == 0.0f);
    t.sensitivity_y = 1.0f;

    // Mouse right turns the view right. The follow camera west of Ratchet looks east (+x); right of
    // that is south (-y), with z up. The game's turn is about up, right-handed (rotate).
    const Vec3 orbit{-4.0f, 0.0f, 1.0f};
    Vec3 turned = kbm::rotate(orbit, up, kbm::DirectLook::yaw(30.0f, 0.0f, t));
    CHECK(view_of(turned).y < -0.1f && near(view_of(turned).x, 4.0f, 0.1f));
    CHECK(near(kbm::length(turned), kbm::length(orbit), 1e-4f) && near(turned.z, orbit.z, 1e-5f));
    turned = kbm::rotate(orbit, up, kbm::DirectLook::yaw(-30.0f, 0.0f, t));
    CHECK(view_of(turned).y > 0.1f);  // left
    // The first-person view's way of looking (east) turns the same way.
    const Vec3 ahead =
        kbm::rotate(Vec3{1.0f, 0.0f, 0.0f}, up, kbm::DirectLook::yaw(30.0f, 0.0f, t));
    CHECK(ahead.y < -0.01f);

    // Mouse up looks up: pitch < 0 (looking down is > 0); the follow camera's eye goes down below
    // Ratchet for it, so the view looks up at him. Invert Y swaps it, and only it.
    CHECK(
        kbm::DirectLook::pitch(0.0f, -10.0f, t) < 0.0f
        && kbm::DirectLook::pitch(0.0f, 10.0f, t) > 0.0f
    );
    CHECK(near(kbm::DirectLook::pitch(0.0f, -10.0f, t), -10.0f * kbm::kRadiansPerPixel, 1e-6f));
    const float e0 = kbm::elevation(orbit, up);
    const Vec3 lowered =
        kbm::with_elevation(orbit, up, e0 + kbm::DirectLook::pitch(0.0f, -40.0f, t));
    CHECK(kbm::elevation(lowered, up) < e0 && view_of(lowered).z > view_of(orbit).z);
    CHECK(near(kbm::length(lowered), kbm::length(orbit), 1e-4f));
    CHECK(
        near(std::atan2(lowered.y, lowered.x), std::atan2(orbit.y, orbit.x), 1e-5f)
    );  // the same way round
    t.invert_y = true;
    CHECK(kbm::DirectLook::pitch(0.0f, -10.0f, t) > 0.0f);
    CHECK(near(kbm::DirectLook::yaw(10.0f, 0.0f, t), one, 1e-6f));
    t.invert_y = false;

    // The vectors' helpers.
    CHECK(near(kbm::elevation(Vec3{1.0f, 0.0f, 1.0f}, up), 0.785398f, 1e-5f));
    CHECK(near(
        kbm::elevation(kbm::with_elevation(Vec3{3.0f, 4.0f, 0.0f}, up, -0.5f), up), -0.5f, 1e-5f
    ));
    const Vec3 straight{0.0f, 0.0f, 2.0f};
    CHECK(kbm::with_elevation(straight, up, 0.0f).z == 2.0f);  // no way round: left alone
    CHECK(
        kbm::unit_or(Vec3{}, up).z == 1.0f
        && near(kbm::unit_or(Vec3{0.0f, 0.0f, 5.0f}, Vec3{}).z, 1.0f)
    );
    CHECK(near(
        kbm::length(
            kbm::rotate(Vec3{1.0f, 2.0f, 3.0f}, kbm::unit_or(Vec3{1.0f, 1.0f, 0.0f}, up), 1.0f)
        ),
        kbm::length(Vec3{1.0f, 2.0f, 3.0f}),
        1e-4f
    ));

    // The follow camera's tilt held through the game's own target: for every elevation the mouse
    // can reach, the stick's y written reads back, through the game's code, as the target that
    // holds it, whichever way the game's invert option is set; never the resting fallback.
    const float limit = kbm::follow_max_elevation();
    CHECK(near(limit, 0.99563f, 1e-4f));
    CHECK(
        near(kbm::follow_target_of(limit), 1.0f, 1e-4f)
        && near(kbm::follow_target_of(-limit), -1.0f, 1e-4f)
    );

    for (float e = -limit; e <= limit; e += 0.05f) {
        const float w = kbm::follow_target_of(e);
        // The game aims the eye at atan(sin(elevation)) = w * 40 degrees.
        CHECK(near(std::atan(std::sin(e)), w * kbm::kFollowTiltRange, 1e-4f));

        for (bool inverts : {false, true}) {
            bool fallback = true;
            const float stick = kbm::follow_stick_for(w, inverts);
            CHECK(std::fabs(stick) <= 1.0f);
            CHECK(near(game_follow_target(stick, inverts, &fallback), w, 1e-5f) && !fallback);
        }
    }

    bool fallback = true;
    CHECK(
        game_follow_target(kbm::follow_stick_for(0.0f, true), true, &fallback) == 0.0f && !fallback
    );
    // The same eye, whatever the game's option: the stick's y it is held with flips with it.
    CHECK(kbm::follow_stick_for(0.5f, true) == -kbm::follow_stick_for(0.5f, false));
}

void quick_select() {
    // The game's ring: slot 0 at the top, clockwise.
    CHECK(kbm::ring_slot(0.0f, -1.0f) == 0);
    CHECK(kbm::ring_slot(0.7071f, -0.7071f) == 1);
    CHECK(kbm::ring_slot(1.0f, 0.0f) == 2);
    CHECK(kbm::ring_slot(0.7071f, 0.7071f) == 3);
    CHECK(kbm::ring_slot(0.0f, 1.0f) == 4);
    CHECK(kbm::ring_slot(-1.0f, 0.0f) == 6);
    CHECK(kbm::ring_slot(-0.7071f, -0.7071f) == 7);
    CHECK(kbm::ring_slot(0.0f, 0.0f) == -1);
    // The mouse's motion points the stick at the slot the way it moves; within the dead zone, none.
    const float dead = 30.0f;

    struct Case {
        float dx, dy;
        int slot;
    };

    const Case cases[] =
        {{0, -50, 0},
         {40, -40, 1},
         {50, 0, 2},
         {35, 35, 3},
         {0, 60, 4},
         {-30, 30, 5},
         {-45, 0, 6},
         {-40, -40, 7},
         {10, -10, -1},
         {45, -10, 2},
         {-5, 45, 4}};

    for (const Case& c : cases) {
        kbm::RingPointer p;
        p.add(c.dx, c.dy, dead);
        const auto [x, y] = p.stick(dead);
        CHECK(kbm::ring_slot(x, y) == c.slot);

        if (c.slot >= 0) {
            CHECK(near(std::sqrt(x * x + y * y), 1.0f, 1e-4f));  // past the game's 0.9
        }
    }

    // Small motions add up; the pointer is held within twice the dead zone, so turning back
    // answers soon: a long way right, then a little left, points left.
    kbm::RingPointer p;

    for (int i = 0; i < 8; ++i) {
        p.add(5.0f, 0.0f, dead);
    }

    CHECK(kbm::ring_slot(p.stick(dead).first, p.stick(dead).second) == 2);

    for (int i = 0; i < 100; ++i) {
        p.add(10.0f, 0.0f, dead);
    }

    CHECK(near(p.x, 60.0f));
    p.add(-100.0f, 0.0f, dead);
    CHECK(kbm::ring_slot(p.stick(dead).first, p.stick(dead).second) == 6);
}

void report() {
    // The game reads our stick bytes back as what we meant, dead zone and all.
    CHECK(kbm::stick_byte(0.0f) == 0x80);
    CHECK(kbm::game_axis(kbm::stick_byte(1.0f)) == 1.0f);
    CHECK(kbm::game_axis(kbm::stick_byte(-1.0f)) == -1.0f);

    for (float v = 0.05f; v <= 1.0f; v += 0.05f) {
        CHECK(near(kbm::game_axis(kbm::stick_byte(v)), v, 0.02f));
        CHECK(near(kbm::game_axis(kbm::stick_byte(-v)), -v, 0.02f));
    }

    // Buttons held on top of the pad's; a pad's stick kept where ours rests.
    std::uint8_t r[18];

    for (std::uint8_t& b : r) {
        b = 0;
    }

    r[0] = 0xFF;
    r[1] = 0xFF & ~(kbm::kSquare >> 8);  // the gamepad holds Square
    r[2] = r[3] = 0x80;
    r[4] = 0x10;  // the gamepad's left stick, pushed left
    r[5] = 0x80;
    kbm::Virtual v;
    v.held = kbm::kCross | kbm::kStart;
    v.right_x = 0.5f;
    kbm::merge(v, r, 18);
    const std::uint16_t buttons = static_cast<std::uint16_t>(r[0] | (r[1] << 8));
    CHECK(
        (buttons & kbm::kCross) == 0 && (buttons & kbm::kStart) == 0
        && (buttons & kbm::kSquare) == 0
    );
    CHECK((buttons & kbm::kCircle) != 0);
    CHECK(r[12] == 0xFF);  // Cross's pressure
    CHECK(r[13] == 0x00);  // Square's: the pad's own report says what it says
    CHECK(near(kbm::game_axis(r[2]), 0.5f) && r[3] == 0x80);
    CHECK(r[4] == 0x10 && r[5] == 0x80);  // ours at rest: the gamepad's
    v = kbm::Virtual{};
    v.left_y = -1.0f;
    kbm::merge(v, r, 18);
    CHECK(r[4] == 0x10 && kbm::game_axis(r[5]) == -1.0f);
}

void weapon_wheel() {
    const std::array<std::int32_t, 8> slots = {0x0F, 0, 0x0A, 0x10, 0, 0, 0, 0x17};
    CHECK(kbm::cycle_item(slots, 0x0F, 1) == 0x0A);  // skips the empty slot
    CHECK(kbm::cycle_item(slots, 0x17, 1) == 0x0F);  // wraps
    CHECK(kbm::cycle_item(slots, 0x0F, -1) == 0x17);
    CHECK(kbm::cycle_item(slots, 0x08, 1) == 0x0F);   // the wrench, not in the ring: the first
    CHECK(kbm::cycle_item(slots, 0x08, -1) == 0x17);  // or the last
    const std::array<std::int32_t, 8> one = {0, 0, 0x0F, 0, 0, 0, 0, 0};
    CHECK(kbm::cycle_item(one, 0x0F, 1) == 0x0F);
    const std::array<std::int32_t, 8> none = {};
    CHECK(kbm::cycle_item(none, 0x08, 1) == 0x08);
}

void contexts() {
    using kbm::Context;
    // A level's mode word with a level loaded; the boot program's without one.
    CHECK(kbm::context_of(3, 0, 99) == Context::Play);
    CHECK(kbm::context_of(3, 1, 0) == Context::Movie);
    CHECK(kbm::context_of(3, 2, 0) == Context::Cutscene);
    CHECK(kbm::context_of(3, 3, 0) == Context::Menu);
    CHECK(kbm::context_of(3, 4, 0) == Context::Dialog);
    CHECK(kbm::context_of(3, 5, 0) == Context::Vendor);
    CHECK(kbm::context_of(3, 6, 0) == Context::Flight);
    CHECK(kbm::context_of(3, 7, 0) == Context::Slideshow);
    CHECK(kbm::context_of(3, -1, 0) == Context::Menu);  // the debug menu, and anything unknown
    CHECK(kbm::context_of(-1, 0, 0) == Context::Title);
    CHECK(kbm::context_of(-1, 0, 3) == Context::Menu);  // the main menu
    CHECK(kbm::context_of(-1, 0, 4) == Context::Dialog);
    CHECK(kbm::context_of(-1, 0, 6) == Context::Flight);
    CHECK(kbm::context_of(-1, 0, 1) == Context::Movie);
}

void buttons() {
    using kbm::Context;
    const auto b = [](Context c, std::initializer_list<Action> actions, int kind = 2) {
        kbm::ActionSet set = 0;

        for (Action a : actions) {
            set |= kbm::bit(a);
        }

        return kbm::buttons_for(c, set, kind);
    };
    // Play: the console's layout.
    CHECK(b(Context::Play, {Action::Jump}) == kbm::kCross);
    CHECK(b(Context::Play, {Action::Pause}) == kbm::kStart);
    CHECK(b(Context::Play, {Action::Fire, Action::Wrench}) == (kbm::kCircle | kbm::kSquare));
    CHECK(
        b(Context::Play,
          {Action::Crouch, Action::Strafe, Action::FirstPerson, Action::R2, Action::Map})
        == (kbm::kR1 | kbm::kL2 | kbm::kL1 | kbm::kR2 | kbm::kSelect)
    );
    CHECK(b(Context::Play, {Action::MenuConfirm, Action::MenuBack}) == 0);

    // The title: Space, Enter, Escape all start.
    for (Action a : {Action::Jump, Action::MenuConfirm, Action::Pause}) {
        CHECK(b(Context::Title, {a}) == kbm::kStart);
    }

    // Menus and the vendor: confirm Cross, back Triangle (Escape too), Select closes, L1 R1 page.
    for (Context c : {Context::Menu, Context::Vendor}) {
        CHECK(b(c, {Action::Jump}) == kbm::kCross);
        CHECK(b(c, {Action::MenuConfirm}) == kbm::kCross);
        CHECK(b(c, {Action::MenuBack}) == kbm::kTriangle);
        CHECK(b(c, {Action::Pause}) == kbm::kTriangle);
        CHECK(b(c, {Action::Map}) == kbm::kSelect);
        CHECK(b(c, {Action::Crouch, Action::FirstPerson}) == (kbm::kR1 | kbm::kL1));
        CHECK(b(c, {Action::MenuCircle}) == kbm::kCircle);
    }

    // The map: Enter and a click travel (Circle), Space turns the page (Cross).
    const auto map = [](std::initializer_list<Action> actions) {
        kbm::ActionSet set = 0;

        for (Action a : actions) {
            set |= kbm::bit(a);
        }

        return kbm::buttons_for(Context::Menu, set, -1, true);
    };
    CHECK(map({Action::MenuConfirm}) == kbm::kCircle);
    CHECK(map({Action::Jump}) == kbm::kCross);
    CHECK(map({Action::Pause}) == kbm::kTriangle && map({Action::MenuBack}) == kbm::kTriangle);
    CHECK(b(Context::Play, {Action::MenuCircle}) == 0);

    // Dialogs: "Quit?" (1, 4): Escape goes on (Cross), Backspace quits (Triangle); "Quit race?" (0):
    // Escape goes on (Start), Backspace Circle; a message: confirm Cross, back Triangle.
    for (int kind : {1, 4}) {
        CHECK(b(Context::Dialog, {Action::Pause}, kind) == kbm::kCross);
        CHECK(b(Context::Dialog, {Action::MenuBack}, kind) == kbm::kTriangle);
        CHECK(b(Context::Dialog, {Action::MenuConfirm}, kind) == kbm::kCross);
    }

    CHECK(b(Context::Dialog, {Action::Pause}, 0) == kbm::kStart);
    CHECK(b(Context::Dialog, {Action::MenuBack}, 0) == kbm::kCircle);
    CHECK(b(Context::Dialog, {Action::Jump}, 0) == kbm::kCross);
    CHECK(b(Context::Dialog, {Action::MenuBack}, 8) == kbm::kTriangle);

    // The memory card's questions (create save data, format) take Circle for yes: Menu: Circle
    // presses it in every dialog.
    for (int kind : {0, 1, 3, 4, 8}) {
        CHECK(b(Context::Dialog, {Action::MenuCircle}, kind) == kbm::kCircle);
    }

    // Scenes, movies, slideshows: everything that confirms or goes back skips (Start); the ship
    // Start and Cross.
    for (Context c : {Context::Cutscene, Context::Movie, Context::Slideshow}) {
        for (Action a : {Action::Jump, Action::MenuConfirm, Action::Pause, Action::MenuBack}) {
            CHECK(b(c, {a}) == kbm::kStart);
        }

        CHECK(b(c, {Action::Fire}) == 0);
    }

    CHECK(b(Context::Flight, {Action::Pause}) == (kbm::kStart | kbm::kCross));
}

void press_edges() {
    using kbm::Context;
    // Held across a change of where the game is: out until let go. Pressed just then: in.
    kbm::Latch latch;
    const kbm::ActionSet esc = kbm::bit(Action::Pause);
    CHECK(latch.filter(Context::Play, esc) == esc);
    CHECK(latch.filter(Context::Menu, esc) == 0);
    CHECK(latch.filter(Context::Menu, esc) == 0);
    CHECK(latch.filter(Context::Menu, 0) == 0);
    CHECK(latch.filter(Context::Menu, esc) == esc);
    const kbm::ActionSet enter = kbm::bit(Action::MenuConfirm);
    CHECK(latch.filter(Context::Menu, 0) == 0);
    CHECK(latch.filter(Context::Cutscene, enter) == enter);  // down this very tick
    // Pulses: each one a tick down and a tick up, in order.
    kbm::Pulser p;
    p.push(kbm::kDown);
    p.push(kbm::kDown);
    p.push(kbm::kCross);
    CHECK(p.step() == kbm::kDown && p.step() == 0 && p.step() == kbm::kDown && p.step() == 0);
    CHECK(p.step() == kbm::kCross && !p.idle() && p.step() == 0 && p.idle());

    for (int i = 0; i < 9; ++i) {
        p.push(kbm::kUp);
    }

    CHECK(p.count == 4);  // at most four wait
    // A held direction repeats: down 24 ticks, then up one tick in six.
    kbm::Repeat r;
    int edges = 0;
    bool was = false;

    for (int t = 1; t <= 60; ++t) {
        const bool now = r.step(true);
        edges += now && !was ? 1 : 0;
        was = now;

        if (t <= 24) {
            CHECK(now);
        }
    }

    CHECK(edges == 6);
    CHECK(!r.step(false) && r.step(true));  // let go: the next press is at once
}

void menu_mouse() {
    // The widget under the point.
    const kbm::Rect rects[3] = {{100, 100, 200, 30}, {100, 140, 200, 30}, {0, 0, 0, 0}};
    CHECK(kbm::hit_widget(rects, 3, 150.0f, 110.0f) == 0);
    CHECK(kbm::hit_widget(rects, 3, 150.0f, 155.0f) == 1);
    CHECK(kbm::hit_widget(rects, 3, 50.0f, 155.0f) == -1);
    CHECK(kbm::hit_widget(rects, 3, 301.0f, 155.0f) == 1);  // a margin of two pixels
    CHECK(kbm::hit_widget(rects, 3, 0.0f, 0.0f) == -1);     // an empty rectangle is nothing
    // The widgets the focus can move to, link by link: the Quick Select's grids (1, 2, 3, linked
    // in a chain, 3 back to 1) but not its label (4) or preview (5), which nothing links to.
    {
        const std::uint32_t page[5] = {1, 2, 3, 4, 5};
        const auto links = [](std::uint32_t w, std::uint32_t* out) {
            switch (w) {
                case 1:
                    out[0] = 2;
                    out[1] = 0;
                    return 2;
                case 2:
                    out[0] = 3;
                    out[1] = 1;
                    return 2;
                case 3:
                    out[0] = 1;
                    out[1] = 99;  // not one of the page's
                    return 2;
                default:
                    return 0;
            }
        };
        CHECK(kbm::reachable_widgets(1, page, 5, links) == 0x7u);
        CHECK(kbm::reachable_widgets(4, page, 5, links) == 0u);  // a label leads nowhere
        CHECK(kbm::reachable_widgets(0, page, 5, links) == 0u);
    }
    // An item grid's cells where the game draws their icons: two rows of three in a 240 by 160
    // rectangle at (60, 120), a 3 by 2 unit frame (80 pixels a unit), 0.8 unit icons, 0.1
    // margins. The icons are 64 pixels, 80 apart, from (68, 128); a gap counts half each side.
    {
        kbm::GridLayout g;
        g.rect = {60, 120, 240, 160};
        g.frame_w = 3.0f;
        g.frame_h = 2.0f;
        g.icon_w = 0.8f;
        g.icon_h = 0.8f;
        g.margin_x = 0.1f;
        g.margin_y = 0.1f;
        g.rows = 2;
        g.cols = 3;
        CHECK(kbm::grid_cell_at(g, 70.0f, 130.0f) == 0);
        CHECK(kbm::grid_cell_at(g, 139.0f, 130.0f) == 0);  // the gap, nearer the first
        CHECK(kbm::grid_cell_at(g, 141.0f, 130.0f) == 1);  // nearer the second
        CHECK(kbm::grid_cell_at(g, 260.0f, 240.0f) == 5);
        CHECK(kbm::grid_cell_at(g, 228.0f, 199.0f) == 2);
        CHECK(kbm::grid_cell_at(g, 228.0f, 201.0f) == 5);
        CHECK(kbm::grid_cell_at(g, 301.0f, 240.0f) == -1);  // past the last column's half gap
        CHECK(kbm::grid_cell_at(g, 100.0f, 110.0f) == -1);  // above the grid
        // One row of three (the Quick Select's first): centred in the frame's height.
        g.rows = 1;
        g.rect = {60, 120, 240, 80};
        g.frame_h = 1.0f;
        CHECK(kbm::grid_cell_at(g, 180.0f, 160.0f) == 1);
        CHECK(kbm::grid_cell_at(g, 180.0f, 112.0f) == -1);
        // A fixed row step (flag 2): rows 0.95 units apart from the margin.
        g.rows = 3;
        g.cols = 1;
        g.flags = 2;
        g.rect = {0, 0, 80, 240};
        g.frame_w = 1.0f;
        g.frame_h = 3.0f;
        CHECK(kbm::grid_cell_at(g, 40.0f, 0.1f * 80.0f + 1.0f) == 0);
        CHECK(kbm::grid_cell_at(g, 40.0f, 1.05f * 80.0f + 1.0f) == 1);
        CHECK(kbm::grid_cell_at(g, 40.0f, 2.0f * 80.0f + 1.0f) == 2);
        // A layout the game could not draw has no cells.
        g.frame_w = 0.0f;
        CHECK(kbm::grid_cell_at(g, 40.0f, 9.0f) == -1);
        g.frame_w = 1.0f;
        g.rows = 0;
        CHECK(kbm::grid_cell_at(g, 40.0f, 9.0f) == -1);
    }
    // The press toward a cell: the row first, then the column.
    CHECK(kbm::grid_step(0, 5, 2, 3) == kbm::kDown);
    CHECK(kbm::grid_step(3, 5, 2, 3) == kbm::kRight);
    CHECK(kbm::grid_step(5, 0, 2, 3) == kbm::kUp);
    CHECK(kbm::grid_step(2, 0, 2, 3) == kbm::kLeft);
    CHECK(kbm::grid_step(4, 4, 2, 3) == 0);
    CHECK(kbm::grid_step(0, 6, 2, 3) == 0 && kbm::grid_step(-1, 2, 2, 3) == 0);
    // A list's rows: spread (row i's text at y + h / (n + 1) * (i + 1)).
    const kbm::Rect list{100, 100, 200, 100};
    CHECK(kbm::list_row(list, 3, 0, 110.0f) == 0);
    CHECK(kbm::list_row(list, 3, 0, 125.0f) == 0);
    CHECK(kbm::list_row(list, 3, 0, 150.0f) == 1);
    CHECK(kbm::list_row(list, 3, 0, 176.0f) == 2);
    CHECK(kbm::list_row(list, 3, 0, 199.0f) == 2);
    CHECK(kbm::list_row(list, 3, 0, 99.0f) == -1);
    CHECK(kbm::list_row(list, 3, 0, 200.0f) == -1);
    // Packed (flag 0x10): 15 apart with the 12 font, 17 with flag 4.
    CHECK(kbm::list_row(list, 5, 0x10, 114.0f) == 0 && kbm::list_row(list, 5, 0x10, 116.0f) == 1);
    CHECK(kbm::list_row(list, 5, 0x14, 116.0f) == 0 && kbm::list_row(list, 5, 0x14, 118.0f) == 1);
    CHECK(kbm::list_row(list, 5, 0x10, 199.0f) == 4);  // past the last row: the last
    // The pointer's 0..1 to the game's screen.
    CHECK(kbm::kScreenWidth == 512.0f && kbm::kScreenHeight == 416.0f);
    // Stepping: 48 pixels along the way it mostly goes.
    kbm::Stepper s;
    CHECK(s.step(30.0f, 0.0f) == 0);
    CHECK(s.step(30.0f, 5.0f) == kbm::kRight);
    CHECK(s.step(0.0f, -60.0f) == kbm::kUp);
    CHECK(s.step(-60.0f, 0.0f) == kbm::kLeft);
    s.reset();
    CHECK(
        s.step(0.0f, 100.0f) == kbm::kDown && s.step(0.0f, 0.0f) == kbm::kDown
        && s.step(0.0f, 0.0f) == 0
    );
}

// The map dragged and zoomed within the game's bounds.
void map_drag() {
    // Dragged right, the pan goes back (the map follows the mouse), less at a closer zoom.
    const std::int32_t far = kbm::map_pan(0x08000000, 10.0f, 1.0f, false);
    const std::int32_t near = kbm::map_pan(0x08000000, 10.0f, 2.0f, false);
    CHECK(far < near && near < 0x08000000);
    CHECK(kbm::map_pan(0x00000100, 100.0f, 1.0f, false) == 0);
    CHECK(kbm::map_pan(0x0FFFFF00, -100.0f, 1.0f, false) == 0x10000000);
    // Down, a margin of 1280 / zoom at each end.
    CHECK(kbm::map_pan(0, 0.0f, 1.0f, true) == (1280 << 15));
    CHECK(kbm::map_zoom(1.0f, 1.0f) > 1.0f && kbm::map_zoom(1.0f, -1.0f) < 1.0f);
    CHECK(kbm::map_zoom(3.9f, 5.0f) == 4.0f && kbm::map_zoom(0.7f, -5.0f) == 0.65f);
}

// A click on a button icon of the game's text, or the text after it, presses that button.
void prompt_spots() {
    const kbm::PromptSpot spots[3] = {
        {0, kbm::Rect{41, 268, 139, 16}},  // Cross: View Missions
        {9, kbm::Rect{41, 340, 24, 16}},   // the right stick: presses nothing
        {2, kbm::Rect{41, 358, 63, 16}},   // Triangle: Exit
    };
    CHECK(kbm::prompt_button_at(spots, 3, 100.0f, 275.0f) == kbm::kCross);
    CHECK(kbm::prompt_button_at(spots, 3, 60.0f, 365.0f) == kbm::kTriangle);
    CHECK(kbm::prompt_button_at(spots, 3, 50.0f, 347.0f) == 0);
    CHECK(kbm::prompt_button_at(spots, 3, 300.0f, 275.0f) == 0);
    CHECK(kbm::button_of_cell(7) == kbm::kR2 && kbm::button_of_cell(8) == 0);
}

// Going Commando's menu texts: the list item under the pointer and the d-pad's step to it.
void text_nav() {
    const auto text = [](std::uint32_t element, std::uint32_t colour, float x, float y, float w) {
        kbm::MenuText m;
        m.element = element;
        m.colour = colour;
        m.x0 = x;
        m.y0 = y;
        m.x1 = x + w;
        m.y1 = y + 16.0f;
        return m;
    };
    // A column of four, the second selected; then a plain print with Triangle's icon.
    kbm::MenuText t[5] = {
        text(1, 0x80f0f0f0, 200, 100, 80),
        text(2, 0x70fffeed, 205, 130, 70),
        text(3, 0x80f0f0f0, 210, 160, 60),
        text(4, 0x80f0f0f0, 200, 190, 80),
        text(0, 0x80808080, 393, 293, 40),
    };
    t[4].cell = 2;

    kbm::TextNav nav = kbm::text_nav(t, 5, 240.0f, 195.0f);
    CHECK(nav.hit == 3 && nav.selected == 1 && nav.step == kbm::kDown);
    nav = kbm::text_nav(t, 5, 240.0f, 105.0f);
    CHECK(nav.hit == 0 && nav.selected == 1 && nav.step == kbm::kUp);
    nav = kbm::text_nav(t, 5, 240.0f, 135.0f);
    CHECK(nav.hit == 1 && nav.step == 0);
    // The print: no list, its icon presses Triangle.
    nav = kbm::text_nav(t, 5, 400.0f, 300.0f);
    CHECK(
        nav.hit == 4 && nav.selected < 0 && kbm::button_of_cell(t[nav.hit].cell) == kbm::kTriangle
    );
    CHECK(kbm::text_nav(t, 5, 20.0f, 20.0f).hit < 0);
    // Two selected colours: no way to tell.
    t[3].colour = 0x70ffffff;
    CHECK(kbm::text_nav(t, 5, 240.0f, 195.0f).selected < 0);
    // A row of three side by side, the right one selected.
    const kbm::MenuText r[3] = {
        text(1, 0x80f0f0f0, 100, 50, 50),
        text(2, 0x80f0f0f0, 200, 50, 50),
        text(3, 0x70fffeed, 300, 50, 50),
    };
    nav = kbm::text_nav(r, 3, 120.0f, 55.0f);
    CHECK(nav.hit == 0 && nav.selected == 2 && nav.step == kbm::kLeft && nav.steps == 2);
    // A list of prints that scrolls round its middle, its ends faded: the middle is selected.
    const kbm::MenuText sc[5] = {
        text(1, 0x40808080, 200, 316, 80),
        text(1, 0x80f0f0f0, 210, 339, 60),
        text(1, 0x80f0f0f0, 205, 362, 70),
        text(1, 0x80f0f0f0, 200, 385, 80),
        text(1, 0x40808080, 215, 408, 50),
    };
    nav = kbm::text_nav(sc, 5, 240.0f, 412.0f);
    CHECK(nav.hit == 4 && nav.selected == 2 && nav.scrolls && nav.step == kbm::kDown);
    CHECK(nav.steps == 2);
}

// A click on a Going Commando grid's cell: the frame, found by a first step, goes there and
// confirms (a grid that wraps round at its edges, its cursor moving a few ticks after a press).
void grid_seek() {
    std::vector<kbm::GuiSprite> s;
    float hx = 56.0f, hy = 88.0f;
    const auto frame = [&] {
        s.clear();
        for (int r = 0; r < 6; ++r) {
            for (int c = 0; c < 4; ++c) {
                s.push_back(
                    {0x2420,
                     56.0f + 53.5f * static_cast<float>(c),
                     88.0f + 48.0f * static_cast<float>(r)}
                );
            }
        }
        s.push_back({0x259c, hx, hy});
        s.push_back({0x2388, 250.0f, 200.0f});
    };
    frame();
    kbm::GridSeek g;
    CHECK(!g.seek(s.data(), static_cast<int>(s.size()), 400.0f, 400.0f, true));
    CHECK(g.seek(s.data(), static_cast<int>(s.size()), 162.7f, 177.6f, true));

    int pending = 0;
    std::uint16_t pressed = 0;
    std::uint16_t last = 0;
    for (int t = 0; t < 200 && g.active(); ++t) {
        if (pending > 0 && --pending == 0) {
            if (pressed == kbm::kLeft) {
                hx = hx < 60.0f ? 216.5f : hx - 53.5f;
            } else if (pressed == kbm::kRight) {
                hx = hx > 210.0f ? 56.0f : hx + 53.5f;
            } else if (pressed == kbm::kUp) {
                hy = hy < 90.0f ? 328.0f : hy - 48.0f;
            } else if (pressed == kbm::kDown) {
                hy = hy > 320.0f ? 88.0f : hy + 48.0f;
            }
        }
        frame();
        const std::uint16_t step = g.update(s.data(), static_cast<int>(s.size()), pending == 0);
        if (step != 0) {
            pressed = step;
            last = step;
            pending = 3;
        }
    }
    CHECK(!g.active() && g.known() && last == kbm::kCross);
    CHECK(hx == 163.0f && hy == 184.0f);
}

// The vendor's monitors under the pointer (window places of a 640x480 run).
void vendor() {
    const auto at = [](float wx, float wy, int count, int cursor, int scroll = 0) {
        return kbm::vendor_hit(wx / 640.0f, wy / 480.0f, count, cursor, scroll);
    };

    CHECK(at(115, 311, 2, 1).spot == kbm::VendorSpot::Buy);
    CHECK(at(527, 309, 2, 1).spot == kbm::VendorSpot::Exit);
    CHECK(at(320, 200, 2, 1).spot == kbm::VendorSpot::None);
    // Two rows: the ammo icon left of the selected one, and the selected one.
    CHECK(at(286, 420, 2, 1).spot == kbm::VendorSpot::Item && at(286, 420, 2, 1).steps == -1);
    CHECK(at(355, 420, 2, 1).spot == kbm::VendorSpot::Item && at(355, 420, 2, 1).steps == 0);
    // Off the narrow monitor of two rows, but on the wider one of seven.
    CHECK(at(450, 420, 2, 1).spot == kbm::VendorSpot::None);
    CHECK(at(450, 420, 7, 0).spot == kbm::VendorSpot::Item && at(450, 420, 7, 0).steps == 5);
    // Many rows: the middle is the selected one, a slot to the right one step on.
    CHECK(at(320, 420, 12, 3).steps == 0 && at(320, 420, 12, 3).spot == kbm::VendorSpot::Item);
    CHECK(at(390, 420, 12, 3).steps == 1);
    CHECK(at(250, 420, 12, 3).steps == -1);
    CHECK(at(320, 420, 0, 0).spot == kbm::VendorSpot::None);

    CHECK(kbm::vendor_step(1, 1, 2) == 0);
    CHECK(kbm::vendor_step(1, 0, 2) == kbm::kLeft);
    CHECK(kbm::vendor_step(0, 4, 7) == kbm::kRight);
    CHECK(kbm::vendor_step(0, 11, 12) == kbm::kLeft);  // round the end
    CHECK(kbm::vendor_step(10, 1, 12) == kbm::kRight);
}

// The action each button icon stands for (key prompts), in the cells' order.
void prompts() {
    using kbm::Action;
    using kbm::Context;
    const auto play = kbm::prompt_actions(Context::Play, -1);
    CHECK(play[0] == Action::Jump);         // Cross
    CHECK(play[1] == Action::Wrench);       // Square
    CHECK(play[2] == Action::QuickSelect);  // Triangle
    CHECK(play[3] == Action::Fire);         // Circle
    CHECK(play[4] == Action::FirstPerson);  // L1
    CHECK(play[5] == Action::Strafe);       // L2
    CHECK(play[6] == Action::Crouch);       // R1
    CHECK(play[7] == Action::R2);           // R2
    // Without lock-strafe the strafe key presses nothing: L2 keeps its icon.
    CHECK(kbm::prompt_actions(Context::Play, -1, false, false)[5] == Action::Count);

    const auto menu = kbm::prompt_actions(Context::Menu, -1);
    CHECK(menu[0] == Action::MenuConfirm);
    CHECK(menu[2] == Action::MenuBack);
    CHECK(menu[3] == Action::MenuCircle);
    // On the map confirm travels (Circle) and Space turns the page (Cross).
    const auto map = kbm::prompt_actions(Context::Menu, -1, true);
    CHECK(map[3] == Action::MenuConfirm);
    CHECK(map[0] == Action::Jump);
    // The race's quit dialog: back is Circle.
    CHECK(kbm::prompt_actions(Context::Dialog, kbm::kDialogQuitRace)[3] == Action::MenuBack);
    // Where only Start counts, no icon changes.
    for (Action a : kbm::prompt_actions(Context::Cutscene, -1)) {
        CHECK(a == Action::Count);
    }
}

void manifest(const char* path) {
    std::ifstream f(path);
    CHECK(f.good());
    std::stringstream s;
    s << f.rdbuf();
    const std::string text = s.str();

    for (const kbm::ActionInfo& a : kbm::kActions) {
        const std::size_t at = text.find(std::string("\"key\": \"") + a.key + "\"");
        CHECK(at != std::string::npos);

        if (at == std::string::npos) {
            continue;
        }

        const std::size_t end = text.find('}', at);
        const std::string object = text.substr(at, end - at);
        CHECK(object.find("\"type\": \"keybind\"") != std::string::npos);
        CHECK(object.find(std::string("\"default\": \"") + a.fallback + "\"") != std::string::npos);
        CHECK(
            object.find(a.group == kbm::Group::Play ? "\"group\": \"play\"" : "\"group\": \"menu\"")
            != std::string::npos
        );
    }
}

}  // namespace

// The edges of each piece: what an odd setting, an odd report or the end of a range does.
void edges() {
    kbm::Binds b;
    // Separators alone, or around a name, hold no bind; a comma at the end is no third bind.
    CHECK(kbm::parse_binds(",, ,", b) && b[0] == 0 && b[1] == 0);
    CHECK(kbm::parse_binds("KeyW,", b) && b[0] == 26 && b[1] == 0);
    CHECK(!kbm::parse_binds("KeyW,,KeyA,KeyS", b) && b[0] == 0 && b[1] == 0);
    CHECK(!kbm::parse_binds("Key W", b));  // no name has a space

    // The keys' stick: a ramp below zero is no ramp, a walk past 1 is held at the stick's reach.
    kbm::MoveTuning move;
    move.rise_ticks = -5.0f;
    kbm::KeyStick keys;
    keys.step(false, false, false, true, false, move);
    CHECK(keys.x == 1.0f);
    // All four keys: they cancel out, and the stick comes back to rest.
    for (int i = 0; i < 3; ++i) {
        keys.step(true, true, true, true, false, move);
    }
    CHECK(keys.x == 0.0f && keys.y == 0.0f);

    // The mouse's stick: no up and down sensitivity, no tilt; a huge flick stops at the stick's end.
    kbm::LookTuning look;
    look.sensitivity_y = 0.0f;
    kbm::MouseLook m;
    m.step(1e6f, 500.0f, false, look);
    CHECK(m.x == 1.0f && m.y == 0.0f);
    m.step(-1e6f, 0.0f, true, look);
    CHECK(m.x == 0.0f || std::fabs(m.x) <= 1.0f);
    m.step(-1e6f, 0.0f, true, look);
    CHECK(m.x == -1.0f && m.y == 0.0f);
    // Levels out: a held tilt goes back to level once the mouse rests.
    look.sensitivity_y = 1.0f;
    look.pitch_holds = false;
    m.reset();
    m.step(0.0f, 80.0f, false, look);
    CHECK(m.y > 0.0f);
    for (int i = 0; i < 200; ++i) {
        m.step(0.0f, 0.0f, false, look);
    }
    CHECK(m.y == 0.0f);

    // The report: a short one (no pressure bytes) takes the buttons and sticks only; one too short
    // for the sticks is left alone.
    kbm::Virtual v;
    v.held = kbm::kCross;
    v.left_x = 1.0f;
    std::uint8_t six[6] = {0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};
    kbm::merge(v, six, 6);
    CHECK(six[1] == static_cast<std::uint8_t>(~(kbm::kCross >> 8)) && six[4] != 0x80);
    std::uint8_t five[5] = {0xFF, 0xFF, 0x80, 0x80, 0x80};
    kbm::merge(v, five, 5);
    CHECK(five[0] == 0xFF && five[1] == 0xFF && five[4] == 0x80);
    // A stick value past its reach is full travel, and the game reads it back so.
    CHECK(
        kbm::stick_byte(5.0f) == kbm::stick_byte(1.0f)
        && kbm::stick_byte(-5.0f) == kbm::stick_byte(-1.0f)
    );
    CHECK(
        kbm::game_axis(kbm::stick_byte(1.0f)) == 1.0f
        && kbm::game_axis(kbm::stick_byte(-1.0f)) == -1.0f
    );
    // The smallest push is past the game's dead zone, never lost in it.
    CHECK(kbm::game_axis(kbm::stick_byte(0.001f)) > 0.0f);

    // The Quick Select: a pointer exactly on the dead zone rests; a dead zone of 0 still caps the
    // pointer (at twice 1 pixel), so turning back answers.
    kbm::RingPointer r;
    r.add(30.0f, 0.0f, 30.0f);
    CHECK(r.stick(30.0f).first == 0.0f);
    r.reset();
    r.add(1000.0f, 0.0f, 0.0f);
    CHECK(r.x == 2.0f && r.stick(0.0f).first == 1.0f);
    r.add(-4.0f, 0.0f, 0.0f);
    CHECK(r.stick(0.0f).first == -1.0f);
    // The ring's slots at their exact borders, and every way round.
    for (int s = 0; s < 8; ++s) {
        const float a = static_cast<float>(s) * 3.14159265f / 4.0f - 3.14159265f / 2.0f;
        CHECK(kbm::ring_slot(std::cos(a), std::sin(a)) == s);
    }

    // The weapon wheel: an empty ring keeps what is in hand; nothing in hand starts at the first.
    const std::array<std::int32_t, 8> empty{};
    CHECK(kbm::cycle_item(empty, 7, 1) == 7 && kbm::cycle_item(empty, 0, -1) == 0);
    const std::array<std::int32_t, 8> one = {0, 0, 5, 0, 0, 0, 0, 0};
    CHECK(kbm::cycle_item(one, 5, 1) == 5 && kbm::cycle_item(one, 0, 1) == 5);
    CHECK(kbm::cycle_item(one, 0, -1) == 5);

    // Where the game is: modes no table has are a menu (its keys are the safe ones).
    CHECK(kbm::context_of(3, 99, -1) == kbm::Context::Menu);
    CHECK(kbm::context_of(-1, -1, -1) == kbm::Context::Menu);

    // The pulses: a fifth waiting one is dropped, the rest go one a tick with a rest between.
    kbm::Pulser p;
    for (int i = 0; i < 5; ++i) {
        p.push(static_cast<std::uint16_t>(1u << i));
    }
    int sent = 0;
    for (int i = 0; i < 12; ++i) {
        sent += p.step() != 0 ? 1 : 0;
    }
    CHECK(sent == 4 && p.idle());
    // A held key in a menu repeats: let go one tick in six after the first 24.
    kbm::Repeat rep;
    int released = 0;
    for (int i = 0; i < 24 + 12; ++i) {
        released += rep.step(true) ? 0 : 1;
    }
    CHECK(released == 2 && !rep.step(false) && rep.ticks == 0);
    // The mouse's steps in a menu: the way it mostly goes wins, a diagonal is not two steps.
    kbm::Stepper st;
    CHECK(st.step(47.0f, 46.0f) == 0);
    CHECK(st.step(1.0f, 0.0f) == kbm::kRight);

    // The menus' rows and cells off the edges.
    const kbm::Rect list{10, 10, 100, 0};
    CHECK(kbm::list_row(list, 3, 0u, 10.0f) == -1);  // no height: no rows
    const kbm::Rect rows{10, 10, 100, 40};
    CHECK(kbm::list_row(rows, 0, 0u, 20.0f) == -1 && kbm::list_row(rows, 3, 0u, 50.0f) == -1);
    CHECK(kbm::list_row(rows, 3, 0u, 49.9f) == 2 && kbm::list_row(rows, 3, 0u, 10.0f) == 0);
    kbm::GridLayout grid;
    CHECK(kbm::grid_cell_at(grid, 0.0f, 0.0f) == -1);  // no layout read
    grid.rect = {0, 0, 100, 100};
    grid.frame_w = grid.frame_h = 10.0f;
    grid.icon_w = grid.icon_h = std::nanf("");
    grid.rows = grid.cols = 2;
    CHECK(!kbm::grid_layout_valid(grid) && kbm::grid_cell_at(grid, 10.0f, 10.0f) == -1);
    CHECK(
        kbm::grid_step(0, 4, 2, 2) == 0 && kbm::grid_step(-1, 0, 2, 2) == 0
        && kbm::grid_step(1, 1, 2, 2) == 0
    );
    CHECK(kbm::grid_step(0, 3, 2, 2) == kbm::kDown && kbm::grid_step(3, 2, 2, 2) == kbm::kLeft);

    // Without the lock-strafe mod the strafe key presses nothing, and no icon is drawn for L2's
    // strafe (the first-person key keeps L1).
    const kbm::ActionSet strafe = kbm::bit(Action::Strafe);
    CHECK(kbm::buttons_for(kbm::Context::Play, strafe, -1, false, false) == 0);
    CHECK(kbm::buttons_for(kbm::Context::Play, strafe, -1, false, true) == kbm::kL2);
    const auto icons = kbm::prompt_actions(kbm::Context::Play, -1, false, false);
    CHECK(icons[5] == Action::Count && icons[4] == Action::FirstPerson);

    // The latch: a key held into a new place stays out until let go; one pressed there counts.
    kbm::Latch latch;
    const kbm::ActionSet esc = kbm::bit(Action::Pause);
    const kbm::ActionSet space = kbm::bit(Action::Jump);
    CHECK(latch.filter(kbm::Context::Play, esc) == esc);
    CHECK(latch.filter(kbm::Context::Menu, esc | space) == space);
    CHECK(latch.filter(kbm::Context::Menu, esc) == 0);
    CHECK(latch.filter(kbm::Context::Menu, 0) == 0);
    CHECK(latch.filter(kbm::Context::Menu, esc) == esc);
}

int main(int argc, char** argv) {
    parsing();
    conflicts();
    key_stick();
    mouse_stick();
    direct_look();
    quick_select();
    report();
    weapon_wheel();
    contexts();
    buttons();
    press_edges();
    menu_mouse();
    vendor();
    prompt_spots();
    map_drag();
    text_nav();
    grid_seek();
    prompts();
    edges();

    manifest(argc > 1 ? argv[1] : KBM_MANIFEST);

    if (g_failures == 0) {
        std::puts("keyboard-mouse logic: all passed");
    }

    return g_failures == 0 ? 0 : 1;
}

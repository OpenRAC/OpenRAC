// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The late camera latch's fade (game/common/frontend/latch_fade.h) with the frame rates a player
// can have: one frame a tick (a frame cap at the game's tick rate), two (120 Hz on NTSC's 60
// ticks), and fewer frames than ticks (ticks without a frame of their own). The game's camera is
// a model: it turns at a steady rate from when the stick is pushed, and the latch's heading for a
// frame is where it is when the frame is shown, missed by a set amount at each new tick when the
// test says so. The picture must follow it: never standing still while the camera turns, never
// jumping, and back on the latch's heading within a few frames.

#include <cmath>
#include <vector>

#include "check.h"
#include "game/common/frontend/latch_fade.h"

namespace {

using openrac::frontend::LatchFade;

constexpr double kTick = 1.0 / 60.0;  // NTSC's tick
constexpr float kTurn = 0.03f;        // what the camera turns a tick, radians (about 100° a second)

// One run: frames every `frame` seconds for `seconds`, the stick pushed from `push` seconds on.
// `miss`: what the latch's heading is off by from each new tick on. Returns the headings shown.
struct Run {
    std::vector<float> shown, target;
    std::vector<bool> turning;
};

Run run(double frame, double seconds, double push, float miss = 0.0f, bool enabled = true) {
    LatchFade fade;
    Run r;
    long last_tick = -1;
    float omega = 0.0f;  // the camera's turn in the kept tick
    for (double t = 0.0; t < seconds; t += frame) {
        const long tick = static_cast<long>(std::floor(t / kTick));
        const bool turning = t >= push;
        const bool new_tick = tick != last_tick;
        if (new_tick) {
            // Turned all through the kept tick: pushed by the tick before it began.
            omega = static_cast<double>(tick - 1) * kTick >= push ? kTurn : 0.0f;
        }
        // Where the game's camera is at t, and the latch's guess of it.
        const float camera = turning ? static_cast<float>((t - push) / kTick) * kTurn : 0.0f;
        const float target = camera + (turning ? miss * static_cast<float>(tick % 2) : 0.0f);
        r.shown.push_back(fade.frame(
            target,
            turning,
            new_tick,
            omega / static_cast<float>(kTick),
            last_tick < 0 ? 0.0 : frame,
            enabled
        ));
        r.target.push_back(target);
        r.turning.push_back(turning);
        last_tick = tick;
    }
    return r;
}

// While the camera turns (after `settle` frames), each frame's picture moves on from the last
// one's, by no more than `most` radians.
bool moves_on(const Run& r, std::size_t settle, float most) {
    std::size_t first = 0;
    while (first < r.turning.size() && !r.turning[first]) {
        ++first;
    }
    for (std::size_t i = first + settle + 1; i < r.shown.size(); ++i) {
        const float step = r.shown[i] - r.shown[i - 1];
        if (step <= 0.0f || step > most) {
            return false;
        }
    }
    return true;
}

// The largest difference between the picture and the latch's heading over the last `frames`.
float off_at_end(const Run& r, std::size_t frames) {
    float most = 0.0f;
    for (std::size_t i = r.shown.size() - frames; i < r.shown.size(); ++i) {
        most = std::max(most, std::abs(r.shown[i] - r.target[i]));
    }
    return most;
}

// One frame a tick (the frame cap at 60 on NTSC's 60 ticks): the regression. The picture stood
// still while the stick was held, its step to the camera carried each frame, and jumped once the
// step grew to a cut.
void test_one_frame_a_tick_follows_the_camera() {
    for (const double phase : {0.0, 0.3, 0.7}) {
        const Run r = run(kTick, 2.0, kTick * (10.0 + phase));
        CHECK(moves_on(r, 1, 2.0f * kTurn));
        CHECK(off_at_end(r, 30) < 0.002f);
    }
}

// Two frames a tick (120 Hz on 60 ticks) and other rates over the tick rate.
void test_more_frames_than_ticks_follow_the_camera() {
    for (const double rate : {120.0, 144.0, 90.0, 75.0}) {
        const Run r = run(1.0 / rate, 2.0, 0.2);
        CHECK(moves_on(r, 1, 2.0f * kTurn));
        CHECK(off_at_end(r, 30) < 0.002f);
    }
}

// Fewer frames than ticks: some ticks have no frame of their own.
void test_ticks_without_a_frame_follow_the_camera() {
    for (const double rate : {50.0, 45.0, 30.0}) {
        const Run r = run(1.0 / rate, 2.0, 0.2);
        CHECK(moves_on(r, 1, 3.0f * kTurn * static_cast<float>(60.0 / rate)));
        CHECK(off_at_end(r, 20) < 0.002f);
    }
}

// A guess missed by a little at every other tick: the step is carried and fades, so that the
// picture keeps moving on and never jumps by more than a frame's turn and the miss.
void test_a_missed_guess_fades() {
    for (const double rate : {60.0, 120.0}) {
        const float miss = 0.01f;
        const Run r = run(1.0 / rate, 2.0, 0.2, miss);
        CHECK(moves_on(r, 3, kTurn + miss));
        CHECK(off_at_end(r, 30) <= miss + 0.002f);
    }
}

// The stick let go: the latch goes off and its heading falls back to the interpolated camera's,
// which is behind it by the latch's lead. The picture comes back to it without a jump.
void test_letting_go_fades_back() {
    for (const double rate : {60.0, 120.0}) {
        LatchFade fade;
        const float lead = 0.04f;  // the latch's turn ahead of the interpolated camera
        const double frame = 1.0 / rate;
        float last = 0.0f;
        bool first = true;
        long last_tick = -1;
        for (double t = 0.0; t < 1.0; t += frame) {
            const long tick = static_cast<long>(std::floor(t / kTick));
            const bool on = t < 0.5;
            // The interpolated camera stops turning as the latch's heading does.
            const float camera = static_cast<float>(std::min(t, 0.5) / kTick) * kTurn;
            const float target = camera + (on ? lead : 0.0f);
            const float shown = fade.frame(
                target,
                on,
                tick != last_tick,
                on ? kTurn / static_cast<float>(kTick) : 0.0f,
                first ? 0.0 : frame,
                true
            );
            if (!first) {
                CHECK(std::abs(shown - last) <= kTurn * static_cast<float>(frame / kTick) + 0.001f);
            }
            last = shown;
            first = false;
            last_tick = tick;
        }
        CHECK(std::abs(fade.carry()) < 0.001f);
    }
}

// A cut (another camera): shown at once.
void test_a_cut_is_not_carried() {
    LatchFade fade;
    fade.frame(0.0f, true, true, 0.0f, 0.0, true);
    CHECK(fade.frame(2.0f, true, true, 0.0f, kTick, true) == 2.0f);
    CHECK(fade.carry() == 0.0f);
}

// The setting off: the latch's heading as it is.
void test_off_shows_the_target() {
    const Run r = run(kTick, 1.0, 0.2, 0.01f, false);
    for (std::size_t i = 0; i < r.shown.size(); ++i) {
        CHECK(r.shown[i] == r.target[i]);
    }
}

}  // namespace

int main() {
    test_one_frame_a_tick_follows_the_camera();
    test_more_frames_than_ticks_follow_the_camera();
    test_ticks_without_a_frame_follow_the_camera();
    test_a_missed_guess_fades();
    test_letting_go_fades_back();
    test_a_cut_is_not_carried();
    test_off_shows_the_target();
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// frontend/camera_latch.h's late camera latch.
//
// A frame between ticks shows the camera part way from the last tick's to the kept one's, that is
// one tick and the lag behind the kept tick (frontend/interpolation.cpp): a turn of the right stick
// shows only once a tick has read it and the frames have caught up with that tick. With the latch
// the frame's camera is the interpolated one turned, about the hero, to where the game's follow
// camera will be when the frame is shown: the rest of the way to the kept tick's heading, and on
// from there by what the game says its next ticks will turn with the stick as it is now
// (openrac_game_camera_turn: the game's own turn speed and the spring it eases the stick through).
// The position, the pitch and everything else stay the interpolated camera's.
//
// When a tick comes the game's real camera takes over from the guess. A guess that was right goes
// on without a step; whatever it missed by (a wall the camera met, a stick that moved between the
// tick's read and the frame's) is carried as a difference that fades within a few frames, so the
// picture never jumps. The same fade covers the latch going off (the stick centred, a cutscene).

#include "game/common/frontend/camera_latch.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

#include "common/log.h"
#include "common/stats.h"
#include "frontend.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "openrac/game_host.h"
#include "settings.h"

namespace openrac::frontend {
namespace {

// How fast a missed guess fades: its difference is down to a third after this long.
constexpr double kFadeSeconds = 0.05;

// A turn bigger than this between two frames is a cut (another camera, a teleport): not faded.
constexpr float kCutRadians = 0.6f;

// What the latch keeps from one frame to the next.
struct Latch {
    std::uint64_t tick = ~0ull;  // the kept tick of the last frame
    unsigned reads = 0;          // the camera's stick reads at that tick's first frame
    unsigned reads_before = 0;   // at the tick before's
    bool steered = false;        // the follow camera read the stick in the kept tick
    std::array<float, 3> hero{}, prev_hero{};
    bool has_hero = false, has_prev_hero = false;
    float picture_yaw = 0.0f;               // the last frame's heading, as shown
    float carry = 0.0f;                     // what a missed guess is off by, fading
    float rate = 0.0f;                      // how fast it turned, radians a second
    float omega = 0.0f, prev_omega = 0.0f;  // the camera's turn in the kept tick, in the one before
    bool was_on = false;                    // the latch turned the last frame
    std::chrono::steady_clock::time_point shown{};
    bool has_shown = false;
    std::optional<std::uint8_t> scripted;  // latch_scripted_stick's
    // OPENRAC_CAMERA_LATCH_LOG: the turn the last tick's frames guessed for one tick, to compare.
    float guessed_tick = 0.0f;
    bool has_guess = false;
};

Latch latch;

float heading(const renderer::Vec3& forward) {
    return std::atan2(forward[1], forward[0]);
}

// a - b, the short way round.
float angle_between(float a, float b) {
    float d = std::fmod(a - b, 2.0f * std::numbers::pi_v<float>);
    if (d > std::numbers::pi_v<float>) {
        d -= 2.0f * std::numbers::pi_v<float>;
    } else if (d < -std::numbers::pi_v<float>) {
        d += 2.0f * std::numbers::pi_v<float>;
    }
    return d;
}

// The camera's turn over the next `ticks` ticks (a fraction too), from its turn in the last two
// (`omega`, `prev_omega`) and the turn a tick the stick asks for (`target`). The game turns its
// camera's offset from the hero at the stick's speed at once, but the camera follows that offset
// through its rig's easing: the picture's turn starts slow and reaches the stick's over some 30
// ticks (measured in Veldin with the stick held right, RAC1 US). So each tick the turn goes on
// changing as it did in the last one, at least a tenth of the way to the stick's, never past it.
float ahead(float omega, float prev_omega, float target, float ticks) {
    constexpr float kEase = 0.1f;
    float total = 0.0f;
    float change = omega - prev_omega;
    for (float left = ticks; left > 0.0f; left -= 1.0f) {
        const float gap = target - omega;
        const float least = gap * kEase;
        if (change * gap <= 0.0f || std::abs(change) < std::abs(least)) {
            change = least;
        }
        if (std::abs(change) > std::abs(gap)) {
            change = gap;
        }
        omega += change;
        total += omega * std::min(left, 1.0f);
    }
    return total;
}

renderer::Vec3 turned(const renderer::Vec3& v, float c, float s) {
    return {v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2]};
}

}  // namespace

CameraTurn latch_camera(
    const Kept& scene, float alpha, std::chrono::steady_clock::time_point display
) {
    static const bool& log_on = stats::probe(
        "OPENRAC_CAMERA_LATCH_LOG", "the late camera latch's guesses against the ticks, in the log"
    );
    const host::GameState shown = scene.has_prev_state && alpha < 1.0f
                                      ? blend_camera(scene.prev_state, scene.state, alpha)
                                      : scene.state;
    const float base = heading(shown.forward);

    // A new tick: the hero's place from it, and whether its camera read the stick.
    const bool new_tick = scene.frame != latch.tick;
    unsigned reads = latch.reads;
    float radians = 0.0f;
    float axis[3] = {0.0f, 0.0f, 1.0f};
    std::uint8_t stick = g && g->window ? g->input.pad(0).right_x : platform::kStickCentre;
    if (stick == platform::kStickCentre && latch.scripted) {
        stick = *latch.scripted;
    }
    const float ticks = static_cast<float>(
        std::chrono::duration<double>(display - pacing.tick).count() / pacing.period.count()
    );
    const bool turning = openrac_game_camera_turn != nullptr
                         && openrac_game_camera_turn(stick, 1.0f, &radians, axis, &reads) != 0;
    if (new_tick) {
        const bool next = scene.frame == latch.tick + 1;
        latch.reads_before = next ? latch.reads : reads;
        latch.reads = reads;
        latch.steered = next && reads != latch.reads_before;
        latch.prev_hero = latch.hero;
        latch.has_prev_hero = latch.has_hero && next;
        latch.has_hero = openrac_game_hero_position != nullptr
                         && openrac_game_hero_position(latch.hero.data()) != 0;
        if (log_on && latch.has_guess && scene.has_prev_state) {
            log::info(
                "camera latch: tick {} turned {:.4f}, guessed {:.4f}",
                scene.frame,
                angle_between(heading(scene.state.forward), heading(scene.prev_state.forward)),
                latch.guessed_tick
            );
        }
        latch.has_guess = false;
        // How far the camera turned in the kept tick and in the one before.
        const float turned =
            scene.has_prev_state
                ? angle_between(heading(scene.state.forward), heading(scene.prev_state.forward))
                : 0.0f;
        latch.prev_omega = next ? latch.omega : 0.0f;
        latch.omega = std::abs(turned) < kCutRadians ? turned : 0.0f;
        latch.tick = scene.frame;
    }

    // The latch's own heading: the rest of the way to the kept tick's camera, then the game's turn
    // from there. The game turns its camera's offset about the camera block's axis, the world's up.
    float wanted = 0.0f;
    const bool on = settings::get().camera_latch && interpolating() && turning && latch.steered
                    && latch.has_hero && std::abs(axis[2]) > 0.9f;
    if (on) {
        const float target = axis[2] > 0.0f ? radians : -radians;
        wanted = angle_between(heading(scene.state.forward), base)
                 + ahead(latch.omega, latch.prev_omega, target, std::clamp(ticks, 0.0f, 2.0f));
    }
    // The guess for one tick, whether the frames are latched or not (a run with one frame a tick
    // has none between ticks), to compare with the turn the tick makes.
    if (log_on && turning && latch.steered && !latch.has_guess) {
        latch.guessed_tick =
            ahead(latch.omega, latch.prev_omega, axis[2] > 0.0f ? radians : -radians, 1.0f);
        latch.has_guess = true;
    }

    // A missed guess fades. Where the latch's heading takes over from another (a new tick, the
    // latch going on or off) the picture goes on turning as it did (the last frames' rate) and the
    // step to the new heading is carried, to fade; a cut is not. Without the latch on either side
    // there is nothing to carry: the interpolated camera is continuous by itself.
    const double since =
        latch.has_shown
            ? std::max(std::chrono::duration<double>(display - latch.shown).count(), 0.0)
            : 0.0;
    latch.carry *= static_cast<float>(std::exp(-since / kFadeSeconds));
    if (latch.has_shown && (on || latch.was_on) && (new_tick || on != latch.was_on)) {
        const float expected = latch.picture_yaw + latch.rate * static_cast<float>(since);
        const float step = angle_between(expected, base + wanted + latch.carry);
        latch.carry = std::abs(step) < kCutRadians ? latch.carry + step : 0.0f;
    }
    if (!settings::get().camera_latch || !interpolating()) {
        latch.carry = 0.0f;
    }
    const float picture = base + wanted + latch.carry;
    if (latch.has_shown && since > 0.0 && since < 0.1) {
        latch.rate = angle_between(picture, latch.picture_yaw) / static_cast<float>(since);
    } else {
        latch.rate = 0.0f;
    }
    latch.was_on = on;
    latch.picture_yaw = picture;
    latch.shown = display;
    latch.has_shown = true;

    CameraTurn result;
    result.yaw = wanted + latch.carry;
    if (result.yaw == 0.0f) {
        return result;
    }
    // About the hero as the frame places him.
    const auto& hero = latch.hero;
    const auto& from = latch.has_prev_hero ? latch.prev_hero : latch.hero;
    const float t = scene.has_prev_state ? alpha : 1.0f;
    for (int i = 0; i < 3; ++i) {
        result.pivot[i] = from[i] + (hero[i] - from[i]) * t;
    }
    return result;
}

void latch_scripted_stick(std::uint8_t right_x) {
    latch.scripted = right_x;
}

host::GameState turned_camera(const host::GameState& state, const CameraTurn& turn) {
    if (turn.yaw == 0.0f) {
        return state;
    }
    host::GameState c = state;
    const float cs = std::cos(turn.yaw);
    const float sn = std::sin(turn.yaw);
    const renderer::Vec3 offset{
        state.camera_position[0] - turn.pivot[0],
        state.camera_position[1] - turn.pivot[1],
        state.camera_position[2] - turn.pivot[2]
    };
    const renderer::Vec3 moved = turned(offset, cs, sn);
    for (int i = 0; i < 3; ++i) {
        c.camera_position[i] = turn.pivot[i] + moved[i];
    }
    c.forward = turned(state.forward, cs, sn);
    c.left = turned(state.left, cs, sn);
    c.up = turned(state.up, cs, sn);
    return c;
}

}  // namespace openrac::frontend

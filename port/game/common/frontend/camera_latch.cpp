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
#include <array>
#include <cmath>
#include <optional>

#include "common/log.h"
#include "common/stats.h"
#include "frontend.h"
#include "game/common/frontend/latch_fade.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "openrac/game_host.h"
#include "settings.h"

namespace openrac::frontend {
namespace {

// The most ticks a frame may follow the last frame's kept tick by and the latch still go on from
// it. A frame cap at or under the game's tick rate (60 frames a second on NTSC's 60 ticks) leaves
// some ticks without a frame of their own: without this the latch went off at each such tick and
// guessed from no turn at all after it, so that the picture stood still and then caught up.
constexpr std::uint64_t kSkippedTicks = 4;

// With the stick let go the game's camera still eases its turn out over some 30 ticks (its rig
// follows the offset the stick turned): the latch goes on guessing that, towards no turn, for at
// most this many ticks or until the turn is under kStillRadians a tick, so that the picture slows
// with the camera rather than stopping ahead of it and turning back.
constexpr unsigned kCoastTicks = 45;
constexpr float kStillRadians = 0.0005f;

// What the latch keeps from one frame to the next.
struct Latch {
    std::uint64_t tick = ~0ull;  // the kept tick of the last frame
    unsigned reads = 0;          // the camera's stick reads at that tick's first frame
    bool steered = false;        // the follow camera read the stick since the last frame's tick
    std::array<float, 3> hero{}, prev_hero{};
    bool has_hero = false, has_prev_hero = false;
    LatchFade fade;       // a missed guess's (latch_fade.h)
    bool was_on = false;  // the latch turned the last frame
    float up = 1.0f;      // the stick's turn's axis (its sign), when last pushed
    unsigned coast = 0;   // the ticks the latch went on after the stick was let go
    float omega = 0.0f, prev_omega = 0.0f;  // the camera's turn in the kept tick, in the one before
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
        // A tick or a few later (the ticks between had no frame): the latch goes on from the last.
        const bool later = scene.frame > latch.tick && scene.frame - latch.tick <= kSkippedTicks;
        latch.steered = later && reads != latch.reads;
        latch.reads = reads;
        latch.prev_hero = latch.hero;
        latch.has_prev_hero = latch.has_hero && next;
        latch.has_hero = openrac_game_hero_position != nullptr
                         && openrac_game_hero_position(latch.hero.data()) != 0;
        // How far the camera turned in the kept tick and in the one before.
        const float turned =
            scene.has_prev_state
                ? angle_between(heading(scene.state.forward), heading(scene.prev_state.forward))
                : 0.0f;
        if (log_on && latch.has_guess && scene.has_prev_state) {
            log::info(
                "camera latch: tick {} turned {:.4f}, guessed {:.4f}",
                scene.frame,
                turned,
                latch.guessed_tick
            );
        }
        latch.has_guess = false;
        // After ticks without a frame, the turn last seen stands for the tick before's.
        latch.prev_omega = later ? latch.omega : 0.0f;
        latch.omega = std::abs(turned) < LatchFade::kCutRadians ? turned : 0.0f;
        latch.tick = scene.frame;
    }

    // The latch's own heading: the rest of the way to the kept tick's camera, then the game's turn
    // from there. The game turns its camera's offset about the camera block's axis, the world's up.
    float wanted = 0.0f;
    const bool pushed = turning && std::abs(axis[2]) > 0.9f;
    if (pushed) {
        latch.up = axis[2] > 0.0f ? 1.0f : -1.0f;
        latch.coast = 0;
    } else if (new_tick && latch.was_on) {
        ++latch.coast;
    }
    const bool coasting = !pushed && latch.was_on && latch.coast <= kCoastTicks
                          && std::abs(latch.omega) >= kStillRadians;
    const bool on = settings::get().responsive_controls && interpolating() && (pushed || coasting)
                    && latch.steered && latch.has_hero;
    if (on) {
        const float target = pushed ? latch.up * radians : 0.0f;
        wanted = angle_between(heading(scene.state.forward), base)
                 + ahead(latch.omega, latch.prev_omega, target, std::clamp(ticks, 0.0f, 2.0f));
    }
    latch.was_on = on;
    // The guess for one tick, whether the frames are latched or not (a run with one frame a tick
    // has none between ticks), to compare with the turn the tick makes.
    if (log_on && turning && latch.steered && !latch.has_guess) {
        latch.guessed_tick =
            ahead(latch.omega, latch.prev_omega, axis[2] > 0.0f ? radians : -radians, 1.0f);
        latch.has_guess = true;
    }

    // A missed guess fades (latch_fade.h). Across a new tick the heading goes on at the turn the
    // game's camera made in the kept tick.
    const double since =
        latch.has_shown
            ? std::max(std::chrono::duration<double>(display - latch.shown).count(), 0.0)
            : 0.0;
    const double period = pacing.period.count();
    latch.fade.frame(
        base + wanted,
        on,
        new_tick,
        period > 0.0 ? latch.omega / static_cast<float>(period) : 0.0f,
        since,
        settings::get().responsive_controls && interpolating()
    );
    latch.shown = display;
    latch.has_shown = true;

    CameraTurn result;
    result.yaw = wanted + latch.fade.carry();
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

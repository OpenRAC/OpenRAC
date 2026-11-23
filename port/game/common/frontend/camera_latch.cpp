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
// A mouse that turns the camera itself (frontend/mouse_turn.h) needs no guess: the next tick turns
// it by exactly the motion read so far. So while it does, each frame shows the kept tick's camera
// turned on by that motion, sideways and up or down, and the picture follows the mouse at the
// display's rate. This is on whatever responsive_controls says: the mouse's turn reaches the game
// at the same tick either way, only the picture shows it sooner.
// The mouse's own motion is never eased: at a new tick only what the game turned besides it fades
// (LatchFade::frame_known), so a quick flick shows at once, as the plain interpolated camera did.
// The mouse has the picture only while it moves and a moment after (MouseHold): a still mouse
// beside a gamepad leaves the stick's frames as they were without the keyboard and mouse.
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
#include "game/common/frontend/mouse_turn.h"
#include "game/common/frontend/scene_frame_internal.h"
#include "openrac/game_host.h"
#include "settings.h"

namespace openrac::frontend {
namespace {

// How many ticks a frame may skip and the latch still carry on from the last frame. A frame cap
// at or under the tick rate (60 on NTSC) leaves some ticks without a frame of their own.
constexpr std::uint64_t kSkippedTicks = 4;

// After the stick is let go, the game's camera still slows down over some 30 ticks. The latch
// keeps predicting that slow-down for up to kCoastTicks ticks, or until the camera turns less than
// kStillRadians a tick, so the picture slows with the camera instead of stopping ahead of it and
// turning back.
constexpr unsigned kCoastTicks = 45;
constexpr float kStillRadians = 0.0005f;

// The longest the mouse's motion is carried on to a frame's showing: a frame is shown within a
// refresh or two of its drawing; a longer wait is a stall, not worth a guess.
constexpr double kMaxLeadSeconds = 0.05;

// What the latch keeps from one frame to the next.
struct Latch {
    std::uint64_t tick = ~0ull;  // the kept tick of the last frame
    unsigned reads = 0;          // the camera's stick reads at that tick's first frame
    bool steered = false;        // the follow camera read the stick since the last frame's tick
    std::array<float, 3> hero{}, prev_hero{};
    bool has_hero = false, has_prev_hero = false;
    LatchFade fade;      // fades a missed guess (latch_fade.h)
    float up = 1.0f;     // the sign of the turn axis when the stick was last pushed
    unsigned coast = 0;  // ticks the latch has run since the stick was let go
    float omega = 0.0f, prev_omega = 0.0f;  // the camera's turn in the kept tick, in the one before
    std::chrono::steady_clock::time_point shown{};
    bool has_shown = false;
    std::optional<std::uint8_t> scripted;  // latch_scripted_stick's
    float mouse_kept = 0.0f;               // the last frame's kept camera heading
    bool mouse_last = false;               // the last frame was turned by the mouse
    std::uint64_t mouse_tick = 0;          // the kept tick of that frame
    MouseHold mouse_hold;                  // whether the mouse has the picture now
    // OPENRAC_CAMERA_LATCH_LOG: the turn the last tick's frames guessed for one tick, to compare.
    float guessed_tick = 0.0f;
    bool has_guess = false;
};

Latch latch;

MouseTurnFn g_mouse_turn = nullptr;

float heading(const renderer::Vec3& forward) {
    return std::atan2(forward[1], forward[0]);
}

// How far a direction points above level, in radians.
float elevation(const renderer::Vec3& v) {
    const float n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    return n > 1e-6f ? std::asin(std::clamp(v[2] / n, -1.0f, 1.0f)) : 0.0f;
}

// `v` turned by `angle` radians about the unit `axis`, right-handed.
renderer::Vec3 rotated(const renderer::Vec3& v, const renderer::Vec3& axis, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const float d = (axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2]) * (1.0f - c);
    const renderer::Vec3 cross{
        axis[1] * v[2] - axis[2] * v[1],
        axis[2] * v[0] - axis[0] * v[2],
        axis[0] * v[1] - axis[1] * v[0]
    };
    return {
        v[0] * c + cross[0] * s + axis[0] * d,
        v[1] * c + cross[1] * s + axis[1] * d,
        v[2] * c + cross[2] * s + axis[2] * d
    };
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
        // One tick on, or a few when some had no frame: carry on from the last frame.
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
        // After skipped ticks, the last turn seen stands in for the tick before.
        latch.prev_omega = later ? latch.omega : 0.0f;
        latch.omega = std::abs(turned) < LatchFade::kCutRadians ? turned : 0.0f;
        latch.tick = scene.frame;
    }

    // The mouse turns the camera itself: the kept tick's heading and tilt, and on from there by
    // the motion the next tick will read. Only about an up that is the world's (not Ratchet on a
    // wall with the boots), which the picture turns about.
    MouseTurn mouse;
    float pending_x = 0.0f;
    float pending_y = 0.0f;
    if (g_mouse_turn != nullptr && g && g->window) {
        g->input.pending_mouse(pending_x, pending_y);
        // And on to when the frame is shown, at the rate the mouse moves now. Frames are drawn at
        // uneven times before they are shown (a tick's at once, the others later): the motion at
        // drawing alone turned the picture unevenly, a fast turn looked doubled at 90 Hz.
        float rate_x = 0.0f;
        float rate_y = 0.0f;
        g->input.mouse_rate(rate_x, rate_y);
        const float lead = static_cast<float>(std::clamp(
            std::chrono::duration<double>(display - std::chrono::steady_clock::now()).count(),
            0.0,
            kMaxLeadSeconds
        ));
        pending_x += rate_x * lead;
        pending_y += rate_y * lead;
    }
    // Asked every frame, so that the ticks' turn it reports is only theirs since the last frame.
    const bool mouse_turns =
        g_mouse_turn != nullptr && g && g->window && g_mouse_turn(pending_x, pending_y, &mouse);
    const float kept_heading = heading(scene.state.forward);
    const float last_kept = latch.mouse_kept;
    latch.mouse_kept = kept_heading;
    // Only while the mouse moves, or did just now: a still mouse beside a gamepad must not take the
    // picture from the stick's latch when the stick comes back to rest.
    const bool moved = mouse.yaw != 0.0f || mouse.pitch != 0.0f || mouse.tick_yaw != 0.0f;
    const double since_shown =
        latch.has_shown
            ? std::max(std::chrono::duration<double>(display - latch.shown).count(), 0.0)
            : 0.0;
    const bool held = latch.mouse_hold.frame(mouse_turns, moved, since_shown);
    const bool mouse_on = interpolating() && held && std::abs(mouse.up[2]) > 0.9f
                          && (mouse.first_person || latch.has_hero);
    // The last frame was the mouse's too, at most a few ticks ago: the move since is known.
    const bool continues = latch.mouse_last && scene.frame >= latch.mouse_tick
                           && scene.frame - latch.mouse_tick <= kSkippedTicks;
    latch.mouse_last = mouse_on;
    latch.mouse_tick = scene.frame;
    if (mouse_on) {
        const float up = mouse.up[2] > 0.0f ? 1.0f : -1.0f;
        const float yaw = angle_between(kept_heading, base) + up * mouse.yaw;
        const double since =
            latch.has_shown
                ? std::max(std::chrono::duration<double>(display - latch.shown).count(), 0.0)
                : 0.0;
        // Where the picture is by the mouse alone: the last frame's kept camera, turned by what
        // the ticks since read from the mouse and by what is still unread. Only the rest (the game
        // turning the camera itself) fades; the mouse's motion shows at once.
        const float expected = last_kept + up * (mouse.tick_yaw + mouse.yaw);
        latch.fade.frame_known(base + yaw, expected, continues && since < 0.1, since);
        latch.shown = display;
        latch.has_shown = true;
        latch.coast = kCoastTicks + 1;  // the stick's coasting does not follow on
        CameraTurn result;
        result.yaw = yaw + latch.fade.carry();
        // The tilt: the kept tick's, then down by the motion's, within the game's limit.
        const float limit = mouse.pitch_limit > 0.0f ? mouse.pitch_limit : 1.5f;
        const float now = elevation(shown.forward);
        const float kept = elevation(scene.state.forward);
        const float wanted = std::clamp(kept - mouse.pitch, -limit, limit);
        result.pitch = std::abs(kept) <= limit ? wanted - now : kept - now;
        if (mouse.first_person) {
            result.pivot = shown.camera_position;
        } else {
            const auto& hero = latch.hero;
            const auto& from = latch.has_prev_hero ? latch.prev_hero : latch.hero;
            const float t = scene.has_prev_state ? alpha : 1.0f;
            for (int i = 0; i < 3; ++i) {
                result.pivot[i] = from[i] + (hero[i] - from[i]) * t;
            }
        }
        return result;
    }

    // The latch's own heading: the rest of the way to the kept tick's camera, then the game's turn
    // from there. The game turns its camera's offset about the camera block's axis, the world's up.
    float wanted = 0.0f;
    const bool pushed = turning && std::abs(axis[2]) > 0.9f;
    if (pushed) {
        latch.up = axis[2] > 0.0f ? 1.0f : -1.0f;
        latch.coast = 0;
    } else if (new_tick && latch.fade.was_on()) {
        ++latch.coast;
    }
    const bool coasting = !pushed && latch.fade.was_on() && latch.coast <= kCoastTicks
                          && std::abs(latch.omega) >= kStillRadians;
    const bool on = settings::get().responsive_controls && interpolating() && (pushed || coasting)
                    && latch.steered && latch.has_hero;
    if (on) {
        const float target = pushed ? latch.up * radians : 0.0f;
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

    // A missed guess fades (latch_fade.h). Across a new tick the heading moves on at the rate the
    // game's camera turned in the kept tick.
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
    if (log_on && !new_tick) {
        // A frame between ticks: how far the latch turns it from the interpolated camera.
        log::info(
            "camera latch: frame between ticks {} alpha {:.2f} yaw {:.4f}",
            scene.frame,
            alpha,
            result.yaw
        );
    }
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

void set_mouse_turn(MouseTurnFn fn) {
    g_mouse_turn = fn;
}

host::GameState turned_camera(const host::GameState& state, const CameraTurn& turn) {
    if (turn.yaw == 0.0f && turn.pitch == 0.0f) {
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
    if (turn.pitch == 0.0f) {
        return c;
    }
    // Up about the side axis: a turn about `left` by a negative angle lifts `forward`.
    const float n =
        std::sqrt(c.left[0] * c.left[0] + c.left[1] * c.left[1] + c.left[2] * c.left[2]);
    if (!(n > 1e-6f)) {
        return c;
    }
    const renderer::Vec3 axis{c.left[0] / n, c.left[1] / n, c.left[2] / n};
    const renderer::Vec3 offset2{
        c.camera_position[0] - turn.pivot[0],
        c.camera_position[1] - turn.pivot[1],
        c.camera_position[2] - turn.pivot[2]
    };
    const renderer::Vec3 moved2 = rotated(offset2, axis, -turn.pitch);
    for (int i = 0; i < 3; ++i) {
        c.camera_position[i] = turn.pivot[i] + moved2[i];
    }
    c.forward = rotated(c.forward, axis, -turn.pitch);
    c.up = rotated(c.up, axis, -turn.pitch);
    return c;
}

}  // namespace openrac::frontend

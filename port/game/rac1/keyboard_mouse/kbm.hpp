// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The keyboard and mouse mod's own logic, apart from the game and the port so it is tested alone:
// the actions and their binds, conflicts between them, the keys to the left stick, the mouse to
// the right stick and to the camera itself, the mouse to the Quick Select ring's direction, and
// the DualShock report the game reads. keyboard_mouse.cpp joins it to the game.
#ifndef KBM_HPP
#define KBM_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "openrac/keys.h"

namespace kbm {

// ---- Actions ----

// What a bind does. The pad buttons are Ratchet & Clank's: Cross jumps, Square swings the wrench,
// Circle fires, Triangle holds the Quick Select open, R1 crouches, L1 looks in first person, L2
// strafes (with the lock-strafe mod), Select opens the map, Start pauses.
enum class Action : int {
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    Walk,
    Jump,
    Fire,
    QuickSelect,
    Wrench,
    Crouch,
    Strafe,
    FirstPerson,
    Map,
    CameraReset,
    Pause,
    Weapon1,
    Weapon2,
    Weapon3,
    Weapon4,
    Weapon5,
    Weapon6,
    Weapon7,
    Weapon8,
    WeaponNext,
    WeaponPrev,
    MenuConfirm,
    MenuBack,
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight,
    MenuCircle,
    Count,
};
inline constexpr int kActionCount = static_cast<int>(Action::Count);

// When an action's binds count: "play" ones always (the keyboard's; the mouse's only in play, with
// the cursor captured), "menu" ones only out of play (the title, a menu, the pause screen, a
// dialog, a cutscene or movie; the mouse's while the cursor is over the game's picture). What a
// bind presses depends on where the game is (Context, buttons_for). Two actions of the same group
// may not share an input; of different groups they may.
enum class Group : int {
    Play,
    Menu
};

// One row of the action table: the setting that binds the action and which group it counts in.
struct ActionInfo {
    const char* key;       // the setting's key in openrac-mod.json
    const char* fallback;  // its default, as the manifest's
    Group group;
};

// In Action's order; the manifest lists the same keys and defaults (a test checks they agree).
inline constexpr std::array<ActionInfo, kActionCount> kActions = {{
    {"move_forward", "KeyW", Group::Play},
    {"move_back", "KeyS", Group::Play},
    {"move_left", "KeyA", Group::Play},
    {"move_right", "KeyD", Group::Play},
    {"walk", "AltLeft", Group::Play},
    {"jump", "Space", Group::Play},
    {"fire", "MouseLeft", Group::Play},
    {"quick_select", "MouseRight", Group::Play},
    {"wrench", "KeyF,MouseMiddle", Group::Play},
    {"crouch", "ControlLeft,KeyC", Group::Play},
    {"strafe", "ShiftLeft", Group::Play},
    {"first_person", "KeyV,MouseBack", Group::Play},
    {"map", "Tab,KeyM", Group::Play},
    {"camera_reset", "KeyQ", Group::Play},
    {"pause", "Escape,KeyP", Group::Play},
    {"weapon_1", "Digit1", Group::Play},
    {"weapon_2", "Digit2", Group::Play},
    {"weapon_3", "Digit3", Group::Play},
    {"weapon_4", "Digit4", Group::Play},
    {"weapon_5", "Digit5", Group::Play},
    {"weapon_6", "Digit6", Group::Play},
    {"weapon_7", "Digit7", Group::Play},
    {"weapon_8", "Digit8", Group::Play},
    {"weapon_next", "WheelDown", Group::Play},
    {"weapon_prev", "WheelUp", Group::Play},
    {"menu_confirm", "Enter,MouseLeft", Group::Menu},
    {"menu_back", "Backspace,MouseRight", Group::Menu},
    {"menu_up", "ArrowUp,WheelUp", Group::Menu},
    {"menu_down", "ArrowDown,WheelDown", Group::Menu},
    {"menu_left", "ArrowLeft", Group::Menu},
    {"menu_right", "ArrowRight", Group::Menu},
    {"menu_circle", "KeyE", Group::Menu},
}};

// How many binds one action may have (assumed: the launcher's two columns).
inline constexpr int kMaxBinds = 2;

// One action's binds (OPENRAC_BIND_*, a key's HID usage); 0 for an empty slot.
using Binds = std::array<int, kMaxBinds>;

// A keybind setting's text ("KeyW,ArrowUp") as binds: names of openrac/keys.h, split at commas or
// spaces. False, and no binds, when a name is unknown, one is there twice, or there are more than
// two: the caller then takes the default (as the port does with a value it would not take).
inline bool parse_binds(std::string_view text, Binds& out) {
    out = {};
    int count = 0;
    std::size_t i = 0;

    // Walk the text one name at a time.
    while (i < text.size()) {
        // A separator between names holds no bind: step over it.
        if (text[i] == ',' || text[i] == ' ') {
            ++i;
            continue;
        }

        std::size_t end = i;

        // Find where this name ends.
        while (end < text.size() && text[end] != ',' && text[end] != ' ') {
            ++end;
        }

        const int bind = openrac_bind_from_name_n(text.data() + i, end - i);

        // An unknown name, a third bind or a repeated one makes the whole setting invalid.
        if (bind == OPENRAC_BIND_NONE || count == kMaxBinds
            || std::find(out.begin(), out.end(), bind) != out.end()) {
            out = {};
            return false;
        }

        out[static_cast<std::size_t>(count++)] = bind;
        i = end;
    }

    return true;
}

// Every action's binds, indexed by Action.
struct Bindings {
    std::array<Binds, kActionCount> binds{};

    const Binds& of(Action a) const { return binds[static_cast<std::size_t>(a)]; }
};

// The bindings from each action's setting text (`text(key)`), the default where a text is not one.
template <class Text>
Bindings read_bindings(Text&& text) {
    Bindings b;

    // Read each action's setting in turn.
    for (int a = 0; a < kActionCount; ++a) {
        const ActionInfo& info = kActions[static_cast<std::size_t>(a)];

        // A text that is not a valid bind list falls back to the action's default.
        if (!parse_binds(text(info.key), b.binds[static_cast<std::size_t>(a)])) {
            parse_binds(info.fallback, b.binds[static_cast<std::size_t>(a)]);
        }
    }

    return b;
}

// Two actions of one group that share an input (the port runs both; the mod only warns).
struct Conflict {
    Action a;
    Action b;
    int bind;
};

// Inputs bound to two actions of the same group: each pair once, a before b.
inline std::vector<Conflict> conflicts(const Bindings& b) {
    std::vector<Conflict> out;

    // Take each action as the first of a pair.
    for (int x = 0; x < kActionCount; ++x) {
        // Pair it with each later action, so a conflict is reported once.
        for (int y = x + 1; y < kActionCount; ++y) {
            // Only actions of the same group conflict: a play bind and a menu bind never count together.
            if (kActions[static_cast<std::size_t>(x)].group
                != kActions[static_cast<std::size_t>(y)].group) {
                continue;
            }

            // Compare each bind of the first action...
            for (int bx : b.binds[static_cast<std::size_t>(x)]) {
                // ...with each bind of the second.
                for (int by : b.binds[static_cast<std::size_t>(y)]) {
                    // An empty slot (0) conflicts with nothing; the same input twice does.
                    if (bx != 0 && bx == by) {
                        out.push_back({static_cast<Action>(x), static_cast<Action>(y), bx});
                    }
                }
            }
        }
    }

    return out;
}

// The mouse's buttons and its wheel: they count only while the cursor is captured.
inline bool is_mouse(int bind) {
    return bind >= OPENRAC_BIND_MOUSE_LEFT;
}

// ---- The left stick from keys ----

// The player's settings for the movement keys.
struct MoveTuning {
    float walk = 0.5f;        // how far the stick goes while walking (0..1)
    float rise_ticks = 4.0f;  // ticks from rest to full (0: at once)
};

// Digital keys to an analog stick: the four directions as a vector (diagonals on the circle, not
// the square), scaled for walking, eased toward that target each tick so a tap is a nudge and a
// hold runs, and let go twice as fast as it rises so a stop is crisp. x > 0 right, y > 0 back
// (down on the stick, as the console's bytes).
struct KeyStick {
    float x = 0.0f;
    float y = 0.0f;

    // One tick of the keys: moves x and y toward what the keys held ask for.
    void step(bool forward, bool back, bool left, bool right, bool walk, const MoveTuning& t) {
        float tx = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
        float ty = (back ? 1.0f : 0.0f) - (forward ? 1.0f : 0.0f);
        const float length = std::sqrt(tx * tx + ty * ty);

        // Two keys held for a diagonal reach past the unit circle: scale back so a diagonal is no faster.
        if (length > 1.0f) {
            tx /= length;
            ty /= length;
        }

        // Walking holds the stick part of the way.
        if (walk) {
            tx *= t.walk;
            ty *= t.walk;
        }

        const float rise = t.rise_ticks <= 0.0f ? 2.0f : 1.0f / t.rise_ticks;
        x = approach(x, tx, rise);
        y = approach(y, ty, rise);
    }

    // `now` moved toward `target` by at most `rise`, or twice that when it is slowing.
    static float approach(float now, float target, float rise) {
        // Toward the centre (a key let go, or the other way pressed): twice as fast.
        const bool slowing = std::fabs(target) < std::fabs(now) || target * now < 0.0f;
        const float step = slowing ? rise * 2.0f : rise;

        // Rising to a target above: do not overshoot it.
        if (now < target) {
            return std::min(now + step, target);
        }

        return std::max(now - step, target);
    }
};

// ---- The right stick from the mouse ----

// The player's settings for the mouse's look.
struct LookTuning {
    float sensitivity = 1.0f;   // the mouse's turn, 1 the default
    float acceleration = 0.0f;  // 0: none; more turns a fast flick further than a slow drag
    bool invert_y = false;
    bool pitch_holds = true;  // the camera's tilt stays where the mouse leaves it
};

// The mouse's motion over a tick, at sensitivity 1, that pushes the stick all the way.
inline constexpr float kPixelsForFullTurn = 24.0f;  // (assumed)
// The tilt's full travel, in pixels at sensitivity 1.
inline constexpr float kPixelsForFullTilt = 160.0f;  // (assumed)
// How much of the held tilt stays each tick while Levels out eases it back (assumed).
inline constexpr float kTiltEaseBack = 0.9f;
// The held tilt below which Levels out snaps it to level (assumed).
inline constexpr float kTiltSnap = 0.01f;

// The mouse to the right stick. The follow camera turns at a rate the stick's x sets, so x is the
// tick's motion (eased over two ticks, so a mouse polled unevenly against the game's ticks does
// not stutter); it tilts toward an elevation the stick's y sets, so y keeps where the mouse put it
// (pitch_holds) or eases back to level. In first person both axes are rates, so y is the tick's
// motion there.
struct MouseLook {
    float x = 0.0f;
    float y = 0.0f;
    float tilt = 0.0f;  // the held elevation, -1 (up) .. 1 (down)
    float last_dx = 0.0f;

    // The gain for a tick's motion of `speed` pixels: 1 without acceleration.
    static float gain(float speed, const LookTuning& t) {
        // Without acceleration every speed has the same gain.
        if (t.acceleration <= 0.0f) {
            return 1.0f;
        }

        return 1.0f + t.acceleration * std::min(speed / kPixelsForFullTurn, 2.0f);
    }

    // One tick of the mouse's motion (dx right, dy down, in pixels) into x and y, as the stick.
    void step(float dx, float dy, bool first_person, const LookTuning& t) {
        const float sens = std::max(t.sensitivity, 0.0f);
        const float ydir = t.invert_y ? -1.0f : 1.0f;
        const float smooth_dx = 0.5f * (dx + last_dx);
        last_dx = dx;
        const float speed = std::sqrt(smooth_dx * smooth_dx + dy * dy);
        x = clamp1(smooth_dx * sens * gain(speed, t) / kPixelsForFullTurn);

        // In first person y is a rate like x, so it follows this tick's motion.
        if (first_person) {
            y = clamp1(dy * ydir * sens * gain(speed, t) / kPixelsForFullTurn);
            return;
        }

        tilt = clamp1(tilt + dy * ydir * sens / kPixelsForFullTilt);

        // The player chose Levels out: the held tilt eases back to level while the mouse rests.
        if (!t.pitch_holds && dy == 0.0f) {
            tilt *= kTiltEaseBack;

            // Snap the last sliver to level so that it does not creep on for ever.
            if (std::fabs(tilt) < kTiltSnap) {
                tilt = 0.0f;
            }
        }

        y = tilt;
    }

    void reset() { *this = MouseLook{}; }

    // `v` kept within -1 to 1, a stick's travel.
    static float clamp1(float v) { return std::max(-1.0f, std::min(1.0f, v)); }
};

// ---- The camera turned by the mouse itself ----
//
// The follow camera (behind Ratchet) and the first-person view are turned by the mouse directly:
// a tick's motion turns the view by an angle in proportion to it, that same tick, with none of the
// stick's easing, ramp or smoothing, and whatever the game's own invert options say (those are the
// pad's). Mouse right turns the view right, mouse up looks up (unless the mod's own Invert is on).
// The game's frame: z up, x and y level; its rotations (build_look_at_matrix) are right-handed, so
// a positive angle about up turns a view to the left as seen from above.

// A vector in the game's frame, for the camera's turns.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Vec3 scaled(Vec3 a, float k) {
    return {a.x * k, a.y * k, a.z * k};
}

inline Vec3 plus(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 minus(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float length(Vec3 a) {
    return std::sqrt(dot(a, a));
}

inline float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

// `v` at unit length; `fallback` for a vector too short to say a direction (or not a number).
inline Vec3 unit_or(Vec3 v, Vec3 fallback) {
    const float n = length(v);

    // A vector too short to give a direction, or not a number, has no unit vector.
    if (!(n > 1e-6f) || !std::isfinite(n)) {
        return fallback;
    }

    return scaled(v, 1.0f / n);
}

// The game's up (z), when the camera block gives none.
inline constexpr Vec3 kWorldUp{0.0f, 0.0f, 1.0f};

// `v` turned by `angle` about the unit axis `axis`, right-handed, as the game turns its vectors.
inline Vec3 rotate(Vec3 v, Vec3 axis, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const Vec3 along = scaled(axis, dot(axis, v) * (1.0f - c));
    return plus(plus(scaled(v, c), scaled(cross(axis, v), s)), along);
}

// How far `v` points above level (radians, below level < 0), against the unit `up`.
inline float elevation(Vec3 v, Vec3 up) {
    const float n = length(v);

    // A zero-length vector points nowhere, so it is level.
    if (!(n > 1e-6f)) {
        return 0.0f;
    }

    return std::asin(std::max(-1.0f, std::min(1.0f, dot(v, up) / n)));
}

// `v` with its elevation set to `e`: the same length and the same way round about up. A vector
// straight up or down, with no way round, is left as it is.
inline Vec3 with_elevation(Vec3 v, Vec3 up, float e) {
    const float n = length(v);
    const Vec3 level = minus(v, scaled(up, dot(v, up)));
    const float h = length(level);

    // A vector with no level part (straight up or down) has no way round to keep.
    if (!(n > 1e-6f) || !(h > 1e-6f * n)) {
        return v;
    }

    return plus(scaled(level, n * std::cos(e) / h), scaled(up, n * std::sin(e)));
}

// A tick's motion at sensitivity 1: radians a pixel (a 1,000-pixel sweep turns about 86 degrees).
inline constexpr float kRadiansPerPixel = 0.0015f;  // (assumed)

// The turn and the tilt a tick's mouse motion asks for. yaw is the angle about up (the game's
// sense: > 0 turns the view left, so mouse right gives < 0); pitch > 0 looks down (mouse down,
// or mouse up with Invert on). The mod's acceleration, off by default, is the only curve.
struct DirectLook {
    static float scale(float dx, float dy, const LookTuning& t) {
        const float speed = std::sqrt(dx * dx + dy * dy);
        return std::max(t.sensitivity, 0.0f) * MouseLook::gain(speed, t) * kRadiansPerPixel;
    }

    static float yaw(float dx, float dy, const LookTuning& t) { return -dx * scale(dx, dy, t); }

    static float pitch(float dx, float dy, const LookTuning& t) {
        return (t.invert_y ? -dy : dy) * scale(dx, dy, t);
    }
};

// The follow camera's tilt. The game keeps the eye at an elevation above Ratchet that its right
// stick sets: the stick's y (past a dead zone of 0.3, then scaled back to 0..1) is a target w in
// -1..1, eased, and the eye is turned toward an elevation whose atan(sin) is w times 40 degrees
// (FollowCamera_ReadStickInput and Cam_SteerHeadingFromStick). An eye above Ratchet looks down.
//
// 40 degrees, the target at w = 1 (seen in game code).
inline constexpr float kFollowTiltRange = 0.69813168f;
// The stick's dead zone the game applies to the tilt (seen in game code).
inline constexpr float kFollowDeadZone = 0.3f;
// 1 / 0.7, what scales the stick past the dead zone back to 0..1 (seen in game code).
inline constexpr float kFollowDeadZoneScale = 1.4285715f;

// The eye's elevation at w = +-1: asin(tan(40 degrees)), about 57 degrees.
inline float follow_max_elevation() {
    return std::asin(std::tan(kFollowTiltRange));
}

// The game's target for an eye at elevation `e` (radians): the w that holds it there.
inline float follow_target_of(float e) {
    const float w = std::atan(std::sin(e)) / kFollowTiltRange;
    return std::max(-1.0f, std::min(1.0f, w));
}

// The stick's y the game reads as the target w, whichever way its own vertical invert option is
// set (`game_inverts`: the option's word nonzero, which makes the game read the stick's y negated).
// Never 0: a stick at rest makes the game use its own resting height instead.
inline float follow_stick_for(float w, bool game_inverts) {
    w = std::max(-1.0f, std::min(1.0f, w));
    float in = kFollowDeadZone;  // just at the dead zone's edge: w = 0

    // Past the dead zone's edge on the side the target is on, so that the game reads it as w.
    if (w > 0.0f) {
        in = w / kFollowDeadZoneScale + kFollowDeadZone;
    } else if (w < 0.0f) {
        in = w / kFollowDeadZoneScale - kFollowDeadZone;
    }

    return game_inverts ? -in : in;
}

// The first-person view's tilt: the game keeps how far it looks down (radians, up < 0) and eases
// it off past 64 degrees either way; the mouse stops there (seen in game code).
inline constexpr float kFirstPersonTiltLimit = 1.1170107f;

// ---- The Quick Select ring from the mouse ----

// While the Quick Select is held open, the mouse's motion adds up to a pointer from the ring's
// centre; past the dead zone it points the left stick all the way along it (the game picks a slot
// only for a stick pushed past 0.9, and keeps a slot until the stick turns well away from it), and
// within it the stick rests. The pointer is held within twice the dead zone, so turning back the
// other way answers at once.
struct RingPointer {
    float x = 0.0f;
    float y = 0.0f;

    void reset() {
        x = 0.0f;
        y = 0.0f;
    }

    void add(float dx, float dy, float dead_zone) {
        x += dx;
        y += dy;
        const float limit = 2.0f * std::max(dead_zone, 1.0f);
        const float length = std::sqrt(x * x + y * y);

        // Keep the pointer within the limit, so that turning back the other way answers at once.
        if (length > limit) {
            x *= limit / length;
            y *= limit / length;
        }
    }

    // The stick (x right, y down), unit length or (0, 0).
    std::pair<float, float> stick(float dead_zone) const {
        const float length = std::sqrt(x * x + y * y);

        // Within the dead zone the stick rests.
        if (length <= dead_zone || length == 0.0f) {
            return {0.0f, 0.0f};
        }

        return {x / length, y / length};
    }
};

// The slot the game's ring gives a stick pointing (x, y), y down: its slots are 45 degrees apart,
// slot 0 at the top and going clockwise (slot i's icon is at angle i * pi / 4 - pi / 2), each
// taking the 45 degrees around it. -1 for a stick at rest. What the tests check the pointer against.
inline int ring_slot(float x, float y) {
    // A stick at rest points at no slot.
    if (x == 0.0f && y == 0.0f) {
        return -1;
    }

    const float pi = 3.14159265f;
    float a = std::atan2(y, x) + pi / 2.0f;  // 0 at the top, clockwise

    // Bring the angle into 0 to 2 pi.
    while (a < 0.0f) {
        a += 2.0f * pi;
    }

    return static_cast<int>(std::floor(a / (pi / 4.0f) + 0.5f)) % 8;
}

// ---- The report the game reads ----

// The pad report's buttons (bytes 0 and 1, little-endian, a bit clear while held) in the console's
// order.
enum Button : std::uint16_t {
    kSelect = 1u << 0,
    kL3 = 1u << 1,
    kR3 = 1u << 2,
    kStart = 1u << 3,
    kUp = 1u << 4,
    kRight = 1u << 5,
    kDown = 1u << 6,
    kLeft = 1u << 7,
    kL2 = 1u << 8,
    kR2 = 1u << 9,
    kL1 = 1u << 10,
    kR1 = 1u << 11,
    kTriangle = 1u << 12,
    kCircle = 1u << 13,
    kCross = 1u << 14,
    kSquare = 1u << 15,
};

// The button of each pressure byte, bytes 6..17: right, left, up, down, triangle, circle, cross,
// square, L1, R1, L2, R2.
inline constexpr std::array<int, 12> kPressureBits = {5, 7, 4, 6, 12, 13, 14, 15, 10, 11, 8, 9};

// The resting byte of a stick axis (seen in game code).
inline constexpr std::uint8_t kStickRestByte = 0x80;
// The byte the game counts a stick axis from (seen in game code).
inline constexpr int kStickCentreByte = 0x7F;
// How far from the centre byte the game ignores (seen in game code).
inline constexpr int kStickDeadBytes = 0x30;
// How far past the dead zone the game reads full travel (seen in game code).
inline constexpr int kStickRangeBytes = 0x4C;

// The game's stick reading: an axis's byte b counts from 0x7F, nothing within 0x30 of it, then
// (|b - 0x7F| - 0x30) / 0x4C up to 1. A value v (-1..1) as the byte the game reads back as about
// v, so a walk at 0.5 is a walk at 0.5 and not lost in the dead zone; 0 is the centre, 0x80.
inline std::uint8_t stick_byte(float v) {
    // A centred axis is the resting byte exactly.
    if (v == 0.0f) {
        return kStickRestByte;
    }

    const float magnitude = std::min(std::fabs(v), 1.0f);
    const float distance = kStickDeadBytes + 1 + magnitude * (kStickRangeBytes - 1);
    const float b = kStickCentreByte + (v > 0.0f ? distance : -distance);
    return static_cast<std::uint8_t>(std::max(0.0f, std::min(255.0f, std::round(b))));
}

// What the game makes of a byte (its own reading, for the tests).
inline float game_axis(std::uint8_t b) {
    const int d = static_cast<int>(b) - kStickCentreByte;
    const int m = d < 0 ? -d : d;

    // Within the game's dead zone the axis reads as 0.
    if (m < kStickDeadBytes) {
        return 0.0f;
    }

    const float v = std::min(1.0f, static_cast<float>(m - kStickDeadBytes) / kStickRangeBytes);
    return d < 0 ? -v : v;
}

// The pad as the keyboard and mouse hold it this tick.
struct Virtual {
    std::uint16_t held = 0;  // Button bits held (set while held)
    float left_x = 0.0f;
    float left_y = 0.0f;
    float right_x = 0.0f;
    float right_y = 0.0f;
};

// Into the game's report (18 bytes: buttons, right stick, left stick, twelve pressures), on top of
// the gamepad's: our buttons are held as well as the pad's, and each stick axis of ours that is
// off centre takes that axis's place.
inline void merge(const Virtual& v, std::uint8_t* report, int size) {
    // A report too short for the sticks is left alone.
    if (size < 6) {
        return;
    }

    std::uint16_t buttons = static_cast<std::uint16_t>(report[0] | (report[1] << 8));
    buttons = static_cast<std::uint16_t>(buttons & ~v.held);
    report[0] = static_cast<std::uint8_t>(buttons);
    report[1] = static_cast<std::uint8_t>(buttons >> 8);
    const float axes[4] = {v.right_x, v.right_y, v.left_x, v.left_y};

    // Each of our axes that is off centre takes the place of the gamepad's.
    for (int i = 0; i < 4; ++i) {
        // An axis at rest leaves the gamepad's byte as it is.
        if (axes[i] != 0.0f) {
            report[2 + i] = stick_byte(axes[i]);
        }
    }

    // Each button of ours that has a pressure byte in the report presses it fully.
    for (int i = 0; i < 12 && 6 + i < size; ++i) {
        // Only held buttons press their pressure byte.
        if ((v.held >> kPressureBits[static_cast<std::size_t>(i)]) & 1u) {
            report[6 + i] = 0xFF;
        }
    }
}

// The Quick Select's slots (seen in game code).
inline constexpr int kQuickSelectSlots = 8;

// The next item in the Quick Select's eight slots from `current` (`step` +1 or -1), skipping
// empty slots (0); `current` itself when no other slot has one. An item not in the ring starts
// from before the first slot (or after the last, going back).
inline int cycle_item(
    const std::array<std::int32_t, kQuickSelectSlots>& slots, std::int32_t current, int step
) {
    int at = -1;

    // Find where the current item sits in the ring.
    for (int i = 0; i < kQuickSelectSlots; ++i) {
        // An empty slot never matches.
        if (slots[static_cast<std::size_t>(i)] != 0
            && slots[static_cast<std::size_t>(i)] == current) {
            at = i;
            break;
        }
    }

    // An item not in the ring starts from before the first slot, or after the last going back.
    if (at < 0) {
        at = step > 0 ? -1 : kQuickSelectSlots;
    }

    // Walk round the ring from there, one slot at a time.
    for (int n = 1; n <= kQuickSelectSlots; ++n) {
        const int i = ((at + step * n) % kQuickSelectSlots + kQuickSelectSlots) % kQuickSelectSlots;
        const std::int32_t item = slots[static_cast<std::size_t>(i)];

        // The first slot that holds a different item is the one to take.
        if (item != 0 && item != current) {
            return item;
        }
    }

    return current;
}

// ---- Where the game is ----

// What the game is doing, which decides what the keys press. From the game's mode word: a level's
// (its program's copy) when a level is loaded, else the boot program's (the title and the main
// menu run there, with no level).
enum class Context : int {
    Play,       // a level, mode 0
    Title,      // the boot program's "Press Start" (mode 0 there): Start or Cross starts
    Menu,       // the main menu, the pause menu and its pages, the ship's map (mode 3)
    Vendor,     // the vendor (mode 5)
    Dialog,     // a frozen game asking (mode 4): quit?, a memory card message
    Cutscene,   // a level's in-game scene (mode 2): Start skips (the port adds what the game needs)
    Movie,      // a movie (mode 1, and the port's own player): Start skips
    Flight,     // the ship: landing, take-off, the flight between planets (mode 6)
    Slideshow,  // mode 7: Start skips
};

// The boot program's and a level's modes (games/rac1 map_002465F8.c's dispatch).
inline Context context_of(int loaded_level, int level_mode, int boot_mode) {
    const int mode = loaded_level >= 0 ? level_mode : boot_mode;

    // The mode word maps to one context each; anything unknown is a menu.
    switch (mode) {
        case 0:
            return loaded_level >= 0 ? Context::Play : Context::Title;
        case 1:
            return Context::Movie;
        case 2:
            return Context::Cutscene;
        case 4:
            return Context::Dialog;
        case 5:
            return Context::Vendor;
        case 6:
            return Context::Flight;
        case 7:
            return Context::Slideshow;
        default:  // 3, the debug menu's -1, and anything new: a menu's keys are the safe ones
            return Context::Menu;
    }
}

// Whether the game is in a scene, a movie, the ship or a slideshow, where Start skips.
inline bool skips(Context c) {
    return c == Context::Cutscene || c == Context::Movie || c == Context::Flight
           || c == Context::Slideshow;
}

// A set of actions, one bit each.
using ActionSet = std::uint64_t;

// The one-bit set of an action.
inline constexpr ActionSet bit(Action a) {
    return ActionSet{1} << static_cast<int>(a);
}

// The freeze dialogs' kinds (the record's +0): 0 "Quit race?" (Cross quits, Circle or Start goes
// on), 1 and 4 "Quit?" (Triangle quits, Cross goes on); the rest are messages Cross or Triangle
// closes, and the memory card's questions (kind 3, update_mode_freeze's states 6 and 13: format,
// create save data), which take Circle for yes and Triangle for no.
inline constexpr int kDialogQuitRace = 0;  // (seen in game code)

// The pad's buttons the held actions press where the game is. In play, the console's layout; in
// the title, menus and dialogs, confirm (Space, Enter, left click) is Cross and back (Escape,
// Backspace, right click) Triangle, except where that would quit: in a "Quit?" dialog, Escape goes
// on (Cross), and only Backspace or right click is Triangle; in a scene, a movie, the ship or a
// slideshow, every one of them is Start (and Cross for the ship's landing and take-off), which
// skips. On the map (map_screen) confirm is Circle, which travels. Menus' own Circle (the quit
// page's quit, a cycled value's step back) has a bind of its own. The directions are not here: the
// stick and the d-pad come apart (step()).
inline std::uint16_t buttons_for(
    Context c, ActionSet held, int dialog_kind, bool map_screen = false
) {
    const auto on = [&](Action a) {
        return (held & bit(a)) != 0;
    };
    const bool confirm = on(Action::Jump) || on(Action::MenuConfirm);
    const bool back = on(Action::MenuBack);
    const bool escape = on(Action::Pause);
    std::uint16_t b = 0;
    const auto set = [&](bool when, std::uint16_t button) {
        // A button is added only when its condition holds.
        if (when) {
            b = static_cast<std::uint16_t>(b | button);
        }
    };

    // Each context presses its own set of buttons.
    switch (c) {
        case Context::Play:
            set(on(Action::Jump), kCross);
            set(on(Action::Fire), kCircle);
            set(on(Action::Wrench), kSquare);
            set(on(Action::Crouch), kR1);
            set(on(Action::Strafe), kL2);
            set(on(Action::FirstPerson), kL1);
            set(on(Action::Map), kSelect);
            set(on(Action::Pause), kStart);
            break;
        case Context::Title:
            set(confirm || escape, kStart);
            break;
        case Context::Menu:
        case Context::Vendor:
            // On the map confirm travels (Circle) and Space turns the page (Cross).
            if (map_screen) {
                // The map (the ship's and the pause menu's): Circle goes to the planet shown,
                // Cross turns the page; so confirm (Enter, a click) travels and Space turns.
                set(on(Action::MenuConfirm), kCircle);
                set(on(Action::Jump), kCross);
            } else {
                set(confirm, kCross);
            }

            set(on(Action::MenuCircle),
                kCircle);  // Circle where a page reads it (quit, a value back)
            set(back || escape, kTriangle);
            set(on(Action::Fire), kCircle);  // the map's Circle: go to the planet
            set(on(Action::Wrench), kSquare);
            set(on(Action::Crouch), kR1);  // next slot, next planet
            set(on(Action::FirstPerson), kL1);
            set(on(Action::Map), kSelect);  // closes the pause menu
            break;
        case Context::Dialog:
            set(confirm, kCross);
            set(on(Action::MenuCircle), kCircle);  // the memory card's yes (create, format)

            // The race dialog goes on with Start and quits with Circle; the others go on with Cross.
            if (dialog_kind == kDialogQuitRace) {
                set(escape, kStart);  // goes on racing
                set(back, kCircle);
            } else {
                set(escape, kCross);  // goes on
                set(back, kTriangle);
            }

            break;
        case Context::Cutscene:
        case Context::Movie:
        case Context::Slideshow:
            set(confirm || back || escape, kStart);
            break;
        case Context::Flight:
            set(confirm || back || escape, static_cast<std::uint16_t>(kStart | kCross));
            break;
    }

    return b;
}

// Actions held when the game changes where it is stay out until they are let go: Escape that
// paused does not also go back in the menu it opened, Space that confirmed does not jump.
struct Latch {
    ActionSet blocked = 0;
    ActionSet before = 0;  // what was held the tick before
    Context last = Context::Play;
    bool first = true;

    // A key pressed just as the game changes (down this tick, not the last) is a new press, and
    // counts.
    ActionSet filter(Context now, ActionSet held) {
        // The first tick, or a change of where the game is, starts a new block.
        if (first || now != last) {
            blocked = first ? 0 : held & before;
            last = now;
            first = false;
        }

        before = held;
        blocked &= held;  // let go: free again
        return held & ~blocked;
    }
};

// Presses the game sees one at a time: each pulse is held one tick and let go the next, so the
// game, which acts on a button's press, sees each. For the wheel in menus, the mouse's steps and
// its hover. At most four wait.
struct Pulser {
    std::array<std::uint16_t, 4> queue{};
    int count = 0;
    bool resting = false;

    void push(std::uint16_t buttons) {
        // A pulse past the four that wait is dropped.
        if (count < static_cast<int>(queue.size())) {
            queue[static_cast<std::size_t>(count++)] = buttons;
        }
    }

    // Whether nothing is waiting or resting, so that a new pulse goes out at once.
    bool idle() const { return count == 0 && !resting; }

    void clear() {
        count = 0;
        resting = false;
    }

    // This tick's buttons.
    std::uint16_t step() {
        // A pulse just sent rests for one tick, so that the game sees the button let go.
        if (resting || count == 0) {
            resting = false;
            return 0;
        }

        const std::uint16_t b = queue[0];

        // Move the rest of the queue up one place.
        for (int i = 1; i < count; ++i) {
            queue[static_cast<std::size_t>(i - 1)] = queue[static_cast<std::size_t>(i)];
        }

        --count;
        resting = true;
        return b;
    }
};

// A held direction in a menu repeats, as a keyboard does: the game moves once a press, so after
// `delay` ticks held the button is let go for one tick every `every` ticks.
struct Repeat {
    int ticks = 0;

    bool step(bool held, int delay = 24, int every = 6) {
        // A key let go starts the count again.
        if (!held) {
            ticks = 0;
            return false;
        }

        ++ticks;
        return !(ticks > delay && (ticks - delay) % every == 0);
    }
};

// The mouse moved over a menu with nothing under it to point at: each `distance` pixels along the
// way it mostly goes is a step of the d-pad that way.
struct Stepper {
    float x = 0.0f;
    float y = 0.0f;

    std::uint16_t step(float dx, float dy, float distance = 48.0f) {
        x += dx;
        y += dy;

        // The mouse went mostly sideways.
        if (std::fabs(x) >= std::fabs(y)) {
            y *= 0.5f;  // across the way it goes: forgotten slowly

            // Far enough right: one step of the d-pad.
            if (x >= distance) {
                x -= distance;
                return kRight;
            }

            // Far enough left: one step of the d-pad.
            if (x <= -distance) {
                x += distance;
                return kLeft;
            }
        } else {
            x *= 0.5f;

            // Far enough down: one step of the d-pad.
            if (y >= distance) {
                y -= distance;
                return kDown;
            }

            // Far enough up: one step of the d-pad.
            if (y <= -distance) {
                y += distance;
                return kUp;
            }
        }

        return 0;
    }

    void reset() {
        x = 0.0f;
        y = 0.0f;
    }
};

// ---- The menus under the mouse ----

// The game's 2D screen, which the pointer (openrac_input.pointer_*, 0..1) maps onto.
inline constexpr float kScreenWidth = 512.0f;   // (seen in game code)
inline constexpr float kScreenHeight = 416.0f;  // (seen in game code)

// A page widget's rectangle as the game writes it each frame (widget +0x18: x, y, width, height in
// the game's screen pixels).
struct Rect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t w = 0;
    std::int32_t h = 0;

    // Whether the point is within the rectangle, with a margin round it (assumed: the pointer's
    // hot spot is not exact).
    bool contains(float px, float py, float margin = 2.0f) const {
        return w > 0 && h > 0 && px >= x - margin && px < x + w + margin && py >= y - margin
               && py < y + h + margin;
    }
};

// The widget under the point, the first of them (-1 for none).
inline int hit_widget(const Rect* rects, int count, float px, float py) {
    // Test the widgets in order.
    for (int i = 0; i < count; ++i) {
        // The first widget whose rectangle holds the point is the one hit.
        if (rects[i].contains(px, py)) {
            return i;
        }
    }

    return -1;
}

// The most widgets a page has (the game's page holds 14 pointers).
inline constexpr int kMaxPageWidgets = 14;

// The most links a widget has to the widgets the d-pad moves the focus to (an item grid's up,
// down, left and right).
inline constexpr int kMaxWidgetLinks = 4;

// The page's widgets (`widgets`, `count` of them) the d-pad can take the focus to from widget
// `from`, one link after another, as bits by index (bit i: widgets[i]); `from` itself is not one
// unless a link comes back to it. `links(w, out)` puts widget w's links into out (at most
// kMaxWidgetLinks) and returns how many: none for a widget the game moves no focus from. Only the
// links to the page's own widgets count.
//
// The game moves the focus only along these links (games/rac1 pause.c: update_menu_grid_selection
// and update_menu_entry_actions set the page's pending focus to a link, nothing else does), and
// some widgets read the focused one as the kind it can be: the Quick Select's item preview takes
// it for an item grid and reads an item from it (update_item_preview_binding). Focusing any other
// widget hands that preview a garbage item.
template <class Links>
std::uint32_t reachable_widgets(
    std::uint32_t from, const std::uint32_t* widgets, int count, Links links
) {
    count = std::min(count, kMaxPageWidgets);
    std::uint32_t found = 0;
    std::uint32_t queue[kMaxPageWidgets + 1];
    int head = 0;
    int tail = 0;
    queue[tail++] = from;

    // Follow the links from each widget reached, once each.
    while (head < tail) {
        std::uint32_t out[kMaxWidgetLinks] = {};
        const int n = std::min(links(queue[head++], out), kMaxWidgetLinks);

        // Each link that leads to one of the page's widgets not reached yet.
        for (int l = 0; l < n; ++l) {
            for (int i = 0; i < count; ++i) {
                // Not this widget, or reached already.
                if (widgets[i] != out[l] || (found & (1u << i)) != 0) {
                    continue;
                }

                found |= 1u << i;
                queue[tail++] = widgets[i];
            }
        }
    }

    return found;
}

// The row of a text list under a point (DrawMenuItemList, games/rac1 pause.c): `count` rows. Spread
// over the rectangle (flags without 0x10), row i's text sits at about rect.y + h / (count + 1) *
// (i + 1); packed (flag 0x10), the rows are the font's height and 3 apart from the top (the font
// 12 high, 14 with flag 4, 10 with flag 8). -1 off the list.
inline int list_row(const Rect& r, int count, std::uint32_t flags, float py) {
    // A point off the list, or a list with no rows, is on no row.
    if (count <= 0 || r.h <= 0 || py < r.y || py >= r.y + r.h) {
        return -1;
    }

    float row;

    // Packed rows are as high as the font and 3 apart from the top; flags 4 and 8 pick the font.
    if ((flags & 0x10u) != 0) {
        const float font = (flags & 4u) != 0 ? 14.0f : (flags & 8u) != 0 ? 10.0f : 12.0f;
        row = std::floor((py - static_cast<float>(r.y)) / (font + 3.0f));
    } else {
        const float spacing = static_cast<float>(r.h) / static_cast<float>(count + 1);
        row = std::floor((py - static_cast<float>(r.y)) / spacing - 0.5f);
    }

    return static_cast<int>(std::max(0.0f, std::min(row, static_cast<float>(count - 1))));
}

// An item grid as the game lays it out (draw_menu_item_grid, games/rac1 pause.c): the widget's
// rectangle; the frame it is laid out in (its menu moby's, in the game's own units) and an icon's
// size in those units; the grid's margins in them, its rows, columns and flags (2: a fixed row
// step).
struct GridLayout {
    Rect rect;
    float frame_w = 0.0f;
    float frame_h = 0.0f;
    float icon_w = 0.0f;
    float icon_h = 0.0f;
    float margin_x = 0.0f;
    float margin_y = 0.0f;
    int rows = 0;
    int cols = 0;
    std::uint32_t flags = 0;
};

// The most rows or columns a grid read from the game may have before it is taken for no grid.
inline constexpr int kMaxGridSide = 16;

// The extra a fixed row step adds to an icon's height (draw_menu_item_grid's 0.15).
inline constexpr float kGridFixedRowGap = 0.15f;

// Whether a layout is one the game could draw: sides in range, sizes positive and finite.
inline bool grid_layout_valid(const GridLayout& g) {
    const auto positive = [](float v) {
        return std::isfinite(v) && v > 0.0f;
    };
    return g.rows > 0 && g.cols > 0 && g.rows <= kMaxGridSide && g.cols <= kMaxGridSide
           && g.rect.w > 0 && g.rect.h > 0 && positive(g.frame_w) && positive(g.frame_h)
           && positive(g.icon_w) && positive(g.icon_h) && std::isfinite(g.margin_x)
           && std::isfinite(g.margin_y);
}

// The cell under a point in the game's screen, as an index (row * cols + column); -1 for none.
// The cells are where the game draws their icons: the columns spread across the frame between the
// margins (one column centred), the rows the same (or a fixed step with flag 2), all scaled so the
// frame's longer side fills the rectangle's longer side, from the rectangle's corner. A point in
// the gap between two icons counts for the nearer one; outside the outer icons, by half a gap
// (at least two pixels).
inline int grid_cell_at(const GridLayout& g, float px, float py) {
    // A grid the game could not draw has no cells.
    if (!grid_layout_valid(g)) {
        return -1;
    }

    float start_x;
    float column_step;
    float start_y;
    float row_step;

    // The columns: spread between the margins, or one in the middle.
    if (g.cols >= 2) {
        start_x = g.margin_x;
        column_step = g.icon_w
                      + (g.frame_w - (start_x + start_x) - g.icon_w * static_cast<float>(g.cols))
                            / static_cast<float>(g.cols - 1);
    } else {
        column_step = 0.0f;
        start_x = (g.frame_w - g.icon_w) * 0.5f;
    }

    // The rows: a fixed step, spread between the margins, or one in the middle.
    if ((g.flags & 2u) != 0) {
        row_step = g.icon_h + kGridFixedRowGap;
        start_y = g.margin_y;
    } else if (g.rows >= 2) {
        start_y = g.margin_y;
        row_step = g.icon_h
                   + (g.frame_h - (start_y + start_y) - g.icon_h * static_cast<float>(g.rows))
                         / static_cast<float>(g.rows - 1);
    } else {
        row_step = 0.0f;
        start_y = (g.frame_h - g.icon_h) * 0.5f;
    }

    // The game's scale is in sixteenths of a pixel per unit; this is pixels per unit.
    const float scale =
        static_cast<float>(std::max(g.rect.w, g.rect.h)) / std::max(g.frame_w, g.frame_h);
    const float icon_w = g.icon_w * scale;
    const float icon_h = g.icon_h * scale;
    const float gap_x = std::max(2.0f, (column_step * scale - icon_w) * 0.5f);
    const float gap_y = std::max(2.0f, (row_step * scale - icon_h) * 0.5f);
    const float lx = px - static_cast<float>(g.rect.x);
    const float ly = py - static_cast<float>(g.rect.y);

    // The column whose icon (with half the gap each side) holds the point.
    int column = -1;

    for (int j = 0; j < g.cols && column < 0; ++j) {
        const float left = (start_x + column_step * static_cast<float>(j)) * scale;

        if (lx >= left - gap_x && lx < left + icon_w + gap_x) {
            column = j;
        }
    }

    // The row, the same way.
    int row = -1;

    for (int i = 0; i < g.rows && row < 0; ++i) {
        const float top = (start_y + row_step * static_cast<float>(i)) * scale;

        if (ly >= top - gap_y && ly < top + icon_h + gap_y) {
            row = i;
        }
    }

    return column < 0 || row < 0 ? -1 : row * g.cols + column;
}

// The d-pad press that takes a grid's selection one step from cell `from` toward cell `to`
// (update_menu_grid_selection: up and down a row, left and right a column): the row first. 0 when
// they are the same cell, or either is off the grid.
inline std::uint16_t grid_step(int from, int to, int rows, int cols) {
    // Cells off the grid, or the same one: nothing to press.
    if (cols <= 0 || from < 0 || to < 0 || from >= rows * cols || to >= rows * cols || from == to) {
        return 0;
    }

    // Another row: up or down. The same row: left or right.
    if (to / cols != from / cols) {
        return to / cols < from / cols ? kUp : kDown;
    }

    return to % cols < from % cols ? kLeft : kRight;
}

}  // namespace kbm

#endif  // KBM_HPP

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// The behaviour presets' sums, apart from the port so they can be tested: where a moby is and which
// way it faces at a time, from where it started (and where the hero is). The game's axes: z up,
// rotation[2] the yaw in radians, a yaw of 0 facing +x.
#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

#include "json.hpp"

namespace behaviour {

using cutscene::json::Value;

constexpr double kPi = 3.14159265358979323846;
constexpr double kTicksPerSecond = 60.0;  // the port runs every disc at 60 ticks a second

struct V3 {
    float x = 0, y = 0, z = 0;
};

enum class Preset {
    Spin,
    Bob,
    Patrol,
    Orbit,
    FaceHero,
    Follow,
    Wander
};

struct Behaviour {
    int spawn_id = -1;
    // A moby the mod makes (another level's class brought for its look): its class, place, turn
    // (radians) and scale; -1 for a placed one, named by spawn_id.
    int oclass = -1;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale = 0;
    Preset preset = Preset::Spin;
    double speed =
        0;  // degrees a second (spin, orbit, face_hero) or units a second (patrol, follow)
    double height = 0;       // bob: how far up and down
    double period = 0;       // bob: seconds for one up and down
    double distance = 0;     // patrol: how far it goes; follow: how near the hero it stops
    double radius = 0;       // orbit
    double range = 0;        // follow: how near the hero must be for it to follow (0: always)
    double pause = 0;        // wander: seconds it stands at each place
    int sequence = -1;       // the class's sequence it plays standing, -1 its own
    int walk_sequence = -1;  // the one it plays while it moves, -1 the standing one
};

inline double radians(double degrees) {
    return degrees * kPi / 180.0;
}

// An angle in -pi..pi.
inline double wrap(double a) {
    a = std::fmod(a + kPi, 2.0 * kPi);

    if (a < 0) {
        a += 2.0 * kPi;
    }

    return a - kPi;
}

// `from` turned towards `to` by at most `step` radians, the short way.
inline double turn(double from, double to, double step) {
    const double d = wrap(to - from);

    if (std::fabs(d) <= step) {
        return wrap(to);
    }

    return wrap(from + (d > 0 ? step : -step));
}

inline std::optional<Preset> preset_of(const std::string& name) {
    if (name == "spin") {
        return Preset::Spin;
    }

    if (name == "bob") {
        return Preset::Bob;
    }

    if (name == "patrol") {
        return Preset::Patrol;
    }

    if (name == "orbit") {
        return Preset::Orbit;
    }

    if (name == "face_hero") {
        return Preset::FaceHero;
    }

    if (name == "follow") {
        return Preset::Follow;
    }

    if (name == "wander") {
        return Preset::Wander;
    }

    return std::nullopt;
}

// A number of `v`, `fallback` when it is missing or not finite, held to lo..hi.
inline double field(const Value& v, double fallback, double lo, double hi) {
    double n = v.number(fallback);

    if (!std::isfinite(n)) {
        n = fallback;
    }

    return n < lo ? lo : n > hi ? hi : n;
}

// One entry of behaviours.json' "mobys", or nothing (no spawn id, a preset this mod does not know).
// What it leaves out takes the preset's own default.
inline std::optional<Behaviour> behaviour_of(const Value& v) {
    const auto preset = preset_of(v["preset"].string());
    const int id = v["spawn_id"].integer(-1);
    const int oclass = v["class"].integer(-1);
    const bool made = oclass >= 0 && oclass < 0x10000 && v["position"].is_array();

    if (!preset || (!made && (id < 0 || id >= 0xFFFF))) {
        return std::nullopt;
    }

    Behaviour b;
    b.spawn_id = made ? -1 : id;
    b.preset = *preset;

    if (made) {
        b.oclass = oclass;

        for (int i = 0; i < 3; ++i) {
            b.position[i] = static_cast<float>(field(v["position"][i], 0, -1e6, 1e6));
            b.rotation[i] = static_cast<float>(field(v["rotation"][i], 0, -10, 10));
        }

        b.scale = static_cast<float>(field(v["scale"], 0, 0, 1000));
    }

    const bool turns =
        *preset == Preset::Spin || *preset == Preset::Orbit || *preset == Preset::FaceHero;
    const double speed = *preset == Preset::Spin       ? 90
                         : *preset == Preset::Orbit    ? 45
                         : *preset == Preset::FaceHero ? 180
                         : *preset == Preset::Wander   ? 3
                                                       : 4;
    b.speed = field(v["speed"], speed, turns ? -3600 : 0, turns ? 3600 : 200);
    b.height = field(v["height"], 1, -100, 100);
    b.period = field(v["period"], 2, 0.05, 600);
    b.distance = field(v["distance"], *preset == Preset::Follow ? 3 : 8, 0, 1000);
    b.radius = field(v["radius"], *preset == Preset::Wander ? 8 : 4, 0, 1000);
    b.range = field(v["range"], 30, 0, 10000);
    b.pause = field(v["pause"], 2, 0, 600);
    b.sequence = v["sequence"].integer(-1);

    if (b.sequence < -1 || b.sequence > 255) {
        b.sequence = -1;
    }

    b.walk_sequence = v["walk_sequence"].integer(-1);

    if (b.walk_sequence < -1 || b.walk_sequence > 255) {
        b.walk_sequence = -1;
    }

    return b;
}

// Where a moby is: its start, and where it is now (face_hero and follow go on from there).
struct State {
    V3 start_position, start_rotation;
    V3 position, rotation;
    V3 target;              // wander: where it goes next
    double wait = 0;        // wander: seconds left standing
    std::uint32_t rng = 1;  // wander: its own dice, so a moby wanders the same way each load
};

inline State start(const V3& position, const V3& rotation, std::uint32_t seed = 1) {
    State s{position, rotation, position, rotation, position};
    s.rng = seed * 2654435761u + 1u;
    return s;
}

// A number in 0..1 from the state's dice (xorshift).
inline double dice(State& s) {
    s.rng ^= s.rng << 13;
    s.rng ^= s.rng >> 17;
    s.rng ^= s.rng << 5;
    return (s.rng & 0xFFFFFF) / double(0x1000000);
}

// The sequence a moby with `b` plays: its walking one while it moved this tick, else its standing one.
inline int sequence_for(const Behaviour& b, bool moved) {
    return moved && b.walk_sequence >= 0 ? b.walk_sequence : b.sequence;
}

// Moves `s` to time `t` (seconds since the behaviour began; `dt` since the last step). `hero`: where
// the hero is, or null for none.
inline void step(const Behaviour& b, State& s, double t, double dt, const V3* hero) {
    const V3& p0 = s.start_position;
    const double yaw0 = s.start_rotation.z;

    switch (b.preset) {
        case Preset::Spin:
            s.rotation.z = static_cast<float>(wrap(yaw0 + radians(b.speed) * t));
            break;
        case Preset::Bob:
            s.position.z = static_cast<float>(p0.z + b.height * std::sin(2.0 * kPi * t / b.period));
            break;
        case Preset::Patrol: {
            // Out along the way it faced, then back, facing the way it goes.
            if (b.speed <= 0 || b.distance <= 0) {
                break;
            }

            const double leg = b.distance / b.speed;
            const double phase = std::fmod(t, 2.0 * leg);
            const bool out = phase < leg;
            const double along = (out ? phase : 2.0 * leg - phase) * b.speed;
            s.position.x = static_cast<float>(p0.x + std::cos(yaw0) * along);
            s.position.y = static_cast<float>(p0.y + std::sin(yaw0) * along);
            s.rotation.z = static_cast<float>(out ? wrap(yaw0) : wrap(yaw0 + kPi));
            break;
        }
        case Preset::Orbit: {
            // Round a centre beside it, starting where it stands and facing the way it goes (to its
            // left for a positive speed, to its right for a negative one).
            const double side = b.speed >= 0 ? kPi / 2 : -kPi / 2;
            const double from = yaw0 - side;  // the angle of it seen from the centre
            const double cx = p0.x - b.radius * std::cos(from);
            const double cy = p0.y - b.radius * std::sin(from);
            const double a = from + radians(b.speed) * t;
            s.position.x = static_cast<float>(cx + b.radius * std::cos(a));
            s.position.y = static_cast<float>(cy + b.radius * std::sin(a));
            s.rotation.z = static_cast<float>(wrap(a + side));
            break;
        }
        case Preset::FaceHero:
            if (hero != nullptr) {
                const double to = std::atan2(hero->y - s.position.y, hero->x - s.position.x);
                s.rotation.z =
                    static_cast<float>(turn(s.rotation.z, to, radians(std::fabs(b.speed)) * dt));
            }

            break;
        case Preset::Follow: {
            if (hero == nullptr) {
                break;
            }

            const double dx = hero->x - s.position.x, dy = hero->y - s.position.y;
            const double far = std::sqrt(dx * dx + dy * dy);

            if (far < 1e-4 || (b.range > 0 && far > b.range)) {
                break;
            }

            s.rotation.z =
                static_cast<float>(turn(s.rotation.z, std::atan2(dy, dx), radians(360.0) * dt));
            const double go = std::fmin(b.speed * dt, far - b.distance);

            if (go > 0) {
                s.position.x = static_cast<float>(s.position.x + dx / far * go);
                s.position.y = static_cast<float>(s.position.y + dy / far * go);
            }

            break;
        }
        case Preset::Wander: {
            // Walks to a place within `radius` of where it started, stands there `pause` seconds,
            // then picks the next.
            if (s.wait > 0) {
                s.wait -= dt;
                break;
            }

            const double dx = s.target.x - s.position.x, dy = s.target.y - s.position.y;
            const double far = std::sqrt(dx * dx + dy * dy);

            if (far < 0.05 || b.speed <= 0) {
                const double a = 2.0 * kPi * dice(s), r = b.radius * std::sqrt(dice(s));
                s.target =
                    {static_cast<float>(p0.x + r * std::cos(a)),
                     static_cast<float>(p0.y + r * std::sin(a)),
                     p0.z};

                if (far < 0.05 && t > 0) {
                    s.wait = b.pause;
                }

                break;
            }

            s.rotation.z =
                static_cast<float>(turn(s.rotation.z, std::atan2(dy, dx), radians(360.0) * dt));
            const double go = std::fmin(b.speed * dt, far);
            s.position.x = static_cast<float>(s.position.x + dx / far * go);
            s.position.y = static_cast<float>(s.position.y + dy / far * go);
            break;
        }
    }
}

}  // namespace behaviour

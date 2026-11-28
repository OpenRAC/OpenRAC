// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// OpenRAC cutscene player: the sums of a cutscene's timeline, apart from the port so they can be
// tested alone (tests/timeline_test.cpp). Everything here reads a parsed .cutscene
// (OpenRAC Editor's docs/CUTSCENE_FORMAT.md) and a tick, and gives a place, a turn, a picture or a line of text.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "json.hpp"

namespace cutscene {

using json::Value;

constexpr double kTickRate = 60.0;  // cutscene ticks a second, every game and disc
constexpr float kPi = 3.14159265f;

// ---------------------------------------------------------------------------------------------
// Small vector maths

struct V3 {
    float x = 0, y = 0, z = 0;
};

inline V3 operator+(V3 a, V3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline V3 operator-(V3 a, V3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline V3 operator*(V3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}

inline float dot(V3 a, V3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline V3 cross(V3 a, V3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(V3 a) {
    return std::sqrt(dot(a, a));
}

inline V3 normal(V3 a, V3 fallback) {
    const float l = length(a);
    return l > 1e-6f && std::isfinite(l) ? a * (1.0f / l) : fallback;
}

inline V3 lerp(V3 a, V3 b, float u) {
    return a + (b - a) * u;
}

// Catmull-Rom through p1 (u 0) and p2 (u 1).
inline V3 catmull(V3 p0, V3 p1, V3 p2, V3 p3, float u) {
    const float u2 = u * u, u3 = u2 * u;
    return (p1 * 2.0f + (p2 - p0) * u + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * u2
            + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * u3)
           * 0.5f;
}

inline V3 bezier(V3 a, V3 b, V3 c, V3 d, float u) {
    const float v = 1.0f - u;
    return a * (v * v * v) + b * (3 * v * v * u) + c * (3 * v * u * u) + d * (u * u * u);
}

// From angle a to b the short way round.
inline float angle_lerp(float a, float b, float u) {
    float d = std::fmod(b - a, 2 * kPi);

    if (d > kPi) {
        d -= 2 * kPi;
    }

    if (d < -kPi) {
        d += 2 * kPi;
    }

    return a + d * u;
}

// [x, y, z] as a vector; anything else (or a part not a number) is the fallback.
inline V3 vec(const Value& v, V3 fallback = {}) {
    if (!v.is_array() || v.size() < 3 || !v[0].is_number() || !v[1].is_number()
        || !v[2].is_number()) {
        return fallback;
    }

    return {
        static_cast<float>(v[0].number()),
        static_cast<float>(v[1].number()),
        static_cast<float>(v[2].number())
    };
}

// A direction from a rotation [x, y, z]: z the yaw (0 along +x), y the pitch (above 0 looks down).
inline V3 direction(V3 rotation) {
    const float cp = std::cos(rotation.y);
    return {std::cos(rotation.z) * cp, std::sin(rotation.z) * cp, -std::sin(rotation.y)};
}

// `v` turned by `yaw` radians about z.
inline V3 turn_z(V3 v, float yaw) {
    const float c = std::cos(yaw), s = std::sin(yaw);
    return {v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

// Euler [x, y, z] (z the yaw, applied last) as a quaternion (x, y, z, w).
inline void quaternion(V3 r, float out[4]) {
    const float cx = std::cos(r.x / 2), sx = std::sin(r.x / 2);
    const float cy = std::cos(r.y / 2), sy = std::sin(r.y / 2);
    const float cz = std::cos(r.z / 2), sz = std::sin(r.z / 2);
    out[0] = sx * cy * cz - cx * sy * sz;
    out[1] = cx * sy * cz + sx * cy * sz;
    out[2] = cx * cy * sz - sx * sy * cz;
    out[3] = cx * cy * cz + sx * sy * sz;
}

// The order of `keys` by their "t", ties kept in the file's order: a file written by hand need
// not list its keys in time.
inline std::vector<std::size_t> by_time(const std::vector<Value>& keys) {
    std::vector<std::size_t> order(keys.size());

    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }

    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return keys[a]["t"].tick() < keys[b]["t"].tick();
    });
    return order;
}

// ---------------------------------------------------------------------------------------------
// Actors

// The track's place, rotation and scale at tick t. Each key gives what it has, the rest carries
// from the key before; what no key gives keeps the value passed in. Before the first key it is
// the first, after the last the last; a "cut" key jumps.
inline void track_at(const Value& track, int t, V3& pos, V3& rot, float& scale) {
    const auto& all = track.items();

    if (all.empty()) {
        return;
    }

    const std::vector<std::size_t> order = by_time(all);
    const std::size_t n = order.size();
    std::vector<V3> P(n), R(n);
    std::vector<float> S(n);
    std::vector<int> T(n);

    for (std::size_t i = 0; i < n; ++i) {
        const Value& k = all[order[i]];
        P[i] = vec(k["position"], i ? P[i - 1] : pos);
        R[i] = vec(k["rotation"], i ? R[i - 1] : rot);
        S[i] = static_cast<float>(k["scale"].number(i ? S[i - 1] : scale));
        T[i] = k["t"].tick();
    }

    if (t <= T[0]) {
        pos = P[0], rot = R[0], scale = S[0];
        return;
    }

    std::size_t i = 0;

    while (i + 1 < n && T[i + 1] <= t) {
        ++i;
    }

    if (i + 1 >= n || all[order[i + 1]]["cut"].boolean()) {
        pos = P[i], rot = R[i], scale = S[i];
        return;
    }

    const float u =
        T[i + 1] > T[i] ? static_cast<float>(t - T[i]) / static_cast<float>(T[i + 1] - T[i]) : 1.0f;

    if (all[order[i]]["ease"].string_or("linear") == "smooth") {
        pos = catmull(P[i ? i - 1 : i], P[i], P[i + 1], P[std::min(i + 2, n - 1)], u);
    } else {
        pos = lerp(P[i], P[i + 1], u);
    }

    rot =
        {angle_lerp(R[i].x, R[i + 1].x, u),
         angle_lerp(R[i].y, R[i + 1].y, u),
         angle_lerp(R[i].z, R[i + 1].z, u)};
    scale = S[i] + (S[i + 1] - S[i]) * u;
}

// Walks `path` from its start tick at its speed (units a second, 8 if not given): the place along
// its points and the yaw of the stretch it is on. Once at the end it stays there. False before it
// starts, or with fewer than two points.
inline bool path_at(const Value& path, int t, V3& pos, V3& rot) {
    const auto& pts = path["points"].items();
    const int start = path["t"].tick();

    if (pts.size() < 2 || t < start) {
        return false;
    }

    float left =
        static_cast<float>((t - start) / kTickRate * std::max(0.0, path["speed"].number(8)));

    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        const V3 a = vec(pts[i]), b = vec(pts[i + 1]);
        const float len = length(b - a);

        // A stretch of no length has no direction: the yaw stays the last one's.
        if (len > 1e-6f) {
            rot.z = std::atan2(b.y - a.y, b.x - a.x);
        }

        if (left <= len || i + 2 == pts.size()) {
            pos = len > 1e-6f ? lerp(a, b, std::min(1.0f, left / len)) : b;
            return true;
        }

        left -= len;
    }

    return true;
}

// The last "visible" key at or before t (keys in time order); true before any.
inline bool visible_at(const Value& keys, int t) {
    bool visible = true;
    int at = -1;

    for (const Value& k : keys.items()) {
        const int kt = k["t"].tick();

        if (kt <= t && kt >= at) {
            visible = k["on"].boolean(true);
            at = kt;
        }
    }

    return visible;
}

// The anims entry that plays at t: the latest one started at or before it, -1 before any.
inline int anim_at(const Value& anims, int t) {
    int found = -1, at = -1;
    const auto& list = anims.items();

    for (std::size_t i = 0; i < list.size(); ++i) {
        const int kt = list[i]["t"].tick();

        if (kt <= t && kt >= at) {
            found = static_cast<int>(i);
            at = kt;
        }
    }

    return found;
}

// The "ride" key in force at t: the latest at or before it, -1 before any or for one with no actor
// (the ride is over).
inline int ride_at(const Value& rides, int t) {
    const int i = anim_at(rides, t);
    return i >= 0 && rides[static_cast<std::size_t>(i)]["actor"].string().empty() ? -1 : i;
}

// ---------------------------------------------------------------------------------------------
// Camera

struct CamKey {
    int t = 0;
    V3 eye;
    bool has_target = false;
    V3 target;  // look_at
    V3 forward;
    float roll = 0;
    bool cut = false;
    std::string ease;
    V3 in, out;  // bezier handles, as offsets from eye
};

// A key from the file: `target` is its look_at point, when it has one (the player finds actors);
// `eye` is where an eye on an actor is now; an eye that is neither carries from the key before.
inline CamKey camera_key(
    const Value& k, const CamKey* before, const V3* target, const V3* eye = nullptr
) {
    CamKey c;
    c.t = k["t"].tick();
    c.eye = eye != nullptr ? *eye : vec(k["eye"], before ? before->eye : V3{});
    c.has_target = target != nullptr;

    if (target) {
        c.target = *target;
    }

    const bool has_rot = k.has("rotation");
    const V3 rot = vec(k["rotation"]);
    // A key with no rotation, or looking at its own eye, keeps the key before's direction.
    const V3 fallback = before ? before->forward : V3{1, 0, 0};

    if (c.has_target) {
        c.forward = normal(c.target - c.eye, has_rot ? direction(rot) : fallback);
    } else {
        c.forward = has_rot ? direction(rot) : fallback;
    }

    c.roll = static_cast<float>(k["roll"].number(has_rot ? rot.x : (before ? before->roll : 0.0f)));
    c.cut = k["cut"].boolean();
    c.ease = k["ease"].string_or("smooth");
    c.in = vec(k["in"]);
    c.out = vec(k["out"]);
    return c;
}

struct CameraPose {
    V3 eye;
    V3 forward{1, 0, 0};
    float roll = 0;
};

// The camera at t from its keys (in time order): held before the first and after the last key, and
// up to a cut; between two keys by the first's ease ("smooth", "linear" or "bezier").
inline bool camera_pose(const std::vector<CamKey>& keys, int t, CameraPose& out) {
    if (keys.empty()) {
        return false;
    }

    std::size_t i = 0;

    while (i + 1 < keys.size() && keys[i + 1].t <= t) {
        ++i;
    }

    if (t <= keys[0].t || i + 1 >= keys.size() || keys[i + 1].cut) {
        const CamKey& k = t <= keys[0].t ? keys[0] : keys[i];
        out = {k.eye, k.forward, k.roll};
        return true;
    }

    const CamKey &a = keys[i], &b = keys[i + 1];
    const float u = b.t > a.t ? static_cast<float>(t - a.t) / static_cast<float>(b.t - a.t) : 1.0f;
    // Neighbours for the smooth curve, not across a cut.
    const CamKey& before = (i > 0 && !a.cut) ? keys[i - 1] : a;
    const CamKey& after = (i + 2 < keys.size() && !keys[i + 2].cut) ? keys[i + 2] : b;

    if (a.ease == "linear") {
        out.eye = lerp(a.eye, b.eye, u);
    } else if (a.ease == "bezier") {
        out.eye = bezier(a.eye, a.eye + a.out, b.eye + b.in, b.eye, u);
    } else {
        out.eye = catmull(before.eye, a.eye, b.eye, after.eye, u);
    }

    if (a.has_target && b.has_target) {
        const V3 target = a.ease == "linear" ? lerp(a.target, b.target, u)
                                             : catmull(
                                                   before.has_target ? before.target : a.target,
                                                   a.target,
                                                   b.target,
                                                   after.has_target ? after.target : b.target,
                                                   u
                                               );
        out.forward = normal(target - out.eye, a.forward);
    } else {
        out.forward = normal(lerp(a.forward, b.forward, u), a.forward);
    }

    out.roll = a.roll + (b.roll - a.roll) * u;
    return true;
}

// The camera's shake at t: a wobble of the eye, fading over each shake's length.
inline V3 shake_at(const Value& shakes, int t) {
    V3 sum;

    for (const Value& s : shakes.items()) {
        const int st = s["t"].tick(), len = std::max(1, s["length"].tick(30));

        if (t < st || t >= st + len) {
            continue;
        }

        const float k =
            static_cast<float>(s["strength"].number(0.3)) * (1.0f - float(t - st) / float(len));
        const float ft = static_cast<float>(t);
        sum = sum
              + V3{std::sin(ft * 1.7f), std::sin(ft * 2.3f + 1.0f), std::sin(ft * 2.9f + 2.0f)} * k;
    }

    return sum;
}

// A camera's axes as the game has them (z up): left = up x forward, so yaw 0 looks along +x with
// +y on the left. The roll turns left towards up.
struct CameraAxes {
    V3 forward, left, up;
};

inline CameraAxes camera_axes(V3 forward, float roll) {
    CameraAxes a;
    a.forward = normal(forward, {1, 0, 0});
    a.left = normal(cross({0, 0, 1}, a.forward), {0, 1, 0});
    a.up = normal(cross(a.forward, a.left), {0, 0, 1});

    if (roll != 0) {
        const float c = std::cos(roll), s = std::sin(roll);
        const V3 l2 = a.left * c + a.up * s, u2 = a.up * c - a.left * s;
        a.left = l2, a.up = u2;
    }

    return a;
}

// ---------------------------------------------------------------------------------------------
// Text

// The printable ASCII of a UTF-8 text (the HUD font has nothing else): a Latin letter with an
// accent as the letter without it (a line the editor did not draw as a picture), the rest left out.
inline std::string ascii(const std::string& utf8) {
    // U+00C0 to U+017F, each as its letter.
    static constexpr char kLatin[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/"
                                     "ouuuuytyAaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGg"
                                     "GgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsS"
                                     "sSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";
    std::string out;

    for (std::size_t i = 0; i < utf8.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);

        if (c >= 32 && c < 127) {
            out += static_cast<char>(c);
        } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size() && (utf8[i + 1] & 0xC0) == 0x80) {
            const unsigned cp =
                (c & 0x1Fu) << 6 | (static_cast<unsigned char>(utf8[i + 1]) & 0x3Fu);

            if (cp >= 0xC0 && cp < 0x180) {
                out += kLatin[cp - 0xC0];
            }

            ++i;
        }
    }

    return out;
}

// A subtitle as rows of up to `width` letters, broken at spaces (a longer word broken where it
// must), at most `rows`; the last row ends in "..." when words were left out.
inline std::vector<std::string> wrap(
    const std::string& text, std::size_t width = 40, std::size_t rows = 3
) {
    std::vector<std::string> out;

    if (width == 0 || rows == 0) {
        return out;
    }

    std::vector<std::string> words;
    std::string w;

    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!w.empty()) {
                words.push_back(w), w.clear();
            }
        } else {
            w += c;
        }
    }

    if (!w.empty()) {
        words.push_back(w);
    }

    bool cut = false;

    for (std::string word : words) {
        while (!word.empty()) {
            if (!out.empty() && out.back().size() + 1 + word.size() <= width) {
                out.back() += " " + word;
                word.clear();
                break;
            }

            if (out.size() == rows) {
                cut = true;
                break;
            }

            out.push_back(word.substr(0, width));
            word.erase(0, std::min(width, word.size()));
        }

        if (cut) {
            break;
        }
    }

    if (cut && !out.empty()) {
        std::string& last = out.back();

        if (last.size() + 3 > width) {
            last.resize(width >= 3 ? width - 3 : 0);
        }

        last += "...";
    }

    return out;
}

// Which language of a by-language text to show: the game's, else the cutscene's first, else
// English, else the first the text has. Empty when it has none.
inline std::string pick_language(
    const Value& by_language, const Value& languages, const std::string& game
) {
    if (!by_language.is_object()) {
        return std::string();
    }

    if (by_language.has(game)) {
        return game;
    }

    if (languages.is_array() && languages.size() > 0 && by_language.has(languages[0].string())) {
        return languages[0].string();
    }

    if (by_language.has("en")) {
        return "en";
    }

    return by_language.members().empty() ? std::string() : by_language.members().begin()->first;
}

// ---------------------------------------------------------------------------------------------
// Sounds

// A playing sound's volume at t (0 to 1 of its own): up over fade_in from its start, down over
// fade_out to its end (-1: none).
inline float fade_gain(int start, int fade_in, int fade_out, int end, int t) {
    float k = 1;

    if (fade_in > 0 && t - start < fade_in) {
        k = std::max(0.0f, float(t - start) / float(fade_in));
    }

    if (end >= 0 && fade_out > 0 && t > end - fade_out) {
        k = std::min(k, std::max(0.0f, float(end - t) / float(fade_out)));
    }

    return k;
}

// ---------------------------------------------------------------------------------------------
// Over the picture: the letterbox and fades

constexpr int kLetterboxTicks = 15;  // ticks the bars take to come in

// How far the letterbox's bars are in at tick t of the cutscene (0 to 1, eased).
inline float letterbox_at(int t) {
    const float u = std::clamp(static_cast<float>(t) / kLetterboxTicks, 0.0f, 1.0f);
    return u * u * (3 - 2 * u);
}

struct Rgba {
    float r = 0, g = 0, b = 0, a = 0;
};

// The colour over the picture at t from the "fade" events: each goes from the colour before it to
// its "to" ([r, g, b] or [r, g, b, a], 0 to 1) over its length (30 ticks if not given); "to"
// null clears the colour. Clear before any.
inline Rgba fade_at(const Value& events, int t) {
    Rgba now;
    const auto& list = events.items();

    for (std::size_t i : by_time(list)) {
        const Value& e = list[i];

        if (e["do"].string() != "fade") {
            continue;
        }

        const int t0 = e["t"].tick();

        if (t < t0) {
            break;
        }

        const Value& to = e["to"];
        auto part = [&](int k, float fallback) {
            return std::clamp(static_cast<float>(to[k].number(fallback)), 0.0f, 1.0f);
        };
        const Rgba target = to.is_array() ? Rgba{part(0, 0), part(1, 0), part(2, 0), part(3, 1)}
                                          : Rgba{now.r, now.g, now.b, 0};
        const int len = std::max(0, e["length"].tick(30));
        const float u =
            len > 0 ? std::min(1.0f, static_cast<float>(t - t0) / static_cast<float>(len)) : 1.0f;
        now =
            {now.r + (target.r - now.r) * u,
             now.g + (target.g - now.g) * u,
             now.b + (target.b - now.b) * u,
             now.a + (target.a - now.a) * u};
    }

    return now;
}

// The scene's air at t from its "air" events: each eases over `length` ticks (30 if not given) to
// its `fog` ({"colour": [r, g, b] 0 to 1, "near", "far" units, "thickness" 0 to 1}) and
// `background` ([r, g, b] 0 to 1); one with neither eases back to the game's (weight 0).
struct Air {
    float weight = 0;
    bool fog = false, background = false;
    float fog_colour[3] = {0, 0, 0};
    float fog_near = 0, fog_far = 100, fog_thickness = 0;
    float back[3] = {0, 0, 0};
};

inline Air air_at(const Value& events, int t) {
    Air now;
    const auto& list = events.items();
    const auto unit = [](const Value& v, float fallback) {
        return std::clamp(static_cast<float>(v.number(fallback)), 0.0f, 1.0f);
    };

    for (std::size_t i : by_time(list)) {
        const Value& e = list[i];

        if (e["do"].string() != "air") {
            continue;
        }

        const int t0 = e["t"].tick();

        if (t < t0) {
            break;
        }

        Air to = now;
        const Value& fog = e["fog"];
        const Value& back = e["background"];
        to.weight = fog.is_object() || back.is_array() ? 1.0f : 0.0f;

        if (fog.is_object()) {
            if (!now.fog) {  // fog new to the scene starts from none, at the new key's colour
                for (int k = 0; k < 3; ++k) {
                    now.fog_colour[k] = unit(fog["colour"][k], 0);
                }

                now.fog_near = static_cast<float>(fog["near"].number(0));
                now.fog_far = static_cast<float>(fog["far"].number(100));
            }

            to.fog = true;

            for (int k = 0; k < 3; ++k) {
                to.fog_colour[k] = unit(fog["colour"][k], 0);
            }

            to.fog_near = std::max(0.0f, static_cast<float>(fog["near"].number(0)));
            to.fog_far = std::max(to.fog_near + 0.01f, static_cast<float>(fog["far"].number(100)));
            to.fog_thickness = unit(fog["thickness"], 1);
        } else if (to.weight > 0) {
            to.fog_thickness = 0;  // a key with only a background clears the scene's fog
        }

        if (back.is_array()) {
            if (!now.background) {
                for (int k = 0; k < 3; ++k) {
                    now.back[k] = unit(back[k], 0);
                }
            }

            to.background = true;

            for (int k = 0; k < 3; ++k) {
                to.back[k] = unit(back[k], 0);
            }
        }

        const int len = std::max(0, e["length"].tick(30));
        const float u =
            len > 0 ? std::min(1.0f, static_cast<float>(t - t0) / static_cast<float>(len)) : 1.0f;
        const auto mix = [u](float a, float b) {
            return a + (b - a) * u;
        };
        Air out = to;
        out.weight = mix(now.weight, to.weight);

        for (int k = 0; k < 3; ++k) {
            out.fog_colour[k] = mix(now.fog_colour[k], to.fog_colour[k]);
            out.back[k] = mix(now.back[k], to.back[k]);
        }

        out.fog_near = mix(now.fog_near, to.fog_near);
        out.fog_far = mix(now.fog_far, to.fog_far);
        out.fog_thickness = mix(now.fog_thickness, to.fog_thickness);
        out.fog = now.fog || to.fog;
        out.background = now.background || to.background;
        now = out;
    }

    return now;
}

// Slow motion at t from the "slow" events: each eases over `length` ticks (0 if not given) to its
// `stretch` (1 the game's speed, up to 8).
inline float stretch_at(const Value& events, int t) {
    float now = 1;
    const auto& list = events.items();

    for (std::size_t i : by_time(list)) {
        const Value& e = list[i];

        if (e["do"].string() != "slow") {
            continue;
        }

        const int t0 = e["t"].tick();

        if (t < t0) {
            break;
        }

        const float to = std::clamp(static_cast<float>(e["stretch"].number(1)), 1.0f, 8.0f);
        const int len = std::max(0, e["length"].tick(0));
        const float u =
            len > 0 ? std::min(1.0f, static_cast<float>(t - t0) / static_cast<float>(len)) : 1.0f;
        now += (to - now) * u;
    }

    return now;
}

// A colour as the overlay takes it, 0xRRGGBBAA.
inline std::uint32_t pack_rgba(Rgba c) {
    // Held to 0 to 1, NaN as 0.
    auto byte = [](float v) {
        return static_cast<std::uint32_t>(std::lround((v > 0 ? std::min(v, 1.0f) : 0.0f) * 255.0f));
    };
    return byte(c.r) << 24 | byte(c.g) << 16 | byte(c.b) << 8 | byte(c.a);
}

// ---------------------------------------------------------------------------------------------
// Triggers

// Whether `at` is in a volume trigger's box (min to max, either way round) or sphere.
inline bool inside(const Value& trigger, V3 at) {
    if (trigger["box"].is_object()) {
        const V3 a = vec(trigger["box"]["min"]), b = vec(trigger["box"]["max"]);
        const V3 lo{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
        const V3 hi{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
        return at.x >= lo.x && at.y >= lo.y && at.z >= lo.z && at.x <= hi.x && at.y <= hi.y
               && at.z <= hi.z;
    }

    if (trigger["sphere"].is_object()) {
        return length(at - vec(trigger["sphere"]["position"]))
               <= trigger["sphere"]["radius"].number(4);
    }

    return false;
}

// ---------------------------------------------------------------------------------------------
// Field of view, look-at, joint layers, the music

// Ratchet & Clank's own vertical field of view, about: what the way back to the game's camera
// eases a cutscene's field of view to before it gives the game's back.
constexpr float kGameFov = 51.0f;

// The camera's vertical field of view at t, in degrees, from the keys that give "fov" (kept within
// 1 to 150): held before the first and after the last, linear between, jumping to a "cut" key. 0
// when no key gives one: the game's own.
inline float fov_at(const Value& keys, int t) {
    const auto& all = keys.items();
    std::vector<std::pair<int, float>> stops;
    std::vector<char> cuts;

    for (std::size_t i : by_time(all)) {
        const Value& k = all[i];

        if (!k["fov"].is_number()) {
            continue;
        }

        const double f = k["fov"].number();

        if (!std::isfinite(f)) {
            continue;
        }

        stops.emplace_back(k["t"].tick(), static_cast<float>(std::clamp(f, 1.0, 150.0)));
        cuts.push_back(k["cut"].boolean() ? 1 : 0);
    }

    if (stops.empty()) {
        return 0.0f;
    }

    if (t <= stops.front().first) {
        return stops.front().second;
    }

    std::size_t i = 0;

    while (i + 1 < stops.size() && stops[i + 1].first <= t) {
        ++i;
    }

    if (i + 1 >= stops.size() || cuts[i + 1]) {
        return stops[i].second;
    }

    const auto [ta, fa] = stops[i];
    const auto [tb, fb] = stops[i + 1];
    const float u = tb > ta ? static_cast<float>(t - ta) / static_cast<float>(tb - ta) : 1.0f;
    return fa + (fb - fa) * u;
}

// The yaw from `from` towards `to` (0 along +x, as a rotation's z); `fallback` when they are on one
// upright line.
inline float yaw_to(V3 from, V3 to, float fallback) {
    const float dx = to.x - from.x, dy = to.y - from.y;
    return dx * dx + dy * dy > 1e-8f ? std::atan2(dy, dx) : fallback;
}

// What an actor looks at at t (its "look_at" keys): the key whose target applies (-1 for none) and
// the weight, which eases from the key before's over the key's "blend" ticks (10 if not given).
// Before the first key, and for a key with no target, the weight is 0.
struct Look {
    int key = -1;
    float weight = 0;
};

inline Look look_at(const Value& keys, int t) {
    const auto& all = keys.items();
    const std::vector<std::size_t> order = by_time(all);
    Look look;
    float before = 0;

    for (std::size_t n = 0; n < order.size(); ++n) {
        const Value& k = all[order[n]];
        const int kt = k["t"].tick();

        if (kt > t) {
            break;
        }

        const bool has_target =
            !k["target"].is_null() && (k["target"].is_array() || k["target"].is_object());
        const float w =
            has_target ? static_cast<float>(std::clamp(k["weight"].number(1), 0.0, 1.0)) : 0.0f;
        const int blend = std::max(0, k["blend"].tick(10));
        const float u = blend > 0
                            ? std::min(1.0f, static_cast<float>(t - kt) / static_cast<float>(blend))
                            : 1.0f;
        const float start = n == 0 ? 0.0f : before;
        look.key = has_target ? static_cast<int>(order[n]) : -1;
        look.weight = start + (w - start) * u;
        before = look.weight;

        // A key with no target keeps easing out towards the one before's target.
        if (!has_target && n > 0 && look.weight > 0) {
            look.key = -2;
        }
    }

    if (look.weight < 1e-4f) {
        look = {};
    }

    return look;
}

// A turn from `now` towards `want` by at most `most` radians, the short way round.
inline float turn_toward(float now, float want, float most) {
    float d = std::fmod(want - now, 2 * kPi);

    if (d > kPi) {
        d -= 2 * kPi;
    }

    if (d < -kPi) {
        d += 2 * kPi;
    }

    return now + std::clamp(d, -most, most);
}

// A joint layer's weight at t: "w" of its weight keys, linear between them, held before the first
// and after the last; 1 with none. Kept within 0 to 1.
inline float weight_at(const Value& keys, int t) {
    const auto& all = keys.items();

    if (all.empty()) {
        return 1.0f;
    }

    const std::vector<std::size_t> order = by_time(all);
    const auto w = [&](std::size_t n) {
        return static_cast<float>(std::clamp(all[order[n]]["w"].number(1), 0.0, 1.0));
    };
    const auto at = [&](std::size_t n) {
        return all[order[n]]["t"].tick();
    };

    if (t <= at(0)) {
        return w(0);
    }

    std::size_t i = 0;

    while (i + 1 < order.size() && at(i + 1) <= t) {
        ++i;
    }

    if (i + 1 >= order.size()) {
        return w(i);
    }

    const float u = at(i + 1) > at(i)
                        ? static_cast<float>(t - at(i)) / static_cast<float>(at(i + 1) - at(i))
                        : 1.0f;
    return w(i) + (w(i + 1) - w(i)) * u;
}

// A layer's frames, as the editor writes them: for each tick, for each of the layer's joints, a
// rotation as a quaternion (x, y, z, w), four little-endian float32s. The last tick holds after.
struct LayerFrames {
    std::size_t joints = 0;
    std::vector<float> values;  // ticks * joints * 4

    std::size_t ticks() const { return joints == 0 ? 0 : values.size() / (joints * 4); }
};

// From a file's bytes: false (and nothing) when it is empty, not whole ticks, or has a value that is
// not a number.
inline bool layer_frames(const std::string& bytes, std::size_t joints, LayerFrames& out) {
    out = {};
    const std::size_t tick = joints * 16;

    if (joints == 0 || bytes.empty() || bytes.size() % tick != 0) {
        return false;
    }

    out.joints = joints;
    out.values.resize(bytes.size() / 4);

    for (std::size_t i = 0; i < out.values.size(); ++i) {
        const auto* b = reinterpret_cast<const unsigned char*>(bytes.data()) + 4 * i;
        const std::uint32_t u = std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8
                                | std::uint32_t(b[2]) << 16 | std::uint32_t(b[3]) << 24;
        float f;
        static_assert(sizeof(f) == sizeof(u));
        std::memcpy(&f, &u, sizeof(f));

        if (!std::isfinite(f)) {
            out = {};
            return false;
        }

        out.values[i] = f;
    }

    return true;
}

// Joint `j`'s rotation at tick t, weighted from none (w 0) to the whole frame (w 1), as a unit
// quaternion; the last tick after the end, the first before it.
inline void layer_rotation(const LayerFrames& f, std::size_t j, int t, float w, float out[4]) {
    out[0] = out[1] = out[2] = 0, out[3] = 1;

    if (j >= f.joints || f.ticks() == 0) {
        return;
    }

    const std::size_t tick = static_cast<std::size_t>(
        std::clamp<long long>(t, 0, static_cast<long long>(f.ticks()) - 1)
    );
    const float* q = &f.values[(tick * f.joints + j) * 4];
    float x = q[0], y = q[1], z = q[2], qw = q[3];

    if (qw < 0) {
        x = -x, y = -y, z = -z, qw = -qw;  // the short way from none
    }

    w = std::clamp(w, 0.0f, 1.0f);
    // From the identity: a straight blend, then made whole again (close to a slerp for the turns a
    // face or a hand makes).
    float r[4] = {x * w, y * w, z * w, 1 + (qw - 1) * w};
    const float l = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);

    if (!(l > 1e-6f)) {
        return;
    }

    for (int k = 0; k < 4; ++k) {
        out[k] = r[k] / l;
    }
}

// The game's music gain at t for an "options.music" of "keep", "duck" or "stop", `k` of the way in
// (less on the way back to the game): ducked to a quarter, stopped to nothing, over half a second.
constexpr float kMusicDucked = 0.25f;
constexpr int kMusicTicks = 30;

inline float music_gain(const std::string& mode, int t, float k) {
    const float low = mode == "stop" ? 0.0f : mode == "keep" ? 1.0f : kMusicDucked;
    const float in =
        std::clamp(static_cast<float>(t) / kMusicTicks, 0.0f, 1.0f) * std::clamp(k, 0.0f, 1.0f);
    return 1.0f + (low - 1.0f) * in;
}

// ---------------------------------------------------------------------------------------------
// Walks and faces: the gait for a walk, talking, blinking, moods, the head's share of a look

// Where a walk is at t: 0 not started, 1 walking, 2 arrived (or a walk of no length).
inline int walk_state(const Value& path, int t) {
    const auto& pts = path["points"].items();
    const int start = path["t"].tick();

    if (pts.size() < 2 || t < start) {
        return 0;
    }

    float total = 0;

    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        total += length(vec(pts[i + 1]) - vec(pts[i]));
    }

    const double speed = std::max(0.0, path["speed"].number(8));

    if (speed <= 0 || total <= 1e-6f) {
        return 2;
    }

    return (t - start) / kTickRate * speed < total ? 1 : 2;
}

// The sequence a walk plays: of `gaits` ([{"seq", "speed"}]), the one with the highest speed not
// above the walk's while walking, the slowest once arrived (standing); -1 for none.
inline int gait_at(const Value& gaits, int state, double speed) {
    int seq = -1;
    double best = -1, slowest = 1e30;
    int slow_seq = -1;

    for (const Value& g : gaits.items()) {
        const double s = g["speed"].number(0);
        const int q = g["seq"].integer(-1);

        if (q < 0) {
            continue;
        }

        if (s < slowest) {
            slowest = s, slow_seq = q;
        }

        if (s <= speed && s > best) {
            best = s, seq = q;
        }
    }

    if (state == 2) {
        return slow_seq;
    }

    return state == 1 ? (seq >= 0 ? seq : slow_seq) : -1;
}

// A turn of `degrees` about `axis` (any length; none gives no turn) as a unit quaternion (x, y, z, w).
inline void axis_angle(V3 axis, float degrees, float out[4]) {
    out[0] = out[1] = out[2] = 0, out[3] = 1;
    const float l = length(axis);

    if (!(l > 1e-6f) || !std::isfinite(degrees)) {
        return;
    }

    const float h = degrees * kPi / 360.0f, s = std::sin(h) / l;
    out[0] = axis.x * s, out[1] = axis.y * s, out[2] = axis.z * s, out[3] = std::cos(h);
}

// a * b as quaternions (x, y, z, w): b's turn, then a's.
inline void quat_mul(const float a[4], const float b[4], float out[4]) {
    const float r[4] = {
        a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
        a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
        a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
        a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2],
    };

    for (int k = 0; k < 4; ++k) {
        out[k] = r[k];
    }
}

// How open the mouth is at tick t (0 shut, 1 wide) for a talk starting at `start` whose envelope is
// `bytes`: a byte every thirtieth of a second (two ticks), 0 to 255, straight between them; shut
// before and after.
inline float envelope_at(const std::string& bytes, int start, int t) {
    if (t < start || bytes.empty()) {
        return 0;
    }

    const double at = (t - start) / 2.0;
    const std::size_t i = static_cast<std::size_t>(at);

    if (i >= bytes.size()) {
        return 0;
    }

    const float a = static_cast<unsigned char>(bytes[i]) / 255.0f;
    const float b = i + 1 < bytes.size() ? static_cast<unsigned char>(bytes[i + 1]) / 255.0f : 0.0f;
    return a + (b - a) * static_cast<float>(at - static_cast<double>(i));
}

// A face on a scene's own pose: each face turn (joint, turn) composed after that joint's pose turn,
// so a jaw opens from where the pose holds it. `pose` holds a turn (x, y, z, w) per joint, joint i
// at i; a face joint the pose does not have is left out (a pose sets every joint it moves).
inline void face_on_pose(
    std::vector<std::array<float, 4>>& pose,
    const std::vector<int>& joints,
    const std::vector<std::array<float, 4>>& turns
) {
    for (std::size_t i = 0; i < joints.size() && i < turns.size(); ++i) {
        if (joints[i] < 0 || static_cast<std::size_t>(joints[i]) >= pose.size()) {
            continue;
        }

        float r[4];
        quat_mul(pose[static_cast<std::size_t>(joints[i])].data(), turns[i].data(), r);
        std::copy(r, r + 4, pose[static_cast<std::size_t>(joints[i])].begin());
    }
}

// How shut the eyelids are at t (0 open, 1 shut): a blink every two to six seconds, the gaps drawn
// from `seed` so a cutscene blinks the same way each time; 3 ticks shutting, 1 shut, 4 opening.
inline float blink_at(std::uint32_t seed, int t) {
    if (t < 0) {
        return 0;
    }

    std::uint32_t h = seed * 2654435761u + 0x9E3779B9u;
    const auto next = [&] {
        h ^= h >> 15, h *= 0x2C1B3C6Du, h ^= h >> 12, h *= 0x297A2D39u, h ^= h >> 15;
        return h;
    };
    int at = 60 + static_cast<int>(next() % 120);

    while (at + 8 <= t) {
        at += 120 + static_cast<int>(next() % 240);
    }

    const int d = t - at;

    if (d < 0) {
        return 0;
    }

    if (d < 3) {
        return (d + 1) / 4.0f;
    }

    if (d < 4) {
        return 1;
    }

    return 1 - (d - 3) / 4.0f;
}

// The moods on the face at t, by name and weight: "moods" keys ({"t", "mood", "weight" 1,
// "blend" 10 ticks}) each fade their mood in while the key before fades out; a key with no mood
// fades back to the face's own.
inline std::vector<std::pair<std::string, float>> moods_at(const Value& keys, int t) {
    std::vector<std::pair<std::string, float>> out;
    const auto& all = keys.items();

    if (all.empty()) {
        return out;
    }

    const std::vector<std::size_t> order = by_time(all);
    int i = -1;

    for (std::size_t n = 0; n < order.size(); ++n) {
        if (all[order[n]]["t"].tick() <= t) {
            i = static_cast<int>(n);
        }
    }

    if (i < 0) {
        return out;
    }

    const auto add = [&](const Value& k, float w) {
        const std::string mood = k["mood"].string();
        w *= static_cast<float>(std::clamp(k["weight"].number(1), 0.0, 1.0));

        if (mood.empty() || w <= 1e-4f) {
            return;
        }

        for (auto& m : out) {
            if (m.first == mood) {
                m.second = std::min(1.0f, m.second + w);
                return;
            }
        }

        out.emplace_back(mood, w);
    };
    const Value& now = all[order[static_cast<std::size_t>(i)]];
    const int blend = std::max(0, now["blend"].tick(10));
    const float u =
        blend == 0 ? 1.0f : std::clamp(static_cast<float>(t - now["t"].tick()) / blend, 0.0f, 1.0f);
    add(now, u);

    if (i > 0 && u < 1) {
        add(all[order[static_cast<std::size_t>(i - 1)]], 1 - u);
    }

    return out;
}

// A look of `want` radians away from where the body faces, shared: the head takes up to `limit`
// radians of it, the body turns the rest.
inline void head_share(float want, float limit, float& head, float& body) {
    float d = std::fmod(want, 2 * kPi);

    if (d > kPi) {
        d -= 2 * kPi;
    }

    if (d < -kPi) {
        d += 2 * kPi;
    }

    limit = std::max(0.0f, limit);
    head = std::clamp(d, -limit, limit);
    body = d - head;
}

// ---------------------------------------------------------------------------------------------
// Subtitles as pictures (a language the game's font cannot write, such as Japanese)

// Standard base64 (with or without "=" padding) to bytes: false for any other character.
inline bool base64_decode(const std::string& in, std::string& out) {
    out.clear();
    std::uint32_t acc = 0;
    int bits = 0;

    for (char c : in) {
        int v;

        if (c >= 'A' && c <= 'Z') {
            v = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            v = c - 'a' + 26;
        } else if (c >= '0' && c <= '9') {
            v = c - '0' + 52;
        } else if (c == '+') {
            v = 62;
        } else if (c == '/') {
            v = 63;
        } else if (c == '=') {
            break;
        } else {
            return false;
        }

        acc = (acc << 6) | static_cast<std::uint32_t>(v);
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((acc >> bits) & 0xFF));
        }
    }

    return true;
}

// One lit stretch of a row of a subtitle's picture.
struct BitmapRun {
    std::uint16_t x = 0, y = 0, length = 0;
};

constexpr int kBitmapMaxWidth = 2048, kBitmapMaxHeight = 256;

// A subtitle's picture ("bitmap": {"width", "height", "bits"}: rows of lit pixels, the first pixel
// in each byte's top bit, every row whole bytes, in base64) as the lit runs of its rows. False for
// a size out of range or bits that do not fill it.
inline bool bitmap_runs(const Value& bitmap, std::vector<BitmapRun>& out) {
    out.clear();
    const int w = bitmap["width"].integer(), h = bitmap["height"].integer();

    if (w <= 0 || h <= 0 || w > kBitmapMaxWidth || h > kBitmapMaxHeight) {
        return false;
    }

    std::string bytes;

    if (!base64_decode(bitmap["bits"].string(), bytes)) {
        return false;
    }

    const std::size_t row = static_cast<std::size_t>((w + 7) / 8);

    if (bytes.size() < row * static_cast<std::size_t>(h)) {
        return false;
    }

    for (int y = 0; y < h; ++y) {
        int start = -1;

        for (int x = 0; x <= w; ++x) {
            const bool lit =
                x < w && (static_cast<unsigned char>(bytes[row * y + x / 8]) >> (7 - x % 8) & 1u);

            if (lit && start < 0) {
                start = x;
            }

            if (!lit && start >= 0) {
                out.push_back(
                    {static_cast<std::uint16_t>(start),
                     static_cast<std::uint16_t>(y),
                     static_cast<std::uint16_t>(x - start)}
                );
                start = -1;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------------------------
// Feet on the ground: a leg's two-bone IK

// A joint's axes and place in the level (moby_joint_frame's 12 floats).
struct JointFrame {
    V3 x, y, z, at;
};

inline JointFrame joint_frame(const float f[12]) {
    return {{f[0], f[1], f[2]}, {f[3], f[4], f[5]}, {f[6], f[7], f[8]}, {f[9], f[10], f[11]}};
}

// `v` turned `radians` about the unit `axis`.
inline V3 rotate(V3 v, V3 axis, float radians) {
    const float c = std::cos(radians), s = std::sin(radians);
    return v * c + cross(axis, v) * s + axis * (dot(axis, v) * (1 - c));
}

// A turn in the level's axes (unit quaternion x, y, z, w) as the same turn in a joint's own axes:
// a joint offset turns a joint in its own frame.
inline void to_joint(const JointFrame& f, const float q[4], float out[4]) {
    const V3 ax = normal(f.x, {1, 0, 0}), ay = normal(f.y, {0, 1, 0}), az = normal(f.z, {0, 0, 1});
    const V3 v{q[0], q[1], q[2]};
    out[0] = dot(v, ax), out[1] = dot(v, ay), out[2] = dot(v, az), out[3] = q[3];
}

inline void axis_radians(V3 unit_axis, float radians, float out[4]) {
    const float s = std::sin(radians / 2);
    out[0] = unit_axis.x * s, out[1] = unit_axis.y * s, out[2] = unit_axis.z * s,
    out[3] = std::cos(radians / 2);
}

// How the port composes a joint offset r with the joint's turn q (the game's joint modifiers): as
// q*r, read by rows, so the joint is turned by r's inverse in its PARENT's axes, after its
// animation. The legs keep their turns that way (each in its parent's axes: the hip's parent, then
// the hip, then the knee) and give the port each one's inverse (offset_of).
inline void offset_of(const float turn[4], float out[4]) {
    out[0] = -turn[0], out[1] = -turn[1], out[2] = -turn[2], out[3] = turn[3];
}

// A leg as its animation has it, from the leg drawn with the turns a mod gave it (`turns`, each in
// its parent's axes, as leg_ik returns them; `parent` is the hip's parent, which they do not move)
// and the body lowered by `lowered`: the joints' places and axes with those taken back off. Each
// turn is about its joint's own place, so the bones keep their lengths and their places in their
// parents' axes.
struct Axes3 {
    V3 x, y, z;  // unit columns
};

inline Axes3 unit_axes(const JointFrame& f) {
    return {normal(f.x, {1, 0, 0}), normal(f.y, {0, 1, 0}), normal(f.z, {0, 0, 1})};
}

inline V3 apply(const Axes3& m, V3 v) {
    return m.x * v.x + m.y * v.y + m.z * v.z;
}

inline V3 apply_t(const Axes3& m, V3 v) {
    return {dot(m.x, v), dot(m.y, v), dot(m.z, v)};
}

inline Axes3 mul(const Axes3& a, const Axes3& b) {
    return {apply(a, b.x), apply(a, b.y), apply(a, b.z)};
}

inline Axes3 transpose(const Axes3& m) {
    return {{m.x.x, m.y.x, m.z.x}, {m.x.y, m.y.y, m.z.y}, {m.x.z, m.y.z, m.z.z}};
}

inline Axes3 quat_axes(const float q[4]) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    return {
        {1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w)},
        {2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w)},
        {2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)}
    };
}

inline std::array<JointFrame, 3> leg_unturned(
    const JointFrame& parent,
    const std::array<JointFrame, 3>& drawn,
    const std::array<std::array<float, 4>, 3>& turns,
    float lowered
) {
    std::array<Axes3, 3> w, a;

    for (std::size_t i = 0; i < 3; ++i) {
        w[i] = unit_axes(drawn[i]);
    }

    // Each joint's turn in its parent's axes as animated: drawn = parent * turn * animated.
    const Axes3 p = unit_axes(parent);
    a[0] = mul(mul(mul(p, transpose(quat_axes(turns[0].data()))), transpose(p)), w[0]);

    for (std::size_t i = 1; i < 3; ++i) {
        const Axes3 local =
            mul(transpose(quat_axes(turns[i].data())), mul(transpose(w[i - 1]), w[i]));
        a[i] = mul(a[i - 1], local);
    }

    std::array<JointFrame, 3> out = drawn;
    out[0].at = drawn[0].at - V3{0, 0, lowered};

    for (std::size_t i = 1; i < 3; ++i) {
        const V3 bone = apply_t(w[i - 1], drawn[i].at - drawn[i - 1].at);
        out[i].at = out[i - 1].at + apply(a[i - 1], bone);
    }

    for (std::size_t i = 0; i < 3; ++i) {
        const float size = length(drawn[i].x);
        out[i].x = a[i].x * size, out[i].y = a[i].y * size, out[i].z = a[i].z * size;
    }

    return out;
}

// The turns (each in its parent's axes: `parent` the hip's) that move a leg drawn as hip, knee, ankle so the ankle
// reaches `target`: the knee bends to the hip-to-target length (as far as the leg reaches), the
// hip swings the ankle onto the target, and the ankle turns back so the foot keeps its tilt in the
// level. All three are identity when the ankle is there already.
struct LegTurns {
    float hip[4] = {0, 0, 0, 1}, knee[4] = {0, 0, 0, 1}, ankle[4] = {0, 0, 0, 1};
};

inline LegTurns leg_ik(
    const JointFrame& parent,
    const JointFrame& hip,
    const JointFrame& knee,
    const JointFrame& ankle,
    V3 target
) {
    LegTurns out;
    const V3 h = hip.at, k = knee.at, a = ankle.at;
    const V3 upper = k - h, lower = a - k;
    const float l1 = length(upper), l2 = length(lower);

    if (!(l1 > 1e-5f) || !(l2 > 1e-5f)) {
        return out;
    }

    const float reach = std::clamp(length(target - h), std::fabs(l1 - l2) + 1e-4f, l1 + l2 - 1e-4f);
    // The knee's bend axis: across the leg; a straight leg bends about the hip's x.
    const V3 bend = normal(cross(upper, lower), normal(hip.x, {1, 0, 0}));
    const float now = std::acos(std::clamp(dot(upper * -1.0f, lower) / (l1 * l2), -1.0f, 1.0f));
    const float want =
        std::acos(std::clamp((l1 * l1 + l2 * l2 - reach * reach) / (2 * l1 * l2), -1.0f, 1.0f));
    const float knee_turn = now - want;
    float qk[4];
    axis_radians(bend, knee_turn, qk);
    const V3 ankle_after = k + rotate(lower, bend, knee_turn);
    const V3 from = normal(ankle_after - h, {0, 0, -1}), to = normal(target - h, from);
    const V3 swing_axis = cross(from, to);
    const float swing = std::atan2(length(swing_axis), dot(from, to));
    float qh[4] = {0, 0, 0, 1};

    if (swing > 1e-6f) {
        axis_radians(normal(swing_axis, bend), swing, qh);
    }

    // The level's turn of the ankle (the hip's after the knee's), undone at the ankle.
    float both[4];
    quat_mul(qh, qk, both);
    const float back[4] = {-both[0], -both[1], -both[2], both[3]};
    // Each in its parent's axes as animated: the knee's turn is taken before the hip's swing, which
    // carries it along, and so is the ankle's.
    to_joint(parent, qh, out.hip);
    to_joint(hip, qk, out.knee);
    to_joint(knee, back, out.ankle);
    return out;
}

// ---------------------------------------------------------------------------------------------
// Scene poses: an actor's whole animation, every joint per 30 Hz frame (game_scenes.py writes them)

// One joint in one frame of a pose file: its turn from its parent (x, y, z, w, int16 / 32767, as
// glTF has it: its columns are the joint's axes in its parent's), its place in its parent (int16,
// model units * 1024) and its scale (uint16 / 4096, the editor's only), all little-endian.
constexpr std::size_t kPoseRecord = 20;

struct PoseKey {
    float q[4] = {0, 0, 0, 1};
    V3 t;
};

inline std::size_t pose_frames(const std::string& bytes, std::size_t joints) {
    return joints == 0 || bytes.size() % (joints * kPoseRecord) != 0
               ? 0
               : bytes.size() / (joints * kPoseRecord);
}

inline PoseKey pose_record(
    const std::string& bytes, std::size_t joints, std::size_t frame, std::size_t j
) {
    const auto* b =
        reinterpret_cast<const unsigned char*>(bytes.data()) + (frame * joints + j) * kPoseRecord;
    const auto s16 = [&](int i) {
        return static_cast<float>(static_cast<std::int16_t>(b[2 * i] | b[2 * i + 1] << 8));
    };
    PoseKey k;
    float n = 0;

    for (int i = 0; i < 4; ++i) {
        k.q[i] = s16(i) / 32767.0f, n += k.q[i] * k.q[i];
    }

    n = std::sqrt(n);

    for (int i = 0; i < 4; ++i) {
        k.q[i] = n > 1e-6f ? k.q[i] / n : (i == 3 ? 1.0f : 0.0f);
    }

    k.t = V3{s16(4), s16(5), s16(6)} * (1.0f / 1024.0f);
    return k;
}

// Joint j at tick t (frame f is tick 2f; between two, the turn and place blended): the first frame
// before the start, the last after the end.
inline PoseKey pose_at(const std::string& bytes, std::size_t joints, std::size_t j, int t) {
    const std::size_t frames = pose_frames(bytes, joints);

    if (frames == 0 || j >= joints) {
        return {};
    }

    const long long at = std::clamp<long long>(t, 0, 2 * (static_cast<long long>(frames) - 1));
    const std::size_t f = static_cast<std::size_t>(at / 2);
    PoseKey a = pose_record(bytes, joints, f, j);

    if (at % 2 == 0) {
        return a;
    }

    const PoseKey b = pose_record(bytes, joints, f + 1, j);
    const float sign =
        a.q[0] * b.q[0] + a.q[1] * b.q[1] + a.q[2] * b.q[2] + a.q[3] * b.q[3] < 0 ? -1.0f : 1.0f;
    float n = 0;

    for (int i = 0; i < 4; ++i) {
        a.q[i] = (a.q[i] + sign * b.q[i]) * 0.5f, n += a.q[i] * a.q[i];
    }

    n = std::sqrt(n);

    for (int i = 0; i < 4; ++i) {
        a.q[i] = n > 1e-6f ? a.q[i] / n : (i == 3 ? 1.0f : 0.0f);
    }

    a.t = lerp(a.t, b.t, 0.5f);
    return a;
}

}  // namespace cutscene

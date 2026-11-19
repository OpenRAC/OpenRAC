// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "program/control.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "openrac/game_host.h"

namespace openrac::program::control {
namespace {

// A JSON value as the commands use them: null, a boolean, a number, a string, an array, an object.
struct Value {
    enum Kind {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    } kind = Null;

    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::vector<Value> items;
    std::map<std::string, Value> fields;

    [[nodiscard]] const Value& operator[](const std::string& key) const {
        static const Value none;
        const auto it = fields.find(key);
        return it == fields.end() ? none : it->second;
    }

    [[nodiscard]] bool has(const std::string& key) const { return fields.count(key) != 0; }

    [[nodiscard]] double num(double fallback) const { return kind == Number ? number : fallback; }

    [[nodiscard]] std::string str() const { return kind == String ? text : std::string(); }

    // Three numbers ([x, y, z]) into `out`: whether it is that.
    bool vec3(float out[3]) const {
        if (kind != Array || items.size() != 3) {
            return false;
        }

        for (int i = 0; i < 3; i++) {
            if (items[i].kind != Number) {
                return false;
            }

            out[i] = static_cast<float>(items[i].number);
        }

        return true;
    }
};

// Reads one JSON text (a small reader: the commands are short lines).
struct Reader {
    const std::string& s;
    std::size_t at = 0;
    bool ok = true;

    void space() {
        while (at < s.size() && (s[at] == ' ' || s[at] == '\t' || s[at] == '\r' || s[at] == '\n')) {
            at++;
        }
    }

    bool eat(char c) {
        space();

        if (at < s.size() && s[at] == c) {
            at++;
            return true;
        }

        return false;
    }

    std::string string() {
        std::string out;
        at++;  // the opening quote

        while (at < s.size() && s[at] != '"') {
            char c = s[at++];

            if (c == '\\' && at < s.size()) {
                c = s[at++];

                if (c == 'n') {
                    c = '\n';
                } else if (c == 't') {
                    c = '\t';
                } else if (c == 'u') {
                    // Only ASCII is asked for; anything else becomes '?'.
                    const unsigned code = at + 4 <= s.size()
                                              ? std::strtoul(s.substr(at, 4).c_str(), nullptr, 16)
                                              : 63;
                    at += 4;
                    c = code < 128 ? static_cast<char>(code) : '?';
                }
            }

            out.push_back(c);
        }

        ok = ok && at < s.size();
        at++;
        return out;
    }

    Value value(int depth = 0) {
        Value v;
        space();

        if (at >= s.size() || depth > 16) {
            ok = false;
            return v;
        }

        const char c = s[at];

        if (c == '{') {
            v.kind = Value::Object;
            at++;

            if (eat('}')) {
                return v;
            }

            do {
                space();

                if (at >= s.size() || s[at] != '"') {
                    ok = false;
                    return v;
                }

                const std::string key = string();

                if (!eat(':')) {
                    ok = false;
                    return v;
                }

                v.fields[key] = value(depth + 1);
            } while (ok && eat(','));

            ok = ok && eat('}');
        } else if (c == '[') {
            v.kind = Value::Array;
            at++;

            if (eat(']')) {
                return v;
            }

            do {
                v.items.push_back(value(depth + 1));
            } while (ok && eat(','));

            ok = ok && eat(']');
        } else if (c == '"') {
            v.kind = Value::String;
            v.text = string();
        } else if (s.compare(at, 4, "true") == 0) {
            v.kind = Value::Bool;
            v.boolean = true;
            at += 4;
        } else if (s.compare(at, 5, "false") == 0) {
            v.kind = Value::Bool;
            at += 5;
        } else if (s.compare(at, 4, "null") == 0) {
            at += 4;
        } else {
            char* end = nullptr;
            v.number = std::strtod(s.c_str() + at, &end);

            if (end == s.c_str() + at) {
                ok = false;
            }

            v.kind = Value::Number;
            at = static_cast<std::size_t>(end - s.c_str());
        }

        return v;
    }
};

// A number as JSON can hold it.
std::string num(double v) {
    if (!std::isfinite(v)) {
        return "0";
    }

    char text[32];
    std::snprintf(text, sizeof(text), "%.4f", v);
    std::string out = text;

    while (out.back() == '0') {
        out.pop_back();
    }

    if (out.back() == '.') {
        out.pop_back();
    }

    return out == "-0" ? "0" : out;
}

std::string vec(const float v[3]) {
    return "[" + num(v[0]) + ", " + num(v[1]) + ", " + num(v[2]) + "]";
}

std::string quoted(const std::string& text) {
    std::string out = "\"";

    for (const char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 32) {
            out += ' ';
        } else {
            out += c;
        }
    }

    return out + "\"";
}

double degrees(double radians) {
    return radians * 180.0 / 3.14159265358979323846;
}

// The commands as they come, read on a thread of their own so the game never waits on stdin
// unless it is held.
std::mutex g_lock;
std::condition_variable g_more;
std::deque<std::string> g_lines;
bool g_closed = false;

bool g_on = false;
bool g_quit = false;
long g_tick = 0;

// The command running ticks now.
struct Run {
    bool on = false;
    Value id;
    std::string kind;           // step, wait, shot
    long left = 0;              // step: ticks still to run; wait and shot: the most
    std::string until;          // wait: playing, level, scene, scene_end
    int level = -1;             // wait for level
    std::uint16_t buttons = 0;  // pressed, active high
    bool sticks = false;
    std::uint8_t analog[4] = {128, 128, 128, 128};
    bool skip = false;  // wait: Start pressed now and then while a scene plays, to skip it
    long ran = 0;
} g_run;

// The picture asked for, and whether it was taken.
std::mutex g_shot_lock;
std::string g_shot_path;
std::atomic<int> g_shot_state{0};  // 0 none, 1 asked, 2 written, 3 failed

const openrac_game_play* play() {
    return openrac_game_play_host;
}

void reader() {
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        std::lock_guard hold(g_lock);
        g_lines.push_back(line);
        g_more.notify_one();
    }

    std::lock_guard hold(g_lock);
    g_closed = true;
    g_more.notify_one();
}

void answer(const Value& id, const std::string& body) {
    std::string idtext = "null";

    if (id.kind == Value::Number) {
        idtext = num(id.number);
    } else if (id.kind == Value::String) {
        idtext = quoted(id.text);
    }

    std::printf("@openrac-control {\"id\": %s, %s}\n", idtext.c_str(), body.c_str());
    std::fflush(stdout);
}

void fail(const Value& id, const std::string& why) {
    answer(id, "\"ok\": false, \"error\": " + quoted(why));
}

const char* phase_name(int phase) {
    switch (phase) {
        case OPENRAC_GAME_PHASE_FRONT_END:
            return "front_end";
        case OPENRAC_GAME_PHASE_LOADING:
            return "loading";
        case OPENRAC_GAME_PHASE_PLAYING:
            return "playing";
        case OPENRAC_GAME_PHASE_SCENE:
            return "scene";
        case OPENRAC_GAME_PHASE_MENU:
            return "menu";
        default:
            return "none";
    }
}

int phase(int32_t* level) {
    *level = -1;
    return play() != nullptr && play()->phase != nullptr ? play()->phase(level)
                                                         : OPENRAC_GAME_PHASE_NONE;
}

bool hero(openrac_game_pose& pose) {
    return play() != nullptr && play()->hero_pose != nullptr && play()->hero_pose(&pose) != 0;
}

int32_t scene_playing(int32_t* frame) {
    *frame = 0;
    return play() != nullptr && play()->scene_playing != nullptr ? play()->scene_playing(frame)
                                                                 : -1;
}

// The game now: tick, phase, level, the hero, the scene playing and the camera.
std::string status() {
    int32_t level = -1;
    const int now = phase(&level);
    std::string out =
        "\"tick\": " + std::to_string(g_tick) + ", \"phase\": \"" + phase_name(now)
        + "\", \"level\": " + std::to_string(level) + ", \"level_name\": "
        + quoted(level >= 0 ? openrac_game_level_name(level) : "") + ", \"loads\": "
        + std::to_string(play() != nullptr && play()->loads != nullptr ? play()->loads() : 0);
    openrac_game_pose pose{};

    if (hero(pose)) {
        out += ", \"hero\": {\"position\": " + vec(pose.position) + ", \"yaw\": "
               + num(degrees(pose.rotation[2])) + ", \"state\": " + std::to_string(pose.state)
               + ", \"health\": " + std::to_string(pose.health) + "}";
    } else {
        out += ", \"hero\": null";
    }

    int32_t frame = 0;
    const int32_t scene = scene_playing(&frame);
    out += ", \"scene\": "
           + (scene >= 0 ? "{\"number\": " + std::to_string(scene)
                               + ", \"frame\": " + std::to_string(frame) + "}"
                         : std::string("null"));
    float eye[3];
    float rows[9];

    if (play() != nullptr && play()->camera_view != nullptr
        && play()->camera_view(eye, rows) != 0) {
        out += ", \"camera\": {\"eye\": " + vec(eye) + ", \"forward\": " + vec(rows) + "}";
    }

    return out;
}

// Whether a wait is over.
bool waited(const Run& r) {
    int32_t level = -1;
    const int now = phase(&level);
    openrac_game_pose pose{};
    int32_t frame = 0;

    if (r.until == "playing") {
        return now == OPENRAC_GAME_PHASE_PLAYING && hero(pose);
    }

    if (r.until == "level") {
        return level == r.level && now == OPENRAC_GAME_PHASE_PLAYING && hero(pose);
    }

    if (r.until == "scene") {
        return scene_playing(&frame) >= 0;
    }

    if (r.until == "scene_end") {
        return scene_playing(&frame) < 0 && now == OPENRAC_GAME_PHASE_PLAYING;
    }

    return true;
}

std::string mobys(const Value& c) {
    if (play() == nullptr || play()->mobys == nullptr) {
        return "\"ok\": false, \"error\": \"no mobys in this game\"";
    }

    std::vector<openrac_game_moby_state> all(4096);
    all.resize(static_cast<std::size_t>(play()->mobys(all.data(), 4096)));
    float near[3];
    const bool by_place = c["near"].vec3(near);
    const double radius = c["radius"].num(0.0);
    const int only = static_cast<int>(c["class"].num(-1));
    const std::size_t limit = static_cast<std::size_t>(c["limit"].num(100));

    struct Row {
        double distance;
        std::string text;
    };

    std::vector<Row> rows;

    for (const openrac_game_moby_state& m : all) {
        if (only >= 0 && m.oclass != only) {
            continue;
        }

        double distance = 0.0;

        if (by_place) {
            const double dx = m.position[0] - near[0], dy = m.position[1] - near[1],
                         dz = m.position[2] - near[2];
            distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (radius > 0.0 && distance > radius) {
                continue;
            }
        }

        const int32_t uid = play()->moby_uid != nullptr ? play()->moby_uid(m.slot) : -1;
        rows.push_back(
            {distance,
             "{\"slot\": " + std::to_string(m.slot) + ", \"spawn_id\": " + std::to_string(uid)
                 + ", \"class\": " + std::to_string(m.oclass)
                 + ", \"state\": " + std::to_string(m.state)
                 + ", \"hidden\": " + ((m.flags & 0x40u) ? "true" : "false") + ", \"dead\": "
                 + ((m.flags & 0x10u) ? "true" : "false") + ", \"position\": " + vec(m.position)
                 + ", \"yaw\": " + num(degrees(m.rotation[2]))
                 + (by_place ? ", \"distance\": " + num(distance) : "") + "}"}
        );
    }

    if (by_place) {
        std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
            return a.distance < b.distance;
        });
    }

    std::string out = "\"ok\": true, \"count\": " + std::to_string(rows.size()) + ", \"mobys\": [";

    for (std::size_t i = 0; i < rows.size() && i < limit; i++) {
        out += (i ? ", " : "") + rows[i].text;
    }

    return out + "]";
}

// The floor under a point: the highest collision triangle facing up below it.
std::string ground(const Value& c) {
    float at[3];

    if (!c["position"].vec3(at)) {
        return "\"ok\": false, \"error\": \"position is [x, y, z]\"";
    }

    if (play() == nullptr || play()->collision_triangles == nullptr) {
        return "\"ok\": false, \"error\": \"no collision in this game\"";
    }

    const float top = at[2] + static_cast<float>(c["above"].num(2.0));
    // A narrow box: the query fills at most its room, so a tall one could leave the floor out.
    const float lo[3] =
        {at[0] - 0.25f, at[1] - 0.25f, top - static_cast<float>(c["depth"].num(60.0))};
    const float hi[3] = {at[0] + 0.25f, at[1] + 0.25f, top};

    struct Triangle {
        float corners[3][3];
        float normal[3];
        uint32_t type;
        int32_t moby;
    };

    std::vector<Triangle> found(4096);
    const int count = play()->collision_triangles(lo, hi, found.data(), 4096);

    if (count > 4096) {
        return "\"ok\": false, \"error\": \"too many triangles there: look from nearer (depth)\"";
    }

    found.resize(static_cast<std::size_t>(std::min(count, 4096)));
    bool any = false;
    float best = 0.0f;
    const Triangle* floor = nullptr;

    for (const Triangle& t : found) {
        if (t.normal[2] <= 0.2f) {
            continue;
        }

        // Where the vertical line through the point meets the triangle (barycentric in x, y).
        const float* a = t.corners[0];
        const float* b = t.corners[1];
        const float* d = t.corners[2];
        const float det = (b[1] - d[1]) * (a[0] - d[0]) + (d[0] - b[0]) * (a[1] - d[1]);

        if (std::fabs(det) < 1e-8f) {
            continue;
        }

        const float u = ((b[1] - d[1]) * (at[0] - d[0]) + (d[0] - b[0]) * (at[1] - d[1])) / det;
        const float v = ((d[1] - a[1]) * (at[0] - d[0]) + (a[0] - d[0]) * (at[1] - d[1])) / det;
        const float w = 1.0f - u - v;

        if (u < -1e-4f || v < -1e-4f || w < -1e-4f) {
            continue;
        }

        const float z = u * a[2] + v * b[2] + w * d[2];

        if (z <= top && (!any || z > best)) {
            any = true;
            best = z;
            floor = &t;
        }
    }

    if (!any) {
        return "\"ok\": true, \"found\": false";
    }

    const float point[3] = {at[0], at[1], best};
    return "\"ok\": true, \"found\": true, \"position\": " + vec(point) + ", \"normal\": "
           + vec(floor->normal) + ", \"surface\": " + std::to_string(floor->type & 31u)
           + ", \"moby\": " + std::to_string(floor->moby);
}

// The camera put where it is asked: an eye and a point to look at, or let go.
std::string camera(const Value& c) {
    if (play() == nullptr || play()->camera_set == nullptr) {
        return "\"ok\": false, \"error\": \"no camera in this game\"";
    }

    float eye[3];
    float look[3];

    if (c["release"].boolean || !c["eye"].vec3(eye) || !c["look_at"].vec3(look)) {
        if (!c["release"].boolean) {
            return "\"ok\": false, \"error\": \"eye and look_at are [x, y, z], or release: true\"";
        }

        play()->camera_set(nullptr, nullptr);
        return "\"ok\": true";
    }

    float f[3] = {look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]};
    const float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);

    if (fl < 1e-4f) {
        return "\"ok\": false, \"error\": \"eye and look_at are the same point\"";
    }

    for (float& x : f) {
        x /= fl;
    }

    // Left is up x forward; up is forward x left (z up).
    float l[3] = {-f[1], f[0], 0.0f};
    float ll = std::sqrt(l[0] * l[0] + l[1] * l[1]);

    if (ll < 1e-4f) {
        l[0] = 0.0f;
        l[1] = 1.0f;
        ll = 1.0f;
    }

    l[0] /= ll;
    l[1] /= ll;
    const float u[3] =
        {f[1] * l[2] - f[2] * l[1], f[2] * l[0] - f[0] * l[2], f[0] * l[1] - f[1] * l[0]};
    const float rows[9] = {f[0], f[1], f[2], l[0], l[1], l[2], u[0], u[1], u[2]};
    return play()->camera_set(eye, rows) != 0 ? "\"ok\": true"
                                              : "\"ok\": false, \"error\": \"not in a level\"";
}

std::uint16_t buttons_of(const Value& c) {
    return static_cast<std::uint16_t>(static_cast<long>(c["buttons"].num(0)) & 0xFFFF);
}

void stick(const Value& v, std::uint8_t& x, std::uint8_t& y) {
    if (v.kind == Value::Array && v.items.size() == 2) {
        x = static_cast<std::uint8_t>(std::clamp(v.items[0].num(128), 0.0, 255.0));
        y = static_cast<std::uint8_t>(std::clamp(v.items[1].num(128), 0.0, 255.0));
    }
}

// One command: answered now (false), or started as a run of ticks (true).
bool handle(const std::string& line) {
    Reader read{line};
    const Value c = read.value();
    const Value id = c["id"];

    if (!read.ok || c.kind != Value::Object) {
        fail(id, "not a JSON object");
        return false;
    }

    const std::string cmd = c["cmd"].str();
    const openrac_game_play* p = play();

    if (cmd == "status") {
        answer(id, "\"ok\": true, " + status());
    } else if (cmd == "step" || cmd == "wait") {
        g_run = Run{};
        g_run.on = true;
        g_run.id = id;
        g_run.kind = cmd;
        g_run.left =
            static_cast<long>(c[cmd == "step" ? "ticks" : "max_ticks"].num(cmd == "step" ? 1 : 3600)
            );
        g_run.left = std::clamp(g_run.left, 1L, 60L * 60L * 10L);
        g_run.until = c["until"].str();
        g_run.level = static_cast<int>(c["level"].num(-1));
        g_run.buttons = buttons_of(c);
        g_run.skip = c["skip"].boolean;

        if (c.has("left") || c.has("right")) {
            g_run.sticks = true;
            stick(c["right"], g_run.analog[0], g_run.analog[1]);
            stick(c["left"], g_run.analog[2], g_run.analog[3]);
        }

        if (cmd == "wait" && waited(g_run)) {
            g_run.on = false;
            answer(id, "\"ok\": true, \"reached\": true, \"ran\": 0, " + status());
            return false;
        }

        return true;
    } else if (cmd == "shot") {
#ifdef OPENRAC_FRONTEND
        const std::string path = c["path"].str();

        if (path.empty()) {
            fail(id, "path is the PNG file to write");
            return false;
        }

        {
            std::lock_guard hold(g_shot_lock);
            g_shot_path = path;
        }
        g_shot_state = 1;
        g_run = Run{};
        g_run.on = true;
        g_run.id = id;
        g_run.kind = "shot";
        g_run.left = 30;  // a frame is drawn within a tick or two
        return true;
#else
        fail(id, "this program has no window (the -states program): no pictures");
#endif
    } else if (cmd == "mobys") {
        answer(id, mobys(c));
    } else if (cmd == "hero_place") {
        float at[3];

        if (!c["position"].vec3(at)) {
            fail(id, "position is [x, y, z]");
        } else if (p == nullptr || p->hero_place == nullptr) {
            fail(id, "this game cannot place the hero");
        } else {
            const float yaw =
                static_cast<float>(c["yaw"].num(0.0) * 3.14159265358979323846 / 180.0);
            const uint32_t flags = (c["ground"].boolean ? 0x2u : 0u) | 0x1u;
            const int asked = p->hero_place(at, yaw, flags);
            answer(
                id,
                asked ? "\"ok\": true, \"note\": \"the hero is there after the next tick (step)\""
                      : "\"ok\": false, \"error\": \"refused: no level or no hero now\""
            );
        }
    } else if (cmd == "travel") {
        const int level = static_cast<int>(c["level"].num(-1));

        if (p == nullptr || p->travel == nullptr || !openrac_game_has_level(level)) {
            fail(id, "no such level");
        } else {
            answer(
                id,
                p->travel(level) ? "\"ok\": true, \"note\": \"on the way: wait until level\""
                                 : "\"ok\": false, \"error\": \"refused: not in a level\""
            );
        }
    } else if (cmd == "scene") {
        const int scene = static_cast<int>(c["number"].num(-1));

        if (p == nullptr || p->scene_start == nullptr) {
            fail(id, "this game cannot start its scenes");
        } else {
            answer(
                id,
                p->scene_start(scene)
                    ? "\"ok\": true, \"note\": \"it starts once the player has control\""
                    : "\"ok\": false, \"error\": \"refused: no level\""
            );
        }
    } else if (cmd == "camera") {
        answer(id, camera(c));
    } else if (cmd == "line") {
        float from[3];
        float to[3];
        int blocked = 0;

        if (!c["from"].vec3(from) || !c["to"].vec3(to)) {
            fail(id, "from and to are [x, y, z]");
        } else if (p == nullptr || p->level_line == nullptr
                   || p->level_line(from, to, &blocked) == 0) {
            fail(id, "not in a level");
        } else {
            answer(id, std::string("\"ok\": true, \"blocked\": ") + (blocked ? "true" : "false"));
        }
    } else if (cmd == "ground") {
        answer(id, ground(c));
    } else if (cmd == "quit") {
        answer(id, "\"ok\": true");
        g_quit = true;
    } else {
        fail(
            id,
            "unknown command " + cmd
                + " (status, step, wait, shot, mobys, hero_place, travel, scene, camera, line, "
                  "ground, quit)"
        );
    }

    return false;
}

}  // namespace

void request() {
    g_on = true;
}

bool requested() {
    return g_on;
}

void tick() {
    if (!g_on) {
        return;
    }

    static bool started = false;

    if (!started) {
        started = true;
        std::thread(reader).detach();
        std::printf(
            "@openrac-control {\"id\": null, \"ok\": true, \"ready\": true, \"game\": %s}\n",
            quoted(openrac_game.id).c_str()
        );
        std::fflush(stdout);
    }

    g_tick++;

    if (g_run.on) {
        g_run.ran++;
        g_run.left--;
        bool done = g_run.left <= 0;
        bool reached = false;

        if (g_run.kind == "wait") {
            reached = waited(g_run);
            done = done || reached;
        } else if (g_run.kind == "shot") {
            const int state = g_shot_state;
            done = done || state >= 2;

            if (done) {
                g_run.on = false;
                std::string path;
                {
                    std::lock_guard hold(g_shot_lock);
                    path = g_shot_path;
                    g_shot_path.clear();
                }
                g_shot_state = 0;

                if (state == 2) {
                    answer(
                        g_run.id,
                        "\"ok\": true, \"path\": " + quoted(path)
                            + ", \"ran\": " + std::to_string(g_run.ran)
                    );
                } else {
                    fail(g_run.id, "no frame was drawn");
                }
            }
        }

        if (done && g_run.on) {
            g_run.on = false;
            answer(
                g_run.id,
                std::string("\"ok\": true, ")
                    + (g_run.kind == "wait"
                           ? std::string("\"reached\": ") + (reached ? "true" : "false") + ", "
                           : "")
                    + "\"ran\": " + std::to_string(g_run.ran) + ", " + status()
            );
        }

        if (g_run.on) {
            return;
        }
    }

    // Held: the commands, until one runs ticks.
    for (;;) {
        std::unique_lock hold(g_lock);
        g_more.wait(hold, [] { return !g_lines.empty() || g_closed; });

        if (g_lines.empty()) {
            g_quit = true;  // the program that drove the game is gone
            return;
        }

        const std::string line = g_lines.front();
        g_lines.pop_front();
        hold.unlock();

        if (handle(line) || g_quit) {
            return;
        }
    }
}

bool quitting() {
    return g_quit;
}

void pad(std::uint16_t& buttons, std::uint8_t analog[4]) {
    if (!g_on || !g_run.on) {
        return;
    }

    if (g_run.kind == "wait" && g_run.skip) {
        // Start for 5 ticks of every 30 while a scene plays (the port's Start skips it, as the
        // game's own skip does); never while the hero plays, where it would pause.
        int32_t level = -1;

        if (phase(&level) == OPENRAC_GAME_PHASE_SCENE && g_run.ran % 30 < 5) {
            buttons = static_cast<std::uint16_t>(buttons & ~0x8u);
        }

        return;
    }

    if (g_run.kind != "step") {
        return;
    }

    buttons = static_cast<std::uint16_t>(buttons & ~g_run.buttons);

    if (g_run.sticks) {
        for (int i = 0; i < 4; i++) {
            analog[i] = g_run.analog[i];
        }
    }
}

std::string take_shot() {
    if (g_shot_state != 1) {
        return std::string();
    }

    std::lock_guard hold(g_shot_lock);
    return g_shot_path;
}

void shot_taken(bool ok) {
    g_shot_state = ok ? 2 : 3;
}

}  // namespace openrac::program::control

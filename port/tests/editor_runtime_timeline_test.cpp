// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// Tests of the built-in editor runtime's JSON reader and timeline sums (game/rac1/editor_runtime),
// without the game.
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "json.hpp"
#include "timeline.hpp"

using namespace cutscene;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* what, int line) {
    ++g_checks;

    if (!ok) {
        ++g_failures;
        std::fprintf(stderr, "FAILED line %d: %s\n", line, what);
    }
}

#define CHECK(x) check((x), #x, __LINE__)

bool near(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) <= eps;
}

bool near3(V3 a, V3 b, float eps = 1e-3f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

Value parse(const std::string& text) {
    std::string error;
    Value v = Value::parse(text, error);

    if (!error.empty()) {
        std::fprintf(stderr, "parse error: %s in %s\n", error.c_str(), text.c_str());
    }

    return v;
}

bool fails(const std::string& text) {
    std::string error;
    Value v = Value::parse(text, error);
    return !error.empty() && v.is_null();
}

// --- JSON

void json_values() {
    const Value v =
        parse(R"({"a": 1, "b": [1, 2.5, -3e2], "c": "x\ny", "d": true, "e": false, "f": null})");
    CHECK(v.is_object());
    CHECK(v["a"].integer() == 1);
    CHECK(v["b"].size() == 3);
    CHECK(v["b"][1].number() == 2.5);
    CHECK(v["b"][2].number() == -300);
    CHECK(v["c"].string() == "x\ny");
    CHECK(v["d"].boolean() && !v["e"].boolean(true));
    CHECK(v["f"].is_null() && v.has("f"));
    // Missing parts read as null and chain.
    CHECK(v["nope"]["deeper"][3].is_null());
    CHECK(v["nope"].integer(7) == 7);
    CHECK(v["b"][-1].is_null() && v["b"][99].is_null());
    CHECK(v["a"].string_or("s") == "s");
    CHECK(v["c"].number(4) == 4);
    // A comment, as the format's examples have.
    CHECK(parse("// a comment\n{\"x\": 1 // after\n}")["x"].integer() == 1);
    CHECK(parse("  [ ]  ").is_array());
    CHECK(parse("{}").is_object());
}

void json_numbers() {
    CHECK(parse("0").number() == 0);
    CHECK(parse("-0.5").number() == -0.5);
    CHECK(parse("1E3").number() == 1000);
    CHECK(parse("2e-2").number() == 0.02);
    CHECK(fails("+1"));
    CHECK(fails("01"));
    CHECK(fails("1."));
    CHECK(fails(".5"));
    CHECK(fails("1e"));
    CHECK(fails("nan"));
    CHECK(fails("inf"));
    CHECK(fails("0x10"));
    CHECK(fails("-"));
    // Past a double: infinite or zero; the integer of it held to int's range.
    CHECK(std::isinf(parse("1e999").number()) && parse("1e999").number() > 0);
    CHECK(parse("-1e999").number() < 0);
    CHECK(parse("1e-999").number() == 0);
    CHECK(parse("1e999").integer() == 2147483647);
    CHECK(parse("-1e999").integer() == -2147483647 - 1);
    CHECK(parse("3000000000").integer() == 2147483647);
    CHECK(parse("-2.9").integer() == -2);

    // The same in a locale whose decimal mark is a comma.
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") || std::setlocale(LC_NUMERIC, "fr_FR.UTF-8")) {
        CHECK(parse("1.5").number() == 1.5);
        std::setlocale(LC_NUMERIC, "C");
    }
}

// A JSON string of `body`, with "~" for a backslash (so the escapes stay escapes in this file).
std::string quoted(std::string body) {
    for (char& c : body) {
        if (c == '~') {
            c = '\\';
        }
    }

    return "\"" + body + "\"";
}

void json_strings() {
    CHECK(parse(quoted("a~\"b~~c~/d~t")).string() == "a\"b\\c/d\t");
    CHECK(parse(quoted("~u00e9")).string() == "\xC3\xA9");
    CHECK(parse(quoted("~u3042")).string() == "\xE3\x81\x82");
    CHECK(parse(quoted("~ud83d~ude00")).string() == "\xF0\x9F\x98\x80");
    CHECK(parse(quoted("~u0041~u004a")).string() == "AJ");
    // UTF-8 as it is passes through.
    CHECK(parse(quoted("\xC3\xA9")).string() == "\xC3\xA9");
    // Half a pair, or a pair whose second half is not one: the replacement character.
    CHECK(parse(quoted("~ud83d")).string() == "\xEF\xBF\xBD");
    CHECK(parse(quoted("~ude00")).string() == "\xEF\xBF\xBD");
    CHECK(
        parse(quoted("~ud83dA")).string()
        == "\xEF\xBF\xBD"
           "A"
    );
    CHECK(
        parse(quoted("~ud83d~u0041")).string()
        == "\xEF\xBF\xBD"
           "A"
    );
    CHECK(parse(quoted("~ud83d~ud83d")).string() == "\xEF\xBF\xBD\xEF\xBF\xBD");
    CHECK(fails(quoted("~u12")));
    CHECK(fails(quoted("~u12zz")));
    CHECK(fails(quoted("~u+123")));
    CHECK(fails(R"("open)"));
    CHECK(fails("\"ends in a backslash\\"));
}

void json_broken() {
    CHECK(fails(""));
    CHECK(fails("   "));
    CHECK(fails("{"));
    CHECK(fails("[1, 2"));
    CHECK(fails("[1 2]"));
    CHECK(fails("{\"a\" 1}"));
    CHECK(fails("{a: 1}"));
    CHECK(fails("{\"a\": 1,}"));
    CHECK(fails("[1,]"));
    CHECK(fails("tru"));
    CHECK(fails("truex"));
    CHECK(fails("{} {}"));
    std::string deep(100, '[');
    deep += std::string(100, ']');
    CHECK(fails(deep));
    std::string fine(60, '[');
    fine += std::string(60, ']');
    CHECK(!fails(fine));
}

// --- Maths

void maths() {
    CHECK(near(angle_lerp(0.1f, 6.2f, 0.5f), 0.1f + (6.2f - 2 * kPi - 0.1f) * 0.5f));
    CHECK(near(angle_lerp(3.0f, -3.0f, 1.0f), 3.0f + (2 * kPi - 6.0f)));
    CHECK(near(angle_lerp(1.0f, 2.0f, 0.25f), 1.25f));
    CHECK(near3(normal({0, 0, 0}, {1, 2, 3}), {1, 2, 3}));
    CHECK(near3(normal({0, 0, NAN}, {1, 0, 0}), {1, 0, 0}));
    CHECK(near3(normal({0, 3, 4}, {}), {0, 0.6f, 0.8f}));
    CHECK(near3(direction({0, 0, 0}), {1, 0, 0}));
    CHECK(near3(direction({0, 0, kPi / 2}), {0, 1, 0}));
    CHECK(near3(direction({0, kPi / 2, 0}), {0, 0, -1}));  // pitch above 0 looks down
    // The curves go through their ends.
    const V3 a{0, 0, 0}, b{1, 2, 3}, c{4, 4, 4}, d{9, 0, 1};
    CHECK(near3(catmull(a, b, c, d, 0), b) && near3(catmull(a, b, c, d, 1), c));
    CHECK(near3(bezier(a, b, c, d, 0), a) && near3(bezier(a, b, c, d, 1), d));
    float q[4];
    quaternion({0, 0, 0}, q);
    CHECK(near(q[3], 1) && near(q[0], 0) && near(q[1], 0) && near(q[2], 0));
    quaternion({0, 0, kPi}, q);
    CHECK(near(q[2], 1) && near(q[3], 0));
    CHECK(near3(vec(parse("[1, 2]"), {7, 7, 7}), {7, 7, 7}));
    CHECK(near3(vec(parse("[1, \"2\", 3]"), {7, 7, 7}), {7, 7, 7}));
    CHECK(near3(vec(parse("[1, 2, 3, 4]")), {1, 2, 3}));
}

// --- Tracks

void tracks() {
    const Value track = parse(R"([
        {"t": 0, "position": [0, 0, 0], "rotation": [0, 0, 0], "scale": 1},
        {"t": 60, "position": [6, 0, 0], "scale": 3},
        {"t": 120, "position": [6, 6, 0], "rotation": [0, 0, 3], "cut": true},
        {"t": 180, "rotation": [0, 0, -3]}
    ])");
    V3 pos, rot;
    float scale = 1;
    track_at(track, -10, pos, rot, scale);
    CHECK(near3(pos, {0, 0, 0}) && near(scale, 1));
    track_at(track, 30, pos, rot, scale);
    CHECK(near3(pos, {3, 0, 0}) && near(scale, 2));
    track_at(track, 60, pos, rot, scale);
    CHECK(near3(pos, {6, 0, 0}) && near(scale, 3));
    // Up to a cut it holds, then jumps.
    track_at(track, 119, pos, rot, scale);
    CHECK(near3(pos, {6, 0, 0}) && near(rot.z, 0));
    track_at(track, 120, pos, rot, scale);
    CHECK(near3(pos, {6, 6, 0}) && near(rot.z, 3));
    // A key with no position keeps the one before; the yaw goes the short way (3 to -3 over pi).
    track_at(track, 150, pos, rot, scale);
    CHECK(near3(pos, {6, 6, 0}));
    CHECK(near(rot.z, 3 + (2 * kPi - 6) * 0.5f));
    track_at(track, 1000, pos, rot, scale);
    CHECK(near(rot.z, -3));

    // Keys out of order are read in time; keys at the same tick: the later in the file wins after.
    const Value shuffled =
        parse(R"([{"t": 60, "position": [6, 0, 0]}, {"t": 0, "position": [0, 0, 0]}])");
    track_at(shuffled, 30, pos, rot, scale);
    CHECK(near3(pos, {3, 0, 0}));
    const Value same =
        parse(R"([{"t": 0, "position": [0, 0, 0]}, {"t": 0, "position": [5, 0, 0]}])");
    track_at(same, 0, pos, rot, scale);
    CHECK(near3(pos, {0, 0, 0}));
    track_at(same, 1, pos, rot, scale);
    CHECK(near3(pos, {5, 0, 0}));

    // No keys: nothing changes. One key: always it.
    pos = {1, 1, 1};
    track_at(parse("[]"), 10, pos, rot, scale);
    CHECK(near3(pos, {1, 1, 1}));
    track_at(parse("null"), 10, pos, rot, scale);
    CHECK(near3(pos, {1, 1, 1}));
    track_at(parse(R"([{"t": 50, "position": [2, 2, 2]}])"), 10, pos, rot, scale);
    CHECK(near3(pos, {2, 2, 2}));
    // A first key with only a place keeps the rotation and scale passed in.
    rot = {0, 0, 1}, scale = 4;
    track_at(parse(R"([{"t": 0, "position": [0, 0, 0]}])"), 0, pos, rot, scale);
    CHECK(near(rot.z, 1) && near(scale, 4));

    // Smooth keys pass through every key.
    const Value smooth = parse(R"([
        {"t": 0, "position": [0, 0, 0], "ease": "smooth"},
        {"t": 10, "position": [10, 5, 0], "ease": "smooth"},
        {"t": 20, "position": [20, 0, 0], "ease": "smooth"}
    ])");
    track_at(smooth, 10, pos, rot, scale);
    CHECK(near3(pos, {10, 5, 0}));
    track_at(smooth, 5, pos, rot, scale);
    CHECK(pos.y > 2.5f);  // the curve bulges past the straight line towards the middle key
}

void paths() {
    V3 pos, rot;
    const Value path =
        parse(R"({"t": 60, "speed": 6, "points": [[0, 0, 0], [6, 0, 0], [6, 6, 0]]})");
    CHECK(!path_at(path, 59, pos, rot));
    CHECK(path_at(path, 60, pos, rot) && near3(pos, {0, 0, 0}) && near(rot.z, 0));
    CHECK(path_at(path, 90, pos, rot) && near3(pos, {3, 0, 0}));
    CHECK(path_at(path, 150, pos, rot) && near3(pos, {6, 3, 0}) && near(rot.z, kPi / 2));
    // At the end it stays there.
    CHECK(path_at(path, 6000, pos, rot) && near3(pos, {6, 6, 0}));
    // A point twice keeps the yaw of the stretch before it.
    const Value twice = parse(R"({"speed": 60, "points": [[0, 0, 0], [0, 5, 0], [0, 5, 0]]})");
    CHECK(path_at(twice, 600, pos, rot) && near3(pos, {0, 5, 0}) && near(rot.z, kPi / 2));
    // No speed is 8; a speed below 0 stands still; one point is no path.
    CHECK(
        path_at(parse(R"({"points": [[0, 0, 0], [100, 0, 0]]})"), 60, pos, rot)
        && near3(pos, {8, 0, 0})
    );
    CHECK(
        path_at(parse(R"({"speed": -5, "points": [[1, 0, 0], [100, 0, 0]]})"), 60, pos, rot)
        && near3(pos, {1, 0, 0})
    );
    CHECK(!path_at(parse(R"({"points": [[0, 0, 0]]})"), 60, pos, rot));
    CHECK(!path_at(parse("null"), 60, pos, rot));
}

void visibility_and_anims() {
    const Value keys = parse(R"([{"t": 30, "on": false}, {"t": 90, "on": true}])");
    CHECK(
        visible_at(keys, 0) && !visible_at(keys, 30) && !visible_at(keys, 89)
        && visible_at(keys, 90)
    );
    CHECK(visible_at(parse("null"), 5));
    // Out of order: still the latest at or before t.
    const Value shuffled = parse(R"([{"t": 90, "on": true}, {"t": 30, "on": false}])");
    CHECK(!visible_at(shuffled, 60) && visible_at(shuffled, 100));
    const Value anims = parse(R"([{"t": 0, "seq": 3}, {"t": 60, "seq": 5}])");
    CHECK(
        anim_at(anims, -1) == -1 && anim_at(anims, 0) == 0 && anim_at(anims, 59) == 0
        && anim_at(anims, 60) == 1
    );
    CHECK(anim_at(parse("[]"), 10) == -1);
}

// --- Camera

std::vector<CamKey> keys_of(const std::string& text) {
    const Value v = parse(text);
    std::vector<CamKey> keys;

    for (std::size_t i : by_time(v.items())) {
        const Value& k = v[i];
        V3 target = vec(k["look_at"]);
        keys.push_back(camera_key(
            k, keys.empty() ? nullptr : &keys.back(), k["look_at"].is_array() ? &target : nullptr
        ));
    }

    return keys;
}

void camera() {
    CameraPose pose;
    CHECK(!camera_pose({}, 0, pose));
    const auto keys = keys_of(R"([
        {"t": 0, "eye": [0, 0, 10], "rotation": [0, 0, 0], "ease": "linear"},
        {"t": 60, "eye": [6, 0, 10], "rotation": [0, 0, 0]},
        {"t": 61, "eye": [0, 50, 10], "rotation": [0, 0, 1.5707963], "cut": true},
        {"t": 120, "eye": [0, 50, 10], "look_at": [0, 50, 0]}
    ])");
    CHECK(
        camera_pose(keys, -5, pose) && near3(pose.eye, {0, 0, 10}) && near3(pose.forward, {1, 0, 0})
    );
    CHECK(camera_pose(keys, 30, pose) && near3(pose.eye, {3, 0, 10}));
    // Held up to the cut, then the new view.
    CHECK(camera_pose(keys, 60, pose) && near3(pose.eye, {6, 0, 10}));
    CHECK(
        camera_pose(keys, 61, pose) && near3(pose.eye, {0, 50, 10})
        && near3(pose.forward, {0, 1, 0})
    );
    // Looking at a point: straight down.
    CHECK(camera_pose(keys, 500, pose) && near3(pose.forward, {0, 0, -1}));
    // Roll from "rotation"'s first part, or "roll".
    const auto rolled = keys_of(
        R"([{"t": 0, "eye": [0, 0, 0], "rotation": [0.5, 0, 0]},
                                    {"t": 10, "eye": [0, 0, 0], "rotation": [0, 0, 0], "roll": -0.5}])"
    );
    CHECK(camera_pose(rolled, 0, pose) && near(pose.roll, 0.5f));
    CHECK(camera_pose(rolled, 5, pose) && near(pose.roll, 0));
    // Looking at its own eye keeps the direction it had; a key without eye keeps the one before.
    const auto blind = keys_of(R"([{"t": 0, "eye": [1, 1, 1], "rotation": [0, 0, 1.5707963]},
                                   {"t": 10, "look_at": [1, 1, 1]}])");
    CHECK(near3(blind[1].eye, {1, 1, 1}) && near3(blind[1].forward, {0, 1, 0}));
    // Two keys at the same tick: no division by zero.
    const auto same =
        keys_of(R"([{"t": 5, "eye": [0, 0, 0], "rotation": [0, 0, 0], "ease": "linear"},
                                  {"t": 5, "eye": [9, 0, 0], "rotation": [0, 0, 0]}])");
    CHECK(camera_pose(same, 5, pose) && std::isfinite(pose.eye.x));
    // Opposite directions between keys: still a direction, not NaN.
    const auto turn =
        keys_of(R"([{"t": 0, "eye": [0, 0, 0], "rotation": [0, 0, 0], "ease": "linear"},
                                  {"t": 10, "eye": [0, 0, 0], "rotation": [0, 0, 3.14159265]}])");
    CHECK(
        camera_pose(turn, 5, pose) && std::isfinite(pose.forward.x) && near(length(pose.forward), 1)
    );
    // Bezier: handles bend the way; the ends are the keys.
    const auto bez = keys_of(
        R"([{"t": 0, "eye": [0, 0, 0], "rotation": [0, 0, 0], "ease": "bezier", "out": [0, 10, 0]},
                                 {"t": 10, "eye": [10, 0, 0], "rotation": [0, 0, 0], "in": [0, 10, 0]}])"
    );
    CHECK(camera_pose(bez, 5, pose) && pose.eye.y > 5);
    CHECK(camera_pose(bez, 10, pose) && near3(pose.eye, {10, 0, 0}));
}

void camera_axes_and_shake() {
    CameraAxes a = camera_axes({1, 0, 0}, 0);
    CHECK(near3(a.left, {0, 1, 0}) && near3(a.up, {0, 0, 1}));
    a = camera_axes({0, 0, -1}, 0);  // straight down: no NaN
    CHECK(near(length(a.left), 1) && near(length(a.up), 1) && near(dot(a.left, a.forward), 0));
    a = camera_axes({1, 0, 0}, kPi / 2);  // the roll turns left towards up
    CHECK(near3(a.left, {0, 0, 1}) && near3(a.up, {0, -1, 0}));
    a = camera_axes({0, 0, 0}, 0);
    CHECK(near3(a.forward, {1, 0, 0}));
    const Value shake = parse(R"([{"t": 10, "length": 20, "strength": 1}])");
    CHECK(near3(shake_at(shake, 9), {}) && near3(shake_at(shake, 30), {}));
    CHECK(length(shake_at(shake, 12)) > 0);
    CHECK(length(shake_at(parse(R"([{"t": 0, "length": 0}])"), 0)) >= 0);  // a length 0 is 1
}

// --- Text, sounds, triggers

void text() {
    CHECK(
        ascii(
            "Gr\xc3\xbc\xc3\x9f"
            "e, se\xc3\xb1or! Caf\xc3\xa9 \xe6\xad\xa6"
        )
        == "Gruse, senor! Cafe "
    );
    CHECK(
        ascii("\xc5") == ""
        && ascii(
               "a\xc5"
               "b"
           ) == "ab"
    );
    CHECK(ascii("caf\xC3\xA9 ok\x01") == "cafe ok");
    auto rows = wrap("one two three", 40, 3);
    CHECK(rows.size() == 1 && rows[0] == "one two three");
    rows = wrap("aaaa bbbb cccc", 9, 3);
    CHECK(rows.size() == 2 && rows[0] == "aaaa bbbb" && rows[1] == "cccc");
    rows = wrap("  spaced   out\nlines  ", 40, 3);
    CHECK(rows.size() == 1 && rows[0] == "spaced out lines");
    CHECK(wrap("", 40, 3).empty() && wrap("    ", 40, 3).empty());
    // A word longer than a row is broken.
    rows = wrap("abcdefghij", 4, 3);
    CHECK(rows.size() == 3 && rows[0] == "abcd" && rows[1] == "efgh" && rows[2] == "ij");
    // Too much text: three rows, the last ending in "...".
    rows = wrap("a b c d e f g h", 3, 3);
    CHECK(rows.size() == 3 && rows[2] == "...");
    rows = wrap("word word word word word word", 10, 2);
    CHECK(rows.size() == 2 && rows[1].size() <= 10 && rows[1].ends_with("..."));
    CHECK(wrap("x", 0, 3).empty() && wrap("x", 3, 0).empty());

    const Value text = parse(R"({"en": "Hi", "fr": "Salut"})");
    CHECK(pick_language(text, parse(R"(["fr", "en"])"), "fr") == "fr");
    CHECK(pick_language(text, parse(R"(["fr", "en"])"), "de") == "fr");
    CHECK(pick_language(text, parse(R"(["de"])"), "it") == "en");
    CHECK(pick_language(parse(R"({"es": "Hola"})"), parse("null"), "it") == "es");
    CHECK(pick_language(parse("{}"), parse("null"), "en").empty());
    CHECK(pick_language(parse("\"plain\""), parse("null"), "en").empty());
}

void fades() {
    CHECK(near(fade_gain(0, 0, 0, -1, 50), 1));
    CHECK(near(fade_gain(10, 20, 0, -1, 20), 0.5f));
    CHECK(near(fade_gain(10, 20, 0, -1, 5), 0));  // before its start: silent, not below 0
    CHECK(near(fade_gain(0, 0, 10, 100, 95), 0.5f));
    CHECK(near(fade_gain(0, 0, 10, 100, 100), 0));
    CHECK(near(fade_gain(0, 0, 10, -1, 1000), 1));  // no end: no fade out
    // Fading in and out at once: the quieter.
    CHECK(near(fade_gain(0, 100, 100, 100, 50), 0.5f));
    CHECK(near(fade_gain(0, 100, 10, 100, 95), 0.5f));
}

void volumes() {
    const Value box = parse(R"({"box": {"min": [0, 0, 0], "max": [2, 2, 2]}})");
    CHECK(inside(box, {1, 1, 1}) && inside(box, {0, 0, 0}) && inside(box, {2, 2, 2}));
    CHECK(!inside(box, {3, 1, 1}) && !inside(box, {1, 1, -0.1f}));
    // min and max the wrong way round still make the box.
    const Value flipped = parse(R"({"box": {"min": [2, 2, 2], "max": [0, 0, 0]}})");
    CHECK(inside(flipped, {1, 1, 1}));
    const Value sphere = parse(R"({"sphere": {"position": [0, 0, 0], "radius": 2}})");
    CHECK(inside(sphere, {0, 2, 0}) && !inside(sphere, {0, 2.01f, 0}));
    const Value plain = parse(R"({"sphere": {"position": [0, 0, 0]}})");  // radius 4
    CHECK(inside(plain, {0, 0, 3.9f}) && !inside(plain, {0, 0, 4.1f}));
    CHECK(!inside(parse("{}"), {0, 0, 0}) && !inside(parse("null"), {0, 0, 0}));
}

void picture() {
    CHECK(
        near(letterbox_at(-5), 0) && near(letterbox_at(0), 0)
        && near(letterbox_at(kLetterboxTicks), 1)
    );
    CHECK(near(letterbox_at(kLetterboxTicks / 2), 0.5f, 0.1f) && near(letterbox_at(9999), 1));
    // To black over 30 ticks, then back to clear over 10.
    const Value events = parse(R"([
        {"t": 100, "do": "fade", "to": null, "length": 10},
        {"t": 0, "do": "moby_hide", "spawn_id": 3},
        {"t": 60, "do": "fade", "to": [0, 0, 0], "length": 30}
    ])");
    CHECK(near(fade_at(events, 59).a, 0));
    CHECK(near(fade_at(events, 75).a, 0.5f));
    CHECK(near(fade_at(events, 95).a, 1));
    CHECK(near(fade_at(events, 105).a, 0.5f));
    CHECK(near(fade_at(events, 200).a, 0));
    // A colour with its own alpha, no length (30), and length 0 (at once); parts held to 0 to 1.
    const Value white = parse(R"([{"t": 0, "do": "fade", "to": [1, 1, 1, 0.5]}])");
    CHECK(near(fade_at(white, 15).a, 0.25f) && near(fade_at(white, 15).r, 0.5f));
    const Value now = parse(R"([{"t": 10, "do": "fade", "to": [2, -1, 0.5], "length": 0}])");
    const Rgba c = fade_at(now, 10);
    CHECK(near(c.a, 1) && near(c.r, 1) && near(c.g, 0) && near(c.b, 0.5f));
    CHECK(near(fade_at(parse("null"), 10).a, 0));
    CHECK(pack_rgba({1, 0, 0, 1}) == 0xFF0000FFu);
    CHECK(pack_rgba({0, 0.5f, 1, 0}) == 0x0080FF00u);
    CHECK(pack_rgba({-1, 2, 0, NAN}) == 0x00FF0000u);
}

void field_of_view() {
    // No key with a field of view: the game's.
    CHECK(fov_at(parse(R"([{"t": 0, "eye": [0, 0, 0]}])"), 5) == 0.0f);
    CHECK(fov_at(parse("null"), 5) == 0.0f);
    // Held before and after, linear between, keys out of order, out of range kept in it.
    const Value keys =
        parse(R"([{"t": 100, "fov": 30}, {"t": 0, "fov": 60}, {"t": 50}, {"t": 200, "fov": 500}])");
    CHECK(near(fov_at(keys, -10), 60));
    CHECK(near(fov_at(keys, 0), 60));
    CHECK(near(fov_at(keys, 50), 45));
    CHECK(near(fov_at(keys, 100), 30));
    CHECK(near(fov_at(keys, 150), 90));
    CHECK(near(fov_at(keys, 999), 150));
    CHECK(near(fov_at(parse(R"([{"t": 0, "fov": 0}])"), 0), 1));
    // A cut jumps.
    const Value cut = parse(R"([{"t": 0, "fov": 60}, {"t": 100, "fov": 20, "cut": true}])");
    CHECK(near(fov_at(cut, 99), 60));
    CHECK(near(fov_at(cut, 100), 20));
    // A field of view that is not a number is not one.
    CHECK(fov_at(parse(R"([{"t": 0, "fov": "wide"}])"), 0) == 0.0f);
}

void looking() {
    CHECK(near(yaw_to({0, 0, 0}, {0, 5, 3}, 9), kPi / 2));
    CHECK(near(yaw_to({1, 1, 0}, {1, 1, 8}, 0.25f), 0.25f));
    CHECK(near(yaw_to({0, 0, 0}, {-1, 0, 0}, 0), kPi));
    // Weights ease in over the blend, from 0 before the first key.
    const Value keys = parse(R"([
        {"t": 10, "target": {"actor": "hero"}, "weight": 1, "blend": 10},
        {"t": 40, "target": [1, 2, 3], "weight": 0.5, "blend": 0},
        {"t": 60, "target": null, "blend": 20}])");
    CHECK(look_at(keys, 0).key == -1 && look_at(keys, 0).weight == 0);
    CHECK(look_at(keys, 10).weight == 0);  // the blend has not begun
    CHECK(look_at(keys, 15).key == 0 && near(look_at(keys, 15).weight, 0.5f));
    CHECK(near(look_at(keys, 25).weight, 1));
    CHECK(look_at(keys, 40).key == 1 && near(look_at(keys, 40).weight, 0.5f));
    // A key with no target eases out, still turned towards the one before.
    CHECK(look_at(keys, 70).key == -2 && near(look_at(keys, 70).weight, 0.25f));
    CHECK(look_at(keys, 80).key == -1 && look_at(keys, 80).weight == 0);
    // A weight past 1 is 1; none given is 1.
    CHECK(near(
        look_at(parse(R"([{"t": 0, "target": [0, 0, 0], "weight": 7, "blend": 0}])"), 0).weight, 1
    ));
    CHECK(near(look_at(parse(R"([{"t": 0, "target": [0, 0, 0], "blend": 0}])"), 3).weight, 1));
    // Turns the short way round, no more than asked.
    CHECK(near(turn_toward(0, 1, 0.25f), 0.25f));
    CHECK(near(turn_toward(0, -1, 0.25f), -0.25f));
    CHECK(near(turn_toward(3.0f, -3.0f, 1.0f), 3.0f + (2 * kPi - 6.0f)));
    CHECK(near(turn_toward(0.5f, 0.6f, 1.0f), 0.6f));
}

void layers() {
    CHECK(near(weight_at(parse("null"), 5), 1));
    const Value w = parse(R"([{"t": 10, "w": 0}, {"t": 20, "w": 1}, {"t": 30, "w": 3}])");
    CHECK(near(weight_at(w, 0), 0));
    CHECK(near(weight_at(w, 15), 0.5f));
    CHECK(near(weight_at(w, 40), 1));
    // Two joints, two ticks.
    std::string bytes;
    const auto put = [&](float f) {
        std::uint32_t u;
        std::memcpy(&u, &f, 4);

        for (int i = 0; i < 4; ++i) {
            bytes.push_back(static_cast<char>(u >> (8 * i)));
        }
    };
    const float h = std::sqrt(0.5f);

    for (float f : {0.f, 0.f, h, h, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, 0.f}) {
        put(f);
    }

    LayerFrames frames;
    CHECK(layer_frames(bytes, 2, frames) && frames.ticks() == 2);
    CHECK(!layer_frames(bytes, 3, frames) && frames.ticks() == 0);
    CHECK(!layer_frames("", 2, frames));
    CHECK(!layer_frames(bytes, 0, frames));
    CHECK(!layer_frames(bytes.substr(0, 20), 1, frames));
    std::string bad = bytes;
    const float nan = NAN;
    std::memcpy(&bad[4], &nan, 4);
    CHECK(!layer_frames(bad, 2, frames));
    CHECK(layer_frames(bytes, 2, frames));
    float q[4];
    layer_rotation(frames, 0, 0, 1, q);
    CHECK(near(q[2], h) && near(q[3], h));
    // Half the weight: half the turn (45 degrees about z of a 90 one).
    layer_rotation(frames, 0, 0, 0.5f, q);
    CHECK(near(std::atan2(q[2], q[3]) * 2, kPi / 4, 0.02f));
    layer_rotation(frames, 0, 0, 0, q);
    CHECK(near(q[3], 1) && near(q[2], 0));
    // A negative w is the same turn the short way; ticks past the end hold the last.
    layer_rotation(frames, 0, 50, 1, q);
    CHECK(near(q[3], 1));
    layer_rotation(frames, 1, -5, 1, q);
    CHECK(near(q[3], 1));
    layer_rotation(frames, 1, 1, 1, q);
    CHECK(near(q[0], 1) && near(q[3], 0));
    // A joint the layer does not have: no turn.
    layer_rotation(frames, 9, 0, 1, q);
    CHECK(q[3] == 1 && q[0] == 0);
}

void music() {
    CHECK(near(music_gain("duck", 0, 1), 1));
    CHECK(near(music_gain("duck", kMusicTicks / 2, 1), (1 + kMusicDucked) / 2));
    CHECK(near(music_gain("duck", 999, 1), kMusicDucked));
    CHECK(near(music_gain("stop", 999, 1), 0));
    CHECK(near(music_gain("keep", 999, 1), 1));
    CHECK(near(music_gain("stop", 999, 0.5f), 0.5f));
    CHECK(near(music_gain("stop", 999, 0), 1));
    CHECK(near(music_gain("", 999, 1), kMusicDucked));
}

void subtitle_pictures() {
    std::string out;
    CHECK(base64_decode("TWFu", out) && out == "Man");
    CHECK(base64_decode("TWE=", out) && out == "Ma");
    CHECK(base64_decode("TQ", out) && out == "M");
    CHECK(base64_decode("", out) && out.empty());
    CHECK(base64_decode("//8=", out) && out == std::string("\xff\xff"));
    CHECK(!base64_decode("TW!u", out));
    CHECK(!base64_decode("TW u", out));
    // 10 x 2: row 0 lit at 0-2 and 9, row 1 lit at 4-9 (two bytes a row).
    // Row 0: 11100000 01000000 -> 0xE0 0x40; row 1: 00001111 11000000 -> 0x0F 0xC0.
    std::vector<BitmapRun> runs;
    CHECK(bitmap_runs(parse(R"({"width": 10, "height": 2, "bits": "4EAPwA=="})"), runs));
    CHECK(runs.size() == 3);
    CHECK(runs.size() == 3 && runs[0].x == 0 && runs[0].y == 0 && runs[0].length == 3);
    CHECK(runs.size() == 3 && runs[1].x == 9 && runs[1].y == 0 && runs[1].length == 1);
    CHECK(runs.size() == 3 && runs[2].x == 4 && runs[2].y == 1 && runs[2].length == 6);
    // Too few bits, sizes out of range, bad base64: nothing.
    CHECK(
        !bitmap_runs(parse(R"({"width": 10, "height": 3, "bits": "4EAPwA=="})"), runs)
        && runs.empty()
    );
    CHECK(!bitmap_runs(parse(R"({"width": 0, "height": 2, "bits": ""})"), runs));
    CHECK(!bitmap_runs(parse(R"({"width": 5000, "height": 2, "bits": "4EAPwA=="})"), runs));
    CHECK(!bitmap_runs(parse(R"({"width": 10, "height": 2, "bits": "4E*PwA=="})"), runs));
    CHECK(!bitmap_runs(parse("null"), runs));
    // All lit: one run a row.
    CHECK(
        bitmap_runs(parse(R"({"width": 8, "height": 2, "bits": "//8="})"), runs) && runs.size() == 2
        && runs[1].length == 8
    );
}

void walks_and_faces() {
    // A walk of 6 units at 3 a second from tick 30: walking for two seconds, then arrived.
    const Value path = parse(R"({"t": 30, "speed": 3, "points": [[0, 0, 0], [6, 0, 0]]})");
    CHECK(walk_state(path, 29) == 0);
    CHECK(walk_state(path, 30) == 1);
    CHECK(walk_state(path, 149) == 1);
    CHECK(walk_state(path, 150) == 2);
    CHECK(walk_state(parse(R"({"t": 0, "speed": 0, "points": [[0, 0, 0], [6, 0, 0]]})"), 5) == 2);
    CHECK(walk_state(parse(R"({"t": 0, "points": [[1, 1, 1], [1, 1, 1]]})"), 5) == 2);
    CHECK(walk_state(parse(R"({"t": 0, "points": [[1, 1, 1]]})"), 5) == 0);

    const Value gaits =
        parse(R"([{"seq": 1, "speed": 0}, {"seq": 4, "speed": 2}, {"seq": 7, "speed": 6}])");
    CHECK(gait_at(gaits, 1, 3) == 4);
    CHECK(gait_at(gaits, 1, 6) == 7);
    CHECK(gait_at(gaits, 1, 9) == 7);
    CHECK(gait_at(gaits, 1, 1) == 1);
    CHECK(gait_at(gaits, 2, 9) == 1);  // arrived: standing
    CHECK(gait_at(gaits, 0, 9) == -1);
    CHECK(gait_at(parse("[]"), 1, 3) == -1);
    CHECK(
        gait_at(parse(R"([{"seq": 5, "speed": 4}])"), 1, 1) == 5
    );                                                       // slower than any: the slowest
    CHECK(gait_at(parse(R"([{"speed": 4}])"), 1, 5) == -1);  // no sequence

    float q[4];
    axis_angle({1, 0, 0}, 90, q);
    CHECK(
        near(q[0], std::sqrt(0.5f)) && near(q[3], std::sqrt(0.5f)) && near(q[1], 0) && near(q[2], 0)
    );
    axis_angle({0, 0, 5}, 180, q);  // any length of axis
    CHECK(near(q[2], 1) && near(q[3], 0));
    axis_angle({0, 0, 0}, 90, q);
    CHECK(q[3] == 1 && q[0] == 0);
    axis_angle({1, 0, 0}, NAN, q);
    CHECK(q[3] == 1);
    float a[4], b[4], r[4];
    axis_angle({0, 0, 1}, 30, a);
    axis_angle({0, 0, 1}, 60, b);
    quat_mul(a, b, r);
    axis_angle({0, 0, 1}, 90, q);
    CHECK(near(r[2], q[2]) && near(r[3], q[3]));
    const float id[4] = {0, 0, 0, 1};
    quat_mul(id, a, r);
    CHECK(near(r[2], a[2]) && near(r[3], a[3]));

    // The envelope: a byte each two ticks, straight between, shut outside.
    const std::string env = std::string("\x00\xff\x80", 3);
    CHECK(envelope_at(env, 10, 9) == 0);
    CHECK(near(envelope_at(env, 10, 10), 0));
    CHECK(near(envelope_at(env, 10, 11), 0.5f));
    CHECK(near(envelope_at(env, 10, 12), 1));
    CHECK(near(envelope_at(env, 10, 14), 128 / 255.0f));
    CHECK(near(envelope_at(env, 10, 15), 64 / 255.0f));  // towards shut after the last
    CHECK(envelope_at(env, 10, 16) == 0);
    CHECK(envelope_at("", 0, 0) == 0);

    // Blinks: the same each time, apart, each one whole.
    int blinks = 0, shut = 0;
    float before = 0;

    for (int t = 0; t < 60 * 60; ++t) {
        const float v = blink_at(7, t);
        CHECK(v >= 0 && v <= 1);
        CHECK(v == blink_at(7, t));

        if (v == 1) {
            ++shut;
        }

        if (v > 0 && before == 0) {
            ++blinks;
        }
        before = v;
    }
    CHECK(blinks >= 10 && blinks <= 30);  // every two to six seconds over a minute
    CHECK(shut == blinks);
    CHECK(blink_at(7, -5) == 0);
    bool differ = false;

    for (int t = 0; t < 600 && !differ; ++t) {
        differ = blink_at(7, t) != blink_at(8, t);
    }
    CHECK(differ);

    // Moods: in over the blend, the one before out, none back to the face's own.
    const Value moods = parse(R"([{"t": 100, "mood": "angry", "blend": 0},
                                  {"t": 0, "mood": "happy", "weight": 0.8},
                                  {"t": 200}])");
    CHECK(moods_at(moods, -1).empty());
    auto m = moods_at(moods, 5);
    CHECK(m.size() == 1 && m[0].first == "happy" && near(m[0].second, 0.4f));
    m = moods_at(moods, 50);
    CHECK(m.size() == 1 && near(m[0].second, 0.8f));
    m = moods_at(moods, 100);
    CHECK(m.size() == 1 && m[0].first == "angry" && near(m[0].second, 1));
    m = moods_at(moods, 205);
    CHECK(m.size() == 1 && m[0].first == "angry" && near(m[0].second, 0.5f));
    CHECK(moods_at(moods, 210).empty());
    m = moods_at(parse(R"([{"t": 0, "mood": "sad"}, {"t": 10, "mood": "sad", "blend": 10}])"), 15);
    CHECK(m.size() == 1 && near(m[0].second, 1));  // the same mood again stays whole

    // The head takes its share of a look, the body the rest, the short way round.
    float head, body;
    head_share(0.5f, 1.0f, head, body);
    CHECK(near(head, 0.5f) && near(body, 0));
    head_share(1.5f, 1.0f, head, body);
    CHECK(near(head, 1.0f) && near(body, 0.5f));
    head_share(-2.0f, 1.0f, head, body);
    CHECK(near(head, -1.0f) && near(body, -1.0f));
    head_share(2 * kPi - 0.2f, 1.0f, head, body);
    CHECK(near(head, -0.2f) && near(body, 0));
    head_share(0.3f, -1.0f, head, body);
    CHECK(head == 0 && near(body, 0.3f));
}

void rides_and_eyes() {
    CHECK(near3(turn_z({1, 0, 0}, kPi / 2), {0, 1, 0}));
    CHECK(near3(turn_z({1, 2, 3}, 0), {1, 2, 3}));
    CHECK(near3(turn_z({0, 1, 5}, kPi), {0, -1, 5}));
    const Value rides =
        parse(R"([{"t": 100, "actor": "ship"}, {"t": 300}, {"t": 400, "actor": "lift"}])");
    CHECK(ride_at(rides, 99) == -1);
    CHECK(ride_at(rides, 100) == 0);
    CHECK(ride_at(rides, 299) == 0);
    CHECK(ride_at(rides, 300) == -1);  // off again
    CHECK(ride_at(rides, 500) == 2);
    CHECK(ride_at(parse("[]"), 5) == -1);
    // A key whose eye is on an actor takes the place given; the next key with no eye keeps it.
    const Value k = parse(R"({"t": 0, "eye": {"actor": "ship"}, "rotation": [0, 0, 0]})");
    const V3 on = {5, 6, 7};
    const CamKey a = camera_key(k, nullptr, nullptr, &on);
    CHECK(near3(a.eye, on));
    const CamKey b = camera_key(parse(R"({"t": 10})"), &a, nullptr);
    CHECK(near3(b.eye, on));
    const CamKey c = camera_key(parse(R"({"t": 10, "eye": [1, 2, 3]})"), &a, nullptr);
    CHECK(near3(c.eye, {1, 2, 3}));
}

void air_and_slow() {
    const Value ev = parse(R"([
        {"t": 10, "do": "air", "fog": {"colour": [1, 0, 0], "near": 5, "far": 50, "thickness": 0.5},
         "length": 10},
        {"t": 40, "do": "air", "background": [0, 0, 1], "length": 0},
        {"t": 60, "do": "air", "length": 20},
        {"t": 5, "do": "slow", "stretch": 4},
        {"t": 20, "do": "slow", "stretch": 1, "length": 10},
        {"t": 30, "do": "slow", "stretch": 20}])");
    CHECK(air_at(ev, 5).weight == 0);
    const Air half = air_at(ev, 15);
    CHECK(half.fog && near(half.weight, 0.5f) && near(half.fog_colour[0], 1));
    CHECK(near(half.fog_thickness, 0.25f) && near(half.fog_near, 5) && near(half.fog_far, 50));
    const Air full = air_at(ev, 30);
    CHECK(near(full.weight, 1) && near(full.fog_thickness, 0.5f) && !full.background);
    // A background-only key clears the fog and keeps its colour.
    const Air back = air_at(ev, 45);
    CHECK(back.background && near(back.back[2], 1) && near(back.fog_thickness, 0));
    CHECK(near(air_at(ev, 70).weight, 0.5f));
    CHECK(air_at(ev, 80).weight == 0);
    CHECK(stretch_at(ev, 0) == 1);
    CHECK(stretch_at(ev, 5) == 4);
    CHECK(near(stretch_at(ev, 25), 2.5f));
    CHECK(near(stretch_at(ev, 29), 1.3f));
    CHECK(stretch_at(ev, 30) == 8);  // up to 8
}

// A vector turned by a unit quaternion (x, y, z, w).
V3 turn_by(const float q[4], V3 v) {
    const V3 u{q[0], q[1], q[2]};
    return v + cross(u, cross(u, v) + v * q[3]) * 2.0f;
}

void feet() {
    // A leg standing a little bent (identity axes: its joints' turns are the level's), the ankle
    // raised 0.2 onto a step.
    const float id[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    float f[12];
    std::copy(id, id + 12, f);
    f[9] = 0, f[10] = 0, f[11] = 1;
    const JointFrame hip = joint_frame(f);
    f[9] = 0, f[10] = 0.1f, f[11] = 0.5f;
    const JointFrame knee = joint_frame(f);
    f[9] = 0, f[10] = 0, f[11] = 0;
    const JointFrame ankle = joint_frame(f);
    const V3 target{0, 0, 0.2f};
    const JointFrame pelvis{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 1.1f}};
    const LegTurns t = leg_ik(pelvis, hip, knee, ankle, target);
    const V3 k2 = hip.at + turn_by(t.hip, knee.at - hip.at);
    float hk[4];
    quat_mul(t.hip, t.knee, hk);
    const V3 a2 = k2 + turn_by(hk, ankle.at - knee.at);
    CHECK(length(a2 - target) < 1e-3f);
    CHECK(
        std::fabs(length(k2 - hip.at) - length(knee.at - hip.at)) < 1e-4f
    );                   // bones keep their length
    CHECK(k2.y > 0.1f);  // the knee bends further the way it bent
    float all[4];
    quat_mul(hk, t.ankle, all);
    CHECK(std::fabs(all[3]) > 0.9999f);  // the foot keeps its tilt
    // Already there: no turns.
    const LegTurns none = leg_ik(pelvis, hip, knee, ankle, ankle.at);
    CHECK(std::fabs(none.hip[3]) > 0.9999f && std::fabs(none.knee[3]) > 0.9999f);
    // Too far: the leg straightens toward it, no further.
    const LegTurns far = leg_ik(pelvis, hip, knee, ankle, {0, 0, -1});
    const V3 k3 = hip.at + turn_by(far.hip, knee.at - hip.at);
    float hk3[4];
    quat_mul(far.hip, far.knee, hk3);
    const V3 a3 = k3 + turn_by(hk3, ankle.at - knee.at);
    CHECK(a3.z < -0.015f && a3.z > -0.03f);
    // The leg drawn with those turns, taken back to its animation.
    std::array<JointFrame, 3> drawn{hip, knee, ankle};
    const Axes3 rh = quat_axes(t.hip);
    float hk2[4], hka[4];
    quat_mul(t.hip, t.knee, hk2);
    quat_mul(hk2, t.ankle, hka);
    const Axes3 rk = quat_axes(hk2), ra = quat_axes(hka);
    drawn[0].x = rh.x, drawn[0].y = rh.y, drawn[0].z = rh.z;
    drawn[1].x = rk.x, drawn[1].y = rk.y, drawn[1].z = rk.z, drawn[1].at = k2;
    drawn[2].x = ra.x, drawn[2].y = ra.y, drawn[2].z = ra.z, drawn[2].at = a2;

    for (JointFrame& j : drawn) {
        j.at.z -= 0.1f;  // and the body lowered
    }
    const std::array<std::array<float, 4>, 3> given{
        {{t.hip[0], t.hip[1], t.hip[2], t.hip[3]},
         {t.knee[0], t.knee[1], t.knee[2], t.knee[3]},
         {t.ankle[0], t.ankle[1], t.ankle[2], t.ankle[3]}}
    };
    const auto back = leg_unturned(pelvis, drawn, given, -0.1f);
    CHECK(length(back[1].at - knee.at) < 1e-4f && length(back[2].at - ankle.at) < 1e-4f);
    CHECK(length(back[2].x - V3{1, 0, 0}) < 1e-4f && length(back[1].z - V3{0, 0, 1}) < 1e-4f);
    // A joint's own axes: a turn about the level's z is about the joint's y when its y is the
    // level's z.
    float g[12] = {1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0};
    float q[4], local[4];
    axis_radians({0, 0, 1}, 1.0f, q);
    to_joint(joint_frame(g), q, local);
    CHECK(
        std::fabs(local[1] - std::sin(0.5f)) < 1e-5f && std::fabs(local[0]) < 1e-6f
        && std::fabs(local[2]) < 1e-6f
    );
}

// The port's own composition (scene/moby_animation.cpp): a joint's quaternion q and a mod's
// offset r make q*r, its matrix read by rows (the joint's axes in its parent's). A leg whose
// joints all turn (as a figure's do) reaches the step through it, tick after tick as the player
// runs it, and is read back to its animation.
Axes3 port_axes(const float q[4], const float r[4]) {
    float qr[4];
    quat_mul(q, r, qr);
    const float x = qr[0], y = qr[1], z = qr[2], w = qr[3];
    // quat_rows: rows of the standard matrix, each the image of an axis.
    return {
        {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
        {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
        {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}
    };
}

void feet_in_port() {
    // The pelvis turned, each joint's own turn from its parent, each bone along its parent's axes.
    float pq[4], q[3][4];
    axis_radians(normal(V3{0.3f, -0.2f, 1}, {0, 0, 1}), 2.3f, pq);
    axis_radians(normal(V3{1, 0.4f, 0.1f}, {1, 0, 0}), 2.9f, q[0]);    // the hip, pointing down
    axis_radians(normal(V3{0.2f, 1, -0.3f}, {0, 1, 0}), -0.5f, q[1]);  // the knee, bent
    axis_radians(normal(V3{-0.4f, 0.1f, 1}, {0, 0, 1}), 0.7f, q[2]);   // the ankle
    const V3 bones[3] = {{0.08f, 0.02f, -0.1f}, {0.01f, 0.02f, 0.2f}, {-0.02f, 0.01f, 0.18f}};
    const float none[4] = {0, 0, 0, 1};
    const Axes3 pelvis_axes = port_axes(pq, none);
    const JointFrame pelvis{pelvis_axes.x, pelvis_axes.y, pelvis_axes.z, {5, 6, 1}};
    // The leg drawn with the given offsets (as the port gets them) and the body lowered by `low`.
    auto draw = [&](const float offs[3][4], float low) {
        std::array<JointFrame, 3> out;
        Axes3 parent = pelvis_axes;
        V3 at = pelvis.at + V3{0, 0, low};

        for (int i = 0; i < 3; ++i) {
            at = at + apply(parent, bones[i]);
            const Axes3 local = port_axes(q[i], offs[i]);
            parent = mul(parent, local);
            out[static_cast<std::size_t>(i)] = {parent.x, parent.y, parent.z, at};
        }
        return out;
    };
    float zero[3][4] = {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}};
    const std::array<JointFrame, 3> animated = draw(zero, 0);
    const V3 target = animated[2].at + V3{0, 0, 0.12f};
    const float low = -0.1f;
    std::array<std::array<float, 4>, 3> turns{{{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}}};
    float lowered = 0;

    for (int tick = 0; tick < 40; ++tick) {
        float offs[3][4];

        for (int i = 0; i < 3; ++i) {
            offset_of(turns[static_cast<std::size_t>(i)].data(), offs[i]);
        }
        const std::array<JointFrame, 3> drawn = draw(offs, lowered);
        std::array<JointFrame, 3> f = leg_unturned(pelvis, drawn, turns, lowered);

        if (tick > 0) {
            // Read back: the animation, wherever the turns are.
            CHECK(
                length(f[1].at - animated[1].at) < 1e-3f && length(f[2].at - animated[2].at) < 1e-3f
            );
            CHECK(length(normal(f[2].x, {}) - normal(animated[2].x, {})) < 1e-3f);
        }

        for (JointFrame& j : f) {
            j.at.z += low;
        }
        const LegTurns t = leg_ik(pelvis, f[0], f[1], f[2], target);
        const float* fresh[3] = {t.hip, t.knee, t.ankle};

        for (std::size_t i = 0; i < 3; ++i) {
            std::copy(fresh[i], fresh[i] + 4, turns[i].data());
        }
        lowered = low;
    }
    float offs[3][4];

    for (int i = 0; i < 3; ++i) {
        offset_of(turns[static_cast<std::size_t>(i)].data(), offs[i]);
    }
    const std::array<JointFrame, 3> drawn = draw(offs, low);
    CHECK(length(drawn[2].at - target) < 2e-3f);  // the ankle on the step
    CHECK(
        length(normal(drawn[2].x, {}) - normal(animated[2].x, {})) < 2e-3f
    );  // the foot's tilt kept
    CHECK(length(normal(drawn[2].z, {}) - normal(animated[2].z, {})) < 2e-3f);
}

// A pose file's frames: tick 2f is frame f, an odd tick halfway, the ends held.
void pose_frames_at() {
    std::string bytes(2 * 2 * kPoseRecord, '\0');
    auto put = [&](std::size_t frame, std::size_t j, std::int16_t w, std::int16_t x) {
        unsigned char* b =
            reinterpret_cast<unsigned char*>(bytes.data()) + (frame * 2 + j) * kPoseRecord;
        b[6] = static_cast<unsigned char>(w & 0xFF),
        b[7] = static_cast<unsigned char>((w >> 8) & 0xFF);
        b[8] = static_cast<unsigned char>(x & 0xFF),
        b[9] = static_cast<unsigned char>((x >> 8) & 0xFF);
    };
    put(0, 1, 32767, 0), put(1, 1, 32767, 2048), put(0, 0, 32767, 0), put(1, 0, 32767, 0);
    CHECK(pose_frames(bytes, 2) == 2 && pose_frames(bytes, 3) == 0);
    CHECK(std::fabs(pose_at(bytes, 2, 1, 1).t.x - 1.0f) < 1e-5f);
    CHECK(
        std::fabs(pose_at(bytes, 2, 1, 2).t.x - 2.0f) < 1e-5f
        && std::fabs(pose_at(bytes, 2, 1, 99).t.x - 2.0f) < 1e-5f
    );
    CHECK(
        std::fabs(pose_at(bytes, 2, 1, -5).t.x) < 1e-5f
        && std::fabs(pose_at(bytes, 2, 1, 0).q[3] - 1.0f) < 1e-5f
    );
}

// Times and lengths far out of a scene's range: held, so no sum of two overflows.
void extreme_ticks() {
    const Value v = parse(R"({"lo": -2147483648, "hi": 2147483647, "huge": 1e300})");
    CHECK(
        v["lo"].tick() == -Value::kMaxTick && v["hi"].tick() == Value::kMaxTick
        && v["huge"].tick() == Value::kMaxTick
    );
    CHECK(v["lo"].integer() == -2147483647 - 1);
    const Value keys = parse(
        R"([{"t": -2147483648, "position": [0, 0, 0]}, {"t": 2147483647, "position": [8, 0, 0]}])"
    );
    V3 pos, rot;
    float scale = 1;
    track_at(keys, 60, pos, rot, scale);
    CHECK(pos.x >= 0 && pos.x <= 8);
    const Value events =
        parse(R"([{"t": -2147483648, "do": "fade", "to": [1, 0, 0], "length": 2147483647},
        {"t": -2147483648, "do": "air", "background": [1, 0, 0], "length": 2147483647},
        {"t": -2147483648, "do": "slow", "stretch": 2, "length": 2147483647}])");
    CHECK(near(fade_at(events, 60).a, 1));  // both held to the same reach: over by tick 0
    CHECK(near(air_at(events, 60).weight, 1));
    CHECK(near(stretch_at(events, 60), 2));
    CHECK(near3(shake_at(parse(R"([{"t": 2147483647, "length": 2147483647}])"), 60), V3{}));
    CHECK(
        fov_at(parse(R"([{"t": -2147483648, "fov": 10}, {"t": 2147483647, "fov": 90}])"), 60) > 10
    );
    CHECK(
        look_at(parse(R"([{"t": -2147483648, "target": [1, 0, 0], "blend": 2147483647}])"), 60)
            .weight
        > 0.99f
    );
    CHECK(
        weight_at(parse(R"([{"t": -2147483648, "w": 0}, {"t": 2147483647, "w": 1}])"), 60) < 0.6f
    );
    CHECK(walk_state(parse(R"({"t": -2147483648, "points": [[0, 0, 0], [1, 0, 0]]})"), 60) == 2);
    CHECK(
        moods_at(parse(R"([{"t": -2147483648, "mood": "a", "blend": 2147483647}])"), 60).size() == 1
    );
    CHECK(
        path_at(parse(R"({"t": -2147483648, "points": [[0, 0, 0], [1, 0, 0]]})"), 60, pos, rot)
        && near(pos.x, 1)
    );
}

// A face's turns go on a scene pose after its own turns; joints the pose lacks are left out.
void face_on_a_pose() {
    std::vector<std::array<float, 4>> pose = {{0, 0, 0, 1}, {0, 0, 0.70710677f, 0.70710677f}};
    float jaw[4];
    axis_angle({1, 0, 0}, 20, jaw);
    face_on_pose(pose, {1, 7, -1}, {{jaw[0], jaw[1], jaw[2], jaw[3]}, {1, 0, 0, 0}, {1, 0, 0, 0}});
    float want[4];
    const float held[4] = {0, 0, 0.70710677f, 0.70710677f};
    quat_mul(held, jaw, want);

    for (int k = 0; k < 4; ++k) {
        CHECK(near(pose[1][k], want[k]));
    }
    CHECK(pose[0] == (std::array<float, 4>{0, 0, 0, 1}));
    CHECK(pose.size() == 2);
    // No talk: the jaw stays where the pose holds it.
    std::vector<std::array<float, 4>> still = {{0, 0, 0.70710677f, 0.70710677f}};
    face_on_pose(still, {}, {});
    CHECK(near(still[0][2], 0.70710677f));
}

}  // namespace

int main() {
    json_values();
    json_numbers();
    json_strings();
    json_broken();
    maths();
    face_on_a_pose();
    tracks();
    paths();
    visibility_and_anims();
    camera();
    camera_axes_and_shake();
    text();
    fades();
    volumes();
    picture();
    field_of_view();
    looking();
    layers();
    music();
    subtitle_pictures();
    walks_and_faces();
    rides_and_eyes();
    air_and_slow();
    feet();
    feet_in_port();
    pose_frames_at();
    extreme_ticks();
    std::printf("timeline: %d check(s), %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

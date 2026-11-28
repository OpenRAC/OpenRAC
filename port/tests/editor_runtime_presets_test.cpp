// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
// Tests of the behaviour presets' sums and how behaviours.json entries are read, without the port.
#include <cmath>
#include <cstdio>
#include <string>

#include "presets.hpp"

using namespace behaviour;

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

bool near(double a, double b, double eps = 1e-3) {
    return std::fabs(a - b) <= eps;
}

Behaviour read(const std::string& text) {
    std::string error;
    const Value v = Value::parse(text, error);
    CHECK(error.empty());
    const auto b = behaviour_of(v);
    CHECK(b.has_value());
    return b.value_or(Behaviour{});
}

bool refused(const std::string& text) {
    std::string error;
    return !behaviour_of(Value::parse(text, error)).has_value();
}

// Runs `b` from `s` for `seconds`, a tick at a time.
void run(const Behaviour& b, State& s, double seconds, const V3* hero = nullptr) {
    const double dt = 1.0 / kTicksPerSecond;
    const int ticks = static_cast<int>(std::lround(seconds * kTicksPerSecond));

    for (int i = 0; i <= ticks; ++i) {
        step(b, s, i * dt, dt, hero);
    }
}

void test_reading() {
    // Defaults by preset; numbers held to their ranges; a sequence out of range is its own.
    const Behaviour spin = read(R"({"spawn_id": 181, "preset": "spin"})");
    CHECK(
        spin.spawn_id == 181 && spin.preset == Preset::Spin && spin.speed == 90
        && spin.sequence == -1
    );
    CHECK(read(R"({"spawn_id": 1, "preset": "follow"})").distance == 3);
    CHECK(read(R"({"spawn_id": 1, "preset": "patrol"})").distance == 8);
    CHECK(read(R"({"spawn_id": 1, "preset": "patrol", "speed": -5})").speed == 0);
    CHECK(read(R"({"spawn_id": 1, "preset": "orbit", "speed": -30})").speed == -30);
    CHECK(read(R"({"spawn_id": 1, "preset": "bob", "period": 0})").period == 0.05);
    CHECK(read(R"({"spawn_id": 1, "preset": "bob", "sequence": 300})").sequence == -1);
    CHECK(read(R"({"spawn_id": 1, "preset": "bob", "sequence": 2})").sequence == 2);
    CHECK(refused(R"({"spawn_id": 1, "preset": "dance"})"));
    CHECK(refused(R"({"preset": "spin"})"));
    CHECK(refused(R"({"spawn_id": 65535, "preset": "spin"})"));
}

void test_angles() {
    CHECK(near(wrap(3 * kPi), kPi) || near(wrap(3 * kPi), -kPi));
    CHECK(near(wrap(-kPi / 2), -kPi / 2));
    // The short way round: from 170 to -170 degrees goes up through 180.
    CHECK(near(turn(radians(170), radians(-170), radians(5)), radians(175)));
    CHECK(near(turn(0, 1, 2), 1));
}

void test_spin_and_bob() {
    const Behaviour spin = read(R"({"spawn_id": 1, "preset": "spin", "speed": 90})");
    State s = start({1, 2, 3}, {0, 0, 0.5f});
    step(spin, s, 1.0, 1.0 / 60, nullptr);
    CHECK(near(s.rotation.z, 0.5 + kPi / 2));
    CHECK(near(s.position.x, 1) && near(s.position.z, 3));

    const Behaviour bob = read(R"({"spawn_id": 1, "preset": "bob", "height": 2, "period": 4})");
    s = start({0, 0, 10}, {});
    step(bob, s, 1.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.z, 12));
    step(bob, s, 3.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.z, 8));
    step(bob, s, 4.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.z, 10));
}

void test_patrol() {
    // 8 units at 4 a second along +y (a yaw of 90 degrees): out in 2 s, back in 2 more.
    const Behaviour b = read(R"({"spawn_id": 1, "preset": "patrol", "distance": 8, "speed": 4})");
    State s = start({0, 0, 0}, {0, 0, static_cast<float>(kPi / 2)});
    step(b, s, 1.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.x, 0) && near(s.position.y, 4) && near(s.rotation.z, kPi / 2));
    step(b, s, 3.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.y, 4) && near(s.rotation.z, -kPi / 2));
    step(b, s, 4.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.y, 0));
}

void test_orbit() {
    // Radius 4 at 90 degrees a second, starting at the origin facing +x: the centre is at (0, 4),
    // and after a second it is at (4, 4), facing +y.
    const Behaviour b = read(R"({"spawn_id": 1, "preset": "orbit", "radius": 4, "speed": 90})");
    State s = start({0, 0, 1}, {});
    step(b, s, 0.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.x, 0) && near(s.position.y, 0) && near(s.rotation.z, 0));
    step(b, s, 1.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.x, 4) && near(s.position.y, 4) && near(s.rotation.z, kPi / 2));
    CHECK(near(s.position.z, 1));
    // The other way: the centre is on its right, at (0, -4).
    const Behaviour back = read(R"({"spawn_id": 1, "preset": "orbit", "radius": 4, "speed": -90})");
    s = start({0, 0, 0}, {});
    step(back, s, 1.0, 1.0 / 60, nullptr);
    CHECK(near(s.position.x, 4) && near(s.position.y, -4) && near(s.rotation.z, -kPi / 2));
}

void test_hero() {
    // Turns at 90 degrees a second towards the hero, who is behind it: half a turn takes 2 s.
    const Behaviour face = read(R"({"spawn_id": 1, "preset": "face_hero", "speed": 90})");
    const V3 behind{-10, 0, 0};
    State s = start({0, 0, 0}, {});
    run(face, s, 1.0, &behind);
    CHECK(std::fabs(s.rotation.z) > 1.4 && std::fabs(s.rotation.z) < 1.7);
    run(face, s, 1.5, &behind);
    CHECK(near(std::fabs(s.rotation.z), kPi));
    State held = start({0, 0, 0}, {});
    run(face, held, 1.0, nullptr);
    CHECK(held.rotation.z == 0);

    // Follows to 3 units from the hero at 4 a second, and no nearer; not when the hero is beyond
    // its range.
    const Behaviour follow =
        read(R"({"spawn_id": 1, "preset": "follow", "speed": 4, "distance": 3, "range": 30})");
    const V3 hero{10, 0, 5};
    s = start({0, 0, 0}, {});
    run(follow, s, 1.0, &hero);
    CHECK(s.position.x > 3.9 && s.position.x < 4.2 && near(s.position.z, 0));
    run(follow, s, 5.0, &hero);
    CHECK(near(s.position.x, 7, 0.01));
    const V3 far{100, 0, 0};
    s = start({0, 0, 0}, {});
    run(follow, s, 1.0, &far);
    CHECK(s.position.x == 0);
}

void test_wander() {
    // Stays within its radius of where it started, stands still between walks, and wanders the
    // same way for the same seed.
    const Behaviour b =
        read(R"({"spawn_id": 1, "preset": "wander", "radius": 5, "speed": 3, "pause": 1})");
    CHECK(near(b.radius, 5) && near(b.speed, 3) && near(b.pause, 1));
    const Behaviour plain = read(R"({"spawn_id": 1, "preset": "wander"})");
    CHECK(near(plain.radius, 8) && near(plain.speed, 3) && near(plain.pause, 2));
    State s = start({10, 20, 3}, {}, 7);
    State again = start({10, 20, 3}, {}, 7);
    const double dt = 1.0 / 60;
    double farthest = 0;
    int still = 0, moving = 0;

    for (int i = 0; i < 60 * 30; ++i) {
        const V3 was = s.position;
        step(b, s, i * dt, dt, nullptr);
        step(b, again, i * dt, dt, nullptr);
        const double moved = std::hypot(s.position.x - was.x, s.position.y - was.y);
        (moved > 1e-4 ? moving : still)++;
        CHECK(moved <= 3 * dt + 1e-4);
        farthest = std::fmax(farthest, std::hypot(s.position.x - 10, s.position.y - 20));
        CHECK(near(s.position.z, 3));
    }

    CHECK(farthest <= 5.001 && farthest > 1);
    CHECK(still > 60 && moving > 60);
    CHECK(s.position.x == again.position.x && s.position.y == again.position.y);
    State other = start({10, 20, 3}, {}, 8);

    for (int i = 0; i < 60 * 5; ++i) {
        step(b, other, i * dt, dt, nullptr);
    }

    CHECK(other.position.x != s.position.x || other.position.y != s.position.y);
}

void test_sequences() {
    const Behaviour b =
        read(R"({"spawn_id": 1, "preset": "patrol", "sequence": 2, "walk_sequence": 5})");
    CHECK(sequence_for(b, false) == 2 && sequence_for(b, true) == 5);
    const Behaviour one = read(R"({"spawn_id": 1, "preset": "patrol", "sequence": 2})");
    CHECK(sequence_for(one, true) == 2);
    CHECK(read(R"({"spawn_id": 1, "preset": "patrol", "walk_sequence": 300})").walk_sequence == -1);
}

}  // namespace

int main() {
    test_reading();
    test_angles();
    test_spin_and_bob();
    test_patrol();
    test_orbit();
    test_hero();
    test_wander();
    test_sequences();
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

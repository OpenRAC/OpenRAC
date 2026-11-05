// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output's arithmetic and choices (common/hdr.h): PQ against ST 2084's published points, the
// game's white at paper white whatever the output, the levels the settings and the display give,
// and the output each mode asks for (Off is SDR, whatever the display says).
#include <cmath>

#include "common/hdr.h"
#include "tests/check.h"

namespace {

namespace hdr = openrac::hdr;
using hdr::Output;

bool near(float a, float b, float tolerance) {
    return std::fabs(a - b) <= tolerance;
}

// ST 2084's values: 0 and 10000 nits at the ends, 100 nits at 0.5081, 203 at 0.5806, 1000 at 0.7518.
void test_pq_points() {
    CHECK(near(hdr::pq_encode(0.0f), 7.3e-7f, 1e-5f));
    CHECK(near(hdr::pq_encode(10000.0f), 1.0f, 1e-6f));
    CHECK(near(hdr::pq_encode(100.0f), 0.5081f, 2e-4f));
    CHECK(near(hdr::pq_encode(203.0f), 0.5806f, 2e-4f));
    CHECK(near(hdr::pq_encode(1000.0f), 0.7518f, 2e-4f));
    // Beyond the range: clamped.
    CHECK(hdr::pq_encode(20000.0f) == hdr::pq_encode(10000.0f));
    CHECK(hdr::pq_encode(-5.0f) == hdr::pq_encode(0.0f));
}

void test_pq_round_trip_and_order() {
    float last = -1.0f;
    for (float nits = 0.0f; nits <= 10000.0f; nits += 37.0f) {
        const float signal = hdr::pq_encode(nits);
        CHECK(signal > last);
        last = signal;
        CHECK(near(hdr::pq_decode(signal), nits, std::max(0.05f, nits * 2e-4f)));
    }
}

// The game's white is paper white in every output, and black is black.
void test_white_at_paper_white() {
    const hdr::Levels levels{250.0f, 1000.0f};
    const hdr::Rgb white{1.0f, 1.0f, 1.0f};
    const hdr::Rgb pq = hdr::encode(white, Output::Hdr10, levels);
    for (float c : pq) {
        CHECK(near(hdr::pq_decode(c), 250.0f, 0.1f));
    }
    const hdr::Rgb sc = hdr::encode(white, Output::ScRgb, levels);
    for (float c : sc) {
        CHECK(near(c * hdr::kScRgbUnitNits, 250.0f, 1e-3f));
    }
    const hdr::Rgb black = hdr::encode({0.0f, 0.0f, 0.0f}, Output::ScRgb, levels);
    CHECK(black[0] == 0.0f && black[1] == 0.0f && black[2] == 0.0f);
}

// SDR passes the bytes through unchanged.
void test_sdr_is_untouched() {
    const hdr::Rgb rgb{0.25f, 0.5f, 0.75f};
    CHECK(hdr::encode(rgb, Output::Sdr, {}) == rgb);
}

// Mid grey through the 2.2 gamma, and a saturated BT.709 red stays inside BT.2020 (all positive).
void test_gamma_and_gamut() {
    const hdr::Levels levels{100.0f, 1000.0f};
    const hdr::Rgb grey = hdr::encode({0.5f, 0.5f, 0.5f}, Output::ScRgb, levels);
    CHECK(near(grey[0] * hdr::kScRgbUnitNits, 100.0f * std::pow(0.5f, 2.2f), 1e-3f));
    const hdr::Rgb red = hdr::encode({1.0f, 0.0f, 0.0f}, Output::Hdr10, levels);
    CHECK(near(hdr::pq_decode(red[0]), 62.74f, 0.05f));
    CHECK(near(hdr::pq_decode(red[1]), 6.91f, 0.05f));
    CHECK(near(hdr::pq_decode(red[2]), 1.64f, 0.05f));
}

// Light above white rolls off towards the peak: the SDR range untouched, the slope 1 at white,
// never past the peak, the hue kept; and a frame value above 1 goes on along the 2.2 curve.
void test_roll_off() {
    const hdr::Levels levels{200.0f, 1000.0f};
    const hdr::Rgb sdr{1.0f, 0.5f, 0.25f};
    CHECK(hdr::roll_off_above_white(sdr, levels) == sdr);
    const hdr::Rgb just = hdr::roll_off_above_white({1.01f, 0.0f, 0.0f}, levels);
    CHECK(near(just[0], 1.01f, 1e-4f));
    const hdr::Rgb far = hdr::roll_off_above_white({1000.0f, 500.0f, 0.0f}, levels);
    CHECK(near(far[0], 5.0f, 1e-3f) && near(far[1] / far[0], 0.5f, 1e-5f));
    float last = 1.0f;
    for (float m = 1.0f; m < 50.0f; m += 0.25f) {
        const float out = hdr::roll_off_above_white({m, m, m}, levels)[0];
        CHECK(out >= last && out <= 5.0f);
        last = out;
    }
    // No headroom: above white is held at white.
    const hdr::Rgb flat = hdr::roll_off_above_white({2.0f, 1.0f, 0.5f}, {400.0f, 400.0f});
    CHECK(near(flat[0], 1.0f, 1e-6f) && near(flat[1], 0.5f, 1e-6f));
    // The frame's 1.5 is 1.5^2.2 of white before the roll-off; below 0 is black.
    CHECK(near(hdr::sdr_to_linear(1.5f), std::pow(1.5f, 2.2f), 1e-5f));
    CHECK(hdr::sdr_to_linear(-0.25f) == 0.0f);
    const hdr::Rgb bright = hdr::encode({1.5f, 1.5f, 1.5f}, Output::ScRgb, levels);
    const float nits = bright[0] * hdr::kScRgbUnitNits;
    CHECK(nits > 200.0f && nits < 1000.0f);
}

void test_levels() {
    const hdr::Display unknown;
    hdr::Levels l = hdr::resolve_levels(0, 0, unknown);
    CHECK(l.paper_white == hdr::kReferenceWhiteNits && l.peak == hdr::kDefaultPeakNits);
    // The system's SDR white and peak where the display says them.
    const hdr::Display windows{true, true, 240.0f, 800.0f};
    l = hdr::resolve_levels(0, 0, windows);
    CHECK(l.paper_white == 240.0f && l.peak == 800.0f);
    // The player's own numbers, clamped to the settings' ranges.
    l = hdr::resolve_levels(5000, 50000, windows);
    CHECK(l.paper_white == hdr::kPaperWhiteMax && l.peak == hdr::kPeakMax);
    l = hdr::resolve_levels(10, 10, windows);
    CHECK(l.paper_white == hdr::kPaperWhiteMin && l.peak == hdr::kPeakMin);
    // The brightest screens on sale: a display's 10000 nits and a bright paper white stay as they are.
    const hdr::Display bright{true, true, 0.0f, 10000.0f};
    l = hdr::resolve_levels(1000, 0, bright);
    CHECK(l.paper_white == 1000.0f && l.peak == 10000.0f);
    l = hdr::resolve_levels(0, 6000, bright);
    CHECK(l.peak == 6000.0f);
    // A peak below paper white is raised to it.
    l = hdr::resolve_levels(600, 400, windows);
    CHECK(l.peak == 600.0f);
}

// A display whose HDR is relative to the system's white (Android's headroom, macOS's EDR): white
// is scRGB's 1.0 whatever the player's paper white, the peak its headroom (or less, if asked), and
// the output linear extended sRGB.
void test_relative_display() {
    hdr::Display android;
    android.known = true;
    android.hdr_enabled = true;
    android.relative = true;
    android.sdr_white_nits = hdr::kScRgbUnitNits;
    android.peak_nits = hdr::kScRgbUnitNits * 3.0f;
    hdr::Levels l = hdr::resolve_levels(300, 0, android);
    CHECK(l.paper_white == hdr::kScRgbUnitNits && near(l.peak, 240.0f, 1e-3f));
    l = hdr::resolve_levels(0, 400, android);  // asks for more than the headroom: the headroom
    CHECK(near(l.peak, 240.0f, 1e-3f));
    android.peak_nits = 2000.0f;
    l = hdr::resolve_levels(0, 400, android);  // asks for less: less
    CHECK(near(l.peak, 400.0f, 1e-3f));
    CHECK(hdr::wanted_output(hdr::Mode::On, android, false) == Output::ScRgb);
    // White comes out at scRGB's 1.0.
    l = hdr::resolve_levels(0, 0, android);
    CHECK(near(hdr::encode({1.0f, 1.0f, 1.0f}, Output::ScRgb, l)[0], 1.0f, 1e-5f));
}

void test_wanted_output() {
    const hdr::Display unknown;
    const hdr::Display sdr_display{true, false, 0.0f, 0.0f};
    const hdr::Display hdr_display{true, true, 203.0f, 1000.0f};
    CHECK(hdr::wanted_output(hdr::Mode::Off, hdr_display, false) == Output::Sdr);
    CHECK(hdr::wanted_output(hdr::Mode::Off, unknown, false) == Output::Sdr);
    // On follows the system where it says; where it cannot (X11), it tries HDR.
    CHECK(hdr::wanted_output(hdr::Mode::On, sdr_display, false) == Output::Sdr);
    CHECK(hdr::wanted_output(hdr::Mode::On, hdr_display, false) == Output::Hdr10);
    CHECK(hdr::wanted_output(hdr::Mode::On, hdr_display, true) == Output::ScRgb);
    CHECK(hdr::wanted_output(hdr::Mode::On, unknown, false) == Output::Hdr10);
}

}  // namespace

int main() {
    test_pq_points();
    test_pq_round_trip_and_order();
    test_white_at_paper_white();
    test_sdr_is_untouched();
    test_gamma_and_gamut();
    test_roll_off();
    test_levels();
    test_relative_display();
    test_wanted_output();
    return openrac::test::result();
}

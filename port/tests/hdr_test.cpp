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
    const hdr::Levels levels{250.0f, 1000.0f, {}};
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
    const hdr::Levels levels{100.0f, 1000.0f, {}};
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
    const hdr::Levels levels{200.0f, 1000.0f, {}};
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
    const hdr::Rgb flat = hdr::roll_off_above_white({2.0f, 1.0f, 0.5f}, {400.0f, 400.0f, {}});
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
    // A display that says it shows no HDR (Mode::On sends HDR anyway): the defaults, not its
    // SDR white and its peak of 1x headroom.
    const hdr::Display says_sdr{true, false, 80.0f, 203.0f};
    l = hdr::resolve_levels(0, 0, says_sdr);
    CHECK(l.paper_white == hdr::kReferenceWhiteNits && l.peak == hdr::kDefaultPeakNits);
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
    // On tries HDR whatever the system says (a compositor can be wrong), except where the
    // system decides (Android).
    CHECK(hdr::wanted_output(hdr::Mode::On, sdr_display, false) == Output::Hdr10);
    hdr::Display android_sdr = sdr_display;
    android_sdr.system_decides = true;
    CHECK(hdr::wanted_output(hdr::Mode::On, android_sdr, false) == Output::Sdr);
    hdr::Display android_hdr = hdr_display;
    android_hdr.system_decides = true;
    CHECK(hdr::wanted_output(hdr::Mode::On, android_hdr, false) == Output::Hdr10);
    CHECK(hdr::wanted_output(hdr::Mode::On, hdr_display, false) == Output::Hdr10);
    CHECK(hdr::wanted_output(hdr::Mode::On, hdr_display, true) == Output::ScRgb);
    CHECK(hdr::wanted_output(hdr::Mode::On, unknown, false) == Output::Hdr10);
}

// OKLab hue in degrees (0 red, about 110 yellow) and chroma of linear light.
float hue_of(const hdr::Rgb& linear) {
    const hdr::Rgb lab = hdr::linear_to_oklab(linear);
    return std::atan2(lab[2], lab[1]) * 57.29578f;
}

float chroma_of(const hdr::Rgb& linear) {
    const hdr::Rgb lab = hdr::linear_to_oklab(linear);
    return std::hypot(lab[1], lab[2]);
}

// A red-orange fire sprite added up three times: SDR stops each channel at white, so it shows
// yellow-orange; HDR keeps the mix, so it shows red. SDR hue takes the SDR hue back and keeps
// HDR's lightness; SDR chroma takes its saturation too; blowout and highlights as named.
// A flame added up more and more: SDR shows it red-orange, then yellow, then white. SDR hue
// follows it without a jump where SDR turns white, and stays yellow there instead of falling back
// to HDR's red (a red core in a yellow flame).
void test_sdr_hue_into_white() {
    hdr::Levels levels{203.0f, 1000.0f, {}};
    const float plain = hue_of(hdr::to_light({0.9f, 0.45f, 0.2f}, levels));
    levels.look.sdr_hue = 1.0f;
    float last = plain;
    for (float k = 1.0f; k <= 8.0f; k += 0.01f) {
        const float hue = hue_of(hdr::to_light({0.9f * k, 0.45f * k, 0.2f * k}, levels));
        CHECK(std::abs(hue - last) < 3.0f);
        last = hue;
    }
    CHECK(last > plain + 40.0f);  // all channels past white: still SDR's yellow
}

void test_look_on_fire() {
    const hdr::Rgb fire{3.0f, 1.35f, 0.3f};
    const hdr::Rgb sdr{1.0f, 1.0f, std::pow(0.3f, 2.2f)};
    hdr::Levels levels{203.0f, 1000.0f, {}};
    const hdr::Rgb plain = hdr::to_light(fire, levels);
    CHECK(hue_of(plain) < 60.0f && hue_of(sdr) > 100.0f);

    levels.look.sdr_hue = 1.0f;
    const hdr::Rgb hue = hdr::to_light(fire, levels);
    CHECK(near(hue_of(hue), hue_of(sdr), 2.0f));
    CHECK(hue[2] >= 0.0f);  // turned back inside the gamut
    CHECK(near(hdr::linear_to_oklab(hue)[0], hdr::linear_to_oklab(plain)[0], 0.02f));
    // Halfway: between the two.
    levels.look.sdr_hue = 0.5f;
    const float half = hue_of(hdr::to_light(fire, levels));
    CHECK(half > hue_of(plain) + 10.0f && half < hue_of(sdr) - 10.0f);

    levels.look.sdr_hue = 1.0f;
    levels.look.sdr_chroma = 1.0f;
    const hdr::Rgb both = hdr::to_light(fire, levels);
    CHECK(near(chroma_of(both), chroma_of(sdr), 0.01f));
    CHECK(near(hue_of(both), hue_of(sdr), 2.0f));

    levels.look = {};
    levels.look.blowout = 1.0f;
    CHECK(chroma_of(hdr::to_light(fire, levels)) < chroma_of(plain) * 0.8f);

    // Highlights 0: the pixel as SDR shows it.
    levels.look = {};
    levels.look.highlights = 0.0f;
    const hdr::Rgb flat = hdr::to_light(fire, levels);
    for (int c = 0; c < 3; ++c) {
        CHECK(near(flat[c], sdr[c], 1e-3f));
    }
}

// Whatever the look: the SDR range is never changed, nothing goes below 0 or past the peak, grey
// stays grey, and the defaults are exactly the plain roll-off.
void test_look_bounds() {
    const hdr::Look looks[] = {
        {},
        {1.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
        {0.5f, 0.5f, 0.5f, 0.5f},
        {0.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 0.0f, 1.0f},
    };
    const float values[] = {0.0f, 0.2f, 0.7f, 1.0f, 1.3f, 2.0f, 4.0f, 9.0f};
    for (const float peak : {203.0f, 1000.0f, 10000.0f}) {
        for (const hdr::Look& look : looks) {
            hdr::Levels levels{203.0f, peak, look};
            const float top = 1.0f + hdr::headroom(levels);
            for (const float r : values) {
                for (const float g : values) {
                    for (const float b : values) {
                        const hdr::Rgb rgb{r, g, b};
                        const hdr::Rgb out = hdr::to_light(rgb, levels);
                        const hdr::Rgb linear{
                            hdr::sdr_to_linear(r), hdr::sdr_to_linear(g), hdr::sdr_to_linear(b)
                        };
                        if (r <= 1.0f && g <= 1.0f && b <= 1.0f) {
                            CHECK(out == linear);
                            continue;
                        }
                        if (look.plain()) {
                            CHECK(out == hdr::roll_off_above_white(linear, levels));
                        }
                        for (int c = 0; c < 3; ++c) {
                            CHECK(std::isfinite(out[c]) && out[c] >= 0.0f);
                            CHECK(out[c] <= top * 1.0001f);
                        }
                        if (r == g && g == b) {
                            CHECK(near(out[0], out[1], 1e-3f * out[0] + 1e-6f));
                            CHECK(near(out[1], out[2], 1e-3f * out[1] + 1e-6f));
                        }
                    }
                }
            }
        }
    }
    // The settings' percents, clamped.
    const hdr::Look l = hdr::look_from_percent(150, -5, 50, 100);
    CHECK(l.highlights == 1.0f && l.sdr_hue == 0.0f && l.sdr_chroma == 0.5f && l.blowout == 1.0f);
    CHECK(hdr::look_from_percent(100, 0, 0, 0).plain());
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
    test_look_on_fire();
    test_sdr_hue_into_white();
    test_look_bounds();
    return openrac::test::result();
}

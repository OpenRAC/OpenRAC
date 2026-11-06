// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// HDR output: the game's SDR picture shown on an HDR screen at the player's paper white, so that it
// looks as it does in SDR (docs/port/HDR.md). This file is the arithmetic and the choices only, with
// no GPU or SDL in it, so that tests/hdr_test.cpp can pin them; the Vulkan backend's encode pass
// (renderer/vulkan/shaders/hdr_encode.comp) does the same per pixel and is checked against it.
//
// With HDR off nothing here runs and the picture goes to the screen as it always did.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace openrac::hdr {

// The player's setting (`hdr` in the settings file).
enum class Mode {
    Off,  // SDR, as always (the default)
    On,   // HDR when the system says the window's display shows HDR; where it cannot say (X11),
          // whenever the screen offers it
    Count,
};

// What the screen is sent.
enum class Output {
    Sdr,    // the picture's bytes as they are
    Hdr10,  // PQ (SMPTE ST 2084) in BT.2020 primaries, 10 bits a channel
    ScRgb,  // linear BT.709 in half floats, 1.0 = 80 nits (Windows' scRGB, macOS's EDR)
    Count,
};

// What the system says of the window's display (SDL's window properties; Android's activity).
// `known` is false where nothing can tell (X11): Mode::On then tries HDR all the same.
struct Display {
    bool known = false;
    bool hdr_enabled = false;
    float sdr_white_nits = 0.0f;  // the system's SDR content brightness; 0 when unknown
    float peak_nits = 0.0f;       // the brightest the display shows; 0 when unknown
    // HDR is relative to the system's SDR white (Android's HDR/SDR headroom; macOS's EDR): white
    // is scRGB's 1.0 (sdr_white_nits 80) and follows the system's brightness, so the player's
    // paper white does not apply. Linear extended sRGB is the format then.
    bool relative = false;
    // For the settings' status line: the most headroom the display gives over SDR white (0
    // unknown).
    float headroom = 0.0f;
};

// Nits of the SDR white an HDR picture of SDR content is usually given (ITU-R BT.2408), and of
// scRGB's 1.0.
inline constexpr float kReferenceWhiteNits = 203.0f;
inline constexpr float kScRgbUnitNits = 80.0f;
// A peak brightness for the metadata where the display does not say.
inline constexpr float kDefaultPeakNits = 1000.0f;
// PQ's range.
inline constexpr float kPqMaxNits = 10000.0f;
// The settings' ranges (0 in the file: the system's, or the default). The peak goes to PQ's top:
// the brightest TVs and phones of 2026 are sold (and some measured) at 10000 nits in a small window.
// Paper white is the whole picture (BT.2408: 203); above 1000 it reaches the peak and the highlights
// stop standing out.
inline constexpr int kPaperWhiteMin = 80;
inline constexpr int kPaperWhiteMax = 1000;
inline constexpr int kPeakMin = 400;
inline constexpr int kPeakMax = static_cast<int>(kPqMaxNits);
// The picture's bytes are display-referred: a monitor shows them with a 2.2 gamma (an sRGB
// monitor's real response; the piecewise sRGB curve would lift the game's dark scenes).
inline constexpr float kSdrGamma = 2.2f;

// The HDR picture's brightness: the game's white at `paper_white` nits; what the frame holds above
// white (blending's sums in the HDR frame, vkgl.h hdr_frame) rolls off towards `peak`, which also
// goes into the screen's metadata.
struct Levels {
    float paper_white = kReferenceWhiteNits;
    float peak = kDefaultPeakNits;
};

// How far above paper white the peak is, in paper whites (peak / paper white - 1, at least 0).
inline float headroom(const Levels& levels) {
    return std::max(levels.peak / levels.paper_white - 1.0f, 0.0f);
}

// The levels for the player's settings (`paper_white`, `peak`: nits, 0 for the system's) on
// `display`. The system's SDR white is what the player set for SDR content in the system's own
// settings (Windows' "SDR content brightness"), so the game looks like the desktop around it.
inline Levels resolve_levels(int paper_white, int peak, const Display& display) {
    Levels levels;
    if (display.relative && display.sdr_white_nits > 0.0f) {
        // White is the system's; the player's peak may only ask for less headroom.
        levels.paper_white = display.sdr_white_nits;
        levels.peak = display.peak_nits > 0.0f ? display.peak_nits : levels.paper_white;
        if (peak > 0) {
            levels.peak =
                std::min(levels.peak, static_cast<float>(std::clamp(peak, kPeakMin, kPeakMax)));
        }
        levels.peak = std::max(levels.peak, levels.paper_white);
        return levels;
    }
    if (paper_white > 0) {
        levels.paper_white =
            static_cast<float>(std::clamp(paper_white, kPaperWhiteMin, kPaperWhiteMax));
    } else if (display.sdr_white_nits > 0.0f) {
        levels.paper_white = std::clamp(
            display.sdr_white_nits,
            static_cast<float>(kPaperWhiteMin),
            static_cast<float>(kPaperWhiteMax)
        );
    }
    if (peak > 0) {
        levels.peak = static_cast<float>(std::clamp(peak, kPeakMin, kPeakMax));
    } else if (display.peak_nits > 0.0f) {
        levels.peak = std::min(display.peak_nits, kPqMaxNits);
    }
    // A peak below paper white would tell the screen the picture is dimmer than it is.
    levels.peak = std::max(levels.peak, levels.paper_white);
    return levels;
}

// The output the player's mode asks for on `display`: SDR, or the HDR kind this platform prefers
// (`prefer_scrgb`: macOS, whose EDR is linear). Whether the screen offers it is the swapchain's
// to find out (renderer/vulkan/surface_format.h), which falls back to the other kind or to SDR.
inline Output wanted_output(Mode mode, const Display& display, bool prefer_scrgb) {
    const bool hdr = mode == Mode::On && (!display.known || display.hdr_enabled);
    if (!hdr) {
        return Output::Sdr;
    }
    return prefer_scrgb || display.relative ? Output::ScRgb : Output::Hdr10;
}

// ---- The arithmetic, as the encode pass does it per pixel ----

// SMPTE ST 2084's inverse EOTF: `nits` (0..10000) to the PQ signal (0..1).
inline float pq_encode(float nits) {
    constexpr float m1 = 2610.0f / 16384.0f;
    constexpr float m2 = 2523.0f / 4096.0f * 128.0f;
    constexpr float c1 = 3424.0f / 4096.0f;
    constexpr float c2 = 2413.0f / 4096.0f * 32.0f;
    constexpr float c3 = 2392.0f / 4096.0f * 32.0f;
    const float y = std::pow(std::clamp(nits / kPqMaxNits, 0.0f, 1.0f), m1);
    return std::pow((c1 + c2 * y) / (1.0f + c3 * y), m2);
}

// ST 2084's EOTF: the PQ signal to nits.
inline float pq_decode(float signal) {
    constexpr float m1 = 2610.0f / 16384.0f;
    constexpr float m2 = 2523.0f / 4096.0f * 128.0f;
    constexpr float c1 = 3424.0f / 4096.0f;
    constexpr float c2 = 2413.0f / 4096.0f * 32.0f;
    constexpr float c3 = 2392.0f / 4096.0f * 32.0f;
    const float e = std::pow(std::clamp(signal, 0.0f, 1.0f), 1.0f / m2);
    return kPqMaxNits * std::pow(std::max(e - c1, 0.0f) / (c2 - c3 * e), 1.0f / m1);
}

using Rgb = std::array<float, 3>;

// Linear BT.709 to linear BT.2020, both D65 (ITU-R BT.2087). Rows sum to 1, so white stays white.
inline constexpr std::array<Rgb, 3> kBt709ToBt2020 = {{
    {0.627403896f, 0.329283038f, 0.043313066f},
    {0.069097289f, 0.919540395f, 0.011362316f},
    {0.016391439f, 0.088013308f, 0.895595253f},
}};

// A channel of the picture (0..1 as in SDR; above 1 where the HDR frame's blending added up past
// white) to linear light, 1.0 being the game's white. The 2.2 curve goes on above 1, as RenoDX's
// gamma correction does; below 0 (a subtraction in a float frame) is black.
inline float sdr_to_linear(float value) {
    return std::pow(std::max(value, 0.0f), kSdrGamma);
}

// Light above the game's white (linear, 1.0 = paper white) rolled off towards the peak: a pixel
// whose brightest channel is at or below 1 is left as it is (the SDR picture is unchanged);
// above, that channel goes along 1 + h (1 - e^(-(m - 1) / h)), h being the headroom over paper
// white, which leaves 1 with a slope of 1 and never passes the peak. All three channels are
// scaled alike, so the colour's hue stays.
inline Rgb roll_off_above_white(const Rgb& linear, const Levels& levels) {
    const float m = std::max({linear[0], linear[1], linear[2]});
    if (m <= 1.0f) {
        return linear;
    }
    const float h = headroom(levels);
    const float to = h > 0.0f ? 1.0f + h * (1.0f - std::exp(-(m - 1.0f) / h)) : 1.0f;
    const float scale = to / m;
    return {linear[0] * scale, linear[1] * scale, linear[2] * scale};
}

// The value the screen is sent for one pixel of the picture (`rgb`, 0..1 as stored): PQ signals
// for Output::Hdr10, scRGB values for Output::ScRgb, the input itself for Output::Sdr.
inline Rgb encode(const Rgb& rgb, Output output, const Levels& levels) {
    if (output == Output::Sdr) {
        return rgb;
    }
    const Rgb linear = roll_off_above_white(
        {sdr_to_linear(rgb[0]), sdr_to_linear(rgb[1]), sdr_to_linear(rgb[2])}, levels
    );
    Rgb out{};
    if (output == Output::ScRgb) {
        const float scale = levels.paper_white / kScRgbUnitNits;
        for (int c = 0; c < 3; ++c) {
            out[c] = linear[c] * scale;
        }
        return out;
    }
    for (int c = 0; c < 3; ++c) {
        const Rgb& row = kBt709ToBt2020[c];
        const float wide = row[0] * linear[0] + row[1] * linear[1] + row[2] * linear[2];
        out[c] = pq_encode(wide * levels.paper_white);
    }
    return out;
}

// The settings file's and the menus' names.
inline constexpr const char* kModeKeys[] = {"off", "on"};
inline constexpr const char* kModeNames[] = {"Off", "On"};
inline constexpr const char* kOutputNames[] = {"SDR", "HDR10", "scRGB"};

inline const char* name(Output output) {
    return kOutputNames[std::clamp(static_cast<int>(output), 0, 2)];
}

}  // namespace openrac::hdr

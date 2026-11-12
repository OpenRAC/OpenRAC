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
// The look of light above white (the HDR section's sliders, 0..1 each). In SDR each channel stops
// at white on its own, so bright mixes of colour turn towards yellow and white (a red-orange fire
// looks orange there); in HDR they keep their mix. These take back as much of the SDR look as the
// player wants. Pixels at or below white are never changed. The defaults are the plain HDR look.
struct Look {
    float highlights = 1.0f;  // how much light above white there is: 0 as in SDR, 1 all of it
    float sdr_hue = 0.0f;     // the hue the SDR picture has there
    float sdr_chroma = 0.0f;  // the saturation the SDR picture has there
    float blowout = 0.0f;     // light far above white fades towards white

    bool operator==(const Look&) const = default;

    // True when only the defaults' arithmetic runs (the encode pass skips the rest).
    bool plain() const {
        return highlights >= 1.0f && sdr_hue <= 0.0f && sdr_chroma <= 0.0f && blowout <= 0.0f;
    }
};

// The settings' sliders (percent) as a Look; each clamped to 0..100.
inline Look look_from_percent(int highlights, int sdr_hue, int sdr_chroma, int blowout) {
    const auto unit = [](int percent) {
        return static_cast<float>(std::clamp(percent, 0, 100)) / 100.0f;
    };
    return {unit(highlights), unit(sdr_hue), unit(sdr_chroma), unit(blowout)};
}

struct Levels {
    float paper_white = kReferenceWhiteNits;
    float peak = kDefaultPeakNits;
    Look look;

    bool operator==(const Levels&) const = default;
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

// OKLab (Björn Ottosson's, from linear BT.709): L is lightness, (a, b) the hue's direction and
// its length the chroma. Used to take the SDR picture's hue and chroma over.
inline Rgb linear_to_oklab(const Rgb& c) {
    const float l = std::cbrt(0.4122214708f * c[0] + 0.5363325363f * c[1] + 0.0514459929f * c[2]);
    const float m = std::cbrt(0.2119034982f * c[0] + 0.6806995451f * c[1] + 0.1073969566f * c[2]);
    const float s = std::cbrt(0.0883024619f * c[0] + 0.2817188376f * c[1] + 0.6299787005f * c[2]);
    return {
        0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
        1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
        0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s,
    };
}

inline Rgb oklab_to_linear(const Rgb& lab) {
    const float l = lab[0] + 0.3963377774f * lab[1] + 0.2158037573f * lab[2];
    const float m = lab[0] - 0.1055613458f * lab[1] - 0.0638541728f * lab[2];
    const float s = lab[0] - 0.0894841775f * lab[1] - 1.2914855480f * lab[2];
    const float l3 = l * l * l;
    const float m3 = m * m * m;
    const float s3 = s * s * s;
    return {
        4.0767416621f * l3 - 3.3077115913f * m3 + 0.2309699292f * s3,
        -1.2684380046f * l3 + 2.6097574011f * m3 - 0.3413193965f * s3,
        -0.0041960863f * l3 - 0.7034186147f * m3 + 1.7076127119f * s3,
    };
}

// Below this an OKLab chroma counts as grey (no hue to take or keep).
inline constexpr float kGreyChroma = 1e-4f;

// Light above white (`hdr`, rolled off) given the look's SDR hue, SDR chroma and blowout. `sdr` is
// the same pixel as SDR shows it (each channel stopped at white). Lightness stays the HDR one; the
// colour is then kept inside BT.2020 (a turned hue can leave it) and under the peak.
inline Rgb apply_look(const Rgb& hdr, const Rgb& sdr, const Levels& levels) {
    const Look& look = levels.look;
    const Rgb lab = linear_to_oklab(hdr);
    const Rgb ref = linear_to_oklab(sdr);
    const float chroma = std::hypot(lab[1], lab[2]);
    const float ref_chroma = std::hypot(ref[1], ref[2]);
    // The hue's direction: the HDR one turned towards the SDR one.
    float da = 0.0f;
    float db = 0.0f;
    if (chroma > kGreyChroma) {
        da = lab[1] / chroma;
        db = lab[2] / chroma;
    }
    if (ref_chroma > kGreyChroma) {
        const float ra = ref[1] / ref_chroma;
        const float rb = ref[2] / ref_chroma;
        if (chroma <= kGreyChroma) {
            da = ra;
            db = rb;
        } else if (look.sdr_hue > 0.0f) {
            const float ta = da + (ra - da) * look.sdr_hue;
            const float tb = db + (rb - db) * look.sdr_hue;
            const float length = std::hypot(ta, tb);
            // Opposite hues halfway have no direction: the HDR one stays.
            if (length > kGreyChroma) {
                da = ta / length;
                db = tb / length;
            }
        }
    }
    float c = chroma + (ref_chroma - chroma) * look.sdr_chroma;
    // Blowout: the chroma falls with brightness above white (BT.709 luminance, 1 = paper white).
    const float y = 0.2126f * hdr[0] + 0.7152f * hdr[1] + 0.0722f * hdr[2];
    if (y > 1.0f) {
        c *= 1.0f + (1.0f / y - 1.0f) * look.blowout;
    }
    Rgb out = oklab_to_linear({lab[0], da * c, db * c});
    // Inside BT.2020: towards grey of the same lightness until no channel there is below 0. The
    // matrix keeps grey grey, so the same step works on the BT.709 values.
    const float grey = lab[0] * lab[0] * lab[0];
    float keep = 1.0f;
    for (const Rgb& row : kBt709ToBt2020) {
        const float wide = row[0] * out[0] + row[1] * out[1] + row[2] * out[2];
        if (wide < 0.0f) {
            keep = std::min(keep, grey / (grey - wide));
        }
    }
    for (float& v : out) {
        v = std::max(grey + (v - grey) * keep, 0.0f);
    }
    // Never past the peak the roll-off keeps to.
    const float h = headroom(levels);
    const float top = h > 0.0f ? 1.0f + h : 1.0f;
    const float m = std::max({out[0], out[1], out[2]});
    if (m > top) {
        for (float& v : out) {
            v *= top / m;
        }
    }
    return out;
}

// One pixel of the picture (0..1 as stored, above 1 where blending added up past white) as linear
// light, 1.0 being paper white: the SDR range as it is, light above white as the look says.
inline Rgb to_light(const Rgb& rgb, const Levels& levels) {
    Rgb linear{sdr_to_linear(rgb[0]), sdr_to_linear(rgb[1]), sdr_to_linear(rgb[2])};
    if (std::max({linear[0], linear[1], linear[2]}) <= 1.0f) {
        return linear;
    }
    if (levels.look.plain()) {
        return roll_off_above_white(linear, levels);
    }
    // The pixel as SDR shows it: each channel stopped at white.
    const Rgb sdr{
        sdr_to_linear(std::min(rgb[0], 1.0f)),
        sdr_to_linear(std::min(rgb[1], 1.0f)),
        sdr_to_linear(std::min(rgb[2], 1.0f)),
    };
    if (levels.look.highlights < 1.0f) {
        for (int c = 0; c < 3; ++c) {
            linear[c] = sdr[c] + (linear[c] - sdr[c]) * levels.look.highlights;
        }
    }
    return apply_look(roll_off_above_white(linear, levels), sdr, levels);
}

// The value the screen is sent for one pixel of the picture (`rgb`, 0..1 as stored): PQ signals
// for Output::Hdr10, scRGB values for Output::ScRgb, the input itself for Output::Sdr.
inline Rgb encode(const Rgb& rgb, Output output, const Levels& levels) {
    if (output == Output::Sdr) {
        return rgb;
    }
    const Rgb linear = to_light(rgb, levels);
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

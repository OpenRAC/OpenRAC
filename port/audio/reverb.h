// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-game/src/audio/reverb.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The reverb the mixer sends the "to reverb" tones through: what the sound
// library's SetReverbEx / AutoReverb select on the console. This is a native
// stereo reverb in the style of a Moorer reverb (four damped feedback combs
// per channel into two series all-passes), not the sound processor's reverb
// engine: no register is modelled. What reaches it is the send of the voices
// flagged for reverb, after their envelope and volume; its wet output is
// scaled by depth / 0x8000 and added to the dry mix before the master volume.
//
// Each libsd effect type is a preset whose character ReRAC derived once from
// libsd's preset table (in RAC1's libsd module) as results: the same-side and
// cross-side loop times, the decay time, the damping and the diffusion, with
// the hardware's 24 kHz internal rate taken into account. Only those
// measured results are here; the table is not used. libsd is the same
// library in all four games; whether their builds carry the same presets is
// not known yet.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace openrac::audio {

// libsd's effect types (SD_REV_MODE_*).
enum class ReverbMode : std::uint8_t {
    Off = 0,
    Room = 1,
    StudioA = 2,
    StudioB = 3,
    StudioC = 4,
    Hall = 5,
    Space = 6,
    Echo = 7,
    Delay = 8,
    Pipe = 9,
};

std::string_view reverb_mode_name(ReverbMode mode);

// A type's character.
struct ReverbPreset {
    std::array<float, 2> loops_ms;  // same-side and cross-side loop times
    float rt60;                     // decay to -60 dB, seconds
    float damp;                     // the loop's one-pole low-pass pole at 48 kHz (0: none)
    std::array<float, 2> allpass_ms;
    std::array<float, 2> allpass_gain;
};

// Room, studio A / B / C, hall, space, pipe; none for off, echo and delay
// (those are one delay line, echo_delay_samples).
std::optional<ReverbPreset> reverb_preset(ReverbMode mode);

// Echo and delay: the delay parameter (0..127) as samples at 48 kHz, (delay
// + 1) steps of 128 samples at the 24 kHz internal rate. Inferred by ReRAC:
// no RAC1 level uses these types.
std::size_t echo_delay_samples(std::uint8_t delay);

class ReverbEffect {
public:
    ReverbEffect();
    ~ReverbEffect();
    ReverbEffect(ReverbEffect&&) noexcept;
    ReverbEffect& operator=(ReverbEffect&&) noexcept;

    // SetReverbEx(core, type, depth, delay, feedback): a new type (or a new
    // echo delay) rebuilds the effect, cutting the tail, as libsd
    // re-initialises its work area; the depth is set at once; Off switches
    // it off.
    void set(ReverbMode mode, int depth, std::uint8_t delay, std::uint8_t feedback);

    // AutoReverb(core, depth, delta, channels): the depth glides linearly to
    // `depth` over `delta` sound-library ticks of 200 samples (the unit is
    // inferred).
    void glide(int depth, int delta);

    // A developer switch for A/B listening: no wet output, nothing processed.
    void set_enabled(bool enabled) { m_enabled = enabled; }

    bool active() const;

    ReverbMode mode() const { return m_mode; }

    float depth() const { return m_depth; }

    // One sample: the send (left, right) in, the wet signal out, already
    // scaled by the depth.
    std::array<int, 2> run(std::array<int, 2> send);

private:
    struct Network;

    static std::unique_ptr<Network> build_network(
        ReverbMode mode, std::uint8_t delay, std::uint8_t feedback
    );

    bool m_enabled = true;
    ReverbMode m_mode = ReverbMode::Off;
    std::uint8_t m_delay = 0;
    std::uint8_t m_feedback = 0;
    float m_depth = 0;
    float m_target = 0;
    float m_step = 0;
    std::uint32_t m_glide_left = 0;
    std::array<float, 2> m_lowpass{};
    std::unique_ptr<Network> m_network;
};

}  // namespace openrac::audio

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native mixer (audio/mixer.h), the pitch as speeds (audio/pitch.h) and the output's limiter
// (audio/limiter.h): samples decoded once with their loops, envelopes as rates and levels, the
// resampler at a speed, the float sum, and a limiter that keeps the sum under full scale.

#include "audio/mixer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "assets/sound/adpcm.h"
#include "assets/sound/sound_bank.h"
#include "audio/limiter.h"
#include "audio/pitch.h"
#include "tests/check.h"

namespace {

using namespace openrac::audio;

// A sine of `frames` frames at `period` frames a cycle, encoded as the banks' ADPCM.
std::vector<std::uint8_t> sine(std::size_t frames, double period, bool loop) {
    std::vector<std::int16_t> pcm(frames);

    for (std::size_t i = 0; i < frames; ++i) {
        pcm[i] = static_cast<std::int16_t>(
            16000.0 * std::sin(2.0 * 3.14159265358979 * static_cast<double>(i) / period)
        );
    }

    return openrac::assets::encode_adpcm(pcm, loop);
}

bool near(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

// Fastest attack, full sustain, held, fastest linear release: what the bank tests use.
constexpr std::uint16_t kAdsr1 = 0x000f;
constexpr std::uint16_t kAdsr2 = 0x1fc0;

void samples_decode_once() {
    const auto adpcm = sine(28 * 20, 56.0, false);
    const PcmData one = decode_sample(adpcm, 0);
    CHECK(one && one->frames.size() == 28 * 20 && !one->looped);
    CHECK(std::all_of(one->frames.begin(), one->frames.end(), [](float v) {
        return std::abs(v) <= 1.0f;
    }));
    const PcmData loop = decode_sample(sine(28 * 4, 28.0, true), 0);
    CHECK(loop && loop->looped && loop->loop_start == 0);
    // Not a sample: a misaligned offset, or data that ends before its end frame.
    CHECK(!decode_sample(adpcm, 3));
    CHECK(!decode_sample(std::vector<std::uint8_t>(32, 0), 0));
}

void envelopes_as_rates() {
    // A linear attack of shift 10, step 7 - 0: 7 * 2^1 = 14 steps of 32767 a frame, so full in
    // about 2340 frames.
    const EnvelopeShape linear = envelope_shape(static_cast<std::uint16_t>(10 << 10), kAdsr2);
    CHECK(!linear.attack.exponential);
    Envelope e;
    e.attack(linear);
    int frames = 0;

    while (e.phase() == EnvelopePhase::Attack && frames < 100000) {
        e.run();
        ++frames;
    }

    CHECK(near(frames, 32767.0 / 14.0, 3.0));
    // Decay then holds at the sustain level of the low four bits: (15 + 1) << 11 is full.
    CHECK(e.phase() == EnvelopePhase::Sustain || e.phase() == EnvelopePhase::Decay);
    const EnvelopeShape half = envelope_shape(0x0007, kAdsr2);
    CHECK(near(half.sustain_level, (8 << 11) / 32767.0, 1e-6));
    // An exponential fall loses the same share every frame: the release of shift 16 (bits 16-20 of
    // the pair, here 0) halves at a steady rate.
    const EnvelopeShape fall = envelope_shape(kAdsr1, 0x1fc0 | 0x0020 | (12 << 0));
    Envelope r;
    r.attack(fall);

    for (int i = 0; i < 100 && r.phase() == EnvelopePhase::Attack; ++i) {
        r.run();
    }

    while (r.phase() == EnvelopePhase::Attack || r.phase() == EnvelopePhase::Decay) {
        r.run();
    }

    r.release();
    const float a = r.level();
    r.run();
    const float b = r.level();
    r.run();
    const float c = r.level();
    CHECK(fall.release.exponential && b < a && near(b / a, c / b, 1e-4));

    // The release ends the voice once it is silent.
    for (int i = 0; i < 1000000 && r.phase() != EnvelopePhase::Stopped; ++i) {
        r.run();
    }

    CHECK(r.phase() == EnvelopePhase::Stopped);
}

void speeds() {
    // A sample at the console's rate (negative centre note) an octave up plays twice as fast.
    CHECK(near(ps1_note_speed(-60, 0, 72, 0), 2.0, 1e-9));
    CHECK(near(ps1_note_speed(-60, 0, 60, 0), 1.0, 1e-9));
    CHECK(near(ps1_note_speed(-60, 0, 59, 64), std::exp2(-0.5 / 12.0), 1e-9));
    // A PS1-style sample (non-negative centre note) is 44.1 kHz.
    CHECK(near(ps1_note_speed(60, 0, 60, 0), 44100.0 / 48000.0, 1e-9));
    openrac::assets::Tone tone;
    tone.center_note = -60;
    tone.bend_up = 2;
    tone.bend_down = 2;
    CHECK(near(tone_speed(tone, 0, 0), 1.0, 1e-6));
    CHECK(tone_speed(tone, 0x7fff, 0) > 1.12f && tone_speed(tone, -0x8000, 0) < 0.9f);
}

void voices_play_at_speed() {
    const PcmData s = decode_sample(sine(28 * 40, 56.0, false), 0);
    Mixer m(4);
    Voice& v = m.voice(0);
    v.key_on(s, 1.0f, envelope_shape(kAdsr1, kAdsr2));
    v.set_gain({1.0f, 0.5f});
    int frames = 0;
    std::array<float, 2> loudest{};

    while (v.active() && frames < 10000) {
        const std::array<float, 2> out = m.mix();
        loudest = {std::max(loudest[0], std::abs(out[0])), std::max(loudest[1], std::abs(out[1]))};
        ++frames;
    }

    // A one-shot ends at its end at speed 1, the right side at half the gain. The envelope's
    // fastest decay halves the level in its one step before the sustain, as it did on the console.
    CHECK(near(frames, 28 * 40, 1.0));
    CHECK(loudest[0] > 0.2f && loudest[0] <= 0.5f && near(loudest[1], loudest[0] / 2.0f, 1e-3));
    // Twice as fast: half as long.
    v.key_on(s, 2.0f, envelope_shape(kAdsr1, kAdsr2));
    frames = 0;

    while (v.active() && frames < 10000) {
        m.mix();
        ++frames;
    }

    CHECK(near(frames, 28 * 20, 1.0));
    // A loop goes on until its key off has released it.
    v.key_on(decode_sample(sine(28 * 4, 28.0, true), 0), 1.0f, envelope_shape(kAdsr1, kAdsr2));

    for (int i = 0; i < 5000; ++i) {
        m.mix();
    }

    CHECK(v.active() && m.active_voices() == 1);
    v.key_off();

    for (int i = 0; i < 100000 && v.active(); ++i) {
        m.mix();
    }

    CHECK(!v.active() && m.active_voices() == 0);
    // Nothing to play: the voice ends at once.
    v.key_on(nullptr, 1.0f, envelope_shape(kAdsr1, kAdsr2));
    CHECK(!v.active());
}

void the_limiter() {
    // Below the ceiling nothing changes.
    std::vector<float> quiet{0.5f, -0.5f, 0.25f, 0.0f};
    Limiter l;
    l.process(quiet);
    CHECK(quiet[0] == 0.5f && quiet[1] == -0.5f && quiet[2] == 0.25f);
    // Over it, the sum is brought under, both sides alike, and recovers after.
    std::vector<float> loud(2 * 4800);

    for (std::size_t i = 0; i < loud.size(); i += 2) {
        loud[i] = 3.0f;
        loud[i + 1] = 1.5f;
    }

    l.process(loud);
    CHECK(std::all_of(loud.begin(), loud.end(), [](float v) {
        return v <= Limiter::kCeiling + 1e-6f;
    }));
    CHECK(near(loud[0] / loud[1], 2.0, 1e-4));
    const Limiter::Taken taken = l.take();
    CHECK(taken.limited == 4800 && taken.lowest < 0.34f);
    std::vector<float> after(2 * 48000, 0.5f);
    l.process(after);
    CHECK(after.back() > 0.49f);
    std::vector<std::int16_t> out(2);
    to_int16(std::vector<float>{1.0f, -1.5f}, out);
    CHECK(out[0] == 32767 && out[1] == -32768);
}

}  // namespace

int main() {
    samples_decode_once();
    envelopes_as_rates();
    speeds();
    voices_play_at_speed();
    the_limiter();
    return openrac::test::result();
}

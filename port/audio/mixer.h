// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The voices the sound player keys its tones onto, mixed natively in float: nothing of the
// console's sound processor is modelled. A tone's sample is decoded once, when its bank is loaded,
// from the bank's ADPCM to float (assets/sound/adpcm.h), with the loop the sample's frame flags
// describe; a voice plays it at a speed (1 = the console's 48 kHz) through a cubic Hermite
// resampler, shaped by an envelope of rates and levels, at a left and right gain, and optionally
// sends it to the reverb. The voices and the reverb are summed in float; the one limiter is the
// output's (limiter.h).
//
// The envelope is the tone's ADSR fields (the four phases a bank describes: attack, decay to a
// sustain level, sustain, release) read once into rates and levels (envelope_shape), so that the
// voice runs a few float operations a sample. The rates are the ones the fields mean on the
// console (psx-spx "SPU ADSR"; PCSX2 and OpenGOAL's game/sound/common/envelope.cpp time them the
// same), so a tone fades in and out as long as it did there: linear phases step a level per
// second, exponential ones fall by a factor per second (and an exponential attack slows to a
// quarter above three quarters of full, as the fields ask).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "audio/reverb.h"

namespace openrac::audio {

inline constexpr int kOutputRate = 48000;

// A tone's sample, decoded: mono, at the console's rate, -1..1. Playing reaches the end and stops,
// or goes back to `loop_start` when it loops.
struct PcmSample {
    std::vector<float> frames;
    std::size_t loop_start = 0;
    bool looped = false;
};

using PcmData = std::shared_ptr<const PcmSample>;

// The sample at byte `offset` of a run of ADPCM (a bank's sample chunk) up to its end frame, with
// its loop; null when there is none there (a misaligned offset, data that ends first).
PcmData decode_sample(std::span<const std::uint8_t> adpcm, std::size_t offset);

// One phase's motion: a linear one moves the level by `rate` (of full scale) a second; an
// exponential fall multiplies it by exp(-rate) a second, and an exponential rise moves like a linear
// one, at a quarter of `rate` above three quarters of full.
struct EnvelopeSegment {
    bool exponential = false;
    float rate = 0.0f;
};

// The envelope a tone's two ADSR fields describe.
struct EnvelopeShape {
    EnvelopeSegment attack;  // from 0 up to full
    EnvelopeSegment decay;   // down to the sustain level
    float sustain_level = 1.0f;
    EnvelopeSegment sustain;  // until the key off: up or down, never past full or 0
    bool sustain_rises = false;
    EnvelopeSegment release;  // down to silence, which ends the voice
};

EnvelopeShape envelope_shape(std::uint16_t adsr1, std::uint16_t adsr2);

enum class EnvelopePhase : std::uint8_t {
    Attack,
    Decay,
    Sustain,
    Release,
    Stopped,
};

class Envelope {
public:
    // Key on: the attack from silence.
    void attack(const EnvelopeShape& shape);
    // Key off: the release from the level now.
    void release();
    // Silent at once.
    void stop();
    // One output frame.
    void run();

    EnvelopePhase phase() const { return m_phase; }

    float level() const { return m_level; }

private:
    void enter(EnvelopePhase phase);

    EnvelopeShape m_shape;
    EnvelopePhase m_phase = EnvelopePhase::Stopped;
    float m_level = 0.0f;
    float m_step = 0.0f;    // a linear phase's step a frame (signed)
    float m_factor = 1.0f;  // an exponential fall's factor a frame
    bool m_slows = false;   // an exponential rise: a quarter of the step above the knee
};

class Voice {
public:
    // Starts `sample` from its first frame at `speed`, with the envelope's attack.
    void key_on(PcmData sample, float speed, const EnvelopeShape& shape);

    void key_off() { m_envelope.release(); }

    // Silent at once (a voice taken for another tone, a sound cut).
    void stop() { m_envelope.stop(); }

    bool active() const { return m_envelope.phase() != EnvelopePhase::Stopped; }

    // The sample's frames a frame of output: 1 at the console's rate, at most kMaxSpeed; 0 holds
    // the position (a paused sound).
    void set_speed(float speed);

    float speed() const { return m_speed; }

    // Left and right gain, 0..1.
    void set_gain(std::array<float, 2> gain) { m_gain = gain; }

    std::array<float, 2> gain() const { return m_gain; }

    bool reverb() const { return m_reverb; }

    void set_reverb(bool reverb) { m_reverb = reverb; }

    // Bumped at every key on, so an owner can tell a reused voice from its own.
    std::uint32_t generation() const { return m_generation; }

    const Envelope& envelope() const { return m_envelope; }

    // One frame of output: (left, right).
    std::array<float, 2> run();

    // The fastest a voice plays its sample: four times, as the tones were made for.
    static constexpr float kMaxSpeed = 4.0f;

private:
    float at(std::ptrdiff_t i) const;

    PcmData m_sample;
    Envelope m_envelope;
    double m_position = 0.0;
    float m_speed = 1.0f;
    std::array<float, 2> m_gain{};
    bool m_reverb = false;
    std::uint32_t m_generation = 0;
};

// The voices and the reverb their sends go through, summed in float.
class Mixer {
public:
    explicit Mixer(std::size_t voices);

    std::span<Voice> voices() { return m_voices; }

    std::span<const Voice> voices() const { return m_voices; }

    Voice& voice(std::size_t i) { return m_voices[i]; }

    ReverbEffect& reverb() { return m_reverb; }

    // One stereo frame of the voices and the reverb (-1..1 each at full scale, unlimited).
    std::array<float, 2> mix();

    std::size_t active_voices() const;

private:
    std::vector<Voice> m_voices;
    ReverbEffect m_reverb;
};

}  // namespace openrac::audio

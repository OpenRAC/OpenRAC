// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "audio/mixer.h"

#include <algorithm>
#include <cmath>

#include "assets/bytes.h"
#include "assets/sound/adpcm.h"

namespace openrac::audio {

namespace {

// The envelope's full scale in the fields' own steps.
constexpr float kFullSteps = 32767.0f;
// An exponential release or sustain fall this far down is silence (-90 dB), where the fields'
// integer steps reached 0.
constexpr float kSilent = 1.0f / 32768.0f;
// An exponential rise slows to a quarter above this level.
constexpr float kRiseKnee = 0x6000 / kFullSteps;

// A phase's (shift, step) pair, as the fields give it, as a rate a second: the level moves by
// step * 2^(11 - shift) steps of full scale every frame; an exponential fall moves by that much
// times the level, so it falls by that fraction a frame.
EnvelopeSegment segment(bool exponential, bool decreasing, int shift, int step) {
    const double steps =
        std::min(std::abs(step) * std::exp2(11.0 - static_cast<double>(shift)), 32768.0);
    EnvelopeSegment s;
    s.exponential = exponential;

    // An exponential fall loses steps / 32768 of the level a frame: as a rate, the natural log of
    // what is kept, a second. Anything else moves a share of full scale a second.
    if (exponential && decreasing) {
        const double kept = 1.0 - std::min(steps / 32768.0, 0.999);
        s.rate = static_cast<float>(-std::log(kept) * kOutputRate);
    } else {
        s.rate = static_cast<float>(steps / kFullSteps * kOutputRate);
    }

    return s;
}

}  // namespace

PcmData decode_sample(std::span<const std::uint8_t> adpcm, std::size_t offset) {
    try {
        const assets::SampleExtent extent = assets::sample_extent(adpcm, offset);
        const std::vector<std::int16_t> pcm = assets::decode_extent(adpcm, extent);
        auto out = std::make_shared<PcmSample>();
        out->frames.reserve(pcm.size());

        // Each sample to -1..1.
        for (const std::int16_t s : pcm) {
            out->frames.push_back(static_cast<float>(s) / 32768.0f);
        }

        out->looped = extent.looped;
        out->loop_start = extent.loop_start.value_or(0) * assets::kAdpcmFrameSamples;
        return out;
    } catch (const assets::AssetError&) {
        return nullptr;
    }
}

EnvelopeShape envelope_shape(std::uint16_t adsr1, std::uint16_t adsr2) {
    const std::uint32_t r = static_cast<std::uint32_t>(adsr2) << 16 | adsr1;
    auto bits = [r](unsigned pos, unsigned n) {
        return static_cast<int>((r >> pos) & ((1u << n) - 1));
    };

    EnvelopeShape shape;
    shape.attack = segment(bits(15, 1) != 0, false, bits(10, 5), 7 - bits(8, 2));
    shape.decay = segment(true, true, bits(4, 4), -8);
    shape.sustain_level = std::min(static_cast<float>((bits(0, 4) + 1) << 11) / kFullSteps, 1.0f);
    const bool sustain_falls = bits(30, 1) != 0;
    const int s = bits(22, 2);
    shape.sustain =
        segment(bits(31, 1) != 0, sustain_falls, bits(24, 5), sustain_falls ? -8 + s : 7 - s);
    shape.sustain_rises = !sustain_falls;
    shape.release = segment(bits(21, 1) != 0, true, bits(16, 5), -8);
    return shape;
}

// ---- The envelope ----

void Envelope::attack(const EnvelopeShape& shape) {
    m_shape = shape;
    m_level = 0.0f;
    enter(EnvelopePhase::Attack);
}

void Envelope::release() {
    // A voice that has already stopped has nothing to release.
    if (m_phase == EnvelopePhase::Stopped) {
        return;
    }

    enter(EnvelopePhase::Release);
}

void Envelope::stop() {
    m_phase = EnvelopePhase::Stopped;
    m_level = 0.0f;
}

void Envelope::enter(EnvelopePhase phase) {
    m_phase = phase;
    const EnvelopeSegment* seg = nullptr;
    bool falls = true;

    // Each phase has its own segment and direction.
    switch (phase) {
        case EnvelopePhase::Attack:
            seg = &m_shape.attack;
            falls = false;
            break;
        case EnvelopePhase::Decay:
            seg = &m_shape.decay;
            break;
        case EnvelopePhase::Sustain:
            seg = &m_shape.sustain;
            falls = !m_shape.sustain_rises;
            break;
        case EnvelopePhase::Release:
            seg = &m_shape.release;
            break;
        case EnvelopePhase::Stopped:
            return;
    }

    const float per_frame = seg->rate / static_cast<float>(kOutputRate);
    m_factor = 1.0f;
    m_step = 0.0f;
    m_slows = seg->exponential && !falls;

    // An exponential fall multiplies; everything else adds.
    if (seg->exponential && falls) {
        m_factor = std::exp(-per_frame);
    } else {
        m_step = falls ? -per_frame : per_frame;
    }
}

void Envelope::run() {
    // Stopped: nothing moves.
    if (m_phase == EnvelopePhase::Stopped) {
        return;
    }

    // An exponential rise slows to a quarter above the knee.
    const bool slowed = m_slows && m_level > kRiseKnee;
    m_level = m_level * m_factor + (slowed ? m_step * 0.25f : m_step);

    // Each phase ends where its target is crossed; sustain has none and holds until the key off.
    switch (m_phase) {
        case EnvelopePhase::Attack:
            if (m_level >= 1.0f) {
                m_level = 1.0f;
                enter(EnvelopePhase::Decay);
            }

            break;
        case EnvelopePhase::Decay:
            // The level the step reached is kept, under the target if it went past: a tone with a
            // fast decay to a high sustain level sounds as loud as it did on the console.
            if (m_level <= m_shape.sustain_level) {
                enter(EnvelopePhase::Sustain);
            }

            break;
        case EnvelopePhase::Sustain:
            m_level = std::clamp(m_level, 0.0f, 1.0f);
            break;
        case EnvelopePhase::Release:
            if (m_level <= kSilent) {
                stop();
            }

            break;
        case EnvelopePhase::Stopped:
            break;
    }
}

// ---- A voice ----

void Voice::key_on(PcmData sample, float speed, const EnvelopeShape& shape) {
    m_sample = std::move(sample);
    m_position = 0.0;
    set_speed(speed);
    m_reverb = false;
    ++m_generation;
    m_envelope.attack(shape);

    // Nothing to play: the voice ends at once.
    if (!m_sample || m_sample->frames.empty()) {
        m_envelope.stop();
    }
}

void Voice::set_speed(float speed) {
    m_speed = std::clamp(speed, 0.0f, kMaxSpeed);
}

// Frame `i` of the sample as the voice plays it: frames before the first are silence, frames past
// the end are the loop's again, or silence.
float Voice::at(std::ptrdiff_t i) const {
    const std::vector<float>& f = m_sample->frames;
    const auto n = static_cast<std::ptrdiff_t>(f.size());

    // Before the first frame there was nothing.
    if (i < 0) {
        return 0.0f;
    }

    // Past the end: the loop's frames again, or silence.
    if (i >= n) {
        if (!m_sample->looped) {
            return 0.0f;
        }

        const auto start = static_cast<std::ptrdiff_t>(m_sample->loop_start);
        i = start + (i - n) % std::max<std::ptrdiff_t>(n - start, 1);
    }

    return f[static_cast<std::size_t>(i)];
}

std::array<float, 2> Voice::run() {
    // A stopped or paused voice outputs silence. Pausing holds the envelope as well as the sample,
    // so an attack or release resumes at the same level after the output has kept mixing.
    if (m_envelope.phase() == EnvelopePhase::Stopped || m_speed == 0.0f) {
        return {0.0f, 0.0f};
    }

    // Cubic Hermite (Catmull-Rom) through the four frames around the position.
    const auto i = static_cast<std::ptrdiff_t>(m_position);
    const auto t = static_cast<float>(m_position - static_cast<double>(i));
    const float p0 = at(i - 1);
    const float p1 = at(i);
    const float p2 = at(i + 1);
    const float p3 = at(i + 2);
    const float c1 = 0.5f * (p2 - p0);
    const float c2 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
    const float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
    const float sample = ((c3 * t + c2) * t + c1) * t + p1;
    const float s = sample * m_envelope.level();
    m_envelope.run();
    m_position += static_cast<double>(m_speed);
    const auto n = static_cast<double>(m_sample->frames.size());

    // The end of the sample: back to its loop start, or the voice ends.
    if (m_position >= n) {
        if (m_sample->looped) {
            const auto start = static_cast<double>(m_sample->loop_start);
            const double span = std::max(n - start, 1.0);
            m_position = start + std::fmod(m_position - n, span);
        } else {
            m_envelope.stop();
        }
    }

    return {s * m_gain[0], s * m_gain[1]};
}

// ---- The mix ----

Mixer::Mixer(std::size_t voices) : m_voices(voices) {}

std::array<float, 2> Mixer::mix() {
    std::array<float, 2> out{};
    std::array<float, 2> send{};

    // Every voice that plays adds to the dry sum, and those flagged for reverb to the send.
    for (Voice& v : m_voices) {
        // A stopped voice adds nothing.
        if (!v.active()) {
            continue;
        }

        const auto [l, r] = v.run();
        out[0] += l;
        out[1] += r;

        // Voices flagged for reverb also feed the reverb send.
        if (v.reverb()) {
            send[0] += l;
            send[1] += r;
        }
    }

    // The reverb is added only when an effect is set, so that unused effects cost nothing.
    if (m_reverb.active()) {
        const auto wet = m_reverb.run(send);
        out[0] += wet[0];
        out[1] += wet[1];
    }

    return out;
}

std::size_t Mixer::active_voices() const {
    return static_cast<std::size_t>(
        std::count_if(m_voices.begin(), m_voices.end(), [](const Voice& v) { return v.active(); })
    );
}

}  // namespace openrac::audio

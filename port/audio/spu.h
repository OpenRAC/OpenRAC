// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-game/src/audio.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The voices the sound library drives, as the console's sound processor
// plays them: ADPCM decoded at the voice's address (assets/sound/adpcm.h),
// 4-tap Gaussian interpolation at the pitch step, the envelope (envelope.h)
// and the left / right volume registers, 48 voices summed at 48 kHz. This is
// a model of what a voice outputs, written from public documentation of the
// hardware (psx-spx; PCSX2 and DuckStation behave the same), not an
// emulation of the console: no sound memory, no DMA, no registers beyond
// what a voice's output depends on.
//
// Not modelled (as in ReRAC): the two cores' separate mix stages and their
// saturation (the sum here is exact and clamped once), noise voices, pitch
// modulation by the previous voice, volume sweeps (the sound library never
// enables them).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "assets/sound/adpcm.h"
#include "audio/envelope.h"
#include "audio/reverb.h"

namespace openrac::audio {

inline constexpr int kOutputRate = 48000;
inline constexpr std::size_t kVoiceCount = 48;  // two cores of 24

using SampleData = std::shared_ptr<const std::vector<std::uint8_t>>;

// The hardware's 512-entry Gaussian interpolation table (psx-spx "SPU
// Gaussian Interpolation"; PCSX2 and DuckStation carry the same numbers).
// A constant of the chip, not disc data.
extern const std::array<std::int16_t, 512> kGaussianTable;

// sum over the four samples s[idx - 3 .. idx] of g * s >> 15, each product
// shifted on its own as the hardware does; `i` is (counter >> 4) & 0xff.
int gaussian_interpolate(std::size_t i, const std::array<int, 4>& s);

// One queued part of a stream: a body up to and including its end frame.
struct StreamPart {
    SampleData data;
    bool looped = false;  // repeat when it ends with nothing queued behind it
};

class Voice {
public:
    // Key on at byte `start` of sound memory (a bank's sample chunk), with
    // the hardware's loop rules: the loop address latches at a loop-start
    // frame; after an end frame the voice jumps there if the repeat flag is
    // set, else it stops (level 0 at once, psx-spx "Loop End without Repeat").
    void key_on_memory(
        SampleData memory,
        std::size_t start,
        std::uint16_t pitch,
        std::uint16_t adsr1,
        std::uint16_t adsr2
    );

    // Key on a stream: after the end frame of the current part the next
    // queued part starts with the decoder history carried over (the stream
    // player feeds one voice from a ring buffer); the last part repeats when
    // looped, otherwise the voice stops. The file's other flags are ignored
    // (inferred by ReRAC).
    void key_on_stream(
        StreamPart first, std::uint16_t pitch, std::uint16_t adsr1, std::uint16_t adsr2
    );

    void key_off() { m_envelope.release(); }

    // Silence at once (a stolen voice, a stopped stream).
    void stop() { m_envelope.stop(); }

    bool active() const { return m_envelope.phase() != EnvelopePhase::Stopped; }

    // Queue a stream part behind the current one (nothing for a memory voice).
    void queue_part(StreamPart part);

    // A held voice outputs nothing and does not advance (a paused stream);
    // its registers can still be written.
    void set_held(bool held) { m_held = held; }

    bool held() const { return m_held; }

    // Source samples left in the current stream part (0 for a memory voice).
    std::size_t stream_remaining() const;

    std::uint16_t pitch() const { return m_pitch; }

    void set_pitch(std::uint16_t pitch) { m_pitch = pitch; }

    // VOLL / VOLR as the sound library writes them (0..0x3fff).
    std::array<std::uint16_t, 2> volume() const { return m_volume; }

    void set_volume(std::array<std::uint16_t, 2> volume) { m_volume = volume; }

    // The voice also feeds the reverb (cleared at key on).
    bool reverb() const { return m_reverb; }

    void set_reverb(bool reverb) { m_reverb = reverb; }

    // Bumped at every key on, so an owner can tell a reused voice from its own.
    std::uint32_t generation() const { return m_generation; }

    const Envelope& envelope() const { return m_envelope; }

    // One output sample: (left, right) after the envelope and the volume.
    std::pair<int, int> run();

private:
    void key_on(std::uint16_t pitch, std::uint16_t adsr1, std::uint16_t adsr2);
    void load_block();
    void next_block();
    const std::vector<std::uint8_t>* source() const;

    Envelope m_envelope;
    std::uint16_t m_pitch = 0;
    std::array<std::uint16_t, 2> m_volume{};
    std::uint32_t m_generation = 0;
    bool m_reverb = false;
    bool m_held = false;
    bool m_streaming = false;
    SampleData m_memory;
    StreamPart m_part;
    std::deque<StreamPart> m_queue;
    std::size_t m_address = 0;       // the next frame (NAX)
    std::size_t m_loop_address = 0;  // LSA
    std::uint8_t m_flags = 0;
    std::array<std::int16_t, assets::kAdpcmFrameSamples> m_block{};
    std::array<std::int16_t, 3> m_previous{};  // the last three samples of the previous block
    assets::AdpcmHistory m_history;
    std::uint32_t m_counter = 0;   // 12 fraction bits
    std::size_t m_part_frame = 0;  // frames played in the current stream part
};

// The master volume stage on the exact sum x of the voices and the reverb:
// x * (reg << 1) as a signed 16-bit level >> 15, clamped to 16 bits. The
// product is taken in 64 bits: many loud voices take a 32-bit product past
// its range (ReRAC hit that with seven missiles at once).
std::int16_t master_out(std::int64_t x, std::uint16_t reg);

// The 48 voices, the reverb, the master volume (0x3fff, as the sound library
// leaves it) and the clamp to 16 bits.
class VoiceMixer {
public:
    std::array<Voice, kVoiceCount>& voices() { return m_voices; }

    const std::array<Voice, kVoiceCount>& voices() const { return m_voices; }

    Voice& voice(std::size_t i) { return m_voices[i]; }

    ReverbEffect& reverb() { return m_reverb; }

    void set_master(std::uint16_t left, std::uint16_t right) { m_master = {left, right}; }

    // One stereo sample; `extra` is added to the dry sum before the master
    // stage (the movie sound).
    std::array<std::int16_t, 2> mix(std::array<int, 2> extra = {0, 0});

    std::size_t active_voices() const;

private:
    std::array<Voice, kVoiceCount> m_voices;
    std::array<std::uint16_t, 2> m_master{0x3fff, 0x3fff};
    ReverbEffect m_reverb;
};

}  // namespace openrac::audio

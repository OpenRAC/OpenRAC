// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The console sound processor's ADPCM (SPU ADPCM, the samples of sound
// banks, VAG files and the movies' audio): the 16-byte frame decoder and the
// walk that finds where a sample ends and where it loops.
//
// A frame is 16 bytes: byte 0 is `shift | filter << 4`, byte 1 the flags
// (bit 0 end, bit 1 repeat, bit 2 loop start), then 14 bytes holding 28
// four-bit samples, low nibble first. The prediction filters are the
// hardware's (psx-spx, "SPU ADPCM"; PCSX2 and DuckStation decode the same).
// The format is the console's, so it is the same in all four games.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

inline constexpr std::size_t kAdpcmFrameBytes = 16;
inline constexpr std::size_t kAdpcmFrameSamples = 28;

// Frame flags (byte 1).
inline constexpr u8 kAdpcmFlagEnd = 1;
inline constexpr u8 kAdpcmFlagRepeat = 2;
inline constexpr u8 kAdpcmFlagLoopStart = 4;

// Which rounding the prediction uses. Nothing on the disc decides between
// the two: one prediction differs by up to 2 LSB and the error then feeds the
// history. The port plays the rounded form.
enum class AdpcmRounding : u8 {
    // s += (K0 * h1 + K1 * h2 + 32) >> 6, as PCSX2 and DuckStation do it.
    Rounded,
    // s += (K0 * h1) >> 6; s += (K1 * h2) >> 6, as OpenGOAL's voice decoder
    // does it (game/sound/common/voice.cpp); kept for trace comparisons.
    Truncated,
};

// The decoder's history: the two previous output samples (h1 the newest),
// zero when a voice keys on.
struct AdpcmHistory {
    int h1 = 0;
    int h2 = 0;
};

// Decodes one frame (`frame` holds at least 16 bytes) and updates `history`.
// The disc only uses shift 0..12 and filter 0..4; a larger filter is clamped
// to 4, a larger shift applied as given.
std::array<s16, kAdpcmFrameSamples> decode_adpcm_frame(
    const u8* frame, AdpcmHistory& history, AdpcmRounding rounding = AdpcmRounding::Rounded
);

// Decodes every whole frame of `adpcm` from zero history. Flags are not
// interpreted: this is the raw signal, not what a voice plays.
std::vector<s16> decode_adpcm(ByteView adpcm, AdpcmRounding rounding = AdpcmRounding::Rounded);

// Where one sample lives in a run of ADPCM and how a voice plays it.
struct SampleExtent {
    std::size_t offset = 0;  // byte offset of the first frame
    // Frames up to and including the first frame with the end flag: what a
    // voice plays before it jumps to the loop start or stops.
    std::size_t frames = 0;
    // The last loop-start frame before the end (relative to `offset`): the
    // sound processor latches the loop address at every flagged frame.
    std::optional<std::size_t> loop_start;
    // The end frame has the repeat flag: the voice jumps back instead of
    // stopping.
    bool looped = false;
    // Flags of the frame after the end frame when the data has one (the disc
    // pads one-shots with a frame of flags 7).
    std::optional<u8> next_flags;

    std::size_t bytes() const { return frames * kAdpcmFrameBytes; }

    std::size_t samples() const { return frames * kAdpcmFrameSamples; }

    // (loop start, end) in samples of a looped sample: multiples of 28.
    std::optional<std::pair<std::size_t, std::size_t>> loop_points() const;
};

// Walks the frames of the sample at `offset` (frame-aligned) to its first
// end flag. Throws AssetError when the data ends first.
SampleExtent sample_extent(ByteView adpcm, std::size_t offset);

// The samples a voice plays for `extent` (one pass, from zero history).
std::vector<s16> decode_extent(ByteView adpcm, const SampleExtent& extent);

}  // namespace openrac::assets

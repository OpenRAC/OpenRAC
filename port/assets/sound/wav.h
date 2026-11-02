// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// WAV files: what a pack replaces a sound with (docs/port/MODS.md, editor/AUDIO_MODDING.md). Only
// 16-bit PCM, mono or stereo, at 500 to 192000 Hz: a pack's check and an import (editor/audio.py,
// check_wav and to_game_wav) keep to the same.

#pragma once

#include <cstddef>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

struct WavFormat {
    u16 channels = 0;
    u32 sample_rate = 0;
    std::size_t data_offset = 0;
    std::size_t data_bytes = 0;  // as its header says: may run past what was given
};

// The format from a file's start, up to and including the data chunk's header. Throws AssetError on
// anything else, or when `bytes` ends first.
WavFormat parse_wav_format(ByteView bytes);

struct WavAudio {
    u16 channels = 0;
    u32 sample_rate = 0;
    std::vector<s16> samples;  // interleaved

    std::size_t frames() const { return channels == 0 ? 0 : samples.size() / channels; }
};

// A whole file; the data chunk is cut to the file when its header says more. Throws AssetError on a
// file with no sound in it.
WavAudio parse_wav(ByteView bytes);

// One channel at `rate`: the channels averaged, the rate changed by straight lines between samples.
std::vector<s16> wav_to_mono(const WavAudio& audio, u32 rate);

}  // namespace openrac::assets

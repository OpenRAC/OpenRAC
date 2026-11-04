// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The WAV reader (wav.h).

#include "assets/sound/wav.h"

#include <algorithm>
#include <cstring>

namespace openrac::assets {

namespace {

constexpr std::size_t kRiffHeader = 12;
constexpr std::size_t kChunkHeader = 8;
constexpr u16 kFormatPcm = 1;
constexpr u16 kFormatExtensible = 0xfffe;
constexpr u32 kLowestRate = 500;  // the disc's slowest effect is about 1030 Hz
constexpr u32 kHighestRate = 192000;

bool tag_is(const u8* at, const char* tag) {
    return std::memcmp(at, tag, 4) == 0;
}

// The 'fmt ' chunk's words, or a refusal saying what the file is instead.
WavFormat read_fmt(ByteView bytes, std::size_t at, std::size_t size) {
    // Its first 16 bytes are the same in every kind of WAV file.
    if (size < 16 || at + 16 > bytes.size()) {
        fail("WAV: the format chunk is too short");
    }

    const u16 format = bytes.u16_at(at);
    const u16 bits = bytes.u16_at(at + 14);

    // An extensible file says what it holds further on, in the first two bytes of its subformat.
    const u16 kind = format == kFormatExtensible && size >= 26 && at + 26 <= bytes.size()
                         ? bytes.u16_at(at + 24)
                         : format;

    // Anything else would need converting first, which an import does (editor/audio.py).
    if (kind != kFormatPcm || bits != 16) {
        fail("WAV: format {}, {} bits; the game reads 16-bit PCM", kind, bits);
    }

    WavFormat out;
    out.channels = bytes.u16_at(at + 2);
    out.sample_rate = bytes.u32_at(at + 4);

    // Mono or stereo, as the mix has two channels.
    if (out.channels != 1 && out.channels != 2) {
        fail("WAV: {} channels; the game reads mono or stereo", out.channels);
    }

    // A rate outside these is a damaged header or nothing a voice could play.
    if (out.sample_rate < kLowestRate || out.sample_rate > kHighestRate) {
        fail("WAV: {} Hz; the game reads {} to {} Hz", out.sample_rate, kLowestRate, kHighestRate);
    }

    return out;
}

}  // namespace

WavFormat parse_wav_format(ByteView bytes) {
    // Every WAV file starts with these two tags.
    if (bytes.size() < kRiffHeader || !tag_is(bytes.data(), "RIFF")
        || !tag_is(bytes.data() + 8, "WAVE")) {
        fail("not a WAV file");
    }

    WavFormat out;
    bool have_format = false;
    std::size_t at = kRiffHeader;

    // Chunks follow one another: the format is before the data in any file that plays.
    while (at + kChunkHeader <= bytes.size()) {
        const std::size_t size = bytes.u32_at(at + 4);
        const std::size_t body = at + kChunkHeader;

        // The format's words are kept until the data is found.
        if (tag_is(bytes.data() + at, "fmt ")) {
            out = read_fmt(bytes, body, size);
            have_format = true;
        } else if (tag_is(bytes.data() + at, "data")) {
            // Data before a format can not be read.
            if (!have_format) {
                fail("WAV: the data comes before the format");
            }

            out.data_offset = body;
            out.data_bytes = size;
            return out;
        }

        // A chunk of odd size has a padding byte after it.
        at = body + size + (size & 1);
    }

    fail("WAV: no data chunk");
}

WavAudio parse_wav(ByteView bytes) {
    const WavFormat format = parse_wav_format(bytes);
    const std::size_t available = bytes.size() - format.data_offset;
    const std::size_t size = std::min(format.data_bytes, available);
    const std::size_t count = size / 2 / format.channels * format.channels;

    // A file with a format but no sound has nothing to play in place of the game's.
    if (count == 0) {
        fail("WAV: no sound in it");
    }

    WavAudio out;
    out.channels = format.channels;
    out.sample_rate = format.sample_rate;
    out.samples.resize(count);
    const u8* data = bytes.data() + format.data_offset;

    // Little-endian on the file, whatever the computer.
    for (std::size_t i = 0; i < count; ++i) {
        out.samples[i] = static_cast<s16>(
            static_cast<u16>(data[i * 2]) | static_cast<u16>(data[i * 2 + 1]) << 8
        );
    }

    return out;
}

std::vector<s16> wav_to_mono(const WavAudio& audio, u32 rate) {
    const std::size_t frames = audio.frames();
    std::vector<s16> mono(frames);

    // One channel: stereo's two averaged.
    for (std::size_t f = 0; f < frames; ++f) {
        mono[f] = audio.channels == 1
                      ? audio.samples[f]
                      : static_cast<s16>((audio.samples[f * 2] + audio.samples[f * 2 + 1]) / 2);
    }

    // At its own rate there is nothing more to do.
    if (audio.sample_rate == rate || frames == 0) {
        return mono;
    }

    const double step = static_cast<double>(audio.sample_rate) / rate;
    const auto length = static_cast<std::size_t>(static_cast<double>(frames) / step);
    std::vector<s16> out(std::max<std::size_t>(length, 1));

    // Each output sample lies between two of the file's.
    for (std::size_t i = 0; i < out.size(); ++i) {
        const double position = static_cast<double>(i) * step;
        const auto left = std::min(static_cast<std::size_t>(position), frames - 1);
        const std::size_t right = std::min(left + 1, frames - 1);
        const double fraction = position - static_cast<double>(left);
        const double value = mono[left] + (mono[right] - mono[left]) * fraction;
        out[i] = static_cast<s16>(std::clamp(value, -32768.0, 32767.0));
    }

    return out;
}

}  // namespace openrac::assets

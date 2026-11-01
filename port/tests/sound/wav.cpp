// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// WAV files made here: what a pack may replace a sound with, and what is refused.

#include "assets/sound/wav.h"

#include <cstdint>
#include <vector>

#include "tests/check.h"

using namespace openrac::assets;

namespace {

void put32(std::vector<u8>& v, u32 x) {
    for (int i = 0; i < 4; ++i) {
        v.push_back(static_cast<u8>(x >> (8 * i)));
    }
}

void put16(std::vector<u8>& v, u16 x) {
    v.push_back(static_cast<u8>(x));
    v.push_back(static_cast<u8>(x >> 8));
}

void tag(std::vector<u8>& v, const char* text) {
    v.insert(v.end(), text, text + 4);
}

std::vector<u8> make_wav(
    u16 channels,
    u32 rate,
    const std::vector<s16>& samples,
    u16 bits = 16,
    u16 format = 1,
    bool extra = false
) {
    std::vector<u8> v;
    tag(v, "RIFF");
    put32(v, 0);
    tag(v, "WAVE");

    // Another chunk first, of odd size: its padding byte must be stepped over.
    if (extra) {
        tag(v, "LIST");
        put32(v, 3);
        v.insert(v.end(), {1, 2, 3, 0});
    }

    tag(v, "fmt ");
    put32(v, 16);
    put16(v, format);
    put16(v, channels);
    put32(v, rate);
    put32(v, rate * channels * bits / 8);
    put16(v, static_cast<u16>(channels * bits / 8));
    put16(v, bits);
    tag(v, "data");
    put32(v, static_cast<u32>(samples.size() * 2));

    // Little-endian, whatever the computer.
    for (const s16 s : samples) {
        put16(v, static_cast<u16>(s));
    }

    return v;
}

bool refused(const std::vector<u8>& file) {
    try {
        (void)parse_wav(file);
        return false;
    } catch (const AssetError&) {
        return true;
    }
}

}  // namespace

int main() {
    const std::vector<s16> samples{100, -200, 300, -400};
    const WavAudio stereo = parse_wav(make_wav(2, 44100, samples));
    CHECK(stereo.channels == 2);
    CHECK(stereo.sample_rate == 44100);
    CHECK(stereo.frames() == 2);
    CHECK(stereo.samples == samples);
    CHECK(parse_wav(make_wav(1, 8000, samples, 16, 1, true)).frames() == 4);

    // The format from a file's start alone: the data may be somewhere past what was read.
    const std::vector<u8> file = make_wav(1, 22050, samples);
    const WavFormat format = parse_wav_format(ByteView(file.data(), 44));
    CHECK(format.channels == 1);
    CHECK(format.sample_rate == 22050);
    CHECK(format.data_offset == 44);
    CHECK(format.data_bytes == 8);

    // A header that says more than the file has is cut to the file.
    std::vector<u8> cut = file;
    cut.resize(cut.size() - 4);
    CHECK(parse_wav(cut).frames() == 2);

    CHECK(refused({}));
    CHECK(refused({'R', 'I', 'F', 'F', 0, 0, 0, 0, 'A', 'V', 'I', ' '}));
    CHECK(refused(make_wav(1, 44100, samples, 8)));
    CHECK(refused(make_wav(1, 44100, samples, 16, 3)));
    CHECK(refused(make_wav(3, 44100, samples)));
    CHECK(refused(make_wav(1, 100, samples)));
    CHECK(!refused(make_wav(1, 1030, samples)));  // the disc's slowest effect
    CHECK(refused(make_wav(1, 44100, {})));

    // Mono: stereo's channels averaged, at the rate asked for.
    const std::vector<s16> mono = wav_to_mono(stereo, 44100);
    CHECK((mono == std::vector<s16>{-50, -50}));
    WavAudio ramp;
    ramp.channels = 1;
    ramp.sample_rate = 24000;
    ramp.samples = {0, 100, 200, 300};
    const std::vector<s16> doubled = wav_to_mono(ramp, 48000);
    CHECK(doubled.size() == 8);
    CHECK(doubled[0] == 0);
    CHECK(doubled[1] == 50);
    CHECK(doubled[2] == 100);
    CHECK(doubled[6] == 300);
    ramp.sample_rate = 96000;
    CHECK((wav_to_mono(ramp, 48000) == std::vector<s16>{0, 200}));
    return openrac::test::result();
}

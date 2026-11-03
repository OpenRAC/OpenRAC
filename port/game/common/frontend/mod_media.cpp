// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// mod_media.h: the mods' sound files decoded. A WAV is read here (its RIFF chunks: "fmt " and
// "data"); an Ogg Vorbis through stb_vorbis, which the port already fetches with stb_image
// (viewer/third_party.c compiles it). Both become 16-bit samples for the sound engine.

#include "mod_media.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "openrac/sound_engine.h"

// stb_vorbis's declarations only: its code is compiled in viewer/third_party.c.
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

namespace openrac::frontend {

namespace {

std::uint32_t le(const std::uint8_t* p, int bytes) {
    std::uint32_t v = 0;
    for (int i = bytes - 1; i >= 0; --i) {
        v = v << 8 | p[i];
    }
    return v;
}

// A WAV file's samples as 16-bit: its format chunk's PCM (1) or float (3) or the extensible form
// (0xFFFE) of either, 1 or 2 channels.
bool decode_wav(
    const std::vector<std::uint8_t>& b,
    std::vector<std::int16_t>& out,
    int& channels,
    std::uint32_t& rate
) {
    if (b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) != 0
        || std::memcmp(b.data() + 8, "WAVE", 4) != 0) {
        return false;
    }
    std::uint32_t format = 0, bits = 0;
    channels = 0;
    for (std::size_t at = 12; at + 8 <= b.size();) {
        const std::uint8_t* chunk = b.data() + at;
        const std::size_t size = le(chunk + 4, 4);
        const std::size_t body = at + 8;
        if (size > b.size() - body) {
            return false;
        }
        if (std::memcmp(chunk, "fmt ", 4) == 0 && size >= 16) {
            format = le(chunk + 8, 2);
            channels = static_cast<int>(le(chunk + 10, 2));
            rate = le(chunk + 12, 4);
            bits = le(chunk + 22, 2);
            if (format == 0xFFFE && size >= 26) {
                format = le(chunk + 32, 2);
            }
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            const bool pcm = format == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32);
            const bool flt = format == 3 && bits == 32;
            if ((!pcm && !flt) || channels < 1 || channels > 2 || rate == 0) {
                return false;
            }
            const std::size_t width = bits / 8;
            const std::size_t count = size / width;
            out.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                const std::uint8_t* s = chunk + 8 + i * width;
                float v = 0.0f;
                if (flt) {
                    std::memcpy(&v, s, 4);
                } else if (bits == 8) {
                    v = (static_cast<float>(s[0]) - 128.0f) / 128.0f;
                } else {
                    const auto raw =
                        static_cast<std::int32_t>(le(s, static_cast<int>(width)) << (32 - bits));
                    v = static_cast<float>(raw) / 2147483648.0f;
                }
                v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
                out[i] = static_cast<std::int16_t>(v * 32767.0f);
            }
            return !out.empty();
        }
        at = body + size + (size & 1);
    }
    return false;
}

}  // namespace

int mod_sound_load(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(in), {}};
    if (!in.good() && !in.eof()) {
        return -1;
    }
    std::vector<std::int16_t> samples;
    int channels = 0;
    std::uint32_t rate = 0;
    if (file.extension() == ".ogg") {
        short* decoded = nullptr;
        int r = 0;
        const int frames = stb_vorbis_decode_memory(
            bytes.data(), static_cast<int>(bytes.size()), &channels, &r, &decoded
        );
        // stb_vorbis's buffer, malloc'd: given back as it goes.
        const std::unique_ptr<short, decltype(&std::free)> owned(decoded, &std::free);
        if (frames <= 0 || decoded == nullptr || channels < 1 || channels > 2) {
            return -1;
        }
        samples.assign(decoded, decoded + static_cast<std::size_t>(frames) * channels);
        rate = static_cast<std::uint32_t>(r);
    } else if (!decode_wav(bytes, samples, channels, rate)) {
        return -1;
    }
    return openrac_sound_mod_add(
        samples.data(), samples.size() / static_cast<std::size_t>(channels), channels, rate
    );
}

std::uint32_t mod_sound_play(int sound, float volume, float pitch, bool loop) {
    return openrac_sound_mod_play(sound, volume, pitch, loop ? 1 : 0);
}

int mod_sound_set(std::uint32_t voice, float volume, float pitch) {
    return openrac_sound_mod_set(voice, volume, pitch);
}

void mod_sound_stop(std::uint32_t voice) {
    openrac_sound_mod_stop(voice);
}

}  // namespace openrac::frontend

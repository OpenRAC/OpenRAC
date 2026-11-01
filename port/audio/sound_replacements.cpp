// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sounds packs replace (sound_replacements.h).

#include "audio/sound_replacements.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "assets/disc/checksum.h"
#include "assets/sound/wav.h"
#include "audio/spu.h"
#include "common/log.h"

namespace openrac::audio {

namespace {

// The first eight bytes of the digest: editor/audio.py's key_of writes them in hex.
SoundKey make_key(const assets::disc::Sha256& hash) {
    const assets::disc::Sha256Digest digest = hash.finish();
    SoundKey key = 0;

    // Most significant byte first, as a hex string reads.
    for (std::size_t i = 0; i < 8; ++i) {
        key = key << 8 | digest[i];
    }

    return key;
}

// A file's stem as a key: exactly 16 lower-case hex digits, or nothing.
std::optional<SoundKey> parse_key(std::string_view text) {
    // Any other length is no key.
    if (text.size() != 16) {
        return std::nullopt;
    }

    SoundKey key = 0;

    // A digit at a time.
    for (const char c : text) {
        // Upper-case digits are refused, as editor/overrides.py refuses them: one name for a file.
        if (c >= '0' && c <= '9') {
            key = key << 4 | static_cast<SoundKey>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            key = key << 4 | static_cast<SoundKey>(c - 'a' + 10);
        } else {
            return std::nullopt;
        }
    }

    return key;
}

}  // namespace

SoundKey stream_key(std::span<const std::uint8_t> start) {
    constexpr std::size_t kHeader = 0x30;
    // The body's size in the header, big-endian: without a header, only what there is.
    const std::size_t size = start.size() < kHeader
                                 ? 0
                                 : std::size_t{start[12]} << 24 | std::size_t{start[13]} << 16
                                       | std::size_t{start[14]} << 8 | start[15];
    assets::disc::Sha256 hash;
    hash.update(start.first(std::min({start.size(), kHeader + size, kStreamKeyBytes})));
    return make_key(hash);
}

SoundKey sound_key(const assets::SoundBank& bank, std::size_t sound) {
    assets::disc::Sha256 hash;
    const assets::ByteView samples(bank.samples);

    // Each grain in the sound's order, as little-endian words on every computer.
    for (const assets::Grain& grain : bank.sound_grains(sound)) {
        const auto kind = static_cast<std::uint32_t>(grain.type);
        const auto delay = static_cast<std::uint32_t>(grain.delay);
        const std::array<std::uint8_t, 8> head{
            static_cast<std::uint8_t>(kind),
            static_cast<std::uint8_t>(kind >> 8),
            static_cast<std::uint8_t>(kind >> 16),
            static_cast<std::uint8_t>(kind >> 24),
            static_cast<std::uint8_t>(delay),
            static_cast<std::uint8_t>(delay >> 8),
            static_cast<std::uint8_t>(delay >> 16),
            static_cast<std::uint8_t>(delay >> 24),
        };
        hash.update(head);

        // A tone's offset says where its sample is in this bank, so its bytes count instead.
        if (grain.is_tone()) {
            hash.update(std::span<const std::uint8_t>(grain.data).first(16));
            const assets::SampleExtent extent =
                assets::sample_extent(samples, grain.tone().sample_offset);
            hash.update(
                std::span<const std::uint8_t>(bank.samples).subspan(extent.offset, extent.bytes())
            );
        } else {
            hash.update(grain.data);
        }
    }

    return make_key(hash);
}

SoundReplacements SoundReplacements::read(const std::filesystem::path& mods) {
    SoundReplacements out;
    out.m_folder = mods / "audio";
    std::error_code absent;

    // Each file of the folder, if there is one: most mods folders replace no sound.
    for (const auto& entry : std::filesystem::directory_iterator(out.m_folder, absent)) {
        const std::filesystem::path& path = entry.path();

        // Only a sound's own file counts: its name is all that says which sound it is.
        if (path.extension() == ".wav") {
            if (const std::optional<SoundKey> key = parse_key(path.stem().string())) {
                out.m_keys.insert(*key);
            }
        }
    }

    // Said once for each mods folder, as the session log's answer to whether a pack was read.
    if (!out.m_keys.empty()) {
        log::info("audio mods: {} sounds replaced", out.m_keys.size());
    }

    return out;
}

std::filesystem::path SoundReplacements::file(SoundKey key) const {
    return m_folder / std::format("{:016x}.wav", key);
}

SampleData SoundReplacements::sample(SoundKey key, bool loop) const {
    const auto known = m_samples.find({key, loop});

    // Encoded before, or refused before: the answer stands for this mods folder.
    if (known != m_samples.end()) {
        return known->second;
    }

    SampleData out;

    try {
        std::ifstream input(file(key), std::ios::binary);
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()
        };
        const std::vector<std::int16_t> mono =
            assets::wav_to_mono(assets::parse_wav(bytes), kOutputRate);
        out = std::make_shared<const std::vector<std::uint8_t>>(assets::encode_adpcm(mono, loop));
    } catch (const std::exception& e) {
        log::warn(
            "audio mods: {}: {}; the sound stays the game's own",
            file(key).filename().string(),
            e.what()
        );
    }

    m_samples.emplace(std::pair{key, loop}, out);
    return out;
}

BankReplacements SoundReplacements::bank(const assets::SoundBank& bank) const {
    BankReplacements out;

    // Without replacements there is nothing to hash.
    if (m_keys.empty()) {
        return out;
    }

    const assets::ByteView samples(bank.samples);

    // Each sound of the bank, by what it is.
    for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
        const SoundKey key = sound_key(bank, i);

        // Most of a bank's sounds are not replaced.
        if (!m_keys.contains(key)) {
            continue;
        }

        // A new sample loops if the first tone's did, the one its file was exported from.
        const auto grains = bank.sound_grains(i);
        const auto first = std::find_if(grains.begin(), grains.end(), [](const assets::Grain& g) {
            return g.is_tone();
        });
        const bool looped = first != grains.end()
                            && assets::sample_extent(samples, first->tone().sample_offset).looped;

        // A file that could not be used leaves the sound as it is.
        if (SampleData new_sample = sample(key, looped)) {
            out.emplace(i, std::move(new_sample));
        }
    }

    return out;
}

std::filesystem::path SoundReplacements::stream(std::span<const std::uint8_t> start) const {
    const SoundKey key = stream_key(start);
    return m_keys.contains(key) ? file(key) : std::filesystem::path{};
}

}  // namespace openrac::audio

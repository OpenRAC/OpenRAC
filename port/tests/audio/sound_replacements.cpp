// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sounds packs replace, on banks and folders made here: the keys, with the answers
// editor/test_audio.py has for the same bytes; a new sample as a voice plays it; what is left alone.

#include "audio/sound_replacements.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

#include "assets/bytes.h"
#include "tests/check.h"

using namespace openrac;
using namespace openrac::audio;
using assets::ByteWriter;

namespace {

constexpr std::uint32_t kTone = 1;
constexpr std::uint32_t kStop = 24;
constexpr std::uint32_t kRandPlay = 25;
constexpr std::uint32_t kKeyOff = 41;
constexpr std::uint32_t kOnStop = 43;

// The same answers as editor/test_audio.py's for the same bytes.
constexpr SoundKey kSoundKey = 0x3d5f9cc75b576b92;
constexpr SoundKey kStreamKey = 0xe9b65507686eaed0;

struct GrainSpec {
    std::uint32_t type;
    std::int32_t delay = 0;
    std::uint8_t sample = 16;  // a tone's: 0 the first frame, 16 the second
};

// editor/test_audio.py's bank_file: a frame that ends by itself and is not the sound's, then one that
// is (silent, with these flags), and one sound of the grains given.
std::vector<std::uint8_t> make_bank(
    const std::vector<GrainSpec>& grains, std::uint8_t last_flags = 1
) {
    std::vector<std::uint8_t> samples(32, 0);
    samples[1] = 1;
    samples[17] = last_flags;
    const std::uint32_t first_sound = 0x3c;
    const std::uint32_t first_grain = 0x48;
    ByteWriter block;
    block.put_bytes(std::vector<std::uint8_t>{'S', 'B', 'l', 'k'});
    block.put<std::uint32_t>(1);
    block.put_at<std::int16_t>(0x16, 1);
    block.put_at<std::int16_t>(0x18, static_cast<std::int16_t>(grains.size()));
    block.put_at<std::int16_t>(0x1a, 1);
    block.put_at<std::uint32_t>(0x1c, first_sound);
    block.put_at<std::uint32_t>(0x20, first_grain);
    block.resize(first_sound);
    block.put<std::int8_t>(127);
    block.put<std::int8_t>(0);
    block.put<std::int16_t>(0);
    block.put(static_cast<std::uint8_t>(grains.size()));
    block.put<std::int8_t>(0);
    block.put<std::uint16_t>(0);
    block.put<std::uint32_t>(0);

    // Each grain: its type and delay, then its 32 bytes.
    for (const GrainSpec& g : grains) {
        block.put(g.type);
        block.put(g.delay);
        std::vector<std::uint8_t> data(32, 0);

        // Only a tone says anything here.
        if (g.type == kTone) {
            data[0] = 5;
            data[1] = 100;
            data[2] = static_cast<std::uint8_t>(static_cast<std::int8_t>(-60));
            data[7] = 127;
            data[8] = 2;
            data[9] = 2;
            data[10] = 0xff;
            data[12] = 0xc0;
            data[13] = 0x1f;
            data[16] = g.sample;
        }

        block.put_bytes(data);
    }

    ByteWriter file;
    const auto block_size = static_cast<std::uint32_t>(block.size());

    for (const std::uint32_t word : {3u, 2u, 0x18u, block_size, 0x18u + block_size, 32u}) {
        file.put(word);
    }

    file.put_bytes(block.bytes());
    file.put_bytes(samples);
    return file.bytes();
}

std::vector<std::uint8_t> stream_sector() {
    std::vector<std::uint8_t> sector(2048, 0);
    const std::vector<std::uint8_t> head{'V', 'A', 'G', 'p', 0, 0,   0, 0x20, 0,    0,
                                         0,   0,   0,   0,   0, 160, 0, 0,    0xac, 0x44};
    std::copy(head.begin(), head.end(), sector.begin());
    const std::string name = "L01_Enemy";
    std::copy(name.begin(), name.end(), sector.begin() + 0x20);
    return sector;
}

std::string hex(SoundKey key) {
    return std::format("{:016x}", key);
}

void write_text(const std::filesystem::path& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

// A mono file at 48 kHz: a square wave, something other than the bank's silence.
void write_wav(const std::filesystem::path& path, std::size_t samples = 280) {
    std::string out = "RIFF    WAVEfmt ";
    const auto put32 = [&](std::uint32_t x) {
        for (int i = 0; i < 4; ++i) {
            out.push_back(static_cast<char>(x >> (8 * i)));
        }
    };
    const auto put16 = [&](std::uint16_t x) {
        out.push_back(static_cast<char>(x));
        out.push_back(static_cast<char>(x >> 8));
    };
    put32(16);
    put16(1);
    put16(1);
    put32(48000);
    put32(96000);
    put16(2);
    put16(16);
    out += "data";
    put32(static_cast<std::uint32_t>(samples * 2));

    // Half a period of each.
    for (std::size_t i = 0; i < samples; ++i) {
        put16(static_cast<std::uint16_t>(i % 40 < 20 ? 12000 : -12000));
    }

    write_text(path, out);
}

// How far the player's output goes from silence over `frames`.
int loudest(SoundPlayer& player, std::size_t frames) {
    std::vector<std::int16_t> out(frames * 2, 0);
    player.render(out);
    int most = 0;

    // Either side.
    for (const std::int16_t s : out) {
        most = std::max(most, std::abs(static_cast<int>(s)));
    }

    return most;
}

}  // namespace

int main() {
    // The keys: the same as the export's.
    const std::vector<std::uint8_t> bank_file = make_bank({{kTone}, {kStop, 3}});
    const assets::SoundBank bank = assets::parse_sound_bank(bank_file);
    CHECK(sound_key(bank, 0) == kSoundKey);
    CHECK(stream_key(stream_sector()) == kStreamKey);
    std::vector<std::uint8_t> more = stream_sector();
    more.push_back(0xff);
    CHECK(stream_key(more) == kStreamKey);

    // Recordings that start alike are different sounds; past the first megabyte it no longer counts.
    {
        std::vector<std::uint8_t> long_file(0x30 + 40000, 0);
        std::copy_n(stream_sector().begin(), 0x30, long_file.begin());
        long_file[13] = 0;
        long_file[14] = 0x9c;
        long_file[15] = 0x40;
        std::vector<std::uint8_t> other = long_file;
        other[30000] = 1;
        CHECK(stream_key(long_file) != stream_key(other));

        std::vector<std::uint8_t> longest(kStreamKeyBytes + 100, 0);
        std::copy_n(long_file.begin(), 0x30, longest.begin());
        longest[13] = 0x10;
        longest[14] = 0;
        longest[15] = 100;
        std::vector<std::uint8_t> later = longest;
        later.back() = 1;
        CHECK(stream_key(longest) == stream_key(later));
    }

    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path mods = std::filesystem::temp_directory_path()
                                       / ("openrac_sound_replacements_" + std::to_string(now));

    // No folder, or one with nothing that names a sound: nothing changes.
    CHECK(SoundReplacements::read(mods).empty());
    std::filesystem::create_directories(mods / "audio");
    CHECK(SoundReplacements::read(mods).empty());
    write_text(mods / "audio" / "readme.txt", "words");
    write_text(mods / "audio" / "not-a-key.wav", "");
    write_text(mods / "audio" / "ABCDEF0123456789.wav", "");
    write_text(mods / "audio" / (hex(kSoundKey) + ".txt"), "");
    CHECK(SoundReplacements::read(mods).empty());

    // A stream's file is named after its key; another stream has none.
    write_wav(mods / "audio" / (hex(kStreamKey) + ".wav"));
    {
        const SoundReplacements replacements = SoundReplacements::read(mods);
        CHECK(!replacements.empty());
        CHECK(replacements.stream(stream_sector()) == mods / "audio" / (hex(kStreamKey) + ".wav"));
        std::vector<std::uint8_t> other = stream_sector();
        other[100] = 1;
        CHECK(replacements.stream(other).empty());
        CHECK(replacements.bank(bank).empty());
    }

    // A new sample in a real bank, played through to the output: the old sample is silence.
    write_wav(mods / "audio" / (hex(kSoundKey) + ".wav"));
    {
        const SoundReplacements replacements = SoundReplacements::read(mods);
        const BankReplacements found = replacements.bank(bank);
        CHECK(found.size() == 1);
        CHECK(found.at(0) != nullptr);
        CHECK(replacements.bank(bank).at(0) == found.at(0));

        SoundPlayer player;
        const BankHandle plain = player.load_bank(bank_file);
        player.play(plain, 0);
        CHECK(loudest(player, 400) == 0);
        player.stop_all();
        const BankHandle replaced = player.load_bank(bank_file, found);
        CHECK(player.bank(replaced)->samples.size() > bank.samples.size());
        CHECK(player.play(replaced, 0) != 0);
        CHECK(loudest(player, 400) > 2000);
        player.stop_all();
    }

    // A looped sound's new sample loops; a one-shot's ends.
    {
        const assets::SoundBank looped =
            assets::parse_sound_bank(make_bank({{kTone}, {kStop, 3}}, 7));
        write_wav(mods / "audio" / (hex(sound_key(looped, 0)) + ".wav"));
        const SoundReplacements replacements = SoundReplacements::read(mods);
        CHECK(assets::sample_extent(*replacements.bank(looped).at(0), 0).looped);
        CHECK(!assets::sample_extent(*replacements.bank(bank).at(0), 0).looped);
    }

    // A loop and then a one-shot to end it: the new sample is the first's, and the sound still ends.
    {
        const std::vector<std::uint8_t> file =
            make_bank({{kTone}, {kKeyOff, 240}, {kTone, 0, 0}, {kStop, 3}}, 7);
        const assets::SoundBank mixed = assets::parse_sound_bank(file);
        write_wav(mods / "audio" / (hex(sound_key(mixed, 0)) + ".wav"));
        const BankReplacements found = SoundReplacements::read(mods).bank(mixed);
        CHECK(assets::sample_extent(*found.at(0), 0).looped);
        SoundPlayer player;
        const BankHandle handle = player.load_bank(file, found);
        CHECK(player.bank(handle)->grains[0].tone().sample_offset == 32);
        CHECK(player.bank(handle)->grains[2].tone().sample_offset == 0);
        CHECK(player.bank(handle)->grains[2].tone().volume == 0);
        const SoundHandle sound = player.play(handle, 0);
        CHECK(loudest(player, 400) > 2000);
        loudest(player, 48000 * 3);
        CHECK(!player.playing(sound));
        CHECK(loudest(player, 400) == 0);
    }

    // A one-shot first: the new sample ends, and the loop after it stays silent on its own.
    {
        const std::vector<std::uint8_t> file =
            make_bank({{kTone, 0, 0}, {kTone, 240}, {kKeyOff, 240}, {kStop, 3}}, 7);
        const assets::SoundBank mixed = assets::parse_sound_bank(file);
        write_wav(mods / "audio" / (hex(sound_key(mixed, 0)) + ".wav"));
        const BankReplacements found = SoundReplacements::read(mods).bank(mixed);
        CHECK(!assets::sample_extent(*found.at(0), 0).looped);
        SoundPlayer player;
        const BankHandle handle = player.load_bank(file, found);
        CHECK(player.bank(handle)->grains[0].tone().sample_offset == 32);
        CHECK(player.bank(handle)->grains[1].tone().sample_offset == 16);
        CHECK(player.bank(handle)->grains[1].tone().volume == 0);
    }

    // A file that is no sound leaves the sound its own, and the game goes on.
    write_text(mods / "audio" / (hex(kSoundKey) + ".wav"), "not a wav");
    CHECK(SoundReplacements::read(mods).bank(bank).empty());

    // Which tones a new sample reaches.
    {
        std::vector<std::uint8_t> sample(16, 0);
        sample[1] = 1;

        // Two tones of one moment: only the first is heard, as both would be the same sample.
        assets::SoundBank layered =
            assets::parse_sound_bank(make_bank({{kTone}, {kTone}, {kStop, 3}}));
        replace_sound_sample(layered, 0, sample);
        CHECK(layered.grains[0].tone().volume == 100);
        CHECK(layered.grains[1].tone().volume == 0);
        CHECK(layered.grains[0].tone().sample_offset == 32);
        CHECK(layered.grains[0].tone().center_note == -60);
        CHECK(layered.grains[0].tone().center_fine == 0);
        CHECK(layered.samples.size() == 48);

        // Tones one after the other all play it.
        assets::SoundBank turns =
            assets::parse_sound_bank(make_bank({{kTone}, {kTone, 240}, {kStop, 3}}));
        replace_sound_sample(turns, 0, sample);
        CHECK(turns.grains[1].tone().volume == 100);

        // A script that picks one needs them all.
        assets::SoundBank picked =
            assets::parse_sound_bank(make_bank({{kRandPlay}, {kTone}, {kTone}, {kStop, 3}}));
        replace_sound_sample(picked, 0, sample);
        CHECK(picked.grains[1].tone().volume == 100);
        CHECK(picked.grains[2].tone().volume == 100);
        CHECK(picked.grains[2].tone().sample_offset == 32);

        // A tone after the ones before it were told to end starts a moment of its own.
        assets::SoundBank ended =
            assets::parse_sound_bank(make_bank({{kTone}, {kKeyOff}, {kTone}, {kStop, 3}}));
        replace_sound_sample(ended, 0, sample);
        CHECK(ended.grains[2].tone().volume == 100);

        // So does the first after the stop marker; one that starts with it is still silent.
        assets::SoundBank stopping =
            assets::parse_sound_bank(make_bank({{kTone}, {kOnStop}, {kTone}, {kTone}}));
        replace_sound_sample(stopping, 0, sample);
        CHECK(stopping.grains[2].tone().volume == 100);
        CHECK(stopping.grains[3].tone().volume == 0);
    }

    std::error_code ignored;
    std::filesystem::remove_all(mods, ignored);
    return test::result();
}

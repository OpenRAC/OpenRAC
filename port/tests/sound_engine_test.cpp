// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The VAG streams of the game's sound engine (port/game/common/lib/sound_engine.cpp) on a disc
// image made here: the time a stream has left, from the frame it starts on, which RAC1's music
// crossfade measures its fade against (games/rac1/src/game/music.c, music_Update).

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "assets/bytes.h"
#include "audio/sound_replacements.h"
#include "openrac/sound_engine.h"
#include "tests/check.h"

extern "C" const char* openrac_game_disc_image;
extern "C" const char* openrac_game_mods_dir;
extern "C" unsigned openrac_game_mods_generation;
const char* openrac_game_disc_image = nullptr;
const char* openrac_game_mods_dir = nullptr;
unsigned openrac_game_mods_generation = 0;

namespace {

constexpr std::uint32_t kRate = 48000;
constexpr std::uint32_t kFrames = 2000;  // 16-byte ADPCM frames: 56,000 samples, 1,166 ms
constexpr std::int32_t kWholeMs = static_cast<std::int32_t>(kFrames * 28ull * 1000 / kRate);

void put_be32(std::vector<std::uint8_t>& v, std::size_t at, std::uint32_t x) {
    for (int i = 0; i < 4; ++i) {
        v[at + i] = static_cast<std::uint8_t>(x >> (24 - 8 * i));
    }
}

// A VAG header at `sector` announcing kFrames frames, with `body_frames` of them written (silence).
void put_vag(std::vector<std::uint8_t>& disc, std::uint32_t sector, std::uint32_t body_frames) {
    const std::size_t at = static_cast<std::size_t>(sector) * 2048;
    disc.resize(std::max(disc.size(), at + 0x30 + body_frames * 16), 0);
    disc[at] = 'V';
    disc[at + 1] = 'A';
    disc[at + 2] = 'G';
    disc[at + 3] = 'p';
    put_be32(disc, at + 4, 0x20);
    put_be32(disc, at + 12, kFrames * 16);
    put_be32(disc, at + 16, kRate);
}

void render_ms(int ms) {
    std::vector<std::int16_t> out(static_cast<std::size_t>(kRate / 1000 * ms) * 2);
    openrac_sound_render(out.data(), out.size() / 2);
}

std::vector<std::uint8_t> make_test_bank() {
    openrac::assets::ByteWriter block;
    block.put_bytes(std::vector<std::uint8_t>{'S', 'B', 'l', 'k'});
    block.put<std::uint32_t>(1);
    block.put_at<std::int16_t>(0x16, 1);
    block.put_at<std::int16_t>(0x18, 1);
    block.put_at<std::int16_t>(0x1a, 1);
    block.put_at<std::uint32_t>(0x1c, 0x3c);
    block.put_at<std::uint32_t>(0x20, 0x48);
    block.resize(0x3c);
    block.put<std::uint8_t>(127);
    block.put<std::uint8_t>(0);
    block.put<std::int16_t>(0);
    block.put<std::uint8_t>(1);
    block.put<std::int8_t>(0);
    block.put<std::uint16_t>(0);
    block.put<std::uint32_t>(0);
    block.put<std::uint32_t>(24);  // stop grain
    block.put<std::int32_t>(0);
    block.put_bytes(std::vector<std::uint8_t>(32));

    openrac::assets::ByteWriter file;
    const auto block_size = static_cast<std::uint32_t>(block.size());

    for (const std::uint32_t word : {3u, 2u, 0x18u, block_size, 0x18u + block_size, 16u}) {
        file.put(word);
    }

    file.put_bytes(block.bytes());
    file.put_bytes(std::vector<std::uint8_t>(16));
    return file.bytes();
}

}  // namespace

int main() {
    std::vector<std::uint8_t> disc;
    put_vag(disc, 4, kFrames);  // a whole track
    put_vag(disc, 200, 0);      // a header whose body is past the image's end
    disc.resize(200 * 2048 + 0x30);
    const std::filesystem::path path =
        std::filesystem::temp_directory_path()
        / ("openrac_sound_engine_test_"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".iso");
    {
        std::FILE* f = std::fopen(path.string().c_str(), "wb");
        CHECK(f != nullptr);

        if (f == nullptr) {
            return openrac::test::result();
        }

        std::fwrite(disc.data(), 1, disc.size(), f);
        std::fclose(f);
    }
    const std::string image = path.string();
    openrac_game_disc_image = image.c_str();

    // Read on its own thread, as in play: the answer the frame it starts is the whole track,
    // decoded or not (it was a made-up minute while the decode ran).
    openrac_stream_read_at_once(0);
    const std::uint32_t h = openrac_stream_play(4, 0x400, 1, 0, 0x20);
    CHECK(h != 0);
    const std::int32_t first = openrac_stream_remaining_ms(h);
    CHECK(first == kWholeMs);

    for (int i = 0; i < 500 && !openrac_stream_buffered(h); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CHECK(openrac_stream_buffered(h));
    CHECK(openrac_stream_remaining_ms(h) == first);

    // The game's crossfade (music_Update, case 2): the old music at vol * (interval - played) /
    // interval, interval a quarter of the first answer. An eighth in, it is at half, not cut.
    const std::int32_t interval = first / 4;
    render_ms(first / 8);
    const std::int32_t now = openrac_stream_remaining_ms(h);
    CHECK(now <= first - first / 8 + 1 && now >= first - first / 8 - 1);
    const std::int32_t fade = 0x400 * (interval - (first - now)) / interval;
    CHECK(fade > 0x400 / 2 - 0x20 && fade < 0x400 / 2 + 0x20);
    openrac_stream_stop(h);
    CHECK(!openrac_stream_live(h));

    // Not decoded (here never: its body is not on the image): still the header's length.
    openrac_stream_read_at_once(1);
    const std::uint32_t broken = openrac_stream_play(200, 0x400, 1, 0, 0x20);
    CHECK(openrac_stream_remaining_ms(broken) == kWholeMs);
    openrac_stream_stop(0);

    std::vector<std::uint8_t> audio_snapshot(openrac_sound_snapshot_size());
    const auto bank_file = make_test_bank();
    const std::uint32_t bank = openrac_sound_load_bank(bank_file.data(), bank_file.size());
    CHECK(bank != 0);
    const std::uint32_t sound = openrac_sound_play(bank, 0, 0x400, -1, 0, 0);
    CHECK(sound != 0);
    CHECK(openrac_sound_snapshot_save(audio_snapshot.data(), audio_snapshot.size()) == 0);
    openrac_sound_unload_bank(bank);
    CHECK(openrac_sound_snapshot_load(audio_snapshot.data(), audio_snapshot.size()) == 0);
    CHECK(openrac_sound_play(bank, 0, 0x400, -1, 0, 0) != 0);
    CHECK(!openrac_stream_live(broken));

    // A pack's WAV file in place of a stream, from a mods folder: found by the stream's first sector,
    // whatever the sector's number is.
    {
        const std::uint32_t loud_sector = 300;
        put_vag(disc, loud_sector, kFrames);
        const std::size_t body = static_cast<std::size_t>(loud_sector) * 2048 + 0x30;
        disc.resize(body + kFrames * 16, 0);

        // Loud frames: shift 0, filter 0, the nibbles alternating.
        for (std::size_t f = 0; f < kFrames; ++f) {
            for (std::size_t i = 2; i < 16; ++i) {
                disc[body + f * 16 + i] = 0x4c;
            }
        }

        std::FILE* again = std::fopen(image.c_str(), "wb");
        CHECK(again != nullptr);

        if (again != nullptr) {
            std::fwrite(disc.data(), 1, disc.size(), again);
            std::fclose(again);
        }

        // As editor/audio.py works it out, from the stream's own bytes.
        const std::string key = std::format(
            "{:016x}",
            openrac::audio::stream_key(
                std::span<const std::uint8_t>(disc).subspan(
                    static_cast<std::size_t>(loud_sector) * 2048
                )
            )
        );
        const std::filesystem::path mods = path.string() + ".mods";
        std::filesystem::create_directories(mods / "audio");
        const std::string folder = mods.string();
        const auto use_mods = [&] {
            openrac_game_mods_dir = folder.c_str();
            ++openrac_game_mods_generation;
        };
        const auto peak = [&](std::uint32_t playing) {
            std::vector<std::int16_t> out(2000);
            openrac_sound_render(out.data(), out.size() / 2);
            int most = 0;

            for (const std::int16_t sample : out) {
                most = std::max(most, std::abs(static_cast<int>(sample)));
            }

            openrac_stream_stop(playing);
            return most;
        };
        openrac_stream_read_at_once(1);

        // Without a file it plays as it did.
        use_mods();
        const std::uint32_t plain = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        const std::int32_t plain_ms = openrac_stream_remaining_ms(plain);
        const int plain_peak = peak(plain);
        CHECK(plain_peak > 1000);

        // A stereo file in its place: its own length, and each side its own.
        std::string wav = "RIFF    WAVEfmt ";
        {
            const auto put32 = [&](std::uint32_t x) {
                for (int i = 0; i < 4; ++i) {
                    wav.push_back(static_cast<char>(x >> (8 * i)));
                }
            };
            const auto put16 = [&](std::uint16_t x) {
                wav.push_back(static_cast<char>(x));
                wav.push_back(static_cast<char>(x >> 8));
            };
            put32(16);
            put16(1);
            put16(2);
            put32(48000);
            put32(192000);
            put16(4);
            put16(16);
            wav += "data";
            put32(48000 * 4);

            for (int i = 0; i < 48000; ++i) {
                put16(static_cast<std::uint16_t>(8000));
                put16(static_cast<std::uint16_t>(-16000));
            }

            std::ofstream(mods / "audio" / (key + ".wav"), std::ios::binary) << wav;
        }
        use_mods();
        const std::uint32_t replaced = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        CHECK(openrac_stream_remaining_ms(replaced) == 1000);
        std::vector<std::int16_t> sides(200);
        openrac_sound_render(sides.data(), sides.size() / 2);
        CHECK(sides[100] == 8000);
        CHECK(sides[101] == -16000);
        openrac_stream_stop(replaced);

        // Read on its own thread as in play: the header's length, then the file's sound.
        openrac_stream_read_at_once(0);
        const std::uint32_t threaded = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        CHECK(openrac_stream_remaining_ms(threaded) == 1000);

        for (int i = 0; i < 500 && !openrac_stream_buffered(threaded); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        CHECK(openrac_stream_buffered(threaded));
        openrac_stream_stop(threaded);
        openrac_stream_read_at_once(1);

        // A long chunk before its sound, as an editor's notes can be: played all the same.
        wav.insert(
            12, std::string("LIST\x70\x11\x01", 7) + std::string(1, '\0') + std::string(70000, '\0')
        );
        std::ofstream(mods / "audio" / (key + ".wav"), std::ios::binary) << wav;
        use_mods();
        const std::uint32_t long_head = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        openrac_sound_render(sides.data(), sides.size() / 2);
        CHECK(sides[100] == 8000);
        openrac_stream_stop(long_head);

        // Its length is known before it is read: the game's fades go by the first answer.
        openrac_stream_read_at_once(0);
        const std::uint32_t long_read = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        CHECK(openrac_stream_remaining_ms(long_read) == 1000);

        for (int i = 0; i < 500 && !openrac_stream_buffered(long_read); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        openrac_stream_stop(long_read);
        openrac_stream_read_at_once(1);

        // A file that is no sound: the stream stays its own, at its own length.
        std::ofstream(mods / "audio" / (key + ".wav"), std::ios::binary) << "not a wav";
        use_mods();
        const std::uint32_t kept_own = openrac_stream_play(loud_sector, 0x400, 1, 0, 0x20);
        CHECK(openrac_stream_remaining_ms(kept_own) == plain_ms);
        CHECK(peak(kept_own) == plain_peak);

        // No folder any more: as it was.
        openrac_game_mods_dir = nullptr;
        ++openrac_game_mods_generation;
        std::error_code ignored_folder;
        std::filesystem::remove_all(mods, ignored_folder);
    }

    // The program is leaving: the readers are stopped and waited for, and no track is read
    // after that (a reader that is still working when the program exits is a crash at quit).
    openrac_stream_read_at_once(0);
    const std::uint32_t late = openrac_stream_play(4, 0x400, 1, 0, 0x20);
    CHECK(late != 0);
    openrac_sound_stop_readers();
    // Stopping again is harmless, and the track is not read after the stop.
    openrac_sound_stop_readers();
    const std::uint32_t after = openrac_stream_play(4, 0x400, 1, 0, 0x20);
    CHECK(after != 0);
    CHECK(!openrac_stream_buffered(after));
    openrac_stream_stop(0);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return openrac::test::result();
}

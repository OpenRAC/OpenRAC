// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac/sound_engine.h: one SoundPlayer for the game, and the VAG streams (music, speech),
// behind a lock, since the audio output renders on its own thread.

#include "openrac/sound_engine.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

#include "assets/sound/adpcm.h"
#include "assets/sound/vag.h"
#include "assets/sound/wav.h"
#include "audio/limiter.h"
#include "audio/reverb.h"
#include "audio/sound_player.h"
#include "audio/sound_replacements.h"
#include "common/env.h"
#include "common/log.h"
#include "openrac/game_host.h"

namespace {

using namespace openrac;

std::mutex g_lock;
// openrac_stream_read_at_once: streams read on the caller's thread.
bool g_read_at_once = false;

// The threads reading a track, and the flag that tells them the program is leaving. A track of
// minutes is read on a thread of its own, and the player may close the window while one is
// reading: the program then waits for them (openrac_sound_stop_readers), so that the globals
// this file has are not taken away under a thread that is using them.
// A reader that has finished says so (done), and the next track's start joins it: a thread
// that ended is not left unjoined, holding its stack, for the rest of the session.
struct Reader {
    std::thread thread;
    std::shared_ptr<std::atomic<bool>> done;
};

std::vector<Reader> g_readers;
bool g_stopping = false;

// `size` bytes of the disc at byte `at`, from the extractor's files (game_host.h's
// openrac_disc_read): the music, speech and banks are read by sector, as the game names them.
bool read_disc(std::uint64_t at, void* dst, std::size_t size) {
    return openrac_disc_read(at, dst, size) != 0;
}

// Never destroyed: the output's thread may still render while the program exits.
audio::SoundPlayer& player() {
    static auto* p = new audio::SoundPlayer;
    return *p;
}

// The master volume of each group, as the game last set it (the streams apply it themselves).
std::array<std::int32_t, audio::kVolumeGroups> g_master = [] {
    std::array<std::int32_t, audio::kVolumeGroups> a{};
    a.fill(audio::kFullVolume);
    return a;
}();

// The port's own gain on each group's streams (openrac_sound_set_stream_gain): where it heads, set
// from any thread, and where the mix has got to, moved a little each mixed frame.
struct StreamGainTargets {
    StreamGainTargets() {
        for (auto& g : gains) {
            g.store(1.0f, std::memory_order_relaxed);
        }
    }

    std::atomic<float>& operator[](std::size_t group) { return gains[group]; }

    std::array<std::atomic<float>, audio::kVolumeGroups> gains;
};

StreamGainTargets g_stream_gain_target;
std::array<double, audio::kVolumeGroups> g_stream_gain = [] {
    std::array<double, audio::kVolumeGroups> a{};
    a.fill(1.0);
    return a;
}();
// How far the eased gain moves in one output frame: all the way in a tenth of a second.
constexpr double kStreamGainStep = 10.0 / audio::kOutputRate;
std::map<audio::BankHandle, std::vector<std::uint8_t>> g_bank_files;

// The sounds packs replace (audio/sound_replacements.h), read again when the mods folder changes
// (a ship destination has its own). Encoding a new sample takes a moment: this has its own lock, so
// that the output's thread never waits for it.
std::mutex g_replacements_lock;
audio::SoundReplacements g_replacements;
bool g_replacements_read = false;
unsigned g_replacements_generation = 0;

// The replacements of the mods folder in use; g_replacements_lock held.
const audio::SoundReplacements& current_replacements() {
    // Read once for each mods folder.
    if (!g_replacements_read || g_replacements_generation != openrac_game_mods_generation) {
        g_replacements_read = true;
        g_replacements_generation = openrac_game_mods_generation;
        g_replacements = openrac_game_mods_dir != nullptr
                             ? audio::SoundReplacements::read(openrac_game_mods_dir)
                             : audio::SoundReplacements{};
    }

    return g_replacements;
}

// The new samples for the sounds of a bank file. Before g_lock: a new sample takes a moment.
audio::BankReplacements replacements_of_bank(std::span<const std::uint8_t> file) {
    std::lock_guard lock(g_replacements_lock);
    const audio::SoundReplacements& replacements = current_replacements();

    // Nothing to read the bank for.
    if (replacements.empty()) {
        return {};
    }

    try {
        audio::BankReplacements found = replacements.bank(assets::parse_sound_bank(file));

        // Said once a bank, for whoever wonders whether a pack reached this disc's sounds.
        if (!found.empty()) {
            log::info("audio mods: {} sounds of this bank replaced", found.size());
        }

        return found;
    } catch (const std::exception& e) {
        log::warn("audio mods: a bank left as it is: {}", e.what());
        return {};
    }
}

// ---- What is heard ----

// The frames openrac_sound_render has rendered, and the output's count of those it has not played
// yet (openrac_sound_set_unheard; none: every frame is played as it is rendered).
std::uint64_t g_rendered = 0;
std::size_t (*g_unheard)(void*) = nullptr;
void* g_unheard_user = nullptr;

// Sounds and streams that ended in frames rendered and not heard yet, with the frame count after
// which they are heard to have ended. Kept only while the output mixes ahead.
struct Ended {
    std::uint32_t handle;
    std::uint64_t at;
};

std::vector<Ended> g_ended;

// The frames rendered that the output has played; under g_lock.
std::uint64_t heard_frames() {
    if (g_unheard == nullptr) {
        return g_rendered;
    }

    return g_rendered - std::min<std::uint64_t>(g_unheard(g_unheard_user), g_rendered);
}

// Whether `handle` ended in frames not heard yet; forgets the ones heard. Under g_lock.
const Ended* ended_unheard(std::uint32_t handle) {
    if (g_ended.empty()) {
        return nullptr;
    }

    const std::uint64_t heard = heard_frames();
    std::erase_if(g_ended, [heard](const Ended& e) { return e.at <= heard; });
    const auto found = std::find_if(g_ended.begin(), g_ended.end(), [handle](const Ended& e) {
        return e.handle == handle;
    });
    return found == g_ended.end() ? nullptr : &*found;
}

void forget_ended(std::uint32_t handle) {
    std::erase_if(g_ended, [handle](const Ended& e) { return e.handle == handle; });
}

struct SoundSnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t used;
    std::uint32_t bank_count;
    audio::BankHandle next_bank;
    audio::SoundHandle next_sound;
    std::array<std::int32_t, audio::kVolumeGroups> master;
};

struct SoundSnapshotBank {
    audio::BankHandle handle;
    std::uint32_t size;
};

// ---- VAG streams ----

// A track read from the disc and decoded on a thread of its own; `ready` (or `failed`) is set
// under g_lock.
struct Track {
    std::vector<std::int16_t> samples;  // interleaved: a pack's may be stereo, the disc's are mono
    int channels = 1;
    std::uint32_t rate = 44100;
    // The frames the header announces (28 per 16-byte frame), known when the stream starts: the
    // time left before the decode has ended (openrac_stream_remaining_ms).
    std::size_t length = 0;
    std::filesystem::path replacement;  // a pack's WAV file to play instead; empty for its own
    bool ready = false;
    bool failed = false;

    std::size_t frames() const { return samples.size() / static_cast<std::size_t>(channels); }
};

struct Part {
    std::shared_ptr<Track> track;
    // Stereo in two mono files (Going Commando's music): `track` the right channel, this the left.
    std::shared_ptr<Track> left;
    bool loop = false;
};

// One stream: what it plays now and what is queued after it, under one or more handles.
struct Stream {
    std::vector<std::uint32_t> handles;
    std::deque<Part> parts;
    double position = 0.0;  // in the front part's samples
    std::int32_t volume = audio::kFullVolume;
    std::int32_t group = 1;
    bool paused = false;
    bool held_paused = false;
    std::int32_t held_volume = audio::kFullVolume;
    // The output frames in which it has moved on (to count its share of the frames not heard).
    std::uint64_t mixed = 0;
};

std::vector<std::unique_ptr<Stream>> g_streams;

// The reverb the game last set (openrac_sound_set_reverb), set again on a snapshot's new player.
struct ReverbSetting {
    audio::ReverbMode mode = audio::ReverbMode::Off;
    std::int32_t depth = 0;
    std::uint8_t delay = 0;
    std::uint8_t feedback = 0;
};

ReverbSetting g_reverb;
std::uint32_t g_next_stream = 1;
bool g_music_held = false;

Stream* find_stream(std::uint32_t handle) {
    if ((handle & 0xFF000000u) != OPENRAC_STREAM_TAG) {
        return nullptr;
    }

    for (auto& s : g_streams) {
        if (std::find(s->handles.begin(), s->handles.end(), handle) != s->handles.end()) {
            return s.get();
        }
    }

    return nullptr;
}

// A mono VAG file at `sector`: {"VAGp", version, pad, data size, rate, ...} big-endian, 0x30 bytes.
constexpr std::size_t kVagHeader = 0x30;

// The body's size and the rate from the header at `sector`, or false.
bool read_vag_header(std::uint32_t sector, std::uint32_t& size, std::uint32_t& rate) {
    std::uint8_t header[kVagHeader];

    if (!read_disc(static_cast<std::uint64_t>(sector) * 2048, header, sizeof header)
        || std::memcmp(header, "VAGp", 4) != 0) {
        return false;
    }

    const auto be32 = [&](int at) {
        return static_cast<std::uint32_t>(header[at]) << 24
               | static_cast<std::uint32_t>(header[at + 1]) << 16
               | static_cast<std::uint32_t>(header[at + 2]) << 8 | header[at + 3];
    };
    size = be32(12);
    rate = be32(16);
    return true;
}

// A pack's WAV file in place of a track, or false: the track is then read from the disc after all.
bool read_replacement(
    const std::filesystem::path& path,
    std::vector<std::int16_t>& samples,
    std::uint32_t& rate,
    int& channels
) {
    try {
        std::ifstream file(path, std::ios::binary);
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
        };
        assets::WavAudio audio = assets::parse_wav(bytes);
        samples = std::move(audio.samples);
        rate = audio.sample_rate;
        channels = audio.channels;
        return true;
    } catch (const std::exception& e) {
        log::warn(
            "audio mods: {}: {}; the stream stays the game's own",
            path.filename().string(),
            e.what()
        );
        return false;
    }
}

void read_track(std::uint32_t sector, std::shared_ptr<Track> track) {
    // The program is leaving: nothing to read for.
    {
        std::lock_guard lock(g_lock);

        if (g_stopping) {
            track->failed = true;
            return;
        }
    }
    std::vector<std::int16_t> samples;
    std::uint32_t rate = 0;
    int channels = 1;
    const bool replaced = !track->replacement.empty()
                          && read_replacement(track->replacement, samples, rate, channels);

    if (!replaced && openrac_disc_available()) {
        const std::uint64_t at = static_cast<std::uint64_t>(sector) * 2048;
        std::uint32_t size = 0;
        std::uint32_t header_rate = 0;

        if (read_vag_header(sector, size, header_rate)) {
            std::vector<std::uint8_t> file(kVagHeader + size);

            if (size > 0 && size < 64u * 1024 * 1024 && read_disc(at, file.data(), file.size())) {
                try {
                    const assets::VagFile vag = assets::parse_vag(file);
                    rate = vag.header.sample_rate;
                    samples = assets::decode_adpcm(vag.body);
                } catch (const std::exception& e) {
                    log::warn("music: the stream at sector {}: {}", sector, e.what());
                }
            }
        }
    }

    std::lock_guard lock(g_lock);

    if (samples.empty() || rate == 0) {
        log::warn("music: no VAG stream at sector {}", sector);
        track->failed = true;
        return;
    }

    track->samples = std::move(samples);
    track->channels = channels;
    track->rate = rate;
    track->ready = true;
}

// Adds the streams to `out`. A part not decoded yet holds its stream silent; an ended part gives
// way to the next, the position's fraction carried over.
void mix_streams(std::span<float> out) {
    const std::size_t frames = out.size() / 2;
    // Each group's eased gain at the buffer's start and end, a straight line between.
    std::array<double, audio::kVolumeGroups> gain_from = g_stream_gain;
    std::array<double, audio::kVolumeGroups> gain_to{};
    for (std::size_t g = 0; g < gain_to.size(); ++g) {
        const double target = std::clamp(
            static_cast<double>(g_stream_gain_target[g].load(std::memory_order_relaxed)), 0.0, 1.0
        );
        const double most = kStreamGainStep * static_cast<double>(frames);
        gain_to[g] = gain_from[g] + std::clamp(target - gain_from[g], -most, most);
    }
    g_stream_gain = gain_to;

    for (auto it = g_streams.begin(); it != g_streams.end();) {
        Stream& s = **it;
        if (g_music_held && s.group == 1) {
            ++it;
            continue;
        }
        const double master =
            static_cast<double>(g_master[static_cast<std::size_t>(s.group)]) / audio::kFullVolume;
        const double gain =
            static_cast<double>(s.volume) / audio::kFullVolume * master * master / 32768.0;
        const auto group = static_cast<std::size_t>(s.group);
        const double eased_from = gain_from[group];
        const double eased_by = frames > 0 ? (gain_to[group] - eased_from) / frames : 0.0;

        for (std::size_t f = 0; f < frames && !s.parts.empty() && !s.paused;) {
            const Track& t = *s.parts.front().track;
            const Track* left = s.parts.front().left.get();

            if (t.failed) {
                s.parts.pop_front();
                s.position = 0.0;
                continue;
            }

            if (!t.ready || (left != nullptr && !left->ready && !left->failed)) {
                break;
            }

            if (left != nullptr && left->failed) {
                left = nullptr;  // The file that is there on both sides.
            }

            const std::size_t n = t.frames();
            const auto channels = static_cast<std::size_t>(t.channels);
            const double step = static_cast<double>(t.rate) / audio::kOutputRate;

            for (; f < frames; ++f) {
                auto i = static_cast<std::size_t>(s.position);

                if (i >= n) {
                    break;
                }

                ++s.mixed;
                const double frac = s.position - static_cast<double>(i);

                for (std::size_t c = 0; c < 2; ++c) {
                    // The disc's streams are mono, heard on both sides.
                    const std::size_t from = channels == 2 ? c : 0;
                    const double a = t.samples[i * channels + from];
                    const double b = i + 1 < n ? t.samples[(i + 1) * channels + from] : a;
                    const double eased = eased_from + eased_by * static_cast<double>(f);
                    double v = (a + (b - a) * frac) * gain * eased;

                    // Stereo in two files: the left side from the left track, its first channel.
                    if (c == 0 && left != nullptr && i < left->frames()) {
                        const auto lc = static_cast<std::size_t>(left->channels);
                        const double la = left->samples[i * lc];
                        const double lb = i + 1 < left->frames() ? left->samples[(i + 1) * lc] : la;
                        v = (la + (lb - la) * frac) * gain * eased;
                    }

                    out[f * 2 + c] += static_cast<float>(v);
                }

                s.position += step;
            }

            if (static_cast<std::size_t>(s.position) >= n) {
                s.position -= static_cast<double>(n);

                if (!s.parts.front().loop || s.parts.size() > 1) {
                    s.parts.pop_front();
                }
            }
        }

        if (s.parts.empty() && g_unheard != nullptr) {
            for (const std::uint32_t handle : s.handles) {
                g_ended.push_back({handle, g_rendered + frames});
            }
        }

        it = s.parts.empty() ? g_streams.erase(it) : it + 1;
    }
}

// ---- The mix's levels (OPENRAC_AUDIO_STATS) ----

// What the mix did over a stretch of sound, said in the log every kStatsSeconds of it.
struct MixStats {
    std::uint64_t frames = 0;
    float peak = 0.0f;
    double square_sum = 0.0;
    std::size_t voices = 0;
    std::size_t streams = 0;
    std::uint64_t limited = 0;
    float lowest = 1.0f;
};

constexpr std::uint64_t kStatsSeconds = 10;
MixStats g_mix_stats;

float decibels(double level) {
    return level > 0.0 ? static_cast<float>(20.0 * std::log10(level)) : -144.0f;
}

// One render's frames counted, under g_lock; a stretch's report when it is over.
void note_mix(std::span<const float> out) {
    static const bool on = openrac::env::flag(openrac::env::Var::AudioStats);

    // Off: nothing counted.
    if (!on) {
        return;
    }

    MixStats& m = g_mix_stats;

    // Each sample's level.
    for (const float v : out) {
        m.peak = std::max(m.peak, std::abs(v));
        m.square_sum += static_cast<double>(v) * v;
    }

    m.frames += out.size() / 2;
    m.voices = std::max(m.voices, player().mixer().active_voices());
    m.streams = std::max(m.streams, g_streams.size());

    // Said once the stretch is long enough.
    if (m.frames >= kStatsSeconds * audio::kOutputRate) {
        const double rms = std::sqrt(m.square_sum / static_cast<double>(m.frames * 2));
        log::info(
            "sound mix: {} s, peak {:.1f} dBFS, RMS {:.1f} dBFS, limited {:.2f}% of frames (lowest "
            "gain "
            "{:.2f}), up to {} voices and {} streams",
            m.frames / audio::kOutputRate,
            decibels(m.peak),
            decibels(rms),
            100.0 * static_cast<double>(m.limited) / static_cast<double>(m.frames),
            m.lowest,
            m.voices,
            m.streams
        );
        m = {};
    }
}

// ---- The mods' own sounds (interface 1.12) ----

// A sound a mod loaded, decoded whole; a voice of one playing.
struct ModSound {
    std::vector<std::int16_t> samples;  // interleaved
    int channels = 1;
    std::uint32_t rate = 44100;
};

struct ModVoice {
    std::uint32_t handle = 0;
    std::size_t sound = 0;
    double position = 0.0;  // in the sound's frames
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
};

std::vector<ModSound> g_mod_sounds;
std::vector<ModVoice> g_mod_voices;
std::uint32_t g_next_mod_voice = 1;
// The most voices at once (OPENRAC_SOUND_VOICES_MAX).
constexpr std::size_t kModVoicesMax = 16;

// Adds the mods' voices to `out`, at group 0's master volume as the streams take theirs; a voice
// that ran off its end (not looped) is let go.
void mix_mod_voices(std::span<float> out) {
    const std::size_t frames = out.size() / 2;
    const double master = static_cast<double>(g_master[0]) / audio::kFullVolume;
    for (auto it = g_mod_voices.begin(); it != g_mod_voices.end();) {
        ModVoice& v = *it;
        const ModSound& s = g_mod_sounds[v.sound];
        const std::size_t n = s.samples.size() / static_cast<std::size_t>(s.channels);
        const double gain = static_cast<double>(v.volume) * master * master / 32768.0;
        const double step = static_cast<double>(s.rate) / audio::kOutputRate * v.pitch;
        bool ended = n == 0;
        for (std::size_t f = 0; f < frames && !ended; ++f) {
            auto i = static_cast<std::size_t>(v.position);
            if (i >= n) {
                if (!v.loop || step <= 0.0) {
                    ended = true;
                    break;
                }
                v.position = std::fmod(v.position, static_cast<double>(n));
                i = static_cast<std::size_t>(v.position);
            }
            const double frac = v.position - static_cast<double>(i);
            const std::size_t j = i + 1 < n ? i + 1 : (v.loop ? 0 : i);
            for (int c = 0; c < 2; ++c) {
                const int from = s.channels == 2 ? c : 0;
                const double a = s.samples[i * static_cast<std::size_t>(s.channels) + from];
                const double b = s.samples[j * static_cast<std::size_t>(s.channels) + from];
                out[f * 2 + c] += static_cast<float>((a + (b - a) * frac) * gain);
            }
            v.position += step;
        }
        it = ended ? g_mod_voices.erase(it) : it + 1;
    }
}

// A pack's file in place of the stream at `sector`, into its track: its own length and rate are then
// the header's answer. read_track reads it whole, or else the disc's.
void apply_stream_replacement(std::uint32_t sector, Track& track) {
    std::filesystem::path file;
    {
        std::lock_guard lock(g_replacements_lock);
        const audio::SoundReplacements& replacements = current_replacements();

        // The game's sound, as the disc has it: no sector to read.
        if (replacements.empty()) {
            return;
        }

        std::vector<std::uint8_t> start(audio::kStreamKeyBytes);

        // Near the disc's end there is less; past it, nothing to go by.
        start.resize(openrac_disc_read_some(
            static_cast<std::uint64_t>(sector) * 2048, start.data(), start.size()
        ));
        file = replacements.stream(start);
    }

    // Most streams are not replaced.
    if (file.empty()) {
        return;
    }

    track.replacement = file;
    log::info("audio mods: the stream at sector {} plays {}", sector, file.filename().string());

    // What the first `most` bytes of it say it is.
    const auto format_in = [&](std::uintmax_t most) {
        std::ifstream input(file, std::ios::binary);
        std::vector<char> head(most);
        input.read(head.data(), static_cast<std::streamsize>(most));
        const auto got = static_cast<std::size_t>(input.gcount());
        return assets::parse_wav_format(
            assets::ByteView(reinterpret_cast<const std::uint8_t*>(head.data()), got)
        );
    };

    try {
        assets::WavFormat format;

        // A file says so at its start, unless a long chunk comes before its sound.
        try {
            format = format_in(65536);
        } catch (const assets::AssetError&) {
            format = format_in(std::filesystem::file_size(file));
        }

        track.channels = format.channels;
        track.rate = format.sample_rate;
        track.length = format.data_bytes / 2 / format.channels;
    } catch (const std::exception&) {
        // No sound in it: the disc's answers, and read_track says why.
    }
}

std::int32_t word(const std::uint8_t* data, std::uint32_t i) {
    std::int32_t v;
    std::memcpy(&v, data + i * 4, 4);
    return v;
}

// The bank file's size from its header: {u32 type, u32 count, {u32 offset, u32 size}[count]}.
std::size_t bank_size(const std::uint8_t* header, std::size_t available) {
    if (available < 8) {
        return 0;
    }

    std::uint32_t count;
    std::memcpy(&count, header + 4, 4);

    if (count == 0 || count > 8 || available < 8 + count * 8) {
        return 0;
    }

    std::size_t end = 0;

    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t offset, size;
        std::memcpy(&offset, header + 8 + i * 8, 4);
        std::memcpy(&size, header + 12 + i * 8, 4);
        end = std::max<std::size_t>(end, std::size_t{offset} + size);
    }

    return end;
}

}  // namespace

extern "C" {

uint32_t openrac_sound_load_bank(const void* bytes, size_t size) {
    const auto* p = static_cast<const std::uint8_t*>(bytes);
    const std::size_t whole = bank_size(p, size);

    if (whole == 0 || whole > size) {
        log::warn("sound: a bank with a bad header");
        return 0;
    }

    const audio::BankReplacements replaced = replacements_of_bank(std::span(p, whole));
    std::lock_guard lock(g_lock);

    try {
        std::vector<std::uint8_t> saved_file(p, p + whole);
        const audio::BankHandle bank = player().load_bank(std::span(p, whole), replaced);

        try {
            g_bank_files.emplace(bank, std::move(saved_file));
        } catch (...) {
            player().unload_bank(bank);
            throw;
        }

        log::info("sound: bank {} loaded, {} sounds", bank, player().bank(bank)->sounds.size());
        return bank;
    } catch (const std::exception& e) {
        log::warn("sound: bank not loaded: {}", e.what());
        return 0;
    }
}

uint32_t openrac_sound_load_bank_from_disc(uint32_t sector, uint32_t offset) {
    if (!openrac_disc_available()) {
        return 0;
    }

    const std::uint64_t at = static_cast<std::uint64_t>(sector) * 2048 + offset;
    std::uint8_t header[72];
    std::vector<std::uint8_t> file;

    if (read_disc(at, header, sizeof header)) {
        const std::size_t size = bank_size(header, sizeof header);
        file.resize(size);

        if (size == 0 || !read_disc(at, file.data(), size)) {
            file.clear();
        }
    }

    if (file.empty()) {
        log::warn("sound: no bank at sector {} + {}", sector, offset);
        return 0;
    }

    return openrac_sound_load_bank(file.data(), file.size());
}

void openrac_sound_unload_bank(uint32_t bank) {
    std::lock_guard lock(g_lock);
    player().unload_bank(bank);
    g_bank_files.erase(bank);
}

size_t openrac_sound_snapshot_size(void) {
    return OPENRAC_SOUND_SNAPSHOT_SIZE;
}

int openrac_sound_snapshot_save(void* output, size_t capacity) {
    if (output == nullptr || capacity != OPENRAC_SOUND_SNAPSHOT_SIZE) {
        return -1;
    }

    std::lock_guard lock(g_lock);
    auto* bytes = static_cast<std::uint8_t*>(output);
    std::memset(bytes, 0, capacity);
    SoundSnapshotHeader header{};
    std::memcpy(header.magic, "ORACSND1", sizeof(header.magic));
    header.version = 1;
    header.bank_count = static_cast<std::uint32_t>(g_bank_files.size());
    header.next_bank = player().next_bank_handle();
    header.next_sound = player().next_sound_handle();
    header.master = g_master;
    std::size_t at = sizeof(header);

    for (const auto& [handle, file] : g_bank_files) {
        if (file.size() > UINT32_MAX || at + sizeof(SoundSnapshotBank) + file.size() > capacity) {
            log::warn(
                "sound: loaded banks exceed the savestate audio payload limit ({} MiB)",
                OPENRAC_SOUND_SNAPSHOT_SIZE / (1024u * 1024u)
            );
            return -1;
        }

        const SoundSnapshotBank bank{handle, static_cast<std::uint32_t>(file.size())};
        std::memcpy(bytes + at, &bank, sizeof(bank));
        at += sizeof(bank);
        std::memcpy(bytes + at, file.data(), file.size());
        at += file.size();
    }

    header.used = static_cast<std::uint32_t>(at);
    std::memcpy(bytes, &header, sizeof(header));
    return 0;
}

int openrac_sound_snapshot_load(const void* input, size_t size) {
    if (input == nullptr || size != OPENRAC_SOUND_SNAPSHOT_SIZE) {
        return -1;
    }

    try {
        const auto* bytes = static_cast<const std::uint8_t*>(input);
        SoundSnapshotHeader header{};
        std::memcpy(&header, bytes, sizeof(header));

        if (std::memcmp(header.magic, "ORACSND1", sizeof(header.magic)) != 0 || header.version != 1
            || header.used < sizeof(header) || header.used > size || header.next_bank == 0
            || header.next_sound == 0) {
            return -1;
        }

        std::map<audio::BankHandle, std::vector<std::uint8_t>> banks;
        audio::BankHandle highest_bank = 0;
        std::size_t at = sizeof(header);

        for (std::uint32_t i = 0; i < header.bank_count; ++i) {
            if (at + sizeof(SoundSnapshotBank) > header.used) {
                return -1;
            }

            SoundSnapshotBank bank{};
            std::memcpy(&bank, bytes + at, sizeof(bank));
            at += sizeof(bank);

            if (bank.handle == 0 || bank.size == 0 || at + bank.size > header.used) {
                return -1;
            }

            highest_bank = std::max(highest_bank, bank.handle);

            if (!banks
                     .emplace(
                         bank.handle, std::vector<std::uint8_t>(bytes + at, bytes + at + bank.size)
                     )
                     .second) {
                return -1;
            }

            at += bank.size;
        }

        if (at != header.used || header.next_bank <= highest_bank) {
            return -1;
        }

        // Parse the full bank set before replacing live audio, so invalid saved
        // data cannot leave the game with only some banks installed.
        audio::SoundPlayer checked;

        for (const auto& [handle, file] : banks) {
            checked.load_bank_at(handle, file, replacements_of_bank(file));
        }

        checked.set_next_handles(header.next_bank, header.next_sound);

        for (std::size_t group = 0; group < header.master.size(); ++group) {
            checked.set_master_volume(group, header.master[group]);
        }

        std::lock_guard lock(g_lock);
        player().swap_snapshot_state(checked);
        player().mixer().reverb().set_enabled(openrac::env::flag(openrac::env::Var::AudioReverb));
        player().mixer().reverb().set(
            g_reverb.mode, g_reverb.depth, g_reverb.delay, g_reverb.feedback
        );
        g_bank_files.swap(banks);
        g_master = header.master;
        g_streams.clear();
        g_music_held = false;
        g_ended.clear();
        player().take_ended();
        return 0;
    } catch (const std::exception&) {
        return -1;
    }
}

uint32_t openrac_sound_play(
    uint32_t bank, int32_t sound, int32_t volume, int32_t pan, int32_t pitch_mod, int32_t pitch_bend
) {
    audio::PlayRequest request;
    request.volume = volume;
    request.pan = pan;
    request.pitch_mod = pitch_mod;
    request.pitch_bend = pitch_bend;
    std::lock_guard lock(g_lock);

    if (player().bank(bank) == nullptr || sound < 0) {
        return 0;
    }

    return player().play(bank, static_cast<std::uint32_t>(sound), request);
}

uint32_t openrac_sound_set_params(
    uint32_t sound, int32_t mask, int32_t volume, int32_t pan, int32_t pitch_mod, int32_t pitch_bend
) {
    std::lock_guard lock(g_lock);

    if (Stream* stream = find_stream(sound)) {
        if ((mask & 1) != 0) {
            stream->volume = volume;
        }

        return sound;
    }

    audio::SoundPlayer& p = player();

    if (sound == 0 || !p.playing(sound)) {
        // Ended, but not heard to yet: still the sound's handle, with nothing left to change.
        return sound != 0 && ended_unheard(sound) != nullptr ? sound : 0;
    }

    if ((mask & 7) != 0) {
        p.set_volume_pan(
            sound,
            (mask & 1) != 0 ? volume : audio::kVolumeKeep,
            (mask & 6) != 0 ? pan : audio::kPanKeep
        );
    }

    if ((mask & 8) != 0) {
        p.set_pitch_mod(sound, pitch_mod);
    }

    if ((mask & 0x10) != 0) {
        p.set_pitch_bend(sound, pitch_bend);
    }

    return sound;
}

void openrac_sound_set_reverb(
    int32_t core, int32_t type, int32_t depth, int32_t delay, int32_t feedback
) {
    (void)core;
    std::lock_guard lock(g_lock);
    const auto mode = type >= 0 && type <= static_cast<int32_t>(audio::ReverbMode::Pipe)
                          ? static_cast<audio::ReverbMode>(type)
                          : audio::ReverbMode::Off;
    g_reverb =
        {mode,
         depth,
         static_cast<std::uint8_t>(std::clamp(delay, 0, 127)),
         static_cast<std::uint8_t>(std::clamp(feedback, 0, 127))};
    audio::ReverbEffect& reverb = player().mixer().reverb();
    reverb.set_enabled(openrac::env::flag(openrac::env::Var::AudioReverb));
    reverb.set(g_reverb.mode, g_reverb.depth, g_reverb.delay, g_reverb.feedback);
}

void openrac_sound_auto_reverb(int32_t core, int32_t depth, int32_t delta) {
    (void)core;
    std::lock_guard lock(g_lock);
    g_reverb.depth = depth;
    player().mixer().reverb().glide(depth, delta);
}

int openrac_sound_playing(uint32_t sound) {
    std::lock_guard lock(g_lock);
    return sound != 0 && (player().playing(sound) || ended_unheard(sound) != nullptr);
}

void openrac_sound_stop(uint32_t sound) {
    std::lock_guard lock(g_lock);
    player().stop(sound);
    forget_ended(sound);
}

void openrac_sound_stop_all(void) {
    std::lock_guard lock(g_lock);
    player().stop_all();
    std::erase_if(g_ended, [](const Ended& e) {
        return (e.handle & 0xFF000000u) != OPENRAC_STREAM_TAG;
    });
}

void openrac_sound_set_master_volume(int32_t group, int32_t volume) {
    if (group < 0 || static_cast<std::size_t>(group) >= audio::kVolumeGroups) {
        return;
    }

    std::lock_guard lock(g_lock);
    g_master[static_cast<std::size_t>(group)] = volume;
    player().set_master_volume(static_cast<std::size_t>(group), volume);
}

void openrac_sound_set_stream_gain(int32_t group, float gain) {
    if (group < 0 || static_cast<std::size_t>(group) >= audio::kVolumeGroups
        || !std::isfinite(gain)) {
        return;
    }
    g_stream_gain_target[static_cast<std::size_t>(group)]
        .store(std::clamp(gain, 0.0f, 1.0f), std::memory_order_relaxed);
}

void openrac_sound_pause_groups(uint32_t mask) {
    std::lock_guard lock(g_lock);
    player().pause_groups(mask);
}

void openrac_sound_continue_groups(uint32_t mask) {
    std::lock_guard lock(g_lock);
    player().continue_groups(mask);
}

uint32_t openrac_sound_command(uint32_t command, const void* data, uint32_t size) {
    const auto* d = static_cast<const std::uint8_t*>(data);

    switch (command) {
        case OPENRAC_SND_PLAY_VOL_PAN_PMPB:  // bank, sound, volume, pan, pitch mod, pitch bend
            if (size >= 24) {
                return openrac_sound_play(
                    static_cast<uint32_t>(word(d, 0)),
                    word(d, 1),
                    word(d, 2),
                    word(d, 3),
                    word(d, 4),
                    word(d, 5)
                );
            }

            return 0;
        case OPENRAC_SND_SET_SOUND_PARAMS:  // handle, mask, volume, pan, pitch mod, pitch bend
            if (size >= 24) {
                return openrac_sound_set_params(
                    static_cast<uint32_t>(word(d, 0)),
                    word(d, 1),
                    word(d, 2),
                    word(d, 3),
                    word(d, 4),
                    word(d, 5)
                );
            }

            return 0;
        default:
            return 0;
    }
}

namespace {

// A track's read, at once or on a thread of its own.
void start_reading(std::uint32_t sector, const std::shared_ptr<Track>& track) {
    if (g_read_at_once) {
        read_track(sector, track);
        return;
    }
    std::lock_guard lock(g_lock);
    if (g_stopping) {
        // The program is leaving: the track is not read, and no thread is left behind.
        track->failed = true;
        return;
    }
    // The readers that have finished are joined (their work is done: the join only waits for
    // the thread to end); the others are kept.
    std::erase_if(g_readers, [](Reader& reader) {
        if (!reader.done->load()) {
            return false;
        }
        reader.thread.join();
        return true;
    });
    auto done = std::make_shared<std::atomic<bool>>(false);
    g_readers.push_back(
        {std::thread([sector, track, done] {
             read_track(sector, track);
             done->store(true);
         }),
         done}
    );
}

// The header now, the body on the reader's thread: the game asks how long the stream has left
// from the next frame on, long before a track of minutes is read and decoded.
std::shared_ptr<Track> new_track(std::uint32_t sector) {
    auto track = std::make_shared<Track>();
    if (openrac_disc_available()) {
        std::uint32_t size = 0;
        std::uint32_t rate = 0;

        if (read_vag_header(sector, size, rate) && rate != 0) {
            track->length = static_cast<std::size_t>(size / 16) * 28;
            track->rate = rate;
        }

        apply_stream_replacement(sector, *track);
    }
    return track;
}

}  // namespace

uint32_t openrac_stream_play_stereo(
    uint32_t sector,
    uint32_t left_sector,
    int32_t volume,
    int32_t group,
    uint32_t queue_on,
    uint32_t flags
) {
    if (sector == 0) {
        return 0;
    }
    if (group == 1) {
        std::lock_guard lock(g_lock);
        if (g_music_held) {
            for (const auto& stream : g_streams) {
                if (stream->group == 1 && !stream->handles.empty()) {
                    return stream->handles.front();
                }
            }
        }
    }
    auto track = new_track(sector);
    auto left = left_sector != 0 ? new_track(left_sector) : nullptr;
    std::uint32_t handle;
    {
        std::lock_guard lock(g_lock);
        handle = OPENRAC_STREAM_TAG | (g_next_stream++ & 0x00FFFFFFu);
        Stream* stream = find_stream(queue_on);

        if (stream == nullptr) {
            g_streams.push_back(std::make_unique<Stream>());
            stream = g_streams.back().get();
            stream->volume = volume;
            stream->group = std::clamp<std::int32_t>(group, 0, audio::kVolumeGroups - 1);
            stream->paused = (flags & 1) != 0;
        }

        // A part queued behind a loop replaces the loop when the loop ends its pass.
        if (!stream->parts.empty() && stream->parts.back().loop) {
            stream->parts.back().loop = false;
        }
        stream->parts.push_back({track, left, (flags & 4) != 0});
        stream->handles.push_back(handle);
    }
    start_reading(sector, track);
    if (left != nullptr) {
        start_reading(left_sector, left);
    }

    return handle;
}

uint32_t openrac_stream_play(
    uint32_t sector, int32_t volume, int32_t group, uint32_t queue_on, uint32_t flags
) {
    return openrac_stream_play_stereo(sector, 0, volume, group, queue_on, flags);
}

// Stops the threads reading a track and waits for them: the program calls this before it
// exits, so that nothing they use is taken away while they are still working (a reader that
// locks g_lock after the static destructors have run is a crash at quit).
void openrac_sound_stop_readers() {
    std::vector<Reader> readers;
    {
        std::lock_guard lock(g_lock);
        g_stopping = true;
        readers.swap(g_readers);
    }

    // Outside the lock: a reader that is waiting for it must be able to take it and end.
    for (auto& reader : readers) {
        if (reader.thread.joinable()) {
            reader.thread.join();
        }
    }
}

void openrac_stream_read_at_once(int at_once) {
    g_read_at_once = at_once != 0;
}

int openrac_stream_live(uint32_t stream) {
    std::lock_guard lock(g_lock);
    return find_stream(stream) != nullptr || ended_unheard(stream) != nullptr;
}

int openrac_stream_buffered(uint32_t stream) {
    std::lock_guard lock(g_lock);
    const Stream* s = find_stream(stream);
    return s != nullptr && !s->parts.empty() && s->parts.front().track->ready;
}

int32_t openrac_stream_remaining_ms(uint32_t stream) {
    std::lock_guard lock(g_lock);
    const Stream* s = find_stream(stream);

    if (s == nullptr || s->parts.empty()) {
        // Ended in frames not heard yet: what is left of them.
        if (const Ended* e = ended_unheard(stream)) {
            return static_cast<int32_t>((e->at - heard_frames()) * 1000 / audio::kOutputRate);
        }

        return 0;
    }

    const Track& t = *s->parts.front().track;
    // Not decoded yet: all of it is left, as its header gives it (the stream holds silent at its
    // start until then). RAC1's crossfade measures the old music's fade against the first answer
    // it gets for the transition (music_Update, case 2), so a made-up length there cut the old
    // music at once and started the next track with it.
    const std::size_t length = t.ready ? t.frames() : t.length;

    if (!t.ready && length == 0) {
        return 60000;  // no header either: the game only needs it to be playing
    }

    const double left = static_cast<double>(length) - s->position;
    // The frames rendered and not heard yet in which it moved on are still to come.
    const std::uint64_t unheard = g_rendered - heard_frames();
    const double ahead_ms =
        t.ready && !s->paused && !(g_music_held && s->group == 1)
            ? static_cast<double>(std::min(unheard, s->mixed)) * 1000.0 / audio::kOutputRate
            : 0.0;
    return static_cast<int32_t>(std::max(0.0, left) * 1000.0 / t.rate + ahead_ms);
}

void openrac_stream_stop(uint32_t stream) {
    std::lock_guard lock(g_lock);

    if (stream == 0) {
        std::erase_if(g_streams, [](const auto& s) { return !g_music_held || s->group != 1; });
        std::erase_if(g_ended, [](const Ended& e) {
            return (e.handle & 0xFF000000u) == OPENRAC_STREAM_TAG;
        });
        return;
    }

    if (g_music_held) {
        if (const auto* s = find_stream(stream); s && s->group == 1) {
            return;
        }
    }
    forget_ended(stream);
    std::erase_if(g_streams, [&](const std::unique_ptr<Stream>& s) {
        return std::find(s->handles.begin(), s->handles.end(), stream) != s->handles.end();
    });
}

void openrac_music_hold(int held) {
    std::lock_guard lock(g_lock);
    if (g_music_held == (held != 0)) {
        return;
    }
    g_music_held = held != 0;
    for (auto& s : g_streams) {
        if (s->group == 1) {
            if (g_music_held) {
                s->held_paused = s->paused;
                s->held_volume = s->volume;
            } else {
                // Vendor exit may finish its native fade/unpause while held. Keep
                // that progress, and recover the pre-movie volume after native stops.
                s->paused = s->held_paused && s->paused;
                s->volume = std::max(s->volume, s->held_volume);
            }
        }
    }
}

void openrac_stream_pause(uint32_t stream, int paused) {
    std::lock_guard lock(g_lock);

    if (Stream* s = find_stream(stream)) {
        s->paused = paused != 0;
    }
}

void openrac_stream_set_volume(uint32_t stream, int32_t volume) {
    std::lock_guard lock(g_lock);

    if (Stream* s = find_stream(stream)) {
        s->volume = volume;
    }
}

void openrac_sound_render_float(float* interleaved, size_t frames) {
    std::lock_guard lock(g_lock);
    const std::span out(interleaved, frames * 2);
    player().render(out);
    mix_streams(out);
    mix_mod_voices(out);
    g_rendered += frames;
    note_mix(out);

    if (g_unheard != nullptr) {
        for (const audio::SoundHandle sound : player().take_ended()) {
            g_ended.push_back({sound, g_rendered});
        }

        // Those heard by now go; the game's reads forget the rest as they are heard.
        const std::uint64_t heard = heard_frames();
        std::erase_if(g_ended, [heard](const Ended& e) { return e.at <= heard; });
    }
}

void openrac_sound_render(int16_t* interleaved, size_t frames) {
    // The mix in float, through the one limiter, to 16 bits.
    static std::vector<float> mix;
    static audio::Limiter limiter;
    mix.resize(frames * 2);
    openrac_sound_render_float(mix.data(), frames);
    limiter.process(mix);
    audio::to_int16(mix, std::span(interleaved, frames * 2));
    const audio::Limiter::Taken taken = limiter.take();
    openrac_sound_limited(taken.limited, taken.lowest);
}

void openrac_sound_limited(size_t limited, float lowest) {
    std::lock_guard lock(g_lock);
    g_mix_stats.limited += limited;
    g_mix_stats.lowest = std::min(g_mix_stats.lowest, lowest);
}

void openrac_sound_set_unheard(size_t (*unheard)(void* user), void* user) {
    std::lock_guard lock(g_lock);
    g_unheard = unheard;
    g_unheard_user = user;
    player().keep_ended(unheard != nullptr);

    if (unheard == nullptr) {
        g_ended.clear();
    }
}

int openrac_sound_mod_add(const int16_t* samples, size_t frames, int channels, uint32_t rate) {
    if (samples == nullptr || frames == 0 || (channels != 1 && channels != 2) || rate == 0) {
        return -1;
    }
    std::lock_guard lock(g_lock);
    ModSound s;
    s.samples.assign(samples, samples + frames * static_cast<std::size_t>(channels));
    s.channels = channels;
    s.rate = rate;
    g_mod_sounds.push_back(std::move(s));
    return static_cast<int>(g_mod_sounds.size() - 1);
}

uint32_t openrac_sound_mod_play(int sound, float volume, float pitch, int loop) {
    std::lock_guard lock(g_lock);
    if (sound < 0 || static_cast<std::size_t>(sound) >= g_mod_sounds.size()
        || g_mod_voices.size() >= kModVoicesMax) {
        return 0;
    }
    ModVoice v;
    v.handle = g_next_mod_voice++;
    if (g_next_mod_voice == 0) {
        g_next_mod_voice = 1;
    }
    v.sound = static_cast<std::size_t>(sound);
    v.volume = volume;
    v.pitch = pitch;
    v.loop = loop != 0;
    g_mod_voices.push_back(v);
    return v.handle;
}

int openrac_sound_mod_set(uint32_t voice, float volume, float pitch) {
    std::lock_guard lock(g_lock);
    for (ModVoice& v : g_mod_voices) {
        if (v.handle == voice) {
            if (volume >= 0.0f) {
                v.volume = volume;
                v.pitch = pitch;
            }
            return 1;
        }
    }
    return 0;
}

void openrac_sound_mod_stop(uint32_t voice) {
    std::lock_guard lock(g_lock);
    std::erase_if(g_mod_voices, [&](const ModVoice& v) { return v.handle == voice; });
}

}  // extern "C"

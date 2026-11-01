// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game's sounds on the player's device (program.h): the output opened at the start, watched
// each frame and opened again when it stalls or was missing, closed before the program's globals
// go. With OPENRAC_AUDIO_TICKED there is no device, and each tick renders its own samples.
//
// On a desktop the output mixes 20 ms ahead of the device on a thread of its own
// (OPENRAC_AUDIO_BUFFER_MS, platform/audio.h): Windows' device asks every 10 ms, and a mix that
// waited for the game's lock or for the processor longer than that was a gap in the sound. The
// sound engine is told which frames are not heard yet, so what the game reads back from it is what
// is heard, as before. Each frame the output's health goes to the performance capture, and gaps to
// the log.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "common/env.h"
#include "common/log.h"
#include "common/stats.h"
#include "openrac/sound_engine.h"
#include "program/program.h"

#ifdef OPENRAC_FRONTEND
#include <SDL3/SDL_timer.h>

#include "platform/audio.h"
#include "settings.h"
#endif

namespace openrac::program {

#ifdef OPENRAC_FRONTEND
namespace {

// The game's sounds (openrac/sound_engine.h), rendered on the output's thread.
std::unique_ptr<platform::AudioOutput> g_sound;
// The frame from which watch_sound next tries to open a device that could not be opened.
long g_sound_retry_frame = 0;

// OPENRAC_AUDIO_DUMP=FILE: the mix as the device gets it, as a WAV file. The header's sizes are
// written when the output closes; a run killed before that leaves them 0, with the samples there.
std::FILE* g_dump = nullptr;
std::uint64_t g_dump_bytes = 0;

// The canonical 44-byte header of 16-bit PCM at the output's rate, little-endian.
std::array<std::uint8_t, 44> wav_header(std::uint32_t data_bytes) {
    std::array<std::uint8_t, 44> h{};
    const auto put = [&h](std::size_t at, std::uint32_t value, int bytes) {
        for (int i = 0; i < bytes; ++i) {
            h[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(value >> (i * 8));
        }
    };
    const auto tag = [&h](std::size_t at, const char* text) {
        std::copy_n(text, 4, h.begin() + static_cast<std::ptrdiff_t>(at));
    };
    const std::uint32_t frame = platform::kAudioChannels * sizeof(std::int16_t);
    tag(0, "RIFF");
    put(4, data_bytes + 36, 4);
    tag(8, "WAVE");
    tag(12, "fmt ");
    put(16, 16, 4);
    put(20, 1, 2);
    put(22, platform::kAudioChannels, 2);
    put(24, platform::kAudioRate, 4);
    put(28, platform::kAudioRate * frame, 4);
    put(32, frame, 2);
    put(34, 16, 2);
    tag(36, "data");
    put(40, data_bytes, 4);
    return h;
}

void open_dump() {
    const std::optional<std::string_view> path = env::value(env::Var::AudioDump);
    if (!path || g_dump != nullptr) {
        return;
    }
    g_dump = std::fopen(std::string(*path).c_str(), "wb");
    if (g_dump == nullptr) {
        log::warn("OPENRAC_AUDIO_DUMP: {} cannot be written", *path);
        return;
    }
    const auto header = wav_header(0);
    std::fwrite(header.data(), 1, header.size(), g_dump);
}

void write_dump(std::span<const std::int16_t> samples) {
    if (g_dump != nullptr) {
        g_dump_bytes += std::fwrite(samples.data(), sizeof(std::int16_t), samples.size(), g_dump)
                        * sizeof(std::int16_t);
    }
}

// After the output has closed: no thread is writing.
void close_dump() {
    if (g_dump == nullptr) {
        return;
    }
    const auto header = wav_header(static_cast<std::uint32_t>(g_dump_bytes));
    std::fseek(g_dump, 0, SEEK_SET);
    std::fwrite(header.data(), 1, header.size(), g_dump);
    std::fclose(g_dump);
    g_dump = nullptr;
}

// The frames mixed ahead of the device: OPENRAC_AUDIO_BUFFER_MS, by default 20 ms on a desktop.
// Android's devices take bigger blocks and have never had gaps, so it mixes on request as before.
int ahead_frames() {
#if defined(__ANDROID__)
    constexpr std::int64_t kDefaultMs = 0;
#else
    constexpr std::int64_t kDefaultMs = 20;
#endif
    const std::int64_t ms =
        std::clamp<std::int64_t>(env::integer(env::Var::AudioBufferMs, kDefaultMs), 0, 200);
    return static_cast<int>(ms * platform::kAudioRate / 1000);
}

std::size_t unheard(void* output) {
    return static_cast<platform::AudioOutput*>(output)->unheard_frames();
}

// The output's health over a stretch of play, said in the log once the stretch is over if the
// device got silence: at most one line every kHealthSeconds.
constexpr std::uint64_t kHealthSeconds = 10;

struct HealthWindow {
    std::uint64_t since_ms = 0;
    std::uint64_t short_frames = 0;
    double worst_gap_ms = 0.0;
    double worst_render_ms = 0.0;
};

HealthWindow g_health;

void report_health() {
    const platform::AudioOutput::Health h = g_sound->take_health();
    if (stats::enabled()) {
        stats::gauge("audio short frames", h.short_frames);
        stats::gauge("audio request gap ms", h.worst_gap_ms);
        stats::gauge("audio mix ms", h.worst_render_ms);
    }
    g_health.short_frames += h.short_frames;
    g_health.worst_gap_ms = std::max(g_health.worst_gap_ms, h.worst_gap_ms);
    g_health.worst_render_ms = std::max(g_health.worst_render_ms, h.worst_render_ms);
    const std::uint64_t now = SDL_GetTicks();
    if (g_health.since_ms == 0) {
        g_health.since_ms = now;
    }
    if (now - g_health.since_ms < kHealthSeconds * 1000) {
        return;
    }
    if (g_health.short_frames > 0) {
        log::warn(
            "audio: {:.1f} ms of silence in the last {} s, the mixer fell behind (longest mix "
            "{:.2f} ms, longest wait between the device's requests {:.1f} ms)",
            static_cast<double>(g_health.short_frames) * 1000.0 / platform::kAudioRate,
            kHealthSeconds,
            g_health.worst_render_ms,
            g_health.worst_gap_ms
        );
    }
    g_health = {now, 0, 0.0, 0.0};
}

}  // namespace
#endif

bool audio_ticked() {
    return env::flag(env::Var::AudioTicked);
}

void audio_tick() {
#ifdef OPENRAC_FRONTEND
    if (!audio_ticked() || !g_window) {
        return;
    }

    static std::vector<std::int16_t> samples(
        static_cast<std::size_t>(platform::kAudioRate / tick_rate()) * 2
    );
    openrac_sound_render(samples.data(), samples.size() / 2);
#endif
}

#ifdef OPENRAC_FRONTEND
void close_sound() {
    // The engine stops asking the output before the output goes.
    openrac_sound_set_unheard(nullptr, nullptr);
    g_sound.reset();
    close_dump();
}

void open_sound(bool report) {
    std::string why;
    const int ahead = ahead_frames();
    open_dump();
    g_sound = platform::AudioOutput::open(
        why,
        [](std::span<std::int16_t> samples) {
            openrac_sound_render(samples.data(), samples.size() / 2);

            // The player's volume (settings.h): at 0 the output is silence itself.
            if (const float gain = settings::output_gain(); gain < 1.0f) {
                for (std::int16_t& sample : samples) {
                    sample = static_cast<std::int16_t>(static_cast<float>(sample) * gain);
                }
            }
            write_dump(samples);
        },
        ahead
    );

    if (!g_sound) {
        if (report) {
            log::warn("no sound: {}", why);
        }
        return;
    }

    if (ahead > 0) {
        openrac_sound_set_unheard(&unheard, g_sound.get());
        log::verbose("sound: mixed {} ms ahead of the device", ahead * 1000 / platform::kAudioRate);
    }
}

void watch_sound() {
    if (!g_window || audio_ticked()) {
        return;
    }

    if (!g_sound) {
        if (g_frame >= g_sound_retry_frame) {
            g_sound_retry_frame = g_frame + 300;
            open_sound(false);

            if (g_sound) {
                log::info("sound is back");
            }
        }

        return;
    }

    report_health();

    std::string why;
    static bool reported = false;

    if (!g_sound->watch(why)) {
        if (!reported) {
            log::warn("sound stopped and the device could not be opened again: {}", why);
            reported = true;
        }
    } else if (reported) {
        log::info("sound is back");
        reported = false;
    }
}
#endif

}  // namespace openrac::program

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/audio.h"

#include <algorithm>
#include <format>
#include <utility>

#include <SDL3/SDL.h>

namespace openrac::platform {
namespace {

constexpr int kFrameBytes = kAudioChannels * static_cast<int>(sizeof(std::int16_t));

// Keeps the larger of `slot` and `value`.
void keep_worst(std::atomic<std::uint64_t>& slot, std::uint64_t value) {
    std::uint64_t seen = slot.load(std::memory_order_relaxed);
    while (value > seen && !slot.compare_exchange_weak(seen, value, std::memory_order_relaxed)) {
    }
}

double milliseconds(std::uint64_t ns) {
    return static_cast<double>(ns) / 1e6;
}

}  // namespace

std::unique_ptr<AudioOutput> AudioOutput::open(
    std::string& error, Callback callback, int ahead_frames
) {
    std::unique_ptr<AudioOutput> out(new AudioOutput());
    out->m_callback = std::move(callback);
    out->m_ahead = out->m_callback ? std::max(ahead_frames, 0) : 0;
    if (!out->start(error)) {
        return nullptr;
    }
    return out;
}

void AudioOutput::start_mixer(int device_frames) {
    // The frames kept ready: those asked for, but never fewer than one request of the device (in
    // the game's 48 kHz, which SDL converts from) and a chunk, in whole chunks, as the mixer renders
    // nothing smaller. A device that waits for the mixer longer than they last gets silence.
    const int request = static_cast<int>(
        static_cast<std::int64_t>(device_frames) * kAudioRate / std::max(m_device_rate, 1)
    );
    const int wanted = std::max(m_ahead, request + kChunkFrames);
    m_target = (wanted + kChunkFrames - 1) / kChunkFrames * kChunkFrames;
    // A chunk of room more, for a request that comes out a frame or two bigger after conversion.
    m_ring = std::make_unique<AudioRing>(m_target + kChunkFrames);
    m_mixer = JoiningThread([this] { mix(); });
    // The device's first request finds the frames ahead ready, rather than a gap. A mixer that
    // has not filled them in a tenth of a second (a machine still busy starting the game) leaves
    // the first requests short, which the health counts.
    const std::uint64_t until = SDL_GetTicks() + 100;
    while (m_ring->ready() < static_cast<std::size_t>(m_target) && SDL_GetTicks() < until) {
        SDL_Delay(1);
    }
}

void AudioOutput::stop_mixer() {
    if (!m_ring) {
        return;
    }
    {
        const std::lock_guard lock(m_mixer_mutex);
        m_mixer_stop = true;
    }
    m_mixer_wake.notify_one();
    m_mixer.join();
}

void AudioOutput::mix() {
    // As SDL's own audio thread: before the game's threads and the graphics driver's, which are
    // the ones it would otherwise wait for.
    SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);
    std::vector<std::int16_t> chunk(static_cast<std::size_t>(kChunkFrames * kAudioChannels));
    for (;;) {
        {
            std::unique_lock lock(m_mixer_mutex);
            // Until a whole chunk fits in the frames ahead, or the output closes.
            m_mixer_wake.wait(lock, [&] {
                return m_mixer_stop
                       || m_ring->ready() + kChunkFrames <= static_cast<std::size_t>(m_target);
            });
            if (m_mixer_stop) {
                return;
            }
        }
        m_reserved.fetch_add(kChunkFrames, std::memory_order_relaxed);
        std::fill(chunk.begin(), chunk.end(), std::int16_t{0});
        const std::uint64_t began = SDL_GetTicksNS();
        m_callback(chunk);
        keep_worst(m_worst_render_ns, SDL_GetTicksNS() - began);
        m_ring->write(chunk);
    }
}

void AudioOutput::take_ahead(std::span<std::int16_t> out) {
    const std::size_t frames = out.size() / kAudioChannels;
    // The mixer was behind: silence for the rest, and the frames it renders next follow it.
    if (const std::size_t had = m_ring->read(out); had < frames) {
        m_short_frames
            .fetch_add(static_cast<std::uint32_t>(frames - had), std::memory_order_relaxed);
    }
    // Taken under the mutex, so the mixer is either before its test (and sees the room) or
    // waiting (and is woken); it holds the mutex only for that test.
    {
        const std::lock_guard lock(m_mixer_mutex);
    }
    m_mixer_wake.notify_one();
}

std::size_t AudioOutput::unheard_frames() const {
    if (!m_ring) {
        return 0;
    }
    return static_cast<std::size_t>(
        m_reserved.load(std::memory_order_relaxed) - m_ring->read_count()
    );
}

AudioOutput::Health AudioOutput::take_health() {
    Health h;
    h.requests = m_requests.exchange(0, std::memory_order_relaxed);
    h.short_frames = m_short_frames.exchange(0, std::memory_order_relaxed);
    h.worst_gap_ms = milliseconds(m_worst_gap_ns.exchange(0, std::memory_order_relaxed));
    h.worst_render_ms = milliseconds(m_worst_render_ns.exchange(0, std::memory_order_relaxed));
    return h;
}

bool AudioOutput::start(std::string& error) {
    const SDL_AudioSpec spec{SDL_AUDIO_S16, kAudioChannels, kAudioRate};
    // SDL converts to whatever the device really runs at; the game always
    // sees 48 kHz stereo.
    m_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        m_callback ? &AudioOutput::feed : nullptr,
        m_callback ? this : nullptr
    );
    m_started_ms = SDL_GetTicks();
    m_last_request_ns.store(0, std::memory_order_relaxed);
    if (m_stream == nullptr) {
        error = std::format("could not open the audio device: {}", SDL_GetError());
        return false;
    }
    SDL_SetAudioStreamGain(m_stream, m_gain);
    // The mixer starts with the first device, sized for its requests; a device opened again later
    // keeps it, and the frames ahead play on it.
    if (m_ahead > 0 && !m_ring) {
        SDL_AudioSpec device{};
        int device_frames = 0;
        if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(m_stream), &device, &device_frames)
            || device_frames <= 0) {
            device_frames = 1024;  // SDL's own default at 48 kHz
            device.freq = kAudioRate;
        }
        m_device_rate = device.freq;
        start_mixer(device_frames);
    }
    // Opened paused; start it unless the game paused it.
    if (!m_paused) {
        SDL_ResumeAudioStreamDevice(m_stream);
    }
    return true;
}

bool AudioOutput::watch(std::string& error) {
    const std::uint64_t now = SDL_GetTicks();
    const bool steady = m_watched_ms != 0 && now - m_watched_ms < kStallMs;
    m_watched_ms = now;
    if (!steady) {
        // The game stood still (in the background, or loading): the device may well have too.
        // Count from now.
        m_started_ms = now;
        return true;
    }
    if (!m_callback || m_paused) {
        return true;
    }
    const std::uint64_t last = std::max(m_fed_ms.load(std::memory_order_relaxed), m_started_ms);
    if (now - last < kStallMs) {
        return true;
    }
    if (m_stream != nullptr) {
        SDL_DestroyAudioStream(m_stream);
        m_stream = nullptr;
    }
    return start(error);
}

AudioOutput::~AudioOutput() {
    // After SDL_Quit (or SDL_QuitSubSystem) the stream is already gone with the audio
    // subsystem: destroying it again touches SDL's freed state and faults. That is what an exit
    // handler or static owner outliving the Platform sees, so leave the stream alone then.
    if (m_stream != nullptr && (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) != 0) {
        SDL_DestroyAudioStream(m_stream);
    }
    // After the stream: SDL's thread no longer reads the frames ahead.
    stop_mixer();
}

void AudioOutput::feed(void* self, SDL_AudioStream* stream, int additional, int /*total*/) {
    auto* out = static_cast<AudioOutput*>(self);
    out->m_fed_ms.store(SDL_GetTicks(), std::memory_order_relaxed);
    const std::uint64_t now = SDL_GetTicksNS();
    out->m_requests.fetch_add(1, std::memory_order_relaxed);
    if (const std::uint64_t last = out->m_last_request_ns.exchange(now, std::memory_order_relaxed);
        last != 0) {
        keep_worst(out->m_worst_gap_ns, now - last);
    }
    if (additional <= 0) {
        return;
    }
    const int frames = (additional + kFrameBytes - 1) / kFrameBytes;
    out->m_scratch.assign(static_cast<std::size_t>(frames * kAudioChannels), 0);
    if (out->m_ring) {
        out->take_ahead(out->m_scratch);
    } else {
        out->m_callback(out->m_scratch);
        keep_worst(out->m_worst_render_ns, SDL_GetTicksNS() - now);
    }
    SDL_PutAudioStreamData(stream, out->m_scratch.data(), frames * kFrameBytes);
}

bool AudioOutput::queue(std::span<const std::int16_t> samples) {
    if (samples.empty() || m_stream == nullptr) {
        return true;
    }
    return SDL_PutAudioStreamData(m_stream, samples.data(), static_cast<int>(samples.size_bytes()));
}

int AudioOutput::queued_frames() const {
    if (m_stream == nullptr) {
        return 0;
    }
    const int bytes = SDL_GetAudioStreamQueued(m_stream);
    return bytes > 0 ? bytes / kFrameBytes : 0;
}

void AudioOutput::pause(bool paused) {
    m_paused = paused;
    // A pause is not a late request.
    m_last_request_ns.store(0, std::memory_order_relaxed);
    if (m_stream == nullptr) {
        return;
    }
    if (paused) {
        SDL_PauseAudioStreamDevice(m_stream);
    } else {
        SDL_ResumeAudioStreamDevice(m_stream);
    }
}

void AudioOutput::set_gain(float gain) {
    m_gain = gain;
    if (m_stream == nullptr) {
        return;
    }
    SDL_SetAudioStreamGain(m_stream, gain);
}

}  // namespace openrac::platform

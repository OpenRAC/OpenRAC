// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sound output: one SDL3 audio stream, 48 kHz, stereo, signed 16-bit,
// which is what the console's sound processor mixes to. The replacement of
// the game's sound library (989snd, at its API) mixes into it in one of two
// ways:
//
//   - pull: a callback that SDL's audio thread calls whenever the device
//     needs more, with a buffer to fill (the way the console's mixer runs);
//   - push: queue() from the game's thread, with queued_frames() to keep the
//     queue about one frame deep.
//
// A callback, when given, runs on SDL's audio thread: it must not touch the
// game's state without a lock of its own.
//
// Pulled with frames ahead (open's `ahead_frames`), the callback runs on a mixer thread of the
// output's own instead, which keeps that many frames rendered before the device asks for them (at
// least one request of the device and a chunk); SDL's thread only copies them out. The device's
// request comes every few milliseconds (WASAPI's period is 10 ms) and must be answered in time or
// the device plays a gap: a callback that waits for the game's lock, or for the processor while
// the graphics driver compiles, makes one. The frames ahead are the time such a wait may take;
// they are heard that much later.

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include "common/joining_thread.h"
#include "platform/audio_ring.h"

struct SDL_AudioStream;

namespace openrac::platform {

inline constexpr int kAudioRate = 48000;
inline constexpr int kAudioChannels = 2;

class AudioOutput {
public:
    // Fill `samples` (interleaved left, right; samples.size() / 2 frames).
    using Callback = std::function<void(std::span<std::int16_t> samples)>;

    // Opens the default playback device and starts it. Without a callback,
    // the output plays what queue() hands it, and silence when it runs dry. With a callback and
    // `ahead_frames` > 0, the callback runs on a mixer thread that many frames ahead of the device.
    static std::unique_ptr<AudioOutput> open(
        std::string& error, Callback callback = {}, int ahead_frames = 0
    );

    ~AudioOutput();
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    // Push model: append interleaved stereo samples.
    bool queue(std::span<const std::int16_t> samples);

    // Frames queued and not played yet.
    int queued_frames() const;

    void pause(bool paused);

    // Output gain, 0 (silent) to 1 (as mixed).
    void set_gain(float gain);

    // Call once a frame from the game's thread. When the callback has not run for kStallMs while
    // playing (the system took the device and never gave it back, as Android does after a call or
    // another app's sound), the stream is opened again on the default device. Returns false, with
    // `error`, when that failed; the next call tries again.
    bool watch(std::string& error);

    static constexpr std::uint64_t kStallMs = 1500;

    // Frames the callback has rendered that the device has not taken yet (the mixer's frames
    // ahead, with the chunk it is rendering); 0 without a mixer. Any thread.
    std::size_t unheard_frames() const;

    // How the device was served since the last take (any thread; the game's thread takes it once a
    // frame, for the log and the performance capture).
    struct Health {
        std::uint32_t requests = 0;      // the device's requests for sound
        std::uint32_t short_frames = 0;  // frames of silence it got because the mixer was behind
        double worst_gap_ms = 0.0;       // the longest time between two requests
        double worst_render_ms = 0.0;    // the longest run of the callback
    };

    Health take_health();

    // The mixer's rendering: a chunk at a time, so the frames ahead are refilled soon after each
    // request. 240 frames are 5 ms.
    static constexpr int kChunkFrames = 240;

private:
    AudioOutput() = default;

    bool start(std::string& error);
    // Starts the mixer for a device that asks for `device_frames` at a time.
    void start_mixer(int device_frames);
    void stop_mixer();
    void mix();
    // Fills `out` from the frames ahead; silence where there are none.
    void take_ahead(std::span<std::int16_t> out);

    static void feed(void* self, SDL_AudioStream* stream, int additional, int total);

    SDL_AudioStream* m_stream = nullptr;
    Callback m_callback;
    std::vector<std::int16_t> m_scratch;
    float m_gain = 1.0f;
    bool m_paused = false;
    // SDL_GetTicks() of the callback's last run (written on the audio thread), of the stream's
    // start, and of the last watch() (a longer gap means the game itself stood still).
    std::atomic<std::uint64_t> m_fed_ms{0};
    std::uint64_t m_started_ms = 0;
    std::uint64_t m_watched_ms = 0;

    // The frames ahead, and the count of frames whose render has begun: a chunk counts from before
    // its render, so that unheard_frames() never misses the frames the callback is rendering.
    int m_ahead = 0;   // asked for
    int m_target = 0;  // kept ready (start_mixer)
    int m_device_rate = kAudioRate;
    std::unique_ptr<AudioRing> m_ring;
    std::atomic<std::uint64_t> m_reserved{0};
    std::mutex m_mixer_mutex;
    std::condition_variable m_mixer_wake;
    bool m_mixer_stop = false;  // under m_mixer_mutex
    JoiningThread m_mixer;

    // Health, in nanoseconds; the worsts are kept by compare and swap.
    std::atomic<std::uint32_t> m_requests{0};
    std::atomic<std::uint32_t> m_short_frames{0};
    std::atomic<std::uint64_t> m_worst_gap_ns{0};
    std::atomic<std::uint64_t> m_worst_render_ns{0};
    std::atomic<std::uint64_t> m_last_request_ns{0};  // 0: none since the start or a pause
};

}  // namespace openrac::platform

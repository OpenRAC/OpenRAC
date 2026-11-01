// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The frames an audio output mixes ahead of its device (audio.h): interleaved stereo in a ring,
// written by one thread (the mixer) and read by one other (SDL's audio thread), without a lock.
// The counts are frames since the ring was made, so the two threads only ever compare them.

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace openrac::platform {

class AudioRing {
public:
    static constexpr int kChannels = 2;

    // Room for `frames` frames.
    explicit AudioRing(int frames)
        : m_frames(static_cast<std::size_t>(std::max(frames, 1))),
          m_samples(m_frames * kChannels, 0) {}

    std::size_t capacity() const { return m_frames; }

    // Frames written and not read yet. Either thread.
    std::size_t ready() const {
        return static_cast<std::size_t>(
            m_written.load(std::memory_order_acquire) - m_read.load(std::memory_order_acquire)
        );
    }

    // Frames that can be written now. Either thread.
    std::size_t room() const { return m_frames - ready(); }

    std::uint64_t read_count() const { return m_read.load(std::memory_order_acquire); }

    // The writer: appends `samples` (whole frames, at most room()). Returns false, writing nothing,
    // when they do not fit.
    bool write(std::span<const std::int16_t> samples) {
        const std::size_t frames = samples.size() / kChannels;
        const std::uint64_t at = m_written.load(std::memory_order_relaxed);
        if (frames
            > m_frames - static_cast<std::size_t>(at - m_read.load(std::memory_order_acquire))) {
            return false;
        }
        copy_in(static_cast<std::size_t>(at % m_frames), samples.data(), frames);
        m_written.store(at + frames, std::memory_order_release);
        return true;
    }

    // The reader: fills `out` (whole frames) with the oldest frames written, and silence past the
    // last one. Returns the frames it had.
    std::size_t read(std::span<std::int16_t> out) {
        const std::size_t frames = out.size() / kChannels;
        const std::uint64_t at = m_read.load(std::memory_order_relaxed);
        const auto have = std::min<std::size_t>(
            frames, static_cast<std::size_t>(m_written.load(std::memory_order_acquire) - at)
        );
        copy_out(static_cast<std::size_t>(at % m_frames), out.data(), have);
        std::fill(out.begin() + static_cast<std::ptrdiff_t>(have * kChannels), out.end(), 0);
        m_read.store(at + have, std::memory_order_release);
        return have;
    }

private:
    // `frames` frames from ring position `first`, wrapping at the end.
    void copy_in(std::size_t first, const std::int16_t* from, std::size_t frames) {
        const std::size_t run = std::min(frames, m_frames - first);
        std::memcpy(&m_samples[first * kChannels], from, run * kFrameBytes);
        std::memcpy(m_samples.data(), from + run * kChannels, (frames - run) * kFrameBytes);
    }

    void copy_out(std::size_t first, std::int16_t* to, std::size_t frames) const {
        const std::size_t run = std::min(frames, m_frames - first);
        std::memcpy(to, &m_samples[first * kChannels], run * kFrameBytes);
        std::memcpy(to + run * kChannels, m_samples.data(), (frames - run) * kFrameBytes);
    }

    static constexpr std::size_t kFrameBytes = kChannels * sizeof(std::int16_t);

    std::size_t m_frames;
    std::vector<std::int16_t> m_samples;
    std::atomic<std::uint64_t> m_written{0};
    std::atomic<std::uint64_t> m_read{0};
};

}  // namespace openrac::platform

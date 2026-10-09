// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/movie.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A movie opened for playback: the PSS split into its streams
// (assets/sound/pss.h), one audio channel decoded to 48 kHz stereo, the
// MPEG-2 video decoder (mpeg2.h), the YCbCr to RGBA conversion, and a small
// player that hands out the frame due at a time and the audio as the output
// pulls it.
//
// Audio channel: RAC1 (NTSC-U `StartPssMovie`) passes the game's language
// as the ADPCM channel (0 English, 2 French, 3 German, 4 Spanish, 5
// Italian); the files carry channel 0 only or 0, 2, 3, 4, 5. A language
// whose channel is missing falls back to channel 0 here (the game would get
// no audio). The other games' channel numbering is not known yet; the caller
// passes the channel it wants.
//
// Colour: the console's image decoder converts limited-range BT.601 YCbCr to
// full-range RGB, each chroma sample covering 2x2 pixels (no chroma
// filtering). This computes the same with the standard BT.601 coefficients
// in 16.16 fixed point, rounded and clamped; the hardware's own fixed-point
// constants are not reproduced.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "media/mpeg2.h"

namespace openrac::media {

// The port's mixer rate.
inline constexpr std::uint32_t kMovieOutputRate = 48000;

class Movie {
public:
    // Demuxes `file` (a whole PSS), decodes the audio of `channel` (channel 0
    // when absent, the first channel when there is no 0) and reads the
    // sequence header. Throws assets::AssetError or VideoError.
    static Movie open(std::span<const std::uint8_t> file, std::uint8_t channel);

    const VideoSequence& sequence() const { return m_sequence; }

    double fps() const { return m_sequence.fps(); }

    // The chosen channel's audio at 48 kHz, interleaved left, right; empty
    // when the file has no audio.
    const std::vector<std::int16_t>& audio() const { return m_audio; }

    std::optional<std::uint8_t> audio_channel() const { return m_audio_channel; }

    std::uint32_t audio_rate() const { return m_audio_rate; }  // in the file

    const std::vector<std::uint8_t>& channels() const { return m_channels; }

    // The next frame in display order; null at the end.
    std::shared_ptr<const VideoFrame> next_frame() { return m_decoder->next_frame(); }

    const VideoStats& video_stats() const { return m_decoder->stats(); }

private:
    Movie() = default;

    std::unique_ptr<Mpeg2Decoder> m_decoder;
    VideoSequence m_sequence;
    std::vector<std::int16_t> m_audio;
    std::optional<std::uint8_t> m_audio_channel;
    std::uint32_t m_audio_rate = 0;
    std::vector<std::uint8_t> m_channels;
};

// Plays a Movie against a clock. The audio may be pulled from the audio
// thread while the game's thread asks for frames: the audio is fixed after
// open and its cursor is atomic.
class MoviePlayer {
public:
    explicit MoviePlayer(Movie movie) : m_movie(std::move(movie)) {}

    const Movie& movie() const { return m_movie; }

    // Copies the next audio into `out` (interleaved stereo, 48 kHz) and
    // returns the frames copied; the rest of `out` is silence.
    std::size_t read_audio(std::span<std::int16_t> out);

    // Seconds of audio handed out so far: the clock to show frames by when
    // the movie has audio.
    double audio_time() const;

    // The frame to show at `seconds` from the start (frame k is due at
    // k / fps): decodes forward as needed and keeps the last frame shown.
    // Null before the first frame is decoded or when the stream has none.
    std::shared_ptr<const VideoFrame> frame_at(double seconds);

    // The video has ended (and the last frame is shown).
    bool video_finished() const { return m_video_ended; }

    bool audio_finished() const;

private:
    Movie m_movie;
    std::atomic<std::size_t> m_audio_cursor{0};  // in frames
    std::shared_ptr<const VideoFrame> m_current;
    std::size_t m_shown = 0;  // frames taken from the decoder
    bool m_video_ended = false;
};

// Linear resampling of interleaved stereo to `to` Hz (48 kHz audio is
// returned as is). The console plays a 44.1 kHz stream at a pitch of
// rate * 0x1000 / 48000 through its Gaussian interpolation; this keeps the
// rate, not that filter.
std::vector<std::int16_t> resample_stereo(
    std::span<const std::int16_t> pcm, std::uint32_t from, std::uint32_t to
);

// One pixel, limited-range BT.601 to full-range RGB.
std::array<std::uint8_t, 3> ycbcr_to_rgb(std::uint8_t y, std::uint8_t cb, std::uint8_t cr);

// The frame as RGBA8 (alpha 255), rows top to bottom; `out` is resized to
// width * height * 4.
void frame_to_rgba(const VideoFrame& frame, std::vector<std::uint8_t>& out);

}  // namespace openrac::media

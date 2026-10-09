// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/movie.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The movie layer: BT.601 colour, the resampler, and a whole synthetic PSS
// (the synthetic MPEG-2 stream and two audio channels) opened, its channel
// chosen by language, and played against the audio clock.

#include "media/movie.h"

#include <cstring>
#include <vector>

#include "assets/bytes.h"
#include "assets/sound/pss.h"
#include "tests/check.h"
#include "tests/media/synthetic_mpeg2.h"

using namespace openrac::media;
using openrac::assets::ByteWriter;

namespace {

void colour() {
    auto rgb = [](std::uint8_t y, std::uint8_t cb, std::uint8_t cr, std::array<std::uint8_t, 3> want
               ) {
        return ycbcr_to_rgb(y, cb, cr) == want;
    };
    CHECK(rgb(16, 128, 128, {0, 0, 0}));
    CHECK(rgb(235, 128, 128, {255, 255, 255}));
    CHECK(rgb(126, 128, 128, {128, 128, 128}));
    // 75 % colour bars (BT.601 limited-range codes, themselves rounded).
    CHECK(rgb(65, 100, 212, {191, 0, 1}));
    CHECK(rgb(112, 72, 58, {0, 191, 0}));
    CHECK(rgb(35, 212, 114, {0, 1, 192}));

    VideoFrame f;
    f.width = 2;
    f.height = 2;
    f.y = {16, 235, 126, 16};
    f.cb = {128};
    f.cr = {128};
    std::vector<std::uint8_t> out;
    frame_to_rgba(f, out);
    CHECK(out.size() == 16);
    CHECK(out[0] == 0 && out[3] == 255 && out[4] == 255 && out[8] == 128 && out[15] == 255);
}

void resampling() {
    std::vector<std::int16_t> pcm;
    for (int i = 0; i < 441; ++i) {
        pcm.push_back(static_cast<std::int16_t>(i));
        pcm.push_back(static_cast<std::int16_t>(-i));
    }
    CHECK(resample_stereo(pcm, 48000, 48000) == pcm);
    const auto r = resample_stereo(pcm, 44100, 48000);
    CHECK(r.size() == 2 * 480);
    CHECK(r[0] == 0 && r[1] == 0);
    // Output sample 100 is at 91.875 in the source.
    CHECK(r[200] == 91 && r[201] == -91);
}

void pack(std::vector<std::uint8_t>& out) {
    const std::uint8_t header[] = {0, 0, 1, 0xba, 0x44, 0, 4, 0, 4, 1, 0x01, 0x89, 0xc3, 0xf8};
    out.insert(out.end(), std::begin(header), std::end(header));
}

void pes(
    std::vector<std::uint8_t>& out, std::uint8_t id, const std::vector<std::uint8_t>& payload
) {
    const std::size_t length = 3 + payload.size();
    out.insert(
        out.end(),
        {0, 0, 1, id, static_cast<std::uint8_t>(length >> 8), static_cast<std::uint8_t>(length)}
    );
    out.insert(out.end(), {0x81, 0, 0});
    out.insert(out.end(), payload.begin(), payload.end());
}

// An audio channel: SShd + SSbd + `blocks` stereo blocks of one frame per
// side, each frame a constant nibble so the decoded level is known.
std::vector<std::uint8_t> audio_channel(
    std::uint8_t channel, std::uint32_t rate, std::size_t blocks, std::uint8_t nibble
) {
    ByteWriter h;
    h.put_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>("SShd"), 4));
    for (const std::uint32_t w : {0x18u, 0x10u, rate, 2u, 0x10u, ~0u, ~0u}) {
        h.put<std::uint32_t>(w);
    }
    h.put_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>("SSbd"), 4));
    h.put<std::uint32_t>(static_cast<std::uint32_t>(blocks * 32));
    for (std::size_t b = 0; b < blocks * 2; ++b) {
        std::vector<std::uint8_t> frame(16, static_cast<std::uint8_t>(nibble | nibble << 4));
        frame[0] = 12;  // shift 12, filter 0: the sample is the nibble
        frame[1] = 0;
        h.put_bytes(frame);
    }
    std::vector<std::uint8_t> payload = {0xff, 0xa1, 0x00, channel};
    payload.insert(payload.end(), h.bytes().begin(), h.bytes().end());
    return payload;
}

std::vector<std::uint8_t> synthetic_pss() {
    const std::vector<std::uint8_t> video = openrac::test::synthetic_ipb_stream();
    std::vector<std::uint8_t> f;
    pack(f);
    const std::size_t half = video.size() / 2;
    pes(f, 0xe0, {video.begin(), video.begin() + static_cast<long>(half)});
    pes(f, 0xbd, audio_channel(0, 44100, 30, 1));
    pes(f, 0xbd, audio_channel(2, 48000, 60, 2));
    pack(f);
    pes(f, 0xe0, {video.begin() + static_cast<long>(half), video.end()});
    f.insert(f.end(), {0, 0, 1, 0xb9});
    f.resize(f.size() + 100, 0);
    return f;
}

void playback() {
    const std::vector<std::uint8_t> file = synthetic_pss();
    Movie french = Movie::open(file, 2);
    CHECK(french.channels() == std::vector<std::uint8_t>({0, 2}));
    CHECK(french.audio_channel() == std::uint8_t{2} && french.audio_rate() == 48000);
    // 60 blocks of 28 samples, both sides 2.
    CHECK(french.audio().size() == 2 * 60 * 28);
    CHECK(french.audio()[0] == 2 && french.audio()[1] == 2);
    CHECK(french.sequence().width == 32 && french.fps() == 30.0);

    // German is not in the file: channel 0, 44.1 kHz resampled to 48 kHz.
    Movie fallback = Movie::open(file, 3);
    CHECK(fallback.audio_channel() == std::uint8_t{0} && fallback.audio_rate() == 44100);
    CHECK(fallback.audio().size() == 2 * (30 * 28 * 48000 / 44100));
    CHECK(fallback.audio()[10] == 1);

    MoviePlayer player(std::move(french));
    CHECK(player.frame_at(0.0) && player.frame_at(0.0)->temporal_reference == 0);
    CHECK(player.frame_at(1.0 / 30.0)->temporal_reference == 1);
    CHECK(player.frame_at(0.04)->temporal_reference == 1);
    CHECK(player.frame_at(2.5 / 30.0)->temporal_reference == 2);
    CHECK(!player.video_finished());
    CHECK(player.frame_at(1.0)->temporal_reference == 2);  // the last frame stays
    CHECK(player.video_finished());

    std::vector<std::int16_t> buffer(2 * 1000, 7);
    CHECK(player.read_audio(buffer) == 1000);
    CHECK(buffer[0] == 2 && buffer[1999] == 2);
    CHECK(player.audio_time() == 1000.0 / 48000.0);
    CHECK(player.read_audio(buffer) == 680);
    CHECK(buffer[2 * 679] == 2 && buffer[2 * 680] == 0 && buffer[1999] == 0);
    CHECK(player.audio_finished());
    CHECK(player.read_audio(buffer) == 0);
}

}  // namespace

int main() {
    colour();
    resampling();
    playback();
    return openrac::test::result();
}

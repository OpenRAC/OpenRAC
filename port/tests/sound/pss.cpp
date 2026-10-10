// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/pss.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The PSS demuxer on a synthetic program stream (packs, a system header,
// video split over packets with a PTS, two audio channels interleaved with
// it, padding, the end code and sector padding), and the audio
// de-interleaving against the ADPCM decoder.

#include "assets/sound/pss.h"

#include <optional>
#include <vector>

#include "assets/sound/adpcm.h"
#include "tests/check.h"

using namespace openrac::assets;

namespace {

void pack(std::vector<u8>& out) {
    // An MPEG-2 pack header with one stuffing byte.
    const u8 header[] = {0, 0, 1, kPssPack, 0x44, 0, 4, 0, 4, 1, 0x01, 0x89, 0xc3, 0xf9, 0xff};
    out.insert(out.end(), std::begin(header), std::end(header));
}

void pes(std::vector<u8>& out, u8 id, std::optional<u64> pts, const std::vector<u8>& payload) {
    std::vector<u8> data;
    u8 flags = 0;
    if (pts) {
        const u64 t = *pts;
        flags = 0x80;
        data = {
            static_cast<u8>(0x21 | ((t >> 29) & 0xe)),
            static_cast<u8>(t >> 22),
            static_cast<u8>((t >> 14) | 1),
            static_cast<u8>(t >> 7),
            static_cast<u8>((t << 1) | 1),
        };
    }
    data.insert(data.end(), {0xff, 0xff, 0xff});  // stuffing inside the header
    const std::size_t length = 3 + data.size() + payload.size();
    out.insert(out.end(), {0, 0, 1, id, static_cast<u8>(length >> 8), static_cast<u8>(length)});
    out.insert(out.end(), {0x81, flags, static_cast<u8>(data.size())});
    out.insert(out.end(), data.begin(), data.end());
    out.insert(out.end(), payload.begin(), payload.end());
}

std::vector<u8> audio_header(u32 rate, u32 channels, u32 interleave, u32 body) {
    ByteWriter h;
    h.put_bytes(std::span<const u8>(reinterpret_cast<const u8*>("SShd"), 4));
    for (const u32 w : {0x18u, kPssAudioAdpcm, rate, channels, interleave, ~0u, ~0u}) {
        h.put<u32>(w);
    }
    h.put_bytes(std::span<const u8>(reinterpret_cast<const u8*>("SSbd"), 4));
    h.put<u32>(body);
    return h.bytes();
}

std::vector<u8> concat(std::vector<u8> a, const std::vector<u8>& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

std::vector<u8> slice(const std::vector<u8>& v, std::size_t from, std::size_t to) {
    return {v.begin() + static_cast<long>(from), v.begin() + static_cast<long>(to)};
}

std::vector<u8> substream(u8 channel, const std::vector<u8>& bytes) {
    return concat({0xff, 0xa1, 0x00, channel}, bytes);
}

template <typename F>
bool throws(F f) {
    try {
        f();
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

void demux_synthetic_stream() {
    std::vector<u8> video(700);
    for (std::size_t i = 0; i < video.size(); ++i) {
        video[i] = static_cast<u8>(i * 7);
    }
    const auto a0 = concat(audio_header(48000, 2, 0x20, 64), std::vector<u8>(64, 0x11));
    const auto a2 = concat(audio_header(48000, 2, 0x20, 64), std::vector<u8>(64, 0x22));
    std::vector<u8> f;
    pack(f);
    pes(f, kPssSystemHeader, std::nullopt, {0x80, 1, 2, 3});
    pes(f, kPssVideo, 0x1'2345'6789ull, slice(video, 0, 300));
    pes(f, kPssPrivate1, 900, substream(0, slice(a0, 0, 50)));
    pes(f, kPssPrivate1, 900, substream(2, slice(a2, 0, 70)));
    pack(f);
    pes(f, kPssVideo, std::nullopt, slice(video, 300, video.size()));
    pes(f, kPssPrivate1, std::nullopt, substream(0, slice(a0, 50, a0.size())));
    pes(f, kPssPrivate1, std::nullopt, substream(2, slice(a2, 70, a2.size())));
    pes(f, kPssPadding, std::nullopt, std::vector<u8>(20, 0xff));
    f.insert(f.end(), {0, 0, 1, kPssEnd});
    f.resize(f.size() + 12, 0);

    const PssStreams d = demux_pss(ByteView(f));
    CHECK(d.video == video);
    CHECK(d.audio.size() == 2 && d.audio.count(0) == 1 && d.audio.count(2) == 1);
    CHECK(d.audio.at(0) == a0);
    CHECK(d.audio.at(2) == a2);
    CHECK(d.first_video_pts == 0x1'2345'6789ull);
    CHECK(d.packs == 2 && d.video_packets == 2 && d.audio_packets == 4 && d.other_packets == 2);
    CHECK(d.ended);
    CHECK(d.trailing == 12);

    const PssAudio a = PssAudio::parse(ByteView(d.audio.at(0)));
    CHECK(a.header.rate == 48000 && a.header.channels == 2 && a.header.interleave == 0x20);
    CHECK(a.header.body_size == 64 && a.header.loop_start == -1);
    CHECK(a.blocks() == 1 && a.samples() == 56);

    // A broken start code and a truncated packet are errors.
    std::vector<u8> bad = f;
    bad[15] = 7;
    CHECK(throws([&] { demux_pss(ByteView(bad)); }));
    CHECK(throws([&] { demux_pss(ByteView(f.data(), 40)); }));
}

// Blocks of `interleave` bytes alternate L, R; each channel decodes as one
// stream (the history carried across its blocks), exactly as its frames
// decoded back to back.
void audio_deinterleave_and_decode() {
    auto frame = [](u8 seed) {
        std::vector<u8> v = {0x14, 0};  // shift 4, filter 1: the history matters
        for (u8 k = 0; k < 14; ++k) {
            v.push_back(static_cast<u8>(seed * 17 + k * 29));
        }
        return v;
    };
    std::vector<std::vector<u8>> left;
    std::vector<std::vector<u8>> right;
    for (u8 k = 0; k < 6; ++k) {
        left.push_back(frame(k));
        right.push_back(frame(static_cast<u8>(100 + k)));
    }
    std::vector<u8> body;
    std::vector<u8> all_left;
    std::vector<u8> all_right;
    for (std::size_t b = 0; b < 3; ++b) {
        for (const auto* side : {&left, &right}) {
            body.insert(body.end(), (*side)[2 * b].begin(), (*side)[2 * b].end());
            body.insert(body.end(), (*side)[2 * b + 1].begin(), (*side)[2 * b + 1].end());
        }
    }
    for (std::size_t k = 0; k < 6; ++k) {
        all_left.insert(all_left.end(), left[k].begin(), left[k].end());
        all_right.insert(all_right.end(), right[k].begin(), right[k].end());
    }
    const auto bytes = concat(audio_header(48000, 2, 0x20, static_cast<u32>(body.size())), body);
    const PssAudio a = PssAudio::parse(ByteView(bytes));
    const std::vector<s16> pcm = a.decode();
    const std::vector<s16> ref_l = decode_adpcm(ByteView(all_left));
    const std::vector<s16> ref_r = decode_adpcm(ByteView(all_right));
    CHECK(pcm.size() == 2 * 6 * kAdpcmFrameSamples);
    bool same = ref_l.size() == 6 * kAdpcmFrameSamples;
    for (std::size_t i = 0; same && i < ref_l.size(); ++i) {
        same = pcm[2 * i] == ref_l[i] && pcm[2 * i + 1] == ref_r[i];
    }
    CHECK(same);
    CHECK(throws([&] { parse_pss_audio_header(ByteView(bytes.data(), 0x20)); }));

    // A mono stream is duplicated on both sides.
    const auto mono = concat(audio_header(44100, 1, 0x10, 32), slice(all_left, 0, 32));
    const std::vector<s16> m = PssAudio::parse(ByteView(mono)).decode();
    CHECK(m.size() == 2 * 56);
    CHECK(m[0] == m[1] && m[2 * 30] == ref_l[30] && m[2 * 30 + 1] == ref_l[30]);
}

}  // namespace

int main() {
    demux_synthetic_stream();
    audio_deinterleave_and_decode();
    return openrac::test::result();
}

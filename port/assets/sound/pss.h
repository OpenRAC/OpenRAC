// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/pss.rs and
// docs/formats/pss.md: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// PSS movies: an MPEG-2 program stream (ISO/IEC 13818-1, a public standard)
// whose audio is SPU ADPCM in private stream 1. This splits one into its
// video elementary stream (decoded by openrac_media) and its audio channels.
//
//   pack header   00 00 01 BA, MPEG-2 form: 14 bytes + (byte 13 & 7) stuffing
//   system header 00 00 01 BB (first pack)
//   video PES     00 00 01 E0: MPEG-2 PES header (PTS / DTS), then video
//   audio PES     00 00 01 BD: PES header, the 4-byte sub-stream header
//                 FF A1 00 cc (cc = the audio channel), then audio bytes
//   padding       00 00 01 BE
//   end code      00 00 01 B9, then zero padding to the sector end
//
// Each audio channel's bytes, concatenated in file order, start with a
// 0x28-byte header ("SShd": type 0x10 = SPU ADPCM, rate, channels,
// interleave; "SSbd": body size), then blocks of `interleave` bytes per
// channel in turn (L, R, L, R...), each channel one continuous ADPCM stream.
//
// The container is the console's movie format. What ReRAC checked is RAC1
// (NTSC-U): 84 files, channels 0 only or 0, 2, 3, 4, 5 (English, French,
// German, Spanish, Italian: the game passes its language as the channel),
// stereo, interleave 0x20, 48 kHz (44.1 kHz in four files). The later games'
// movies are not surveyed yet.

#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include "assets/bytes.h"

namespace openrac::assets {

// Stream ids.
inline constexpr u8 kPssPack = 0xba;
inline constexpr u8 kPssSystemHeader = 0xbb;
inline constexpr u8 kPssPrivate1 = 0xbd;
inline constexpr u8 kPssPadding = 0xbe;
inline constexpr u8 kPssEnd = 0xb9;
inline constexpr u8 kPssVideo = 0xe0;  // the first video stream; RAC1's movies use only this one

inline constexpr std::size_t kPssAudioHeaderSize = 0x28;
inline constexpr u32 kPssAudioAdpcm = 0x10;  // SShd type of SPU ADPCM

enum class PssStreamKind : u8 {
    Video,  // MPEG video stream 0xE0 + n
    Audio,  // private stream 1 with the FF A1 00 cc header: audio channel cc
    Other,  // padding, the system header, anything else
};

// One PES packet.
struct PssPacket {
    std::size_t offset = 0;  // of its start code in the file
    PssStreamKind kind = PssStreamKind::Other;
    u8 id = 0;               // the stream id; for audio, the channel
    std::optional<u64> pts;  // 90 kHz time stamps (33 bits)
    std::optional<u64> dts;
    ByteView payload;  // after the PES header (and the sub-stream header for audio)
};

// The packets of a program stream in file order.
class PssDemuxer {
public:
    explicit PssDemuxer(ByteView data) : m_data(data) {}

    // The next packet, or none after the end code or at the end of the data.
    // Throws AssetError on a malformed stream.
    std::optional<PssPacket> next();

    std::size_t position() const { return m_pos; }

    std::size_t packs() const { return m_packs; }

    bool ended() const { return m_ended; }

private:
    ByteView m_data;
    std::size_t m_pos = 0;
    std::size_t m_packs = 0;
    bool m_ended = false;
};

// A whole movie split by stream.
struct PssStreams {
    std::vector<u8> video;                // the 0xE0 payloads in file order
    std::map<u8, std::vector<u8>> audio;  // channel -> header + interleaved body
    std::optional<u64> first_video_pts;   // of the first video packet with one
    std::size_t packs = 0;
    std::size_t video_packets = 0;
    std::size_t audio_packets = 0;
    std::size_t other_packets = 0;
    bool ended = false;        // the end code was found
    std::size_t trailing = 0;  // bytes after the end code (sector padding)
};

PssStreams demux_pss(ByteView data);

// The SShd / SSbd header at the start of an audio channel.
struct PssAudioHeader {
    u32 type = 0;  // 0x10: SPU ADPCM
    u32 rate = 0;
    u32 channels = 0;
    u32 interleave = 0;  // bytes per channel block
    s32 loop_start = 0;
    s32 loop_end = 0;
    u32 body_size = 0;  // SSbd
};

// Throws AssetError unless the header is SPU ADPCM in 1 or 2 channels with a
// frame-aligned interleave.
PssAudioHeader parse_pss_audio_header(ByteView bytes);

// One audio channel of a movie (PssStreams::audio[c]).
struct PssAudio {
    PssAudioHeader header;
    ByteView body;  // clipped to the SSbd size

    static PssAudio parse(ByteView bytes);

    std::size_t blocks() const;   // whole blocks per channel
    std::size_t samples() const;  // per channel
    double seconds() const;       // at the header's rate

    // Both channels (a mono stream duplicated) decoded with the rounded SPU
    // ADPCM decoder, each channel one stream from zero history, at the
    // header's rate, interleaved left, right. A trailing partial block is
    // dropped.
    std::vector<s16> decode() const;
};

}  // namespace openrac::assets

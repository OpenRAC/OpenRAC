// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "sound.h"

#include <algorithm>
#include <cstring>

namespace sys {
namespace {

// The console's ADPCM: each 16-byte block has a shift and a predictor
// number, a flag byte and 28 four-bit samples; a sample is predicted from
// the two before it with one of five pairs of weights (in 64ths).
constexpr s32 kWeights[5][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};

u32 big32(const u8* p) {
    return (u32{p[0]} << 24) | (u32{p[1]} << 16) | (u32{p[2]} << 8) | p[3];
}

}  // namespace

// A stream file: "VAGp", then in big-endian words a version, nothing, the
// size of the sound data and the sampling rate, a name, and from byte 48 on
// the blocks.
bool Sound::load(u32 sector, Piece& piece) {
    u8 head[Disc::kSector];
    if (!disc_.read(sector, 1, head) || std::memcmp(head, "VAGp", 4) != 0) {
        return false;
    }
    u32 bytes = big32(head + 12);
    piece.rate = big32(head + 16);
    if (bytes == 0 || bytes > 64u * 1024 * 1024 || piece.rate < 4000 || piece.rate > 96000) {
        return false;
    }
    u32 sectors = static_cast<u32>((48 + u64{bytes} + Disc::kSector - 1) / Disc::kSector);
    std::vector<u8> file(std::size_t{sectors} * Disc::kSector);
    if (!disc_.read(sector, sectors, file.data())) {
        return false;
    }
    piece.data.assign(file.begin() + 48, file.begin() + 48 + (bytes & ~15u));
    // A last block flagged "the end, and not to be played" is not sound.
    while (piece.data.size() >= 16 && piece.data[piece.data.size() - 15] == 7) {
        piece.data.resize(piece.data.size() - 16);
    }
    return !piece.data.empty();
}

bool Sound::play(u32 handle, u32 sector, int volume, u32 group, bool repeat, u32 after) {
    Piece piece;
    if (!load(sector, piece)) {
        return false;
    }
    piece.volume = volume;
    piece.repeat = repeat;
    auto behind = after ? streams_.find(after) : streams_.end();
    if (behind != streams_.end()) {
        // Only what is playing stays ahead of it.
        Stream& stream = behind->second;
        stream.pieces.resize(1);
        stream.pieces.push_back(std::move(piece));
        return true;
    }
    Stream stream;
    stream.group = group;
    stream.pieces.push_back(std::move(piece));
    streams_[handle] = std::move(stream);
    return true;
}

void Sound::pause(u32 handle, bool paused) {
    auto found = streams_.find(handle);
    if (found != streams_.end()) {
        found->second.paused = paused;
    }
}

void Sound::stop(u32 handle) {
    streams_.erase(handle);
}

void Sound::stop_all() {
    streams_.clear();
}

u32 Sound::remaining(u32 handle) const {
    auto found = streams_.find(handle);
    if (found == streams_.end()) {
        return 0;
    }
    const Stream& stream = found->second;
    const Piece& piece = stream.pieces.front();
    u64 samples = (piece.data.size() / 16 - std::min(stream.block, piece.data.size() / 16)) * 28
                  + (28 - stream.at);
    return static_cast<u32>(samples * 60 / piece.rate);
}

void Sound::set_group_volume(u32 group, int volume) {
    group_volume_[group] = volume;
}

// Move a stream one sample on: the next sample becomes the last one, and a
// new next one is decoded, from the next block, the next file stitched on,
// or the same file again.
bool Sound::advance(Stream& stream) {
    if (stream.at == 28) {
        Piece* piece = &stream.pieces.front();
        if (stream.block * 16 >= piece->data.size()) {
            if (stream.pieces.size() > 1) {
                stream.pieces.pop_front();
                piece = &stream.pieces.front();
            } else if (!piece->repeat) {
                return false;
            }
            stream.block = 0;
            stream.before[0] = stream.before[1] = 0;
        }
        const u8* block = piece->data.data() + stream.block * 16;
        stream.block++;
        unsigned shift = block[0] & 15, predictor = std::min<unsigned>((block[0] >> 4) & 7, 4);
        s32 w0 = kWeights[predictor][0], w1 = kWeights[predictor][1];
        for (unsigned n = 0; n < 28; n++) {
            s32 nibble = (block[2 + n / 2] >> ((n & 1) * 4)) & 15;
            s32 value = static_cast<s16>(nibble << 12) >> shift;
            value += (stream.before[0] * w0 + stream.before[1] * w1) >> 6;
            value = std::clamp(value, -32768, 32767);
            stream.samples[n] = static_cast<s16>(value);
            stream.before[1] = stream.before[0];
            stream.before[0] = value;
        }
        stream.at = 0;
    }
    stream.last = stream.next;
    stream.next = stream.samples[stream.at++];
    return true;
}

void Sound::mix(std::size_t frames, std::vector<s16>& out) {
    std::size_t first = out.size();
    out.resize(first + frames * 2, 0);
    std::vector<s32> sum(frames, 0);
    for (auto it = streams_.begin(); it != streams_.end();) {
        Stream& stream = it->second;
        bool over = false;
        if (!stream.paused) {
            auto group = group_volume_.find(stream.group);
            s32 master = group == group_volume_.end() ? 1024 : group->second;
            for (std::size_t n = 0; n < frames && !over; n++) {
                const Piece& piece = stream.pieces.front();
                // The output's rate against the file's: a straight line between
                // the two samples either side.
                stream.phase += static_cast<double>(piece.rate) / kRate;
                while (stream.phase >= 1.0 && !over) {
                    stream.phase -= 1.0;
                    over = !advance(stream);
                }
                if (!over) {
                    double value = stream.last + (stream.next - stream.last) * stream.phase;
                    s32 gain = (stream.pieces.front().volume * master) >> 10;
                    sum[n] += static_cast<s32>(value) * gain >> 10;
                }
            }
        }
        it = over ? streams_.erase(it) : std::next(it);
    }
    for (std::size_t n = 0; n < frames; n++) {
        s16 value = static_cast<s16>(std::clamp(sum[n], -32768, 32767));
        out[first + n * 2] = value;
        out[first + n * 2 + 1] = value;
    }
}

}  // namespace sys

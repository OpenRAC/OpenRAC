// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <deque>
#include <unordered_map>
#include <vector>

#include "disc.h"
#include "ps2/types.h"

namespace sys {

using ps2::s16;
using ps2::s32;
using ps2::u32;
using ps2::u64;
using ps2::u8;

// What the games hear, as far as it is made here: the streams of the sound
// library (music and speech), which are plain ADPCM files on the disc that
// a program names by sector. Sound effects from the banks are not made yet.
//
// Sound is produced in step with the machine: one field's worth after each
// field. So what the program is told about a stream (still playing, how
// long is left) and what is heard agree, and a run gives the same sound
// every time.
class Sound {
public:
    static constexpr int kRate = 48000;  // of what `mix` produces

    explicit Sound(Disc& disc) : disc_(disc) {}

    // Play the file at a sector: at once, as a stream of its own with the
    // handle given, or, when `after` names a stream that is playing, stitched
    // on behind what that one is playing (and in place of its repeating).
    // Volumes are in 1,024ths. True if it could be read.
    bool play(u32 handle, u32 sector, int volume, u32 group, bool repeat, u32 after);
    void pause(u32 handle, bool paused);
    void stop(u32 handle);
    void stop_all();

    bool playing(u32 handle) const { return streams_.count(handle) != 0; }

    // Sixtieths of a second of the stream's current file still to play.
    u32 remaining(u32 handle) const;
    void set_group_volume(u32 group, int volume);

    // The next `frames` of sound, appended to `out` as left and right.
    void mix(std::size_t frames, std::vector<s16>& out);

private:
    struct Piece {
        std::vector<u8> data;  // 16-byte blocks of 28 samples
        u32 rate = 44100;
        int volume = 1024;
        bool repeat = false;
    };

    struct Stream {
        std::deque<Piece> pieces;  // the first is playing
        u32 group = 0;
        bool paused = false;
        std::size_t block = 0;   // the next block to decode
        s32 before[2] = {0, 0};  // the two samples before it, for the decoder
        s16 samples[28] = {};    // the block being played
        unsigned at = 28;        // the next of those
        double phase = 0;        // how far between two samples the output is
        s16 last = 0, next = 0;  // the two samples around the output's position
    };

    bool load(u32 sector, Piece& piece);
    bool advance(Stream& stream);  // one sample on; false when the stream is over

    Disc& disc_;
    std::unordered_map<u32, Stream> streams_;
    std::unordered_map<u32, int> group_volume_;
};

}  // namespace sys

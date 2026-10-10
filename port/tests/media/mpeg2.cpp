// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/mpeg2.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The MPEG-2 decoder on a synthetic I, P, B stream (synthetic_mpeg2.h):
// the display order I(0), B(1), P(2) and samples computed by hand from the
// standard: DC / 8 (the mismatch toggle on F[7][7] adds less than 0.25),
// half-sample averaging, the non-intra dequantisation.

#include "media/mpeg2.h"

#include <memory>
#include <vector>

#include "tests/check.h"
#include "tests/media/synthetic_mpeg2.h"

using namespace openrac::media;

namespace {

void display_order_and_samples() {
    Mpeg2Decoder d(openrac::test::synthetic_ipb_stream());
    const auto seq = d.sequence();
    CHECK(seq && seq->width == 32 && seq->height == 16);
    CHECK(seq && seq->fps_num == 30 && seq->fps_den == 1 && seq->profile_level == 0x48);
    CHECK(seq && seq->progressive && seq->chroma_format == 1);
    std::vector<std::shared_ptr<const VideoFrame>> frames;
    while (auto f = d.next_frame()) {
        frames.push_back(std::move(f));
    }
    CHECK(frames.size() == 3);
    if (frames.size() != 3) {
        return;
    }
    CHECK(frames[0]->temporal_reference == 0 && frames[0]->type == PictureType::I);
    CHECK(frames[1]->temporal_reference == 1 && frames[1]->type == PictureType::B);
    CHECK(frames[2]->temporal_reference == 2 && frames[2]->type == PictureType::P);
    const VideoFrame& i = *frames[0];
    const VideoFrame& b = *frames[1];
    const VideoFrame& p = *frames[2];
    // I: macroblock 0 block 0 has DC 128, so F0 = 1024 and, the sum being
    // even, F[7][7] = 1: 128 plus a fraction, 128.
    CHECK(i.y[0] == 128);
    // Macroblock 1 block 0: DC 136 (the predictor 128 + 8); its blocks 1..3
    // continue from 136.
    CHECK(i.y[16] == 136);
    CHECK(i.y[31] == 136);
    bool grey = true;
    for (const std::uint8_t v : i.cb) {
        grey = grey && v == 128;
    }
    CHECK(grey);
    // P: macroblock 0 predicted at +1/2 sample: columns 0..14 are 128,
    // column 15 the average of 128 and 136, 132.
    bool row = true;
    for (int x = 0; x < 15; ++x) {
        row = row && p.y[x] == 128;
    }
    CHECK(row && p.y[15] == 132);
    CHECK(p.y[16] == 136);
    // B: backward from P; macroblock 0 block 3 has the residual (0, 2),
    // non-intra at qscale 16: F0 = (5 * 16 * 16) / 32 = 40, +5 on every
    // sample (and the mismatch toggle's fraction).
    CHECK(b.y[0] == 128);
    CHECK(b.y[8 * 32 + 8] == 133);
    CHECK(b.y[15 * 32 + 15] == 132 + 5);
    CHECK(d.stats().pictures[0] == 1 && d.stats().pictures[1] == 1 && d.stats().pictures[2] == 1);
    CHECK(d.stats().frames_out == 3);
    CHECK(d.stats().macroblocks == 6 && d.stats().skipped_macroblocks == 0);
    CHECK(!d.next_frame());
}

template <typename F>
bool throws_video_error(F f) {
    try {
        f();
    } catch (const VideoError&) {
        return true;
    }
    return false;
}

void errors() {
    // A field picture is reported, not guessed.
    {
        openrac::test::BitWriter w;
        openrac::test::mpeg2_sequence(w, 32, 16);
        w.start(0x00);
        w.put(0, 10);
        w.put(1, 3);
        w.put(0xffff, 16);
        w.put(0, 1);
        w.start(0xb5);
        w.put(8, 4);
        for (int k = 0; k < 4; ++k) {
            w.put(15, 4);
        }
        w.put(0, 2);
        w.put(1, 2);  // top field
        w.put(0, 14);
        w.start(0x01);
        w.put(0, 32);
        Mpeg2Decoder d(w.bytes());
        CHECK(throws_video_error([&] { d.next_frame(); }));
    }
    // A picture before any sequence header.
    {
        openrac::test::BitWriter w;
        openrac::test::mpeg2_picture(w, 0, 1, 15);
        w.start(0x01);
        w.put(0, 32);
        Mpeg2Decoder d(w.bytes());
        CHECK(throws_video_error([&] { d.next_frame(); }));
    }
    // An empty stream has no sequence and no frames.
    {
        Mpeg2Decoder d({});
        CHECK(!d.sequence());
        CHECK(!d.next_frame());
    }
}

}  // namespace

int main() {
    display_order_and_samples();
    errors();
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/mpeg2.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// A synthetic 32x16 MPEG-2 stream (two macroblocks, one slice per picture),
// written from the standard's syntax: an I picture with DC-only blocks, a P
// picture predicted at a half-sample vector, and a B picture predicted
// backward with one coded block. Shared by the media tests.

#pragma once

#include <cstdint>
#include <vector>

#include "tests/media/bit_writer.h"

namespace openrac::test {

inline void mpeg2_sequence(BitWriter& w, std::uint32_t width, std::uint32_t height) {
    w.start(0xb3);
    w.put(width, 12);
    w.put(height, 12);
    w.put(1, 4);       // aspect ratio
    w.put(5, 4);       // 30 fps
    w.put(10000, 18);  // bit rate
    w.put(1, 1);       // marker
    w.put(100, 10);    // vbv buffer size
    w.put(0, 1);       // constrained parameters
    w.put(0, 1);       // no intra matrix
    w.put(0, 1);       // no non-intra matrix
    w.start(0xb5);
    w.put(1, 4);  // sequence extension
    w.put(0x48, 8);
    w.put(1, 1);  // progressive
    w.put(1, 2);  // 4:2:0
    w.put(0, 2);
    w.put(0, 2);
    w.put(0, 12);
    w.put(1, 1);
    w.put(0, 8);
    w.put(0, 1);
    w.put(0, 2);
    w.put(0, 5);
}

inline void mpeg2_picture(
    BitWriter& w, std::uint32_t temporal_reference, std::uint32_t type, std::uint32_t f_code
) {
    w.start(0x00);
    w.put(temporal_reference, 10);
    w.put(type, 3);
    w.put(0xffff, 16);  // vbv delay
    if (type >= 2) {
        w.put(0, 1);
        w.put(7, 3);
    }
    if (type == 3) {
        w.put(0, 1);
        w.put(7, 3);
    }
    w.put(0, 1);
    w.start(0xb5);
    w.put(8, 4);  // picture coding extension
    for (int k = 0; k < 4; ++k) {
        w.put(f_code, 4);
    }
    w.put(0, 2);  // intra_dc_precision 8 bits
    w.put(3, 2);  // frame picture
    w.put(0, 1);
    w.put(1, 1);  // frame_pred_frame_dct
    w.put(0, 1);
    w.put(0, 1);  // q_scale_type 0
    w.put(0, 1);  // intra_vlc_format 0
    w.put(0, 1);
    w.put(0, 1);
    w.put(0, 1);
    w.put(1, 1);  // progressive_frame
    w.put(0, 1);
}

// A luma DC differential (sizes up to 4).
inline void mpeg2_dc_luma(BitWriter& w, int diff) {
    unsigned size = 0;
    for (unsigned a = static_cast<unsigned>(diff < 0 ? -diff : diff); a != 0; a >>= 1) {
        ++size;
    }
    static const char* const kCodes[] = {"100", "00", "01", "101", "110"};
    w.code(kCodes[size]);
    if (size > 0) {
        w.put(static_cast<std::uint32_t>(diff > 0 ? diff : diff + (1 << size) - 1), size);
    }
}

// An intra macroblock whose blocks carry only DC (luma differentials d, chroma 0).
inline void mpeg2_intra_macroblock(BitWriter& w, const int (&d)[4]) {
    w.code("1");  // macroblock_type intra (I picture)
    for (const int v : d) {
        mpeg2_dc_luma(w, v);
        w.code("10");  // end of block
    }
    for (int k = 0; k < 2; ++k) {
        w.code("00");  // chroma size 0
        w.code("10");
    }
}

inline std::vector<std::uint8_t> synthetic_ipb_stream() {
    BitWriter w;
    mpeg2_sequence(w, 32, 16);
    // I (temporal reference 0).
    mpeg2_picture(w, 0, 1, 15);
    w.start(0x01);
    w.put(8, 5);  // quantiser_scale_code
    w.put(0, 1);
    w.code("1");  // address increment 1
    mpeg2_intra_macroblock(w, {0, 0, 0, 0});
    w.code("1");
    mpeg2_intra_macroblock(w, {8, 0, 0, 0});
    // P (2): both macroblocks forward, f_code 1.
    mpeg2_picture(w, 2, 2, 1);
    w.start(0x01);
    w.put(8, 5);
    w.put(0, 1);
    w.code("1");    // address increment 1
    w.code("001");  // forward, not coded
    w.code("010");  // horizontal motion code 1 ("01"), sign bit 0: +1 half sample
    w.code("1");    // vertical motion code 0
    w.code("1");    // increment 1
    w.code("001");  // forward, not coded
    w.code("1");    // horizontal delta 0: the predictor's +1 again
    w.code("1");    // vertical 0
    // B (1): backward.
    mpeg2_picture(w, 1, 3, 1);
    w.start(0x01);
    w.put(8, 5);
    w.put(0, 1);
    w.code("1");    // address increment 1
    w.code("011");  // backward, coded
    w.code("1");    // motion 0, 0
    w.code("1");
    w.code("1101");  // cbp 4 (block 3)
    w.code("0100"
    );  // (0, 2): the first-coefficient "1s" rule does not apply to a code starting with 0
    w.put(0, 1);
    w.code("10");   // end of block
    w.code("1");    // increment 1: macroblock 1
    w.code("010");  // backward, not coded
    w.code("1");    // motion 0, 0
    w.code("1");
    w.start(0xb7);
    return w.bytes();
}

}  // namespace openrac::test

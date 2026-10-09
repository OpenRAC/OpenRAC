// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/wad.rs
// (tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// WAD decompression on hand-made packets, and compression round trips.

#include "assets/disc/wad.h"

#include <random>

#include "tests/check.h"

using namespace openrac::assets;
using namespace openrac::assets::disc;

namespace {

std::vector<u8> wad(std::initializer_list<u8> packets) {
    std::vector<u8> v = {'W', 'A', 'D', 0, 0, 0, 0, 'T', 'E', 'S', 'T', 0, 0, 0, 0, 0};
    v.insert(v.end(), packets);
    const u32 size = static_cast<u32>(v.size());
    std::memcpy(v.data() + 3, &size, 4);
    return v;
}

std::vector<u8> text(std::string_view s) {
    return {s.begin(), s.end()};
}

bool throws(const std::vector<u8>& stream) {
    try {
        wad_decompress(stream);
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

void decoding() {
    CHECK(wad_decompress(wad({0x01, 'a', 'b', 'c', 'd', 0xc0, 0x00})) == text("abcdddddddd"));
    CHECK(
        wad_decompress(wad({0x01, 'x', 'y', 'z', 'w', 0x21, 0x02, 0x00, '!', '?'}))
        == text("xyzwwww!?")
    );
    // A dummy packet carrying two literals, and a pad packet ending the stream.
    CHECK(
        wad_decompress(wad({0x01, 'a', 'b', 'c', 'd', 0x11, 0x02, 0x00, 'e', 'f'}))
        == text("abcdef")
    );
    CHECK(wad_decompress(wad({0x01, 'a', 'b', 'c', 'd', 0x12, 0x00, 0x00, 0xee})) == text("abcd"));
    // Two literal packets in a row are malformed.
    CHECK(throws(wad({0x01, 'a', 'b', 'c', 'd', 0x01, 'e', 'f', 'g', 'h'})));
    // A match before the start of the output.
    CHECK(throws(wad({0x01, 'a', 'b', 'c', 'd', 0x21, 0xfc, 0x00})));
    CHECK(throws(text("WAD")));
    CHECK(!is_wad(text("WAX0000000000000000")));
    CHECK(wad_compressed_size(wad({0x01, 'a', 'b', 'c', 'd'})) == 21);
}

void round_trip(const std::vector<u8>& data) {
    const std::vector<u8> packed = wad_compress(data);
    CHECK(is_wad(packed));
    CHECK(wad_compressed_size(packed) == packed.size());
    std::vector<u8> back;
    try {
        back = wad_decompress(packed);
    } catch (const AssetError& e) {
        std::fprintf(stderr, "round trip of %zu bytes: %s\n", data.size(), e.what());
    }
    CHECK(back == data);
}

void compression() {
    std::mt19937 rng(1234);
    round_trip({});
    for (const std::size_t n : {1, 2, 3, 4, 5, 17, 18, 19, 272, 273, 274, 275, 276, 277, 600}) {
        std::vector<u8> random(n);
        for (u8& b : random) {
            b = static_cast<u8>(rng());
        }
        round_trip(random);
    }
    // Long incompressible data: many literal packets, chunk padding.
    std::vector<u8> noise(100000);
    for (u8& b : noise) {
        b = static_cast<u8>(rng());
    }
    round_trip(noise);
    // Runs, text, repeats at every window: little, medium, big and far matches.
    round_trip(std::vector<u8>(70000, 0));
    std::vector<u8> mixed;
    for (int i = 0; i < 4000; ++i) {
        const std::string line =
            std::format("line {} of a level's text, value {}\n", i % 97, (i * 7919) % 1000);
        mixed.insert(mixed.end(), line.begin(), line.end());
        if (i % 50 == 0) {
            for (int k = 0; k < 300; ++k) {
                mixed.push_back(static_cast<u8>(rng()));
            }
        }
    }
    round_trip(mixed);
    std::vector<u8> far(16384 + 9000);
    for (u8& b : far) {
        b = static_cast<u8>(rng());
    }
    const std::vector<u8> head(far.begin(), far.begin() + 4000);
    far.insert(far.end(), head.begin(), head.end());  // repeats 25384 bytes back: a far match
    round_trip(far);
    const std::vector<u8> packed = wad_compress(std::vector<u8>(70000, 0));
    CHECK(packed.size() < 1000);
}

}  // namespace

int main() {
    decoding();
    compression();
    return openrac::test::result();
}

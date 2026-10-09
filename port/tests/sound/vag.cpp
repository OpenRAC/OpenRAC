// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The VAG header on a synthetic file.

#include "assets/sound/vag.h"

#include <cstring>
#include <vector>

#include "tests/check.h"

using namespace openrac::assets;

namespace {

void put_be32(std::vector<u8>& v, u32 x) {
    for (int s = 24; s >= 0; s -= 8) {
        v.push_back(static_cast<u8>(x >> s));
    }
}

}  // namespace

int main() {
    std::vector<u8> v = {'V', 'A', 'G', 'p'};
    put_be32(v, 0x20);
    put_be32(v, 0);
    put_be32(v, 32);
    put_be32(v, 44100);
    v.resize(0x20, 0);
    const char name[16] = "L01_Enemy_Loop";
    v.insert(v.end(), name, name + 16);
    v.resize(v.size() + 32, 0x11);

    const VagFile f = parse_vag(ByteView(v));
    CHECK(f.header.version == 0x20);
    CHECK(f.header.data_size == 32);
    CHECK(f.header.sample_rate == 44100);
    CHECK(f.header.name == "L01_Enemy_Loop");
    CHECK(f.body.size() == 32 && f.body.data()[0] == 0x11);

    // A body shorter than data_size is clipped to the file.
    const VagFile clipped = parse_vag(ByteView(v.data(), v.size() - 8));
    CHECK(clipped.body.size() == 24);

    bool threw = false;
    try {
        parse_vag(ByteView(v.data(), 0x20));
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
    v[0] = 'X';
    threw = false;
    try {
        parse_vag(ByteView(v));
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
    return openrac::test::result();
}

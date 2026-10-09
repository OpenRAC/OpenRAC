// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/sound_bank.rs
// (its tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The 989snd bank reader on a synthetic two-sound bank: sound 0 is one tone
// on a looped sample, sound 1 a marker, a random delay and a tone on a
// one-shot sample.

#include "assets/sound/sound_bank.h"

#include <initializer_list>
#include <vector>

#include "tests/check.h"

using namespace openrac::assets;

namespace {

std::vector<u8> tone_grain(u32 offset, s8 note) {
    ByteWriter g;
    g.put<u32>(1);
    g.put<s32>(0);
    g.put<s8>(90);   // priority
    g.put<s8>(127);  // volume
    g.put<s8>(note);
    g.put<s8>(0);
    g.put<s16>(0);
    g.put<u16>(0);
    g.put<u16>(0);
    g.put<u16>(0x80ff);
    g.put<u16>(0x9fc0);
    g.put<u16>(0);
    g.put<u32>(offset);
    g.resize(0x28);
    return g.bytes();
}

std::vector<u8> control_grain(u32 type, s32 delay, s32 first_param) {
    ByteWriter g;
    g.put<u32>(type);
    g.put<s32>(delay);
    g.put<s32>(first_param);
    g.resize(0x28);
    return g.bytes();
}

std::vector<u8> synthetic_bank() {
    std::vector<u8> samples;
    for (const u8 flags : std::initializer_list<u8>{6, 2, 3, 0, 1, 7}) {
        std::vector<u8> f(16, 0);
        f[1] = flags;
        samples.insert(samples.end(), f.begin(), f.end());
    }
    ByteWriter block;
    block.put_bytes(std::span<const u8>(reinterpret_cast<const u8*>("SBlk"), 4));
    block.put<u32>(1);
    block.put_at<s16>(0x16, 2);
    block.put_at<s16>(0x18, 4);
    block.put_at<s16>(0x1a, 2);
    block.put_at<u32>(0x1c, 0x3c);
    block.put_at<u32>(0x20, 0x54);
    block.resize(0x3c);
    // The sounds: {volume, group, pan, grains, limit, flags, first}.
    for (const u8 b : std::initializer_list<u8>{127, 0, 0, 0, 1, 0, 1, 0}) {
        block.put<u8>(b);
    }
    block.put<u32>(0);
    for (const u8 b : std::initializer_list<u8>{90, 4, 0, 0, 3, 0, 0, 0}) {
        block.put<u8>(b);
    }
    block.put<u32>(0x28);
    block.put_bytes(tone_grain(0, -60));
    block.put_bytes(control_grain(35, 0, 0));
    block.put_bytes(control_grain(26, 1800, 2400));
    block.put_bytes(tone_grain(0x30, 72));

    ByteWriter file;
    const auto block_size = static_cast<u32>(block.size());
    for (const u32 w :
         {3u, 2u, 0x18u, block_size, 0x18u + block_size, static_cast<u32>(samples.size())}) {
        file.put<u32>(w);
    }
    file.put_bytes(block.bytes());
    file.put_bytes(samples);
    return file.bytes();
}

}  // namespace

int main() {
    const std::vector<u8> bytes = synthetic_bank();
    const SoundBank b = parse_sound_bank(ByteView(bytes));
    CHECK(b.header.version == 1);
    CHECK(b.header.sound_count == 2 && b.sounds.size() == 2);
    CHECK(b.sounds[0].looped());
    CHECK(b.sounds[1].volume == 90 && b.sounds[1].volume_group == 4);
    CHECK(b.grains.size() == 4);
    const auto g1 = b.sound_grains(1);
    CHECK(g1.size() == 3);
    CHECK(g1[0].type == GrainType::Marker);
    CHECK(g1[1].type == GrainType::RandDelay && g1[1].delay == 1800);
    CHECK(g1[1].rand_delay_amount() == 2400);
    CHECK(g1[1].params()[0] == 2400 && g1[1].params()[1] == 0);
    CHECK(g1[2].is_tone());
    const Tone t = g1[2].tone();
    CHECK(t.priority == 90 && t.volume == 127 && t.center_note == 72);
    CHECK(t.adsr1 == 0x80ff && t.adsr2 == 0x9fc0 && t.sample_offset == 0x30);
    CHECK(b.sound_grains(0)[0].tone().center_note == -60);
    CHECK(b.sample_extents.size() == 2);
    CHECK(b.sample_extents[0].looped && !b.sample_extents[1].looped);
    CHECK(b.sample_extents[1].next_flags == u8{7});
    CHECK(b.extent_at(0x30) == std::size_t{1});
    CHECK(!b.extent_at(0x10));
    CHECK(b.samples.size() == 96);

    // Not a 989snd bank, and an unknown block version.
    std::vector<u8> bad = bytes;
    bad[0] = 4;
    bool threw = false;
    try {
        parse_sound_bank(ByteView(bad));
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
    bad = bytes;
    bad[0x18 + 4] = 2;
    threw = false;
    try {
        parse_sound_bank(ByteView(bad));
    } catch (const AssetError&) {
        threw = true;
    }
    CHECK(threw);
    return openrac::test::result();
}

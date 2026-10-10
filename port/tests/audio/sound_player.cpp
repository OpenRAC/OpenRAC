// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The sound player on banks made here: volumes and pans, a tone keyed on a
// voice and released, the register and skipping grains, instance limits,
// pausing, group volumes and unloading.

#include "audio/sound_player.h"

#include <algorithm>

#include "tests/check.h"

using namespace openrac;
using namespace openrac::audio;
using assets::ByteWriter;

namespace {

// A grain: type, delay, 32 bytes of parameters.
struct GrainSpec {
    std::uint32_t type;
    std::int32_t delay = 0;
    std::array<std::int16_t, 4> params{};
};

constexpr std::uint32_t kTone = 1;
constexpr std::uint32_t kStop = 24;
constexpr std::uint32_t kSetRegister = 30;
constexpr std::uint32_t kTestRegister = 34;
constexpr std::uint32_t kKeyOff = 41;

struct SoundSpec {
    std::uint8_t volume = 127;
    std::uint8_t group = 0;
    std::uint8_t limit = 0;
    std::uint16_t flags = 0;
    std::vector<GrainSpec> grains;
};

// A bank file: one looping sample (a single ADPCM frame of loud samples)
// and the sounds given; every tone plays it with a fast attack and release.
std::vector<std::uint8_t> make_bank(const std::vector<SoundSpec>& sounds) {
    std::vector<std::uint8_t> sample(16, 0x77);
    sample[0] = 0x00;  // shift 0, filter 0
    sample[1] = 0x07;  // loop start, loop end, repeat

    std::size_t grain_total = 0;
    for (const SoundSpec& s : sounds) {
        grain_total += s.grains.size();
    }
    const std::uint32_t first_sound = 0x3c;
    const auto first_grain = static_cast<std::uint32_t>(first_sound + 12 * sounds.size());
    ByteWriter block;
    block.put_bytes(std::vector<std::uint8_t>{'S', 'B', 'l', 'k'});
    block.put<std::uint32_t>(1);
    block.put_at<std::int16_t>(0x16, static_cast<std::int16_t>(sounds.size()));
    block.put_at<std::int16_t>(0x18, static_cast<std::int16_t>(grain_total));
    block.put_at<std::int16_t>(0x1a, 1);
    block.put_at<std::uint32_t>(0x1c, first_sound);
    block.put_at<std::uint32_t>(0x20, first_grain);
    block.resize(first_sound);
    std::uint32_t grain_offset = 0;
    for (const SoundSpec& s : sounds) {
        block.put(s.volume);
        block.put(s.group);
        block.put<std::int16_t>(0);  // pan
        block.put(static_cast<std::uint8_t>(s.grains.size()));
        block.put(s.limit);
        block.put(s.flags);
        block.put(grain_offset);
        grain_offset += static_cast<std::uint32_t>(0x28 * s.grains.size());
    }
    for (const SoundSpec& s : sounds) {
        for (const GrainSpec& g : s.grains) {
            const std::size_t at = block.size();
            block.put(g.type);
            block.put(g.delay);
            if (g.type == kTone) {
                block.put<std::int8_t>(64);   // priority
                block.put<std::int8_t>(127);  // volume
                block.put<std::int8_t>(-60);  // a 48 kHz sample at note 60
                block.put<std::int8_t>(0);
                block.put<std::int16_t>(0);  // pan
                block.put<std::int8_t>(0);
                block.put<std::int8_t>(127);
                block.put<std::int8_t>(2);
                block.put<std::int8_t>(2);
                block.put<std::uint16_t>(0x000f);  // fastest attack, full sustain
                block.put<std::uint16_t>(0x1fc0);  // held sustain, fastest linear release
                block.put<std::uint16_t>(0);
                block.put<std::uint32_t>(0);  // the sample
            } else {
                for (const std::int16_t p : g.params) {
                    block.put(p);
                }
            }
            block.resize(at + 0x28);
        }
    }
    ByteWriter file;
    const auto size = static_cast<std::uint32_t>(block.size());
    for (const std::uint32_t w : {3u, 2u, 0x18u, size, 0x18u + size, 16u}) {
        file.put(w);
    }
    file.put_bytes(block.bytes());
    file.put_bytes(sample);
    return file.bytes();
}

std::size_t active_voices(SoundPlayer& p) {
    return p.mixer().active_voices();
}

// Renders `ticks` script ticks and returns the loudest sample.
int render_ticks(SoundPlayer& p, int ticks) {
    std::vector<std::int16_t> out(static_cast<std::size_t>(2 * kFramesPerTick * ticks));
    p.render(out);
    int loudest = 0;
    for (const std::int16_t s : out) {
        loudest = std::max(loudest, std::abs(static_cast<int>(s)));
    }
    return loudest;
}

void volume_and_pan() {
    // Ahead: equal on both sides; 90 degrees: all right; 270: all left.
    const auto ahead = make_volume(127, 0, 127, 0, 127, 0);
    CHECK(ahead[0] == ahead[1] && ahead[0] > 20000);
    const auto right = make_volume(127, 0, 127, 90, 127, 0);
    CHECK(right[0] == 0 && right[1] > 32000);
    const auto left = make_volume(127, 0, 127, 0, 127, 270);
    CHECK(left[1] == 0 && left[0] > 32000);
    CHECK((make_volume(127, 0, 0, 0, 127, 0) == std::array<std::int16_t, 2>{0, 0}));
    // Group volume is square law.
    CHECK(group_volume(0x7ffe, kFullVolume) == 0x7ffe);
    CHECK(group_volume(0x7ffe, kFullVolume / 2) < 0x7ffe / 3);
    CHECK(group_volume(0x7ffe, 0) == 0);
    CHECK(group_volume(-0x4000, kFullVolume) < 0);
}

void tone_on_and_off() {
    SoundPlayer p;
    const auto bytes = make_bank({{127, 0, 0, 0, {{kTone}}}});
    const BankHandle bank = p.load_bank(bytes);
    const SoundHandle s = p.play(bank, 0);
    CHECK(s != 0 && p.playing(s) && active_voices(p) == 1);
    CHECK(render_ticks(p, 4) > 1000);
    // The script is done (one grain) but its voice still plays.
    p.tick();
    CHECK(p.playing(s));
    p.stop(s);
    render_ticks(p, 200);
    CHECK(!p.playing(s) && active_voices(p) == 0);
    CHECK(p.play(bank, 7) == 0);      // no such sound
    CHECK(p.play(bank + 1, 0) == 0);  // no such bank
}

void registers_and_skips() {
    SoundPlayer p;
    // r0 = 5, then "skip the next grain unless r0 == cmp": the tone plays
    // when cmp is 5 and is skipped when it is 4.
    const auto bank = p.load_bank(make_bank({
        {127, 0, 0, 0, {{kSetRegister, 0, {0, 5}}, {kTestRegister, 0, {0, 1, 5}}, {kTone}, {kStop}}
        },
        {127, 0, 0, 0, {{kSetRegister, 0, {0, 5}}, {kTestRegister, 0, {0, 1, 4}}, {kTone}, {kStop}}
        },
    }));
    p.play(bank, 0);
    CHECK(active_voices(p) == 1);
    p.stop_all();
    p.play(bank, 1);
    CHECK(active_voices(p) == 0);

    // A delayed grain runs on its tick: key off after 10 ticks.
    SoundPlayer q;
    const auto b2 = q.load_bank(make_bank({{127, 0, 0, 0, {{kTone}, {kKeyOff, 10}}}}));
    const SoundHandle s = q.play(b2, 0);
    for (int i = 0; i < 9; ++i) {
        q.tick();
    }
    CHECK(q.mixer().voice(0).envelope().phase() != EnvelopePhase::Release);
    q.tick();
    CHECK(q.mixer().voice(0).envelope().phase() == EnvelopePhase::Release);
    render_ticks(q, 200);
    CHECK(!q.playing(s));
}

void instance_limits() {
    // Limit 1, the oldest gives way.
    SoundPlayer p;
    const auto bank = p.load_bank(make_bank({
        {127, 0, 1, 8 | 0x20, {{kTone}}},
        {127, 0, 1, 8, {{kTone}}},
    }));
    const SoundHandle first = p.play(bank, 0);
    const SoundHandle second = p.play(bank, 0);
    CHECK(first != 0 && second != 0);
    render_ticks(p, 200);
    CHECK(!p.playing(first) && p.playing(second));
    // Limit 1 with neither mode: the new one does not start.
    const SoundHandle a = p.play(bank, 1);
    CHECK(a != 0 && p.play(bank, 1) == 0);
}

void pausing_and_groups() {
    SoundPlayer p;
    const auto bank = p.load_bank(make_bank({{127, 3, 0, 0, {{kTone}}}}));
    p.play(bank, 0);
    const auto before = p.mixer().voice(0).volume();
    CHECK(before[0] != 0);
    p.pause_groups(1u << 3);
    CHECK((p.mixer().voice(0).volume() == std::array<std::uint16_t, 2>{0, 0}));
    CHECK(p.mixer().voice(0).pitch() == 0);
    p.continue_groups(1u << 3);
    CHECK(p.mixer().voice(0).volume() == before && p.mixer().voice(0).pitch() != 0);
    p.set_master_volume(3, 0);
    CHECK((p.mixer().voice(0).volume() == std::array<std::uint16_t, 2>{0, 0}));
    p.set_master_volume(3, kFullVolume);
    CHECK(p.mixer().voice(0).volume() == before);
    // Unloading the bank silences its sounds.
    p.unload_bank(bank);
    CHECK(p.sounds_playing() == 0 && active_voices(p) == 0 && !p.bank(bank));
}

}  // namespace

int main() {
    volume_and_pan();
    tone_on_and_off();
    registers_and_skips();
    instance_limits();
    pausing_and_groups();
    return openrac::test::result();
}

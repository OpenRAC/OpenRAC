// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu's sounds: silence until one plays, a soft start (no click), a quiet peak, an end (with the
// room it rings in) within 0.7 s, mixed over what is there; and a held arrow's fast repeats, or every sound
// pressed at once again and again, never louder than a few tones together nor longer.

#include "audio/ui_sounds.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "tests/check.h"

using namespace openrac::audio;

namespace {

// 10 ms blocks, as an output asks for them.
constexpr std::size_t kBlock = ui_sounds::kRate / 100;

int peak(const std::vector<std::int16_t>& samples) {
    int p = 0;
    for (const std::int16_t s : samples) {
        p = std::max(p, std::abs(static_cast<int>(s)));
    }
    return p;
}

// Mixes `blocks` blocks into silence; returns every sample.
std::vector<std::int16_t> render(int blocks) {
    std::vector<std::int16_t> all;
    std::vector<std::int16_t> block(kBlock * 2);
    for (int i = 0; i < blocks; ++i) {
        std::fill(block.begin(), block.end(), 0);
        ui_sounds::mix(block);
        all.insert(all.end(), block.begin(), block.end());
    }
    return all;
}

}  // namespace

int main() {
    using ui_sounds::Sound;
    const Sound all[] =
        {Sound::Cursor,
         Sound::Confirm,
         Sound::Back,
         Sound::Denied,
         Sound::Open,
         Sound::Close,
         Sound::Page};

    // Nothing plays: silence.
    ui_sounds::reset();
    CHECK(peak(render(5)) == 0);
    CHECK(!ui_sounds::sounding());

    for (const Sound s : all) {
        ui_sounds::reset();
        ui_sounds::play(s);
        CHECK(ui_sounds::sounding());
        const std::vector<std::int16_t> out = render(70);  // 0.7 s
        const int p = peak(out);
        CHECK(p > 300);        // heard
        CHECK(p < 32767 / 4);  // and soft: under -12 dB
        // A soft start: its first 0.2 ms is near silence (no click).
        int first = 0;
        for (std::size_t i = 0; i < 10 * 2; ++i) {
            first = std::max(first, std::abs(static_cast<int>(out[i])));
        }
        CHECK(first < p / 4);
        CHECK(!ui_sounds::sounding());  // over, its room too, within 0.7 s
    }

    // Mixed over the game's sound, saturating rather than wrapping.
    ui_sounds::reset();
    ui_sounds::play(Sound::Open);
    std::vector<std::int16_t> loud(kBlock * 2 * 10, 32000);
    for (std::size_t at = 0; at < loud.size(); at += kBlock * 2) {
        ui_sounds::mix(std::span<std::int16_t>(loud.data() + at, kBlock * 2));
    }
    CHECK(*std::min_element(loud.begin(), loud.end()) > 20000);

    // A held arrow: a cursor sound asked for every frame. The repeats are thinned and fade the one
    // before out, so the level stays that of one tone.
    ui_sounds::reset();
    ui_sounds::play(Sound::Cursor);
    const int one = peak(render(20));
    ui_sounds::reset();
    std::vector<std::int16_t> held;
    for (int frame = 0; frame < 120; ++frame) {
        ui_sounds::play(Sound::Cursor);
        const std::vector<std::int16_t> block = render(1);
        held.insert(held.end(), block.begin(), block.end());
    }
    CHECK(peak(held) <= one + one / 10);

    // Every sound pressed together, again and again: bounded, and over soon after.
    ui_sounds::reset();
    std::vector<std::int16_t> storm;
    for (int frame = 0; frame < 60; ++frame) {
        for (const Sound s : all) {
            ui_sounds::play(s);
        }
        const std::vector<std::int16_t> block = render(1);
        storm.insert(storm.end(), block.begin(), block.end());
    }
    CHECK(peak(storm) < 32767 * 3 / 4);
    render(70);
    CHECK(!ui_sounds::sounding());

    return openrac::test::result();
}

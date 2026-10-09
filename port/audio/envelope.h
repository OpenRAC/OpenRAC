// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-game/src/audio.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The sound processor's envelope generator (ADSR). The two 16-bit registers
// a tone carries select, per phase, linear or exponential steps of
// step << max(0, 11 - shift) every 2^max(0, shift - 11) samples, with the
// exponential increase slowed four times above 0x6000 and the exponential
// decrease scaled by the level. The state machine and step rule are the
// hardware's as psx-spx documents it ("SPU ADSR"); PCSX2 and OpenGOAL
// (game/sound/common/envelope.cpp) run the same. A console fact, the same
// in all four games.

#pragma once

#include <cstdint>

namespace openrac::audio {

enum class EnvelopePhase : std::uint8_t {
    Attack,
    Decay,
    Sustain,
    Release,
    Stopped,
};

class Envelope {
public:
    void set_registers(std::uint16_t adsr1, std::uint16_t adsr2) {
        m_adsr1 = adsr1;
        m_adsr2 = adsr2;
    }

    std::uint16_t adsr1() const { return m_adsr1; }

    std::uint16_t adsr2() const { return m_adsr2; }

    EnvelopePhase phase() const { return m_phase; }

    int level() const { return m_level; }  // 0..0x7fff

    // Key on: the attack from level 0.
    void attack();

    // Key off: the release from the current level.
    void release();

    // The end of a sample that does not repeat: silent at once.
    void stop();

    // One sample.
    void run();

private:
    void update_settings();
    void step();

    std::uint16_t m_adsr1 = 0;
    std::uint16_t m_adsr2 = 0;
    EnvelopePhase m_phase = EnvelopePhase::Stopped;
    int m_level = 0;
    std::uint32_t m_counter = 0;
    int m_shift = 0;
    int m_step = 0;
    bool m_exponential = false;
    bool m_decrease = false;
    int m_target = 0;
};

}  // namespace openrac::audio

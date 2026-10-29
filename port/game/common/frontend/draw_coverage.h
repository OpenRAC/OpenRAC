// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Developer tools: what the game asked to draw that the port leaves undrawn (OPENRAC_DRAW_COVERAGE,
// a JSON file). The port runs the game's own code, so each tick's display list holds every VU1
// program the game starts, and the particle pool every particle: a program start that no renderer
// of the port reads (only the effect strips' are), or a particle particle_quads leaves out, is
// something missing from the picture. Counted per level, by the program's signature (its
// microcode, its entry and the layout of the data unpacked for it) and by particle type, with the
// game memory the data came from, so a missing effect can be traced back to the code that sent it
// (port/tools/regress coverage merges the files of a run).
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace openrac::frontend::draw_coverage {

// Whether OPENRAC_DRAW_COVERAGE names a file: every other call does nothing otherwise.
bool armed();

// The tick whose display list and particles are read next, on `level`. The file is rewritten every
// so many ticks and when the program ends.
void begin_tick(int level, std::uint64_t tick);

// A VIF MPG: microcode loaded at VU1 address `at` (in 64-bit words).
void program(std::uint32_t at, std::span<const std::uint8_t> code);

// A VIF UNPACK to VU1 address `address` (vn, vl as in the code), from game memory at `ram`.
void unpack(
    std::uint32_t address,
    unsigned vn,
    unsigned vl,
    unsigned count,
    std::uint32_t ram,
    std::span<const std::uint8_t> data
);

// A VIF MSCAL, MSCALF or MSCNT (`command`; `entry` its immediate); `drawn` when a renderer of the
// port reads what it draws. The GS registers are those the DIRECT data before it left.
void start(
    std::uint32_t command,
    std::uint32_t entry,
    bool drawn,
    std::uint64_t alpha,
    std::uint64_t test,
    std::uint64_t tex0
);

// A particle of `type` that particle_quads draws (`reason` empty) or leaves out (lines,
// untextured, beyond), updated by `update` (the function and its name).
void particle(unsigned type, std::string_view reason, std::string_view update);

}  // namespace openrac::frontend::draw_coverage

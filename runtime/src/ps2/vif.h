// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <functional>
#include <vector>

#include "gif.h"
#include "types.h"

namespace ps2 {

// VIF1: the interface between the display list and VU1. It unpacks vertex
// data into VU1's data memory, loads microprograms, starts them, and passes
// GIF packets straight through to the GS (DIRECT).
class Vif1 {
public:
    static constexpr u32 kMemoryBytes = 16 * 1024;

    explicit Vif1(Gif& gif) : gif_(gif) { reset(); }

    void reset();

    // Display list data, in any chunking; a command split across calls is
    // finished when the rest arrives.
    void write(const u8* data, std::size_t bytes);

    // Start the microprogram at `address` (in instructions), or continue the
    // one that stopped (MSCNT) when `resume` is true.
    std::function<void(u32 address, bool resume)> on_start;

    // Called when MPG has written program memory.
    std::function<void()> on_program;

    std::array<u8, kMemoryBytes> data{};   // VU1 data memory, 1,024 quadwords
    std::array<u8, kMemoryBytes> micro{};  // VU1 program memory, 2,048 instructions

    // Registers, named as on the machine.
    u32 cl = 1, wl = 1, mode = 0, mask = 0;
    std::array<u32, 4> row{}, col{};
    u32 base = 0, ofst = 0, tops = 0, top = 0, itops = 0, itop = 0, mark = 0;
    bool dbf = false;
    bool path3_masked = false;

    u64 unknown_codes = 0;  // commands met that are not VIF codes

private:
    // Words of data that follow a command, or 0.
    std::size_t operand_words(u32 code) const;
    void execute(u32 code, const u32* operands, std::size_t words);
    void unpack(u32 code, const u32* operands, std::size_t words);
    void start(u32 address, bool resume);

    Gif& gif_;
    std::vector<u32> pending_;
};

}  // namespace ps2

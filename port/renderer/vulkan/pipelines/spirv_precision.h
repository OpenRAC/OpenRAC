// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstdint>
#include <span>
#include <unordered_map>

namespace openrac::vkgl {
struct SpirvPrecision {
    std::uint64_t fp16 = 0,
                  fp32 = 0;  // Static typed arithmetic instructions, not loads/conversions.
    bool storage16 = false, float64 = false, int16 = false;
};

// Inspect executable result types; a Float16 type/texture format alone is not evidence of arithmetic.
inline SpirvPrecision spirv_precision(std::span<const std::uint32_t> code) {
    SpirvPrecision out;
    std::unordered_map<std::uint32_t, unsigned> widths;
    for (std::size_t at = 5; at < code.size();) {
        const unsigned count = code[at] >> 16, op = code[at] & 65535u;
        if (!count || at + count > code.size()) {
            break;
        }
        if (op == 22 && count == 3) {
            widths[code[at + 1]] = code[at + 2];  // OpTypeFloat
        }
        if (op == 23 && count == 4) {
            widths[code[at + 1]] = widths[code[at + 2]];  // OpTypeVector
        }
        if (op == 17 && count == 2) {  // OpCapability, deliberately distinguish storage16.
            const auto cap = code[at + 1];
            out.storage16 |= cap >= 4433 && cap <= 4436;
            out.float64 |= cap == 10;
            out.int16 |= cap == 22;  // Int16
        }
        // Floating operations and GLSL extended arithmetic (pow, clamp, normalize, etc.).
        const bool arithmetic = op == 12 || op == 127 || op == 129 || op == 131 || op == 133
                                || op == 136 || op == 140 || op == 141 || (op >= 142 && op <= 148);
        if (arithmetic && count >= 3) {
            const auto width = widths[code[at + 1]];
            out.fp16 += width == 16;
            out.fp32 += width == 32;
        }
        at += count;
    }
    return out;
}
}  // namespace openrac::vkgl

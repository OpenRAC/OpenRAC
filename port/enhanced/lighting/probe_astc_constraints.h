// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Worker-owned interpolation constraints for paired ASTC probe borders.
#pragma once

#include <array>
#include <map>
#include <vector>

struct decimation_info;

namespace openrac::lighting::compression {
// One integer displacement for each stored weight in an 8x8 block.
using AstcWeightMove = std::array<int, 64>;

// Descriptor-derived moves remain valid while their encoder owns the immutable descriptors.
class AstcInterpolation {
public:
    // Returns cached null-space moves whose decoded border interpolation stays equal.
    const std::vector<AstcWeightMove>& moves(const decimation_info& descriptor);

private:
    // Immutable descriptor identities map to moves for this encoder only.
    std::map<const decimation_info*, std::vector<AstcWeightMove>> m_moves;
};
}  // namespace openrac::lighting::compression

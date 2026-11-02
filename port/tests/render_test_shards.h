// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic test partitioning by measured cost, independent of registry order.

#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <span>
#include <vector>

namespace openrac::test {

// Assigns the longest remaining case to the least-loaded shard, preserving ties by index.
// An invalid shard count returns no assignments; valid assignments cover every case once.
inline std::vector<std::size_t> balanced_test_shards(
    std::span<const unsigned> costs, std::size_t count
) {
    // A nonempty shard needs at least one case, and division into zero shards is undefined.
    if (count == 0 || count > costs.size()) {
        return {};
    }
    std::vector<std::size_t> order(costs.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) {
        return costs[a] > costs[b];
    });
    std::vector<std::size_t> loads(count, 0);
    std::vector<std::size_t> assigned(costs.size());
    // Sorting by cost prevents one expensive group from accumulating on a single shard.
    for (const auto index : order) {
        const auto least = std::min_element(loads.begin(), loads.end());
        assigned[index] = static_cast<std::size_t>(least - loads.begin());
        *least += costs[index];
    }
    return assigned;
}

}  // namespace openrac::test

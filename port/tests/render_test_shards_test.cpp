// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Partition coverage, deterministic ties and load balancing without a graphics device.

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <vector>

#include "tests/check.h"
#include "tests/render_equivalence_costs.h"
#include "tests/render_equivalence_policy.h"
#include "tests/render_test_shards.h"

namespace {

// Checks every supported shard count and a deliberately imbalanced modulo partition.
void test_balanced_partition() {
    constexpr std::array<unsigned, 6> kCosts{10, 1, 9, 1, 8, 1};
    CHECK(openrac::test::balanced_test_shards(kCosts, 0).empty());
    CHECK(openrac::test::balanced_test_shards(kCosts, kCosts.size() + 1).empty());
    // Each case has exactly one valid owner; positive costs populate every shard.
    for (std::size_t count = 1; count <= kCosts.size(); ++count) {
        const auto assigned = openrac::test::balanced_test_shards(kCosts, count);
        CHECK(assigned == openrac::test::balanced_test_shards(kCosts, count));
        CHECK(assigned.size() == kCosts.size());
        CHECK(std::all_of(assigned.begin(), assigned.end(), [count](auto owner) {
            return owner < count;
        }));
        std::vector<unsigned> loads(count);
        for (std::size_t index = 0; index < assigned.size(); ++index) {
            loads.at(assigned[index]) += kCosts[index];
        }
        CHECK(std::all_of(loads.begin(), loads.end(), [](auto load) { return load > 0; }));
        CHECK(std::accumulate(loads.begin(), loads.end(), 0u) == 30);
    }
    // Modulo puts 27 units on shard zero; the greedy partition's maximum is 17.
    const auto two_shards = openrac::test::balanced_test_shards(kCosts, 2);
    std::array<unsigned, 2> loads{};
    for (std::size_t index = 0; index < two_shards.size(); ++index) {
        loads.at(two_shards[index]) += kCosts[index];
    }
    CHECK(*std::max_element(loads.begin(), loads.end()) == 17);
    constexpr std::array<unsigned, 4> kTies{1, 1, 1, 1};
    CHECK((openrac::test::balanced_test_shards(kTies, 2) == std::vector<std::size_t>{0, 1, 0, 1}));
}

// Checks calibration keys stay unique and new registry entries retain positive costs.
void test_registry_costs() {
    const auto costs = openrac::test::equivalence_entry_costs();
    std::vector<bool> seen(costs.size());
    for (const auto& [opt, milliseconds] : openrac::test::kEquivalenceMeasuredCosts) {
        const auto index = static_cast<std::size_t>(opt);
        CHECK(index < costs.size());
        CHECK(!seen.at(index));
        seen.at(index) = true;
        CHECK(milliseconds > 0);
        CHECK(costs.at(index) == milliseconds);
    }
    CHECK(std::all_of(costs.begin(), costs.end(), [](auto cost) { return cost > 0; }));
    const auto assigned = openrac::test::balanced_test_shards(costs, 2);
    CHECK(assigned.size() == costs.size());
    CHECK(std::all_of(assigned.begin(), assigned.end(), [](auto owner) { return owner < 2; }));
}

// Every focused optimisation has one nonempty owner, and cache tests retain fresh objects.
void test_feature_owners() {
    std::vector<bool> seen(openrac::renderer::render_opt::kOptCount);
    for (const auto& [opt, owner] : openrac::test::kFocusedEquivalence) {
        const auto index = static_cast<std::size_t>(opt);
        CHECK(!seen.at(index));
        seen.at(index) = true;
        CHECK(owner[0] != '\0');
        CHECK(openrac::test::focused_equivalence_test(opt) == owner);
    }
    using openrac::renderer::render_opt::Opt;
    CHECK(!openrac::test::may_reuse_test_programs(Opt::PipelinePrewarm));
    CHECK(!openrac::test::may_reuse_test_programs(Opt::SpirvDiskCache));
    CHECK(openrac::test::focused_equivalence_test(Opt::RegistrySelftest).empty());
    CHECK(openrac::test::may_reuse_test_programs(Opt::RegistrySelftest));
}

}  // namespace

// Runs partition arithmetic and actual registry coverage without a graphics device.
int main() {
    test_balanced_partition();
    test_registry_costs();
    test_feature_owners();
    return openrac::test::result();
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Deterministic diagnostic text; never performs readbacks, logging or file writes.
#include <array>
#include <format>
#include <string_view>

#include "renderer/graph/frame_graph.h"

namespace openrac::renderer::graph {
namespace {
// Names follow the public enums, independently of Vulkan's numeric flags.
constexpr std::array<std::string_view, 4> kAccess{"none", "read", "write", "read/write"};
constexpr std::array<std::string_view, 10> kLayout{
    "undefined",
    "buffer",
    "general",
    "sampled",
    "colour-attachment",
    "depth-attachment",
    "depth-read-only",
    "transfer-source",
    "transfer-destination",
    "present"
};

// Retains every participating reader stage in the diagnostic barrier scope.
std::string stages_name(Stage stages) {
    // All is an external scope, not a list of individually known stages.
    if (stages == Stage::All) {
        return "all";
    }

    constexpr std::array<std::string_view, 8> kNames{
        "transfer",
        "vertex-input",
        "vertex",
        "fragment",
        "compute",
        "colour-output",
        "depth-tests",
        "indirect"
    };
    std::string result;

    // Stable enum bit order makes repeated diagnostic dumps directly comparable.
    for (unsigned bit = 0; bit < kNames.size(); ++bit) {
        // A barrier can wait for multiple earlier readers in different stages.
        if ((static_cast<unsigned>(stages) & (1u << bit)) != 0) {
            // Separate combined scopes without leading or trailing delimiters.
            if (!result.empty()) {
                result += '|';
            }

            result += kNames[bit];
        }
    }

    return result.empty() ? "none" : result;
}

// Formats the stage, memory access and layout that together determine a barrier.
std::string state_name(State state) {
    return std::format(
        "{}/{}/{}",
        stages_name(state.stages),
        kAccess.at(static_cast<unsigned>(state.access)),
        kLayout.at(static_cast<unsigned>(state.layout))
    );
}

// Describes exact barrier coordinates in subresources or bytes.
std::string range_name(const Range& range) {
    // Image and buffer coordinates have different units.
    if (const auto* image = std::get_if<ImageRange>(&range)) {
        return std::format(
            "aspect {} mip {}+{} layer {}+{}",
            static_cast<unsigned>(image->aspects),
            image->first_mip,
            image->mip_count,
            image->first_layer,
            image->layer_count
        );
    }

    const auto& buffer = std::get<BufferRange>(range);
    return std::format("bytes {}+{}", buffer.offset, buffer.size);
}
}  // namespace

std::string Plan::describe() const {
    std::string text;

    // Include unused resources to make accidental missing declarations visible.
    for (unsigned index = 0; index < resources.size(); ++index) {
        const auto& resource = resources[index];
        const auto key =
            std::visit([](const auto& value) { return value.key; }, resource.description);
        text += std::format(
            "resource {} {} key {}:{} uses {}..{}\n",
            index,
            resource.name,
            key.identity,
            key.generation,
            resource.first ? std::to_string(*resource.first) : "unused",
            resource.last ? std::to_string(*resource.last) : "unused"
        );
    }

    // Show barriers at the pass entry where an executor must record them.
    for (unsigned index = 0; index < passes.size(); ++index) {
        const auto& pass = passes[index];
        text += std::format(
            "pass {} {} {}\n", index, pass.kind == PassKind::Export ? "export" : "work", pass.name
        );

        // Exact ranges explain which overlapping accesses caused the dependency.
        for (const auto& barrier : pass.barriers) {
            text += std::format(
                "  barrier {} {} {} -> {}\n",
                resources[barrier.resource].name,
                range_name(barrier.range),
                state_name(barrier.before),
                state_name(barrier.after)
            );
        }
    }

    // Edges name the shared allocation, rather than implying all passes are mutually dependent.
    for (const auto& edge : dependencies) {
        text += std::format(
            "dependency {} -> {} via {}\n",
            edge.source,
            edge.destination,
            resources[edge.resource].name
        );
    }

    return text;
}
}  // namespace openrac::renderer::graph

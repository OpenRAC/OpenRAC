// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Descriptor bindings declared by one compiled SPIR-V stage, independent of its CPU reflection.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace openrac::vkgl {
inline std::vector<std::uint32_t> spirv_set0_bindings(std::span<const std::uint32_t> code) {
    // SPIR-V OpDecorate, Binding and DescriptorSet (instruction and Decoration tables).
    constexpr std::uint32_t kHeaderWords = 5, kOpDecorate = 71, kBinding = 33, kDescriptorSet = 34;
    std::unordered_map<std::uint32_t, std::uint32_t> binding_of, set_of;

    for (std::size_t at = kHeaderWords; at < code.size();) {
        const std::uint32_t count = code[at] >> 16;

        if (count == 0 || count > code.size() - at) {
            break;
        }

        if ((code[at] & 0xffffu) == kOpDecorate && count >= 4) {
            if (code[at + 2] == kBinding) {
                binding_of[code[at + 1]] = code[at + 3];
            } else if (code[at + 2] == kDescriptorSet) {
                set_of[code[at + 1]] = code[at + 3];
            }
        }

        at += count;
    }

    std::vector<std::uint32_t> out;

    for (const auto& [id, binding] : binding_of) {
        const auto set = set_of.find(id);

        if (set == set_of.end() || set->second == 0) {
            out.push_back(binding);
        }
    }

    return out;
}
}  // namespace openrac::vkgl

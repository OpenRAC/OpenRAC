// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// water_bake.h's numerical pipeline; publication and cache ownership remain with preparation.
#include "enhanced/tools/water/water_bake.h"

#include <stdexcept>

#include "common/profiler.h"
#include "enhanced/tools/water/water_internal.h"

namespace openrac::lighting::water {
Result<void> bake(const std::filesystem::path& input_path, const std::filesystem::path& output) {
    // Malformed input and write failures must leave previously published assets untouched.
    try {
        const profiling::Zone timing("Bake water");
        auto mesh = output;
        mesh.replace_extension(".mesh");

        // Published or previous staged files must never be overwritten by a failed retry.
        if (std::filesystem::exists(output) || std::filesystem::exists(mesh)) {
            throw std::runtime_error("water output already exists; choose a new staging path");
        }

        const auto input = read_input(input_path);
        std::vector<Body> bodies;
        std::size_t count = 0;

        // Body order, not task completion order, defines atlas offsets and runtime IDs.
        for (const auto& surface : input.surfaces) {
            bodies.push_back(bake_body(surface, input));
            count += bodies.back().cells.size();

            // The combined atlas must stay within its runtime allocation limit.
            if (count > kAtlasCells) {
                throw std::runtime_error("water atlas exceeds budget");
            }
        }

        write_fields(output, bodies, input);
        write_mesh(mesh, input);
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting::water

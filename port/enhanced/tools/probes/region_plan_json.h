// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Bounded JSON input and numeric report helpers shared by native region planning stages.
#pragma once
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "enhanced/tools/probes/bake_json.h"
#include "enhanced/tools/probes/region_plan_internal.h"

namespace openrac::lighting::region_plan {
// Requires finite numeric vectors before lattice conversion or collision queries.
template <std::size_t Size>
std::array<double, Size> coordinates(const json::Value& value) {
    // Coordinate arity is checked before indexing.
    if (!value.is_array() || value.items().size() != Size) {
        throw std::runtime_error("invalid region planning coordinate count");
    }

    std::array<double, Size> result{};

    // Validate every component before lattice or collision arithmetic.
    for (std::size_t index = 0; index < Size; ++index) {
        // Nonfinite input cannot become an integer endpoint.
        if (!value[index].is_number() || !std::isfinite(value[index].number())) {
            throw std::runtime_error("nonfinite region planning coordinate");
        }

        result[index] = value[index].number();
    }

    return result;
}

// Emits numeric coordinates and source ID arrays without lowering double precision.
template <typename Range>
std::string sequence_json(const Range& values) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << '[';
    bool comma = false;

    // Retain numeric source order without narrowing coordinates.
    for (const auto& value : values) {
        // JSON separators belong between values only.
        if (comma) {
            stream << ',';
        }

        comma = true;
        stream << value;
    }

    return stream.str() + ']';
}

// Shared bounded input and serialization keep planner evidence in the same JSON domain as preparation.
using bake_json::document;
using bake_json::json_text;
using bake_json::source_bytes;
using bake_json::source_hash;
}  // namespace openrac::lighting::region_plan

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A named map supplied by an enabled data pack. It uses a retail level's program and resources,
// with its own world, gameplay and start. The disc and the campaign's memory card stay separate.

#pragma once

#include <array>
#include <string>
#include <string_view>

namespace openrac::program {

struct CustomMap {
    std::string id, name;
    int level = 0;
    std::array<float, 3> spawn{};
    float heading = 0.0f;
    bool skip_movies = true;
};

// The stacker's map.json, checked again by the game before any mod data is read.
bool parse_custom_map(
    std::string_view text,
    std::string_view game,
    std::string_view version,
    CustomMap& out,
    std::string& error
);

}  // namespace openrac::program

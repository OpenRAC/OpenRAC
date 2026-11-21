// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The joint number a glTF node's name gives (scene/level.cpp): joint_NNN, the form the exporters
// name a skin's joints in, numbered as the game numbers them.

#pragma once

#include <algorithm>
#include <limits>
#include <string_view>

namespace openrac::scene {

// The NNN of a name `joint_` followed by digits only (joint_12 is 12), else `fallback`: a name with
// no digits, a non-digit after the prefix or a sign, and any other name. Not clamped: the caller
// decides what a number past the palette's 255 is. A number too big for an int is INT_MAX.
inline int joint_number(std::string_view name, int fallback) {
    constexpr std::string_view prefix = "joint_";

    if (name.size() <= prefix.size() || name.substr(0, prefix.size()) != prefix) {
        return fallback;
    }

    constexpr long long kMax = std::numeric_limits<int>::max();
    long long number = 0;

    for (const char c : name.substr(prefix.size())) {
        if (c < '0' || c > '9') {
            return fallback;
        }

        number = std::min(number * 10 + (c - '0'), kMax);
    }

    return static_cast<int>(number);
}

}  // namespace openrac::scene

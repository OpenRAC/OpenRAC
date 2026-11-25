// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A game's draw distance exceptions: the moby classes that keep the game's own draw distance at
// every Draw distance setting. Each game lists them in port/game/<game>/draw_distance.tsv.

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace openrac::host {

/**
 * One game's draw_distance.tsv, read. Lines that start with # and empty lines are skipped. Every
 * other line is one class, four fields split by tabs: class number, levels, name, reason. A row
 * that does not fit is left out and named in errors().
 */
class DrawDistanceList {
public:
    explicit DrawDistanceList(std::string_view tsv);

    // Whether the class keeps the game's draw distance.
    bool has(int class_id) const;

    // The listed classes, sorted.
    const std::vector<int>& classes() const { return m_classes; }

    // One line for each row left out, with its line number.
    const std::vector<std::string>& errors() const { return m_errors; }

private:
    std::vector<int> m_classes;
    std::vector<std::string> m_errors;
};

}  // namespace openrac::host

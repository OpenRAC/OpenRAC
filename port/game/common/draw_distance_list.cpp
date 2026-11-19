// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "game/common/draw_distance_list.h"

#include <algorithm>
#include <charconv>
#include <format>

namespace openrac::host {

namespace {

std::vector<std::string_view> split(std::string_view line, char by) {
    std::vector<std::string_view> parts;
    for (;;) {
        const auto at = line.find(by);
        parts.push_back(line.substr(0, at));
        if (at == std::string_view::npos) {
            return parts;
        }
        line.remove_prefix(at + 1);
    }
}

}  // namespace

DrawDistanceList::DrawDistanceList(std::string_view tsv) {
    int number = 0;
    for (std::string_view line : split(tsv, '\n')) {
        ++number;
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto fields = split(line, '\t');
        int class_id = 0;
        const auto [end, error] =
            std::from_chars(fields[0].data(), fields[0].data() + fields[0].size(), class_id);
        if (fields.size() != 4 || error != std::errc{} || end != fields[0].data() + fields[0].size()
            || class_id < 0) {
            m_errors.push_back(std::format("line {}: needs class, levels, name, reason", number));
            continue;
        }
        if (std::any_of(fields.begin() + 1, fields.end(), [](auto f) { return f.empty(); })) {
            m_errors.push_back(std::format("line {}: an empty field", number));
            continue;
        }
        if (has(class_id)) {
            m_errors.push_back(std::format("line {}: class {} is listed twice", number, class_id));
            continue;
        }
        m_classes.insert(std::upper_bound(m_classes.begin(), m_classes.end(), class_id), class_id);
    }
}

bool DrawDistanceList::has(int class_id) const {
    return std::binary_search(m_classes.begin(), m_classes.end(), class_id);
}

}  // namespace openrac::host

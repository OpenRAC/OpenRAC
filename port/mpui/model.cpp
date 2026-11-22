// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The screens' data: model.h.

#include "mpui/model.h"

#include <algorithm>

namespace openrac::mpui {

std::string_view Row::cell(std::string_view field) const {
    auto it = cells.find(field);
    return it == cells.end() ? std::string_view{} : std::string_view{it->second};
}

std::size_t Table::find(std::uint32_t key) const {
    auto it = std::find_if(m_rows.begin(), m_rows.end(), [key](const Row& row) {
        return row.key == key;
    });
    return static_cast<std::size_t>(it - m_rows.begin());
}

Table& Model::table(std::string_view name) {
    auto it = m_tables.find(name);

    // A table nobody filled yet starts empty.
    if (it == m_tables.end()) {
        it = m_tables.emplace(std::string(name), Table{}).first;
    }

    return it->second;
}

const Table* Model::find(std::string_view name) const {
    auto it = m_tables.find(name);
    return it == m_tables.end() ? nullptr : &it->second;
}

}  // namespace openrac::mpui

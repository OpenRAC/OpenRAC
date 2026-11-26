// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The data a multiplayer screen shows: named tables of rows (the games listed, the room's players,
// the scores), each row a stable key and text cells by field. The game binding fills them from the
// game's memory and the port's room every frame; the screens read them.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openrac::mpui {

// One row of a table: its key, its cells by field, and whether choosing it does anything.
struct Row {
    std::uint32_t key = 0;  // Stable across refreshes: a game's world id, a player's client id.
    bool enabled = true;    // A disabled row is drawn dim and refuses Cross.
    std::map<std::string, std::string, std::less<>> cells;  // The row's texts, by column field.

    // Returns the cell of `field`, or an empty text when the row has none.
    std::string_view cell(std::string_view field) const;
};

// A table of rows, in the order they are shown.
class Table {
public:
    // Replaces the rows.
    void set_rows(std::vector<Row> rows) { m_rows = std::move(rows); }

    // Returns the rows in order.
    const std::vector<Row>& rows() const { return m_rows; }

    // Sets the most rows the table can ever hold (a room's player limit), 0 for no limit known.
    void set_capacity(std::size_t capacity) { m_capacity = capacity; }

    // Returns the most rows the table can hold, 0 when none is known.
    std::size_t capacity() const { return m_capacity; }

    // Returns the index of the row with `key`, or rows().size() when there is none.
    std::size_t find(std::uint32_t key) const;

private:
    std::vector<Row> m_rows;     // The rows, in the order the screen shows them.
    std::size_t m_capacity = 0;  // The most rows it can hold, 0 when unknown.
};

// The tables of every screen, by name (`games`, `players`, `scores`, a server's own).
class Model {
public:
    // Returns the table `name`, made empty the first time it is asked for.
    Table& table(std::string_view name);

    // Returns the table `name`, or null when nothing has filled it.
    const Table* find(std::string_view name) const;

private:
    std::map<std::string, Table, std::less<>> m_tables;  // The tables, by name.
};

}  // namespace openrac::mpui

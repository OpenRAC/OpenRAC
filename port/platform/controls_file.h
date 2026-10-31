// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One controls file as text: `key = value` a line, `#` for a comment, as the settings file is
// (docs/port/CONTROLS.md). The player's global controls and each game's own are one each
// (game/common/controls_store.h); the launcher writes the same files.
//
// A change rewrites only the line of its key: every other line, a comment or a key this build does
// not know (one a newer build or the launcher wrote), is kept as it was. A file of a newer format
// than this build's is read for what it knows and never written (read_only()), so a newer build's
// controls are not lost by going back to an older one.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openrac::platform {

// The format this build writes, the file's `format` key. 1: bind_* and pad_* keys, and the global
// file's player* keys.
inline constexpr int kControlsFormat = 1;

class ControlsFile {
public:
    ControlsFile() = default;

    // The file's text, read as it is: its lines are kept.
    static ControlsFile parse(std::string_view text);

    // The file as text, a line each, ended by a newline; empty for a file with no lines.
    std::string text() const;

    // The value of `key`: its last line's, as the settings file reads it, or nothing.
    std::optional<std::string> get(std::string_view key) const;

    // Sets `key` to `value`: its last line is rewritten in place and any earlier one removed, or a
    // line is added at the end. A file without a format line gets one first.
    void set(std::string_view key, std::string_view value);

    // Removes every line of `key`. Returns whether there was one.
    bool erase(std::string_view key);

    // Removes every line whose key starts with `prefix`. Returns how many keys went.
    int erase_prefix(std::string_view prefix);

    // Every key and its value (each key's last line), the format line's too.
    std::map<std::string, std::string, std::less<>> values() const;

    // The file's format: its `format` key, 0 when it has none (an empty file, or one written by
    // hand).
    int format() const;

    // A newer build wrote it: read what is known, never write it (set and erase do nothing).
    bool read_only() const { return format() > kControlsFormat; }

    // No key but the format's, comments or not.
    bool empty() const;

private:
    std::vector<std::string> m_lines;
};

// The key and value of one line, trimmed; nothing for a comment, a blank line or one without "=".
std::optional<std::pair<std::string, std::string>> controls_line(std::string_view line);

}  // namespace openrac::platform

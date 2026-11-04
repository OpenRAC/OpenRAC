// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/controls_file.h"

#include <charconv>

namespace openrac::platform {
namespace {

std::string_view trimmed(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
        s.remove_suffix(1);
    }
    return s;
}

}  // namespace

std::optional<std::pair<std::string, std::string>> controls_line(std::string_view line) {
    line = trimmed(line);
    const std::size_t equals = line.find('=');
    if (line.empty() || line.front() == '#' || equals == std::string_view::npos) {
        return std::nullopt;
    }
    const std::string_view key = trimmed(line.substr(0, equals));
    if (key.empty()) {
        return std::nullopt;
    }
    return std::pair{std::string(key), std::string(trimmed(line.substr(equals + 1)))};
}

ControlsFile ControlsFile::parse(std::string_view text) {
    ControlsFile file;
    for (std::size_t at = 0; at < text.size();) {
        const std::size_t end = text.find('\n', at);
        const std::size_t stop = end == std::string_view::npos ? text.size() : end;
        std::string_view line = text.substr(at, stop - at);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        file.m_lines.emplace_back(line);
        at = stop + 1;
    }
    return file;
}

std::string ControlsFile::text() const {
    std::string out;
    for (const std::string& line : m_lines) {
        out += line;
        out += '\n';
    }
    return out;
}

std::optional<std::string> ControlsFile::get(std::string_view key) const {
    std::optional<std::string> found;
    for (const std::string& line : m_lines) {
        if (const auto kv = controls_line(line); kv && kv->first == key) {
            found = kv->second;
        }
    }
    return found;
}

void ControlsFile::set(std::string_view key, std::string_view value) {
    if (read_only()) {
        return;
    }
    if (key != "format" && !get("format")) {
        m_lines.insert(m_lines.begin(), "format = " + std::to_string(kControlsFormat));
        m_lines.insert(m_lines.begin(), "# OpenRAC controls (docs/port/CONTROLS.md)");
    }
    const std::string line = std::string(key) + " = " + std::string(value);
    std::ptrdiff_t last = -1;
    for (std::size_t i = 0; i < m_lines.size(); ++i) {
        if (const auto kv = controls_line(m_lines[i]); kv && kv->first == key) {
            last = static_cast<std::ptrdiff_t>(i);
        }
    }
    if (last < 0) {
        m_lines.push_back(line);
        return;
    }
    m_lines[static_cast<std::size_t>(last)] = line;
    // Earlier lines of the key said something the last one overrode: they go.
    for (std::ptrdiff_t i = last - 1; i >= 0; --i) {
        const auto kv = controls_line(m_lines[static_cast<std::size_t>(i)]);
        if (kv && kv->first == key) {
            m_lines.erase(m_lines.begin() + i);
        }
    }
}

bool ControlsFile::erase(std::string_view key) {
    if (read_only()) {
        return false;
    }
    const auto before = m_lines.size();
    std::erase_if(m_lines, [key](const std::string& line) {
        const auto kv = controls_line(line);
        return kv && kv->first == key;
    });
    return m_lines.size() != before;
}

int ControlsFile::erase_prefix(std::string_view prefix) {
    if (read_only()) {
        return 0;
    }
    int gone = 0;
    for (const auto& [key, value] : values()) {
        if (key.starts_with(prefix) && erase(key)) {
            ++gone;
        }
    }
    return gone;
}

std::map<std::string, std::string, std::less<>> ControlsFile::values() const {
    std::map<std::string, std::string, std::less<>> out;
    for (const std::string& line : m_lines) {
        if (auto kv = controls_line(line)) {
            out[kv->first] = std::move(kv->second);
        }
    }
    return out;
}

bool ControlsFile::empty() const {
    const auto all = values();
    return all.empty() || (all.size() == 1 && all.contains("format"));
}

int ControlsFile::format() const {
    const std::optional<std::string> value = get("format");
    int number = 0;
    if (!value
        || std::from_chars(value->data(), value->data() + value->size(), number).ec
               != std::errc{}) {
        return 0;
    }
    return number;
}

}  // namespace openrac::platform

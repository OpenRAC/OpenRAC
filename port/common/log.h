// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's log: one line per message, to stderr and, once a file is set, to
// that file too. Messages are formatted with std::format.

#pragma once

#include <cstdio>
#include <format>
#include <string_view>

namespace openrac::log {

enum class Level {
    Debug,
    Info,
    Warn,
    Error
};

// Messages below this level are dropped. Info by default; OPENRAC_LOG=debug
// in the environment lowers it.
void set_level(Level level);
Level level();

// Also write every message to this file (appended). An empty path stops it.
void set_file(const char* path);

void write(Level level, std::string_view message);

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args) {
    if (level() <= Level::Debug) {
        write(Level::Debug, std::format(fmt, std::forward<Args>(args)...));
    }
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
    if (level() <= Level::Info) {
        write(Level::Info, std::format(fmt, std::forward<Args>(args)...));
    }
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
    if (level() <= Level::Warn) {
        write(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
    }
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
    write(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

// Logs the message as an error and ends the program.
[[noreturn]] void fatal(std::string_view message);

template <typename... Args>
[[noreturn]] void fatalf(std::format_string<Args...> fmt, Args&&... args) {
    fatal(std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace openrac::log

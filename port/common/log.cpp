// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "log.h"

#include <cstdlib>
#include <cstring>
#include <mutex>

namespace openrac::log {
namespace {

Level initial_level() {
    const char* env = std::getenv("OPENRAC_LOG");
    if (env != nullptr && std::strcmp(env, "debug") == 0) {
        return Level::Debug;
    }
    return Level::Info;
}

std::mutex g_lock;
Level g_level = initial_level();
std::FILE* g_file = nullptr;

const char* prefix(Level level) {
    switch (level) {
        case Level::Debug:
            return "debug";
        case Level::Info:
            return "info";
        case Level::Warn:
            return "warning";
        case Level::Error:
            return "error";
    }
    return "?";
}

}  // namespace

void set_level(Level level) {
    g_level = level;
}

Level level() {
    return g_level;
}

void set_file(const char* path) {
    std::scoped_lock hold(g_lock);
    if (g_file != nullptr) {
        std::fclose(g_file);
        g_file = nullptr;
    }
    if (path != nullptr && path[0] != '\0') {
        g_file = std::fopen(path, "a");
    }
}

void write(Level level, std::string_view message) {
    std::scoped_lock hold(g_lock);
    std::fprintf(
        stderr, "[%s] %.*s\n", prefix(level), static_cast<int>(message.size()), message.data()
    );
    if (g_file != nullptr) {
        std::fprintf(
            g_file, "[%s] %.*s\n", prefix(level), static_cast<int>(message.size()), message.data()
        );
        std::fflush(g_file);
    }
}

void fatal(std::string_view message) {
    write(Level::Error, message);
    std::exit(1);
}

}  // namespace openrac::log

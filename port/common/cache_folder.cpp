// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "common/cache_folder.h"

#include <optional>
#include <string>

#include "common/env.h"

namespace openrac {

std::filesystem::path cache_folder() {
#if defined(__APPLE__)
    const std::string home = env::system_variable("HOME").value_or(".");
    return std::filesystem::path(home) / "Library/Caches/OpenRAC";
#elif defined(__ANDROID__)
    const std::string home = env::system_variable("HOME").value_or(".");
    return std::filesystem::path(home) / "cache";
#else
    if (const std::optional<std::string> xdg = env::system_variable("XDG_CACHE_HOME");
        xdg.has_value() && !xdg->empty()) {
        return std::filesystem::path(*xdg) / "openrac";
    }
    std::optional<std::string> home = env::system_variable("USERPROFILE");
    if (!home.has_value() || home->empty()) {
        home = env::system_variable("HOME");
    }
    return std::filesystem::path(home.value_or(".")) / ".cache/openrac";
#endif
}

}  // namespace openrac

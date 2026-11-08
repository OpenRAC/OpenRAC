// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace openrac::lighting {
void inspect_emissive_task(
    const std::filesystem::path& levels,
    const std::filesystem::path& review,
    std::string_view task_id,
    std::ostream& out
);
}

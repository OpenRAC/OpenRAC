// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Local staged output replacement for native level preparation.
#pragma once

#include <filesystem>
#include <string_view>

#include "common/result.h"

namespace openrac::lighting {

// Writes and closes a unique staged file beside `output`, then atomically replaces it.
// Leaves the previous file intact on failure; the caller owns synchronization between bakers.
[[nodiscard]] Result<void> write_stage_file(
    const std::filesystem::path& output, std::string_view text
);

// Renames one closed file or directory, retrying only brief Windows sharing/access locks.
[[nodiscard]] Result<void> rename_stage_file(
    const std::filesystem::path& staged, const std::filesystem::path& output
);

// Atomically replaces one closed staged file on the same filesystem; does not copy its payload.
[[nodiscard]] Result<void> replace_stage_file(
    const std::filesystem::path& staged, const std::filesystem::path& output
);

}  // namespace openrac::lighting

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Same-directory atomic file publication with exclusive staging ownership.
#pragma once
#include <filesystem>

#include "common/result.h"

namespace openrac {
// Owns one exclusively created staging folder; failed writes never replace the destination.
class StagedFile {
public:
    StagedFile(const StagedFile&) = delete;
    StagedFile& operator=(const StagedFile&) = delete;
    // Transfers exclusive cleanup ownership, leaving the old owner empty.
    StagedFile(StagedFile&& other) noexcept;
    // Releases any current staging and takes the new owner's paths.
    StagedFile& operator=(StagedFile&& other) noexcept;
    // Removes only the owned staged file and its newly created directory.
    ~StagedFile();
    // Creates a private sibling directory; reports invalid targets and filesystem failures explicitly.
    [[nodiscard]] static Result<StagedFile> create(const std::filesystem::path& target);

    // Returns the staged path to write and close before publishing.
    [[nodiscard]] const std::filesystem::path& path() const { return m_file; }

    // Atomically replaces the target; on failure its old contents remain intact.
    [[nodiscard]] Result<void> publish();

private:
    // Takes ownership of a directory successfully reserved by create.
    StagedFile(std::filesystem::path target, std::filesystem::path directory);
    // Cleans only this owner's paths, including after a failed write or publication.
    void clear() noexcept;
    std::filesystem::path m_target;     // Destination file, never removed during cleanup.
    std::filesystem::path m_directory;  // Exclusively owned private staging directory.
    std::filesystem::path m_file;       // Only file owned inside that directory.
};
}  // namespace openrac

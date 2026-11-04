// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// staged_file.h's exclusive staging and atomic replacement using filesystem operations.
#include "common/staged_file.h"

#include <utility>

namespace openrac {
StagedFile::StagedFile(std::filesystem::path target, std::filesystem::path directory)
    : m_target(std::move(target)),
      m_directory(std::move(directory)),
      m_file(m_directory / "contents") {}

StagedFile::StagedFile(StagedFile&& other) noexcept
    : m_target(std::move(other.m_target)),
      m_directory(std::move(other.m_directory)),
      m_file(std::move(other.m_file)) {
    other.m_directory.clear();
    other.m_file.clear();
}

StagedFile& StagedFile::operator=(StagedFile&& other) noexcept {
    // Self-move must retain ownership of the existing private directory.
    if (this != &other) {
        clear();
        m_target = std::move(other.m_target);
        m_directory = std::move(other.m_directory);
        m_file = std::move(other.m_file);
        other.m_directory.clear();
        other.m_file.clear();
    }
    return *this;
}

StagedFile::~StagedFile() {
    clear();
}

void StagedFile::clear() noexcept {
    // Moved-from instances own nothing; cleanup never walks or recursively removes a directory.
    if (m_directory.empty()) {
        return;
    }
    std::error_code ignored;
    std::filesystem::remove(m_file, ignored);
    std::filesystem::remove(m_directory, ignored);
    m_directory.clear();
    m_file.clear();
}

Result<StagedFile> StagedFile::create(const std::filesystem::path& target) {
    // A profile save must name a file, never replace an authoring folder.
    if (target.empty() || target.filename().empty()) {
        return unexpected(Error{"atomic file save requires a destination filename"});
    }
    const auto parent =
        target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();
    std::error_code error;
    std::filesystem::create_directories(parent, error);
    if (error) {
        return unexpected(Error{"cannot create save folder: " + error.message()});
    }
    // Exclusive directory creation gives concurrent editors independent publication files.
    constexpr unsigned kMaximumAttempts = 1000;
    for (unsigned attempt = 0; attempt < kMaximumAttempts; ++attempt) {
        auto name = target.filename();
        name += ".save-" + std::to_string(attempt);
        const auto directory = parent / name;
        if (std::filesystem::create_directory(directory, error)) {
            return StagedFile(target, directory);
        }
        if (error && error != std::errc::file_exists) {
            return unexpected(Error{"cannot reserve save staging: " + error.message()});
        }
    }
    return unexpected(Error{"cannot reserve private save staging: all candidates are in use"});
}

Result<void> StagedFile::publish() {
    std::error_code error;
    std::filesystem::rename(m_file, m_target, error);
    // Standard filesystem rename publishes a complete file or retains the old destination.
    if (error) {
        return unexpected(
            Error{"cannot replace file; previous contents retained: " + error.message()}
        );
    }
    return {};
}
}  // namespace openrac

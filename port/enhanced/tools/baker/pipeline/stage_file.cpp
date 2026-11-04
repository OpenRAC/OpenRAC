// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Staged native output replacement with same-filesystem atomic publication on Windows and POSIX.
#include "enhanced/tools/baker/pipeline/stage_file.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace openrac::lighting {
namespace {
// Owns only the newly created staging directory and its one output file.
class StagedFile {
public:
    // Rejects a colliding staging name rather than removing another writer's data.
    explicit StagedFile(const std::filesystem::path& target) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_directory = target.parent_path() / (".bake-stage-" + std::to_string(stamp));
        m_path = m_directory / "output";

        // Directory creation supplies exclusive ownership for cleanup.
        if (!std::filesystem::create_directory(m_directory)) {
            throw std::runtime_error("cannot create unique bake staging directory");
        }
    }

    StagedFile(const StagedFile&) = delete;
    StagedFile& operator=(const StagedFile&) = delete;

    // Cleans only this writer's temporary file and directory, including failed writes.
    ~StagedFile() {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
        std::filesystem::remove(m_directory, ignored);
    }

    // Returns the one temporary path inside the exclusively owned directory.
    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_directory;  // Created exclusively by this instance.
    std::filesystem::path m_path;       // Closed before replacement or cleanup.
};

// Whether an existing file destination may be atomically replaced.
enum class MoveMode {
    Rename,
    Replace
};

#ifdef _WIN32
// Matches the reference file helper's 13 attempts and 2.06-second total wait budget.
void move_windows(
    const std::filesystem::path& staged, const std::filesystem::path& target, MoveMode mode
) {
    const DWORD flags = mode == MoveMode::Replace ? MOVEFILE_REPLACE_EXISTING : 0;
    std::chrono::milliseconds delay(10);

    // Closed files may remain briefly held by Windows filesystem scanners.
    for (unsigned attempt = 0; attempt < 13; ++attempt) {
        // Successful atomic publication ends the retry budget immediately.
        if (MoveFileExW(staged.c_str(), target.c_str(), flags)) {
            return;
        }

        const DWORD error = GetLastError();

        // Other errors and persistent locks keep their original failure; never copy over the target.
        if (attempt == 12
            || (error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION
                && error != ERROR_LOCK_VIOLATION)) {
            throw std::system_error(
                static_cast<int>(error), std::system_category(), "cannot move staged bake output"
            );
        }

        std::this_thread::sleep_for(delay);
        delay = std::min(delay * 2, std::chrono::milliseconds(250));
    }
}
#endif

// Keeps platform move failures inside the staged-output Result boundary.
Result<void> move_file(
    const std::filesystem::path& staged, const std::filesystem::path& target, MoveMode mode
) {
    // Platform move errors leave through the shared Result boundary.
    try {
#ifdef _WIN32
        move_windows(staged, target, mode);
#else
        // POSIX rename supplies both atomic file replacement and directory publication.
        std::filesystem::rename(staged, target);
        (void)mode;  // Both modes use the same POSIX atomic rename primitive.
#endif
        return {};
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace

Result<void> rename_stage_file(
    const std::filesystem::path& staged, const std::filesystem::path& output
) {
    return move_file(staged, output, MoveMode::Rename);
}

Result<void> replace_stage_file(
    const std::filesystem::path& staged, const std::filesystem::path& output
) {
    return move_file(staged, output, MoveMode::Replace);
}

Result<void> write_stage_file(const std::filesystem::path& output, std::string_view text) {
    // Staging errors must preserve the previous published file.
    try {
        StagedFile staged(output);
        std::ofstream stream(staged.path(), std::ios::binary);

        // A partial write cannot replace the previous verified file.
        if (!stream || !stream.write(text.data(), static_cast<std::streamsize>(text.size()))) {
            return unexpected(Error{"cannot write staged bake output"});
        }

        stream.close();

        // Closing can expose delayed filesystem errors.
        if (!stream) {
            return unexpected(Error{"cannot close staged bake output"});
        }

        return replace_stage_file(staged.path(), output);
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting

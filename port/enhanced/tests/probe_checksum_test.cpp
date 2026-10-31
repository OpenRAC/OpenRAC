// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Saved-file checksum checks against deterministic bytes, including corruption across a hash-reader block boundary.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "tests/check.h"

namespace {
namespace fs = std::filesystem;
using openrac::lighting::bake_json::verify_file_checksum;

// Owns one atomically reserved temporary directory and only the file created by this fixture.
class ScratchFile {
public:
    // Concurrent tests reserve distinct directory names without depending on the clock.
    ScratchFile() {
        constexpr unsigned kAttempts = 1024;  // Bound retries after interrupted test runs.
        for (unsigned attempt = 0; attempt < kAttempts; ++attempt) {
            m_directory =
                fs::temp_directory_path() / ("openrac-checksum-" + std::to_string(attempt));
            m_file = m_directory / "asset.bin";
            // Existing reservations belong to another test process or an interrupted run.
            if (fs::create_directory(m_directory)) {
                return;
            }
        }
        throw std::runtime_error("cannot reserve checksum test directory");
    }

    ScratchFile(const ScratchFile&) = delete;
    ScratchFile& operator=(const ScratchFile&) = delete;

    // Nonrecursive cleanup leaves any unrelated files untouched, even during exception unwinding.
    ~ScratchFile() {
        std::error_code error;
        fs::remove(m_file, error);
        fs::remove(m_directory, error);
    }

    // Returns the file owned until this fixture is destroyed.
    const fs::path& path() const { return m_file; }

    // Replaces this fixture's file with exactly the requested deterministic payload.
    void write(std::span<const std::uint8_t> bytes) const {
        std::ofstream output(path(), std::ios::binary | std::ios::trunc);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        // Empty files exercise the SHA-256 empty-input digest without passing a null data pointer.
        if (!bytes.empty()) {
            output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())
            );
        }
        output.close();
    }

private:
    fs::path m_directory;  // Atomically created test-owned directory.
    fs::path m_file;       // File owned exclusively by this fixture.
};

// A nonuniform payload crosses the existing 64 KiB file-reader block by one byte.
std::vector<std::uint8_t> fixture_bytes() {
    std::vector<std::uint8_t> bytes(65'537);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(i % 251);  // Prime period exposes ordering errors.
    }
    return bytes;
}

// Exact nonempty and empty files retain their expected size and content digests.
void test_exact_files_pass() {
    ScratchFile file;
    const auto bytes = fixture_bytes();
    file.write(bytes);
    CHECK(verify_file_checksum(file.path(), bytes));
    file.write({});
    CHECK(verify_file_checksum(file.path(), {}));
}

// Same-sized corruption is detected in both full and partial streaming blocks.
void test_content_changes_fail() {
    ScratchFile file;
    const auto expected = fixture_bytes();
    for (const auto index : {std::size_t{0}, expected.size() - 1}) {
        auto changed = expected;
        changed[index] ^= 1;
        file.write(changed);
        const auto result = verify_file_checksum(file.path(), expected);
        CHECK(!result);
        CHECK(result.error().message == "bake read-back checksum mismatch");
    }
}

// Truncated, extended and absent files cannot authorize completed bake output.
void test_incomplete_files_fail() {
    ScratchFile file;
    const auto expected = fixture_bytes();
    file.write(std::span(expected).first(expected.size() - 1));
    CHECK(!verify_file_checksum(file.path(), expected));
    auto extended = expected;
    extended.push_back(0);
    file.write(extended);
    CHECK(!verify_file_checksum(file.path(), expected));
    fs::remove(file.path());
    CHECK(!verify_file_checksum(file.path(), expected));
}
}  // namespace

// Filesystem errors remain ordinary test failures rather than escaping the test process.
int main() {
    try {
        test_exact_files_pass();
        test_content_changes_fail();
        test_incomplete_files_fail();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}

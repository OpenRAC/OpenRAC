// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Atomic metadata publication: replacement, exclusive staging and failure retention in owned temporary paths.
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "common/staged_file.h"
#include "tests/check.h"

namespace {
// Writes complete synthetic contents and checks both write and close status.
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    file << text;
    file.close();
    CHECK(!file.fail());
}

// Reads synthetic fixture contents, independent of host newline conventions.
std::string read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    CHECK(file);
    return {std::istreambuf_iterator<char>(file), {}};
}

// Two publishers own separate staging files and can replace an existing target without partial writes.
void test_publication_and_failure_retention() {
    auto fixture = openrac::StagedFile::create(
        std::filesystem::temp_directory_path() / std::filesystem::path(u8"openrac-staged-\u6c34")
    );
    CHECK(fixture);
    if (!fixture) {
        return;
    }
    const auto target = fixture->path();
    auto first = openrac::StagedFile::create(target);
    auto second = openrac::StagedFile::create(target);
    CHECK(first && second);
    if (!first || !second) {
        return;
    }
    CHECK(first->path() != second->path());
    write(first->path(), "old complete profile");
    CHECK(first->publish());
    CHECK(read(target) == "old complete profile");
    write(second->path(), "new complete profile");
    CHECK(second->publish());
    CHECK(read(target) == "new complete profile");
    // A file cannot act as a parent folder; the failed save must retain its existing contents.
    CHECK(!openrac::StagedFile::create(target / "child.json"));
    CHECK(read(target) == "new complete profile");
}
}  // namespace

// No existing folder or user asset is removed; each fixture owns an exclusively reserved directory.
int main() {
    try {
        test_publication_and_failure_retention();
        return openrac::test::result();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}

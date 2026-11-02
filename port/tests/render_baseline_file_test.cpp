// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Ephemeral baseline validation: identity, shape, corruption, truncation and trailing bytes.

#include <filesystem>
#include <fstream>
#include <vector>

#include "tests/check.h"
#include "tests/render_baseline_file.h"

namespace {

// Rejects invalid baseline data without using a GPU or a driver's reference images.
void test_checked_baselines(const std::filesystem::path& path) {
    namespace test = openrac::test;
    const std::vector<std::uint8_t> pixels{0, 127, 255, 255, 1, 2, 3, 4};
    CHECK(test::write_render_baseline(path, "build/device/settings", pixels));
    const auto executable = test::baseline_executable_checksum(path);
    CHECK(executable.has_value());
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size()) == pixels);
    CHECK(test::read_render_baseline(path, "other build/device/settings", pixels.size()).empty());
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size() + 1).empty());
    {
        std::ofstream append(path, std::ios::binary | std::ios::app);
        append.put('x');
    }
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size()).empty());
    CHECK(test::baseline_executable_checksum(path) != executable);
    CHECK(test::write_render_baseline(path, "build/device/settings", pixels));
    {
        std::fstream corrupt(path, std::ios::binary | std::ios::in | std::ios::out);
        corrupt.seekp(-1, std::ios::end);
        corrupt.put('x');
    }
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size()).empty());
    CHECK(test::write_render_baseline(path, "build/device/settings", pixels));
    std::filesystem::resize_file(path, std::filesystem::file_size(path) - 1);
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size()).empty());
    std::filesystem::remove(path);
    CHECK(!test::baseline_executable_checksum(path));
    CHECK(test::read_render_baseline(path, "build/device/settings", pixels.size()).empty());
}

// Chunked executable hashing must include complete chunks and the final short chunk.
void test_streamed_identity(const std::filesystem::path& path) {
    namespace test = openrac::test;
    constexpr std::size_t kPayload = 2 * 64 * 1024 + 3;  // Two stream buffers and a short tail.
    std::vector<std::uint8_t> bytes(kPayload);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(index);
    }
    {
        std::ofstream out(path, std::ios::binary);
        out.write(
            reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())
        );
    }
    CHECK(test::baseline_executable_checksum(path) == test::baseline_checksum(bytes));
    std::filesystem::remove(path);
}

}  // namespace

// Uses CTest's build-local working directory; no player cache or retail data is involved.
int main() {
    test_checked_baselines("render-baseline-file-test.bin");
    test_streamed_identity("render-baseline-executable-test.bin");
    return openrac::test::result();
}

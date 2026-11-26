// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// File publication and cached CPU compilation results.
#include "tools/shader_compiler/file_io.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <fstream>
#include <stdexcept>

#include "common/staged_file.h"

namespace openrac::shader_compiler {
using namespace vkgl;

namespace {
// Avoids rebuilding generated C++ when its bytes are unchanged.
bool same_file(const std::filesystem::path& path, std::span<const char> bytes) {
    std::ifstream old(path, std::ios::binary | std::ios::ate);

    if (old && old.tellg() == static_cast<std::streamoff>(bytes.size())) {
        old.seekg(0);
        std::array<char, 65536> buffer;
        std::size_t at = 0;

        while (at < bytes.size()) {
            const auto count = std::min(buffer.size(), bytes.size() - at);
            old.read(buffer.data(), static_cast<std::streamsize>(count));

            if (old.gcount() != static_cast<std::streamsize>(count)
                || std::memcmp(buffer.data(), bytes.data() + at, count) != 0) {
                break;
            }

            at += count;
        }

        if (at == bytes.size()) {
            return true;
        }
    }

    return false;
}
}  // namespace

void write_file(const std::filesystem::path& path, std::span<const char> bytes) {
    if (same_file(path, bytes)) {
        return;
    }

    std::filesystem::create_directories(path.parent_path());
    auto staged = StagedFile::create(path);

    if (!staged) {
        throw std::runtime_error(staged.error().message);
    }

    std::ofstream out(staged->path(), std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.close();

    if (!out) {
        throw std::runtime_error("cannot write " + path.string());
    }

    const auto published = staged->publish();

    if (!published) {
        throw std::runtime_error(published.error().message);
    }
}

std::string cache_name(ShaderKey key) {
    return std::format("{:016x}-{:016x}.bin", key.key, key.check);
}

std::vector<char> read_cached_program(const std::filesystem::path& output, ShaderKey key) {
    std::ifstream input(output / ".cache" / cache_name(key), std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});

    if (bytes.size() < 8 || bytes.size() > std::size_t{16} * 1024 * 1024) {
        return {};
    }

    std::uint64_t raw_size = 0;

    for (unsigned n = 0; n < 8; ++n) {
        raw_size |= std::uint64_t(static_cast<unsigned char>(bytes[n])) << (n * 8);
    }

    return decompress_shader_blob(std::span(bytes).subspan(8), raw_size);
}

void cache_program(
    const std::filesystem::path& output, ShaderKey key, std::span<const char> value
) {
    auto compressed = compress_shader_blob(value);
    std::vector<char> bytes;
    bytes.reserve(8 + compressed.size());

    for (unsigned n = 0; n < 8; ++n) {
        bytes.push_back(static_cast<char>(value.size() >> (n * 8)));
    }

    bytes.insert(bytes.end(), compressed.begin(), compressed.end());
    write_file(output / ".cache" / cache_name(key), bytes);
}

}  // namespace openrac::shader_compiler

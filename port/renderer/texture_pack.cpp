// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/texture_pack.h"

#include <format>
#include <mutex>
#include <set>
#include <unordered_map>

#include "common/log.h"

namespace openrac::renderer::texture_pack {
namespace {

namespace fs = std::filesystem;

struct State {
    std::mutex lock;  // levels load on another thread than the 2D layer converts on
    Io io;
    std::unordered_map<std::uint64_t, fs::path> files;  // the pack's pictures by hash
    fs::path dump;
    std::set<std::uint64_t> dumped;
    unsigned replaced = 0;
};

State& state() {
    static State s;
    return s;
}

// A pack file's hash from its name, or nothing when it is not one.
std::optional<std::uint64_t> hash_of(const fs::path& file) {
    const std::string stem = file.stem().string();
    if (file.extension() != ".png" || stem.size() != 16) {
        return {};
    }
    std::uint64_t value = 0;
    for (const char c : stem) {
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (digit < 0) {
            return {};
        }
        value = value << 4 | static_cast<std::uint64_t>(digit);
    }
    return value;
}

}  // namespace

void configure(const fs::path& pack, const fs::path& dump, Io io) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.io = io;
    s.files.clear();
    s.dumped.clear();
    s.replaced = 0;
    s.dump = dump;
    std::error_code e;
    if (!pack.empty() && fs::is_directory(pack, e)) {
        for (const auto& entry : fs::recursive_directory_iterator(pack, e)) {
            if (entry.is_regular_file(e)) {
                if (const auto hash = hash_of(entry.path())) {
                    s.files.emplace(*hash, entry.path());
                }
            }
        }
        log::info("texture pack {}: {} texture(s)", pack.string(), s.files.size());
    }
    if (!dump.empty()) {
        fs::create_directories(dump, e);
        log::info("textures dumped to {}", dump.string());
    }
}

std::uint64_t image_hash(const Rgba8Image& image) {
    std::uint64_t h = 1469598103934665603ull;
    const auto mix = [&](std::uint8_t byte) {
        h ^= byte;
        h *= 1099511628211ull;
    };
    for (const int side : {image.width, image.height}) {
        for (int i = 0; i < 4; ++i) {
            mix(static_cast<std::uint8_t>(static_cast<std::uint32_t>(side) >> (8 * i)));
        }
    }
    for (const std::uint8_t byte : image.pixels) {
        mix(byte);
    }
    return h;
}

std::string file_name(std::uint64_t hash) {
    return std::format("{:016x}.png", hash);
}

bool replace(Rgba8Image& image) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    if ((s.files.empty() && s.dump.empty()) || image.pixels.empty()) {
        return false;
    }
    const std::uint64_t hash = image_hash(image);
    if (!s.dump.empty() && s.io.save != nullptr && s.dumped.insert(hash).second) {
        if (!s.io.save(s.dump / file_name(hash), image)) {
            log::warn("texture {} could not be dumped", file_name(hash));
        }
    }
    const auto found = s.files.find(hash);
    if (found == s.files.end() || s.io.load == nullptr) {
        return false;
    }
    auto picture = s.io.load(found->second);
    if (!picture || picture->pixels.empty()) {
        log::warn("texture pack: {} cannot be read; the original is kept", found->second.string());
        s.files.erase(found);
        return false;
    }
    image = std::move(*picture);
    ++s.replaced;
    return true;
}

unsigned replaced_count() {
    State& s = state();
    const std::lock_guard guard(s.lock);
    return s.replaced;
}

}  // namespace openrac::renderer::texture_pack

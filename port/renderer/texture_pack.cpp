// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "renderer/texture_pack.h"

#include <format>
#include <mutex>
#include <string_view>
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
    LevelNamer level_name = nullptr;
    std::unordered_map<std::uint64_t, fs::path> dumped;  // where each dumped texture lies, by hash
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

// Every texture file under a folder, by hash.
std::unordered_map<std::uint64_t, fs::path> files_under(const fs::path& root) {
    std::unordered_map<std::uint64_t, fs::path> files;
    std::error_code e;
    if (root.empty() || !fs::is_directory(root, e)) {
        return files;
    }
    for (const auto& entry : fs::recursive_directory_iterator(root, e)) {
        if (entry.is_regular_file(e)) {
            if (const auto hash = hash_of(entry.path())) {
                files.emplace(*hash, entry.path());
            }
        }
    }
    return files;
}

// A planet's name as a folder name: no separators or characters Windows refuses, no trailing
// dots or spaces.
std::string folder_name(std::string name) {
    for (char& c : name) {
        if (std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos
            || static_cast<unsigned char>(c) < 32) {
            c = ' ';
        }
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
        name.pop_back();
    }
    while (!name.empty() && name.front() == ' ') {
        name.erase(name.begin());
    }
    return name;
}

// Writes or moves the texture's dump file; the lock is held.
void dump(State& s, std::uint64_t hash, const Rgba8Image& image, const Origin& origin) {
    std::error_code e;
    const fs::path folder = dump_folder(origin);
    const auto found = s.dumped.find(hash);
    if (found == s.dumped.end()) {
        fs::create_directories(s.dump / folder, e);
        const fs::path file = s.dump / folder / file_name(hash);
        if (!s.io.save(file, image)) {
            log::warn("texture {} could not be dumped", file_name(hash));
        }
        s.dumped.emplace(hash, file);
        return;
    }
    // A level's texture dumped under another planet is shared: it moves to Shared/.
    const fs::path where = found->second.parent_path().lexically_relative(s.dump);
    if (origin.interface || origin.level < 0 || where.empty() || *where.begin() != "Planets"
        || where == folder) {
        return;
    }
    fs::create_directories(s.dump / "Shared", e);
    const fs::path shared = s.dump / "Shared" / file_name(hash);
    fs::rename(found->second, shared, e);
    if (e) {
        log::warn("texture {} could not be moved to Shared: {}", file_name(hash), e.message());
        return;
    }
    found->second = shared;
}

}  // namespace

fs::path dump_folder(const Origin& origin) {
    if (origin.interface) {
        return "Interface";
    }
    if (origin.level < 0) {
        return "Shared";
    }
    const State& s = state();
    std::string name = s.level_name != nullptr ? folder_name(s.level_name(origin.level)) : "";
    if (name.empty()) {
        name = std::format("Level {}", origin.level);
    }
    return fs::path("Planets") / name;
}

void configure(const fs::path& pack, const fs::path& dump, Io io, LevelNamer level_name) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    s.io = io;
    s.level_name = level_name;
    s.files.clear();
    s.dumped.clear();
    s.replaced = 0;
    s.dump = dump;
    std::error_code e;
    if (!pack.empty() && fs::is_directory(pack, e)) {
        s.files = files_under(pack);
        log::info("texture pack {}: {} texture(s)", pack.string(), s.files.size());
    }
    if (!dump.empty()) {
        fs::create_directories(dump, e);
        // Textures dumped by an earlier run are not written again, but may still move to Shared/.
        s.dumped = files_under(dump);
        log::info("textures dumped to {} ({} there already)", dump.string(), s.dumped.size());
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

bool replace(Rgba8Image& image, const Origin& origin) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    if ((s.files.empty() && s.dump.empty()) || image.pixels.empty()) {
        return false;
    }
    const std::uint64_t hash = image_hash(image);
    if (!s.dump.empty() && s.io.save != nullptr) {
        dump(s, hash, image, origin);
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

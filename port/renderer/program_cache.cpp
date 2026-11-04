// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/program_cache.h"

#include <array>
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "common/cache_folder.h"
#include "common/env.h"
#include "common/log.h"

namespace openrac::renderer::program_cache {

using namespace openrac::gl;

namespace {

// A file: the magic, the binary's format, a word of nothing, the key's check, the binary's length
// and its hash, all little-endian as the machine writes them (a cache never leaves the machine),
// then the binary.
constexpr std::array<char, 8> kMagic = {'O', 'R', 'G', 'L', 'P', 'R', 'G', '1'};
constexpr std::size_t kHeader = 8 + 4 + 4 + 8 + 8 + 8;
// A file nobody loaded or wrote for this long goes (an old build's programs, another driver's).
constexpr auto kUnusedFor = std::chrono::hours(24 * 30);

std::uint64_t fnv1a(std::string_view bytes, std::uint64_t hash) {
    for (const char c : bytes) {
        hash = (hash ^ static_cast<unsigned char>(c)) * 0x100000001B3ull;
    }
    return hash;
}

std::uint64_t fnv1a(const std::vector<std::uint8_t>& bytes) {
    return fnv1a(
        std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()),
        0xCBF29CE484222325ull
    );
}

struct Entry {
    GLenum format = 0;
    std::uint64_t check = 0;
    std::vector<std::uint8_t> binary;
};

struct State {
    bool decided = false;
    bool on = false;
    std::filesystem::path folder;  // this driver's; empty: memory only
    std::unordered_map<std::uint64_t, Entry> memory;
    Counts counts;
};

State g;

std::string gl_string(GLenum name) {
    const GLubyte* s = glGetString(name);
    return s != nullptr ? reinterpret_cast<const char*>(s) : "";
}

// Files of `root`'s driver folders that nobody used for kUnusedFor, and folders left empty.
void prune(const std::filesystem::path& root) {
    std::error_code ec;
    const auto now = std::filesystem::file_time_type::clock::now();
    for (const auto& driver : std::filesystem::directory_iterator(root, ec)) {
        if (!driver.is_directory(ec)) {
            continue;
        }
        for (const auto& file : std::filesystem::directory_iterator(driver.path(), ec)) {
            const auto written = file.last_write_time(ec);
            if (!ec && now - written > kUnusedFor) {
                std::filesystem::remove(file.path(), ec);
            }
        }
        if (std::filesystem::is_empty(driver.path(), ec)) {
            std::filesystem::remove(driver.path(), ec);
        }
    }
}

// Once per context: whether programs are kept, and where.
void decide() {
    if (g.decided) {
        return;
    }
    g.decided = true;
    g.on = false;
    const std::optional<std::string_view> chosen = env::value(env::Var::GlProgramCache);
    if (chosen && (*chosen == "off" || *chosen == "0" || *chosen == "none")) {
        log::verbose("OpenGL program cache: off (OPENRAC_GL_PROGRAM_CACHE)");
        return;
    }
    if (glGetProgramBinary == nullptr || glProgramBinary == nullptr
        || glProgramParameteri == nullptr) {
        return;
    }
    GLint formats = 0;
    glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
    if (formats <= 0) {
        log::verbose("OpenGL program cache: the driver gives no program binaries");
        return;
    }
    g.on = true;
    const std::filesystem::path root = chosen && !chosen->empty() ? std::filesystem::path(*chosen)
                                                                  : cache_folder() / "gl_programs";
    // The driver as it names itself: a binary is its own, for this version of it.
    const std::string driver = gl_string(GL_VENDOR) + '\n' + gl_string(GL_RENDERER) + '\n'
                               + gl_string(GL_VERSION) + '\n'
                               + gl_string(GL_SHADING_LANGUAGE_VERSION);
    g.folder = root / std::format("{:016x}", fnv1a(driver, 0xCBF29CE484222325ull));
    prune(root);
    log::verbose("OpenGL program cache: {}", g.folder.string());
}

std::filesystem::path file_of(std::uint64_t name) {
    return g.folder / std::format("{:016x}.bin", name);
}

template <typename T>
T read_at(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    T value;
    std::memcpy(&value, bytes.data() + at, sizeof value);
    return value;
}

// The entry in `name`'s file, when the file is one whole save.
std::optional<Entry> read_file(std::uint64_t name) {
    std::ifstream in(file_of(name), std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    if (bytes.size() < kHeader || std::memcmp(bytes.data(), kMagic.data(), kMagic.size()) != 0) {
        return std::nullopt;
    }
    Entry e;
    e.format = read_at<std::uint32_t>(bytes, 8);
    e.check = read_at<std::uint64_t>(bytes, 16);
    const auto length = read_at<std::uint64_t>(bytes, 24);
    const auto hash = read_at<std::uint64_t>(bytes, 32);
    if (length != bytes.size() - kHeader) {
        return std::nullopt;
    }
    e.binary.assign(bytes.begin() + kHeader, bytes.end());
    if (fnv1a(e.binary) != hash) {
        return std::nullopt;
    }
    return e;
}

// Through a temporary file of this save's own, renamed into place: a reader never sees half a
// file, and two programs saving one at once leave one whole save.
void write_file(std::uint64_t name, const Entry& e) {
    std::error_code ec;
    std::filesystem::create_directories(g.folder, ec);
    const std::filesystem::path path = file_of(name);
    const std::filesystem::path temporary =
        path.string()
        + std::format(".{}.tmp", std::chrono::steady_clock::now().time_since_epoch().count());
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) {
            return;
        }
        std::array<std::uint8_t, kHeader> header{};
        const std::uint32_t format = e.format;
        const std::uint64_t length = e.binary.size();
        const std::uint64_t hash = fnv1a(e.binary);
        std::memcpy(header.data(), kMagic.data(), kMagic.size());
        std::memcpy(header.data() + 8, &format, 4);
        std::memcpy(header.data() + 16, &e.check, 8);
        std::memcpy(header.data() + 24, &length, 8);
        std::memcpy(header.data() + 32, &hash, 8);
        out.write(reinterpret_cast<const char*>(header.data()), header.size());
        out.write(
            reinterpret_cast<const char*>(e.binary.data()), static_cast<std::streamsize>(length)
        );
        if (!out) {
            out.close();
            std::filesystem::remove(temporary, ec);
            return;
        }
    }
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        std::filesystem::remove(temporary, ec);
    }
}

}  // namespace

Key key(std::string_view vertex, std::string_view fragment) {
    // Each stage's length first, so that no two pairs of sources run together into one.
    const std::string lengths = std::format("{}:{}:", vertex.size(), fragment.size());
    Key k;
    k.name = fnv1a(fragment, fnv1a(vertex, fnv1a(lengths, 0xCBF29CE484222325ull)));
    k.check = fnv1a(fragment, fnv1a(vertex, fnv1a(lengths, 0x84222325CBF29CE4ull)));
    return k;
}

bool enabled() {
    decide();
    return g.on;
}

GLuint load(const Key& key) {
    if (!enabled()) {
        return 0;
    }
    auto found = g.memory.find(key.name);
    if (found == g.memory.end()) {
        std::optional<Entry> saved = read_file(key.name);
        if (!saved) {
            return 0;
        }
        // Used: it stays past the pruning.
        std::error_code ec;
        std::filesystem::last_write_time(
            file_of(key.name), std::filesystem::file_time_type::clock::now(), ec
        );
        found = g.memory.emplace(key.name, std::move(*saved)).first;
    }
    const Entry& e = found->second;
    if (e.check != key.check) {
        return 0;  // another program's, by a chance of the hash: compiled, and kept over it
    }
    const GLuint program = glCreateProgram();
    glProgramBinary(program, e.format, e.binary.data(), static_cast<GLsizei>(e.binary.size()));
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == 0) {
        // Refused: a format the driver no longer takes is an error of GL's, which goes with it.
        glDeleteProgram(program);
        while (glGetError() != GL_NO_ERROR) {
        }
        g.memory.erase(found);
        std::error_code ec;
        std::filesystem::remove(file_of(key.name), ec);
        ++g.counts.refused;
        return 0;
    }
    ++g.counts.loaded;
    return program;
}

void prepare(GLuint program) {
    if (enabled()) {
        glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    }
}

void store(const Key& key, GLuint program) {
    if (!enabled()) {
        return;
    }
    GLint length = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);
    if (length <= 0) {
        return;
    }
    Entry e;
    e.check = key.check;
    e.binary.resize(static_cast<std::size_t>(length));
    GLsizei written = 0;
    glGetProgramBinary(program, length, &written, &e.format, e.binary.data());
    if (written <= 0) {
        return;
    }
    e.binary.resize(static_cast<std::size_t>(written));
    write_file(key.name, e);
    g.memory[key.name] = std::move(e);
    ++g.counts.compiled;
}

Counts counts() {
    return g.counts;
}

void reset() {
    g = State{};
}

}  // namespace openrac::renderer::program_cache

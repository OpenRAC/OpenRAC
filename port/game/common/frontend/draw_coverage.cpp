// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "game/common/frontend/draw_coverage.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/env.h"

namespace openrac::frontend::draw_coverage {
namespace {

// Ticks between two writes of the file.
constexpr std::uint64_t kWriteEvery = 300;
// Unpacks a signature lists at most (a batch's layout repeats past them).
constexpr std::size_t kLayoutUnpacks = 12;

std::uint64_t fnv(std::span<const std::uint8_t> bytes) {
    std::uint64_t h = 0xCBF29CE484222325ull;
    for (const std::uint8_t b : bytes) {
        h = (h ^ b) * 0x100000001B3ull;
    }
    return h;
}

struct Program {
    std::uint32_t command, entry;
    std::uint64_t code;  // the hash of the microcode loaded where the entry is, 0 for none seen
    std::string layout;
    auto operator<=>(const Program&) const = default;
};

struct ProgramCount {
    std::uint64_t calls = 0, drawn = 0, ticks = 0, last_tick = UINT64_MAX;
    std::uint32_t ram = 0;  // where the first batch's data came from
    std::uint64_t alpha = 0, test = 0, tex0 = 0;
    std::string sample;  // the first 64 bytes of that data, in hex
};

struct ParticleCount {
    std::uint64_t drawn = 0;
    std::map<std::string, std::uint64_t, std::less<>> left_out;
    std::string update;
};

struct Level {
    std::uint64_t ticks = 0;
    std::map<Program, ProgramCount> programs;
    std::map<unsigned, ParticleCount> particles;
};

struct Coverage {
    std::string path;
    std::map<int, Level> levels;
    // Microcode by its VU1 load address, kept across ticks: the game loads it when it changes.
    std::map<std::uint32_t, std::uint64_t> code;
    Level* level = nullptr;
    std::uint64_t tick = 0, ticks = 0;
    // The batch since the last program start.
    std::string layout;
    std::size_t unpacks = 0;
    std::optional<std::uint32_t> ram;
    std::string sample;

    ~Coverage() { write(); }

    void write() const {
        if (path.empty() || levels.empty()) {
            return;
        }
        std::string out = "{\"levels\": {";
        bool first_level = true;
        for (const auto& [number, l] : levels) {
            out += std::format(
                "{}\n \"{}\": {{\"ticks\": {}, \"programs\": [",
                first_level ? "" : ",",
                number,
                l.ticks
            );
            first_level = false;
            std::vector<std::pair<const Program*, const ProgramCount*>> rows;
            for (const auto& [p, c] : l.programs) {
                rows.emplace_back(&p, &c);
            }
            // Undrawn first, the most called first.
            std::ranges::sort(rows, [](const auto& a, const auto& b) {
                const auto left = [](const ProgramCount& c) {
                    return c.calls - c.drawn;
                };
                return std::tuple(left(*b.second), b.second->calls)
                       < std::tuple(left(*a.second), a.second->calls);
            });
            bool first = true;
            for (const auto& [p, c] : rows) {
                out += std::format(
                    "{}\n  {{\"command\": \"{}\", \"entry\": \"{:#x}\", \"code\": \"{:016x}\", "
                    "\"layout\": \"{}\", \"calls\": {}, \"drawn\": {}, \"ticks\": {}, "
                    "\"ram\": \"{:#x}\", \"alpha\": \"{:#x}\", \"test\": \"{:#x}\", "
                    "\"tex0\": \"{:#x}\", \"sample\": \"{}\"}}",
                    first ? "" : ",",
                    p->command == 0x14   ? "MSCAL"
                    : p->command == 0x15 ? "MSCALF"
                                         : "MSCNT",
                    p->entry,
                    p->code,
                    p->layout,
                    c->calls,
                    c->drawn,
                    c->ticks,
                    c->ram,
                    c->alpha,
                    c->test,
                    c->tex0,
                    c->sample
                );
                first = false;
            }
            out += "], \"particles\": [";
            first = true;
            for (const auto& [type, c] : l.particles) {
                std::string left;
                for (const auto& [reason, n] : c.left_out) {
                    left += std::format("{}\"{}\": {}", left.empty() ? "" : ", ", reason, n);
                }
                out += std::format(
                    "{}\n  {{\"type\": {}, \"update\": \"{}\", \"drawn\": {}, "
                    "\"left_out\": {{{}}}}}",
                    first ? "" : ",",
                    type,
                    c.update,
                    c.drawn,
                    left
                );
                first = false;
            }
            out += "]}";
        }
        out += "\n}}\n";
        // Written beside and moved in place: a run stopped half way leaves the last whole file.
        const std::string part = path + ".part";
        if (std::FILE* f = std::fopen(part.c_str(), "wb")) {
            const bool ok = std::fwrite(out.data(), 1, out.size(), f) == out.size();
            if (std::fclose(f) == 0 && ok) {
                std::remove(path.c_str());
                std::rename(part.c_str(), path.c_str());
            }
        }
    }
};

Coverage* coverage() {
    static Coverage* c = [] {
        const std::optional<std::string_view> path = env::value(env::Var::DrawCoverage);
        if (!path || path->empty()) {
            return static_cast<Coverage*>(nullptr);
        }
        static Coverage owned;
        owned.path = std::string(*path);
        return &owned;
    }();
    return c;
}

}  // namespace

bool armed() {
    return coverage() != nullptr;
}

void begin_tick(int level, std::uint64_t tick) {
    Coverage* c = coverage();
    if (c == nullptr) {
        return;
    }
    if (++c->ticks % kWriteEvery == 0) {
        c->write();
    }
    c->level = &c->levels[level];
    c->level->ticks++;
    c->tick = tick;
    c->layout.clear();
    c->unpacks = 0;
    c->ram.reset();
    c->sample.clear();
}

void program(std::uint32_t at, std::span<const std::uint8_t> code) {
    if (Coverage* c = coverage()) {
        c->code[at] = fnv(code);
    }
}

void unpack(
    std::uint32_t address,
    unsigned vn,
    unsigned vl,
    unsigned count,
    std::uint32_t ram,
    std::span<const std::uint8_t> data
) {
    Coverage* c = coverage();
    if (c == nullptr || c->level == nullptr) {
        return;
    }
    if (!c->ram) {
        c->ram = ram;
        for (std::size_t i = 0; i < std::min<std::size_t>(data.size(), 64); ++i) {
            c->sample += std::format("{:02x}", data[i]);
        }
    }
    if (c->unpacks++ < kLayoutUnpacks) {
        // V<n>-<bits>, with the count only for a single quadword (headers, GIF tags).
        const unsigned bits = vl == 0 ? 32 : vl == 1 ? 16 : vl == 2 ? 8 : 5;
        c->layout += std::format(
            "{}{:x}:V{}-{}{}",
            c->layout.empty() ? "" : " ",
            address,
            vl == 3 ? 4 : vn + 1,
            bits,
            count == 1 ? "x1" : ""
        );
    }
}

void start(
    std::uint32_t command,
    std::uint32_t entry,
    bool drawn,
    std::uint64_t alpha,
    std::uint64_t test,
    std::uint64_t tex0
) {
    Coverage* c = coverage();
    if (c == nullptr || c->level == nullptr) {
        return;
    }
    std::uint64_t code = 0;
    if (auto it = c->code.upper_bound(entry); it != c->code.begin()) {
        code = std::prev(it)->second;
    }
    ProgramCount& n =
        c->level->programs[Program{command, command == 0x17 ? 0 : entry, code, c->layout}];
    if (n.calls == 0) {
        n.ram = c->ram.value_or(0);
        n.alpha = alpha;
        n.test = test;
        n.tex0 = tex0;
        n.sample = c->sample;
    }
    n.calls++;
    n.drawn += drawn ? 1 : 0;
    if (n.last_tick != c->tick) {
        n.last_tick = c->tick;
        n.ticks++;
    }
    c->layout.clear();
    c->unpacks = 0;
    c->ram.reset();
    c->sample.clear();
}

void particle(unsigned type, std::string_view reason, std::string_view update) {
    Coverage* c = coverage();
    if (c == nullptr || c->level == nullptr) {
        return;
    }
    ParticleCount& n = c->level->particles[type];
    if (n.update.empty()) {
        n.update = std::string(update);
    }
    if (reason.empty()) {
        n.drawn++;
    } else if (auto it = n.left_out.find(reason); it != n.left_out.end()) {
        it->second++;
    } else {
        n.left_out.emplace(std::string(reason), 1);
    }
}

}  // namespace openrac::frontend::draw_coverage

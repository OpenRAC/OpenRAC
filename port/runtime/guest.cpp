// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The runtime side of guest.h: the game stack, string literals, the table of
// functions by code address, and the list of functions the game called that
// have no C yet.

#include "openrac/guest.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/log.h"
#include "openrac/memory.h"

extern "C" {
gaddr openrac_guest_sp = GUEST_STACK_TOP;
}

namespace {

using openrac::log::error;
using openrac::log::fatalf;
using openrac::log::info;
using openrac::log::warn;

// ---- String literals ----

std::mutex g_strings_lock;
std::unordered_map<std::string, gaddr> g_strings;
gaddr g_strings_next = GUEST_STRINGS;

// ---- Functions by address ----

struct Entry {
    openrac_host_fn fn;
    const char* name;
};

using Table = std::unordered_map<gaddr, Entry>;

Table g_exe;
std::map<int, Table> g_overlays;
// Functions registered at two or more addresses of one program: tiny functions the catalogue
// folded into one name (their bytes are the same but for the address they call), each place a
// different function. A call through one of their addresses goes where its code calls.
std::unordered_map<openrac_host_fn, int> g_ambiguous;
int g_overlay = OPENRAC_OVERLAY_EXE;
thread_local gaddr g_unknown_target = 0;

bool strict() {
    const char* env = std::getenv("OPENRAC_STRICT");
    return env != nullptr && env[0] == '1';
}

int (*g_overlay_source)(void) = nullptr;

const Entry* find(gaddr address) {
    if (g_overlay_source != nullptr) {
        g_overlay = g_overlay_source();
    }
    if (g_overlay != OPENRAC_OVERLAY_EXE) {
        auto level = g_overlays.find(g_overlay);
        if (level != g_overlays.end()) {
            auto it = level->second.find(address);
            if (it != level->second.end()) {
                return &it->second;
            }
        }
    }
    auto it = g_exe.find(address);
    return it == g_exe.end() ? nullptr : &it->second;
}

// What a call through an unknown code address runs: it logs and returns, so
// that bring-up can go on (OPENRAC_STRICT=1 stops instead).
void unknown_function() {
    static std::mutex lock;
    static std::map<gaddr, int> seen;
    std::scoped_lock hold(lock);
    if (seen[g_unknown_target]++ == 0) {
        error(
            "call to code address {:#010x}, which no function of the port has (overlay {})",
            g_unknown_target,
            g_overlay
        );
    }
}

// ---- Missing functions ----

std::mutex g_missing_lock;
std::map<std::string, unsigned long> g_missing;
bool g_stop_on_missing = false;

}  // namespace

extern "C" {

gaddr openrac_guest_frame(uint32_t size) {
    const gaddr sp = (openrac_guest_sp - size) & ~0xFu;
    if (sp < GUEST_STACK_BOTTOM || sp > openrac_guest_sp) {
        fatalf("the game stack is full ({} bytes asked)", size);
    }
    openrac_guest_sp = sp;
    return sp;
}

gaddr openrac_guest_static(uint32_t size) {
    static std::mutex lock;
    static gaddr next = GUEST_STATICS;
    std::scoped_lock hold(lock);
    const gaddr at = next;
    if (at + size > GUEST_STATICS + GUEST_STATICS_SIZE) {
        fatalf("the statics region is full");
    }
    next = (at + size + 15) & ~15u;
    return at;
}

gaddr openrac_guest_string(const char* s, size_t size) {
    std::scoped_lock hold(g_strings_lock);
    std::string key(s, size);
    auto it = g_strings.find(key);
    if (it != g_strings.end()) {
        return it->second;
    }
    const gaddr at = g_strings_next;
    if (at + size > GUEST_STRINGS + GUEST_STRINGS_SIZE) {
        fatalf("the string region is full");
    }
    std::memcpy(G(at), s, size);
    g_strings_next = (at + static_cast<gaddr>(size) + 3) & ~3u;
    g_strings.emplace(std::move(key), at);
    return at;
}

void openrac_guest_register(int overlay, const openrac_fn_entry* entries, size_t count) {
    Table& table = overlay == OPENRAC_OVERLAY_EXE ? g_exe : g_overlays[overlay];
    std::unordered_map<openrac_host_fn, int> places;
    for (size_t i = 0; i < count; i++) {
        if (++places[entries[i].fn] == 2) {
            g_ambiguous[entries[i].fn] = 1;
        }
    }
    for (size_t i = 0; i < count; i++) {
        auto [it, added] = table.emplace(entries[i].address, Entry{entries[i].fn, entries[i].name});
        if (!added && it->second.fn != entries[i].fn) {
            warn(
                "two functions at {:#010x} in overlay {}: {} and {}; keeping the first",
                entries[i].address,
                overlay,
                it->second.name,
                entries[i].name
            );
        }
    }
}

void openrac_guest_set_overlay(int overlay) {
    g_overlay = overlay;
}

int openrac_guest_overlay(void) {
    return g_overlay;
}

openrac_host_fn openrac_guest_function(gaddr address) {
    if (const Entry* e = find(address)) {
        // A folded wrapper (addiu sp, -16; sq ra; jal T; nop; lq ra; jr ra; addiu sp, 16): this
        // place's own target, read from the program in memory.
        for (int depth = 0; depth < 4 && g_ambiguous.count(e->fn) != 0; ++depth) {
            std::uint32_t w[7];
            std::memcpy(w, G(address), sizeof(w));
            if (w[0] != 0x27BDFFF0u || w[1] != 0x7FBF0000u || (w[2] >> 26) != 3 || w[3] != 0
                || w[4] != 0x7BBF0000u || w[5] != 0x03E00008u || w[6] != 0x27BD0010u) {
                break;
            }
            const gaddr target = (address & 0xF0000000u) | ((w[2] & 0x03FFFFFFu) << 2);
            const Entry* t = find(target);
            if (t == nullptr) {
                break;
            }
            address = target;
            e = t;
        }
        return e->fn;
    }
    if (strict()) {
        fatalf(
            "call to code address {:#010x}, which no function of the port has (overlay {})",
            address,
            g_overlay
        );
    }
    g_unknown_target = address;
    return unknown_function;
}

gaddr openrac_guest_nearest(const uint32_t* places, int count, gaddr caller) {
    // places: (overlay + 1) << 24 | address; the loaded program's place nearest the caller (the
    // caller and the copy it calls come from one object file, so they sit together).
    const int overlay = g_overlay_source != nullptr ? g_overlay_source() : g_overlay;
    gaddr best = 0;
    gaddr fallback = 0;
    for (int i = 0; i < count; ++i) {
        const int o = static_cast<int>(places[i] >> 24) - 1;
        const gaddr a = places[i] & 0x00FFFFFFu;
        if (fallback == 0 || o == OPENRAC_OVERLAY_EXE) {
            fallback = a;
        }
        if (o == overlay) {
            const auto distance = [caller](gaddr x) { return x > caller ? x - caller : caller - x; };
            if (best == 0 || distance(a) < distance(best)) {
                best = a;
            }
        }
    }
    return best != 0 ? best : fallback;
}

const char* openrac_guest_function_name(gaddr address) {
    const Entry* e = find(address);
    return e == nullptr ? nullptr : e->name;
}

void openrac_guest_missing(const char* name) {
    {
        std::scoped_lock hold(g_missing_lock);
        if (g_missing[name]++ != 0) {
            return;
        }
    }
    if (g_stop_on_missing) {
        error("the game called {}, which has no C in the port yet; stopping here", name);
        std::exit(2);
    }
    warn("{} has no C in the port yet; it does nothing", name);
}

void openrac_guest_stop_on_missing(int stop) {
    g_stop_on_missing = stop != 0;
}

void openrac_guest_set_overlay_source(int (*current)(void)) {
    g_overlay_source = current;
}

void openrac_guest_report_missing(void) {
    std::scoped_lock hold(g_missing_lock);
    if (g_missing.empty()) {
        return;
    }
    std::vector<std::pair<std::string, unsigned long>> calls(g_missing.begin(), g_missing.end());
    std::sort(calls.begin(), calls.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    warn("{} functions without C were called:", calls.size());
    for (const auto& [name, count] : calls) {
        warn("  {:>8}  {}", count, name);
    }
}

}  // extern "C"

// ---- Per-level relocation (guest.h) ----

namespace {

struct LevelMap {
    std::vector<std::pair<gaddr, std::int32_t>> data;  // executable address, delta; sorted
    std::unordered_map<gaddr, gaddr> code;              // executable function -> level copy
};

gaddr g_relocate_gp = 0;
std::vector<std::uint8_t> g_exe_code;
gaddr g_exe_base = 0;
std::map<int, LevelMap> g_level_maps;
std::mutex g_relocate_lock;

int current_overlay() {
    if (g_overlay_source != nullptr) {
        g_overlay = g_overlay_source();
    }
    return g_overlay;
}

std::uint32_t exe_word(gaddr at) {
    std::uint32_t w = 0;
    if (at >= g_exe_base && at + 4 <= g_exe_base + g_exe_code.size()) {
        std::memcpy(&w, g_exe_code.data() + (at - g_exe_base), 4);
    }
    return w;
}

std::uint32_t game_word(gaddr at) {
    std::uint32_t w;
    std::memcpy(&w, G(at), 4);
    return w;
}

// Pairs the address operands of a function's executable code and its copy: known register values
// (lui, then addiu or ori), loads and stores through them or through $gp. The two codes are the
// same instructions apart from addresses, so every pair computed in step is a correspondence.
using WordReader = std::uint32_t (*)(int program, gaddr at);

std::uint32_t exe_reader(int, gaddr at) {
    return exe_word(at);
}

void pair_code(WordReader reference, int program, gaddr exe, gaddr level, std::uint32_t size,
               gaddr low, gaddr high, std::map<gaddr, std::int32_t>& out) {
    std::uint32_t ve[32] = {}, vl[32] = {};
    bool known[32] = {};
    for (std::uint32_t i = 0; i + 4 <= size; i += 4) {
        const std::uint32_t we = reference(program, exe + i), wl = game_word(level + i);
        const std::uint32_t op = we >> 26;
        if (op != (wl >> 26)) {
            break;  // not the same code any more (the copy ends, or another function)
        }
        const std::uint32_t rs = (we >> 21) & 31, rt = (we >> 16) & 31;
        const std::int32_t se = static_cast<std::int16_t>(we & 0xFFFF);
        const std::int32_t sl = static_cast<std::int16_t>(wl & 0xFFFF);
        if (op == 0x0F) {  // lui
            ve[rt] = (we & 0xFFFF) << 16;
            vl[rt] = (wl & 0xFFFF) << 16;
            known[rt] = true;
            continue;
        }
        const bool load = (op >= 0x20 && op <= 0x27) || op == 0x37 || op == 0x1E;
        const bool store = (op >= 0x28 && op <= 0x2F) || op == 0x3F || op == 0x1F;
        const bool fpu = op == 0x31 || op == 0x39;  // lwc1, swc1: an address, no GPR written
        const bool add = op == 0x09 || op == 0x19 || op == 0x0D;  // addiu, daddiu, ori
        std::uint32_t ae = 0, al = 0;
        bool have = false;
        if ((load || store || fpu || add) && rs == 28 && g_relocate_gp != 0) {
            ae = g_relocate_gp + se;
            al = g_relocate_gp + sl;
            have = true;
        } else if ((load || store || fpu || add) && known[rs]) {
            ae = op == 0x0D ? (ve[rs] | (we & 0xFFFF)) : ve[rs] + se;
            al = op == 0x0D ? (vl[rs] | (wl & 0xFFFF)) : vl[rs] + sl;
            have = true;
        }
        // What the instruction writes is no longer a known value (unless it is the address just
        // formed by addiu/ori); a call leaves only the callee-saved registers.
        if (op == 0x00 || op == 0x1C) {
            known[(we >> 11) & 31] = false;
        } else if (op == 0x03) {  // jal
            for (int r = 1; r < 32; ++r) {
                if (!((r >= 16 && r <= 23) || r == 28 || r == 29 || r == 30)) {
                    known[r] = false;
                }
            }
        } else if (add) {
            known[rt] = have;
            if (have) {
                ve[rt] = ae;
                vl[rt] = al;
            }
        } else if (load || (op >= 0x08 && op <= 0x0E) || op == 0x18) {
            known[rt] = false;
        }
        if (!have) {
            continue;
        }
        if (ae - low < high - low) {
            out.emplace(ae, static_cast<std::int32_t>(al - ae));
        }
    }
}

LevelMap& level_map(int overlay) {
    auto found = g_level_maps.find(overlay);
    if (found != g_level_maps.end()) {
        return found->second;
    }
    LevelMap& map = g_level_maps[overlay];
    auto level = g_overlays.find(overlay);
    if (level == g_overlays.end()) {
        return map;
    }
    // The executable's functions in address order, for their sizes.
    std::vector<std::pair<gaddr, openrac_host_fn>> exe;
    exe.reserve(g_exe.size());
    for (const auto& [address, entry] : g_exe) {
        exe.emplace_back(address, entry.fn);
    }
    std::sort(exe.begin(), exe.end());
    std::unordered_map<openrac_host_fn, std::vector<gaddr>> copies;
    for (const auto& [address, entry] : level->second) {
        copies[entry.fn].push_back(address);
    }
    std::map<gaddr, std::int32_t> pairs;
    for (std::size_t k = 0; k < exe.size(); ++k) {
        auto copy = copies.find(exe[k].second);
        if (copy == copies.end()) {
            continue;
        }
        const std::uint32_t size =
            k + 1 < exe.size() ? std::min<std::uint32_t>(exe[k + 1].first - exe[k].first, 0x8000) : 0x400;
        for (const gaddr place : copy->second) {
            if (place != exe[k].first) {
                map.code.emplace(exe[k].first, place);
            }
            pair_code(exe_reader, 0, exe[k].first, place, size, openrac_relocate_low,
                      openrac_relocate_high, pairs);
        }
    }
    map.data.assign(pairs.begin(), pairs.end());
    info("level program {}: {} functions and {} global addresses relocated", overlay, map.code.size(),
         map.data.size());
    return map;
}

// ---- Level to level ----
//
// A function several levels' programs carry is written with the addresses of the level its name
// gives (func_L05_...: level 5's). Its globals and the functions whose addresses it takes are
// looked up the same way as an executable function's, by pairing its code in that level's program
// (read from the player's data, through openrac_guest_set_level_programs) with its copy in the
// level loaded.

struct Chunk {
    gaddr base;
    std::vector<std::uint8_t> bytes;
};

void (*g_level_program_source)(int level) = nullptr;
std::map<int, std::vector<Chunk>> g_level_programs;
std::map<std::pair<int, int>, LevelMap> g_level_pair_maps;  // (from, loaded)
constexpr gaddr kLevelLow = 0x0015F000u, kLevelHigh = 0x00400000u;

const std::vector<Chunk>& level_program(int level) {
    auto found = g_level_programs.find(level);
    if (found == g_level_programs.end()) {
        g_level_programs[level];
        if (g_level_program_source != nullptr) {
            g_level_program_source(level);  // fills g_level_programs[level]
        }
        found = g_level_programs.find(level);
    }
    return found->second;
}

std::uint32_t level_reader(int level, gaddr at) {
    for (const Chunk& c : level_program(level)) {
        if (at >= c.base && at + 4 <= c.base + c.bytes.size()) {
            std::uint32_t w;
            std::memcpy(&w, c.bytes.data() + (at - c.base), 4);
            return w;
        }
    }
    return 0;
}

LevelMap& level_pair_map(int from, int loaded) {
    const auto key = std::make_pair(from, loaded);
    auto found = g_level_pair_maps.find(key);
    if (found != g_level_pair_maps.end()) {
        return found->second;
    }
    LevelMap& map = g_level_pair_maps[key];
    auto source = g_overlays.find(from);
    auto level = g_overlays.find(loaded);
    if (source == g_overlays.end() || level == g_overlays.end() || level_program(from).empty()) {
        return map;
    }
    std::vector<std::pair<gaddr, openrac_host_fn>> own;
    own.reserve(source->second.size());
    for (const auto& [address, entry] : source->second) {
        own.emplace_back(address, entry.fn);
    }
    std::sort(own.begin(), own.end());
    std::unordered_map<openrac_host_fn, std::vector<gaddr>> copies;
    for (const auto& [address, entry] : level->second) {
        copies[entry.fn].push_back(address);
    }
    std::map<gaddr, std::int32_t> pairs;
    for (std::size_t k = 0; k < own.size(); ++k) {
        auto copy = copies.find(own[k].second);
        if (copy == copies.end()) {
            continue;
        }
        const std::uint32_t size =
            k + 1 < own.size() ? std::min<std::uint32_t>(own[k + 1].first - own[k].first, 0x8000) : 0x400;
        for (const gaddr place : copy->second) {
            map.code.emplace(own[k].first, place);
            pair_code(level_reader, from, own[k].first, place, size, kLevelLow, kLevelHigh, pairs);
        }
    }
    map.data.assign(pairs.begin(), pairs.end());
    info("level program {} for level {}'s functions: {} functions and {} global addresses relocated",
         loaded, from, map.code.size(), map.data.size());
    return map;
}

gaddr relocate_by(const LevelMap& map, gaddr address) {
    auto it = std::upper_bound(map.data.begin(), map.data.end(), std::make_pair(address, INT32_MAX));
    if (it == map.data.begin()) {
        return address;
    }
    --it;
    if (address - it->first > 0x100) {
        return address;
    }
    return address + static_cast<gaddr>(it->second);
}

}  // namespace

extern "C" {

gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;

void openrac_guest_set_relocation(gaddr low, gaddr high, gaddr gp, const uint8_t* exe_code,
                                  gaddr exe_base, uint32_t exe_size) {
    openrac_relocate_low = low;
    openrac_relocate_high = high;
    g_relocate_gp = gp;
    g_exe_code.assign(exe_code, exe_code + exe_size);
    g_exe_base = exe_base;
}

gaddr openrac_relocate_data(gaddr address) {
    const int overlay = current_overlay();
    if (overlay == OPENRAC_OVERLAY_EXE) {
        return address;
    }
    std::scoped_lock hold(g_relocate_lock);
    const LevelMap& map = level_map(overlay);
    // A global's own address is what the code names (offsets are added after): exact pairs first;
    // otherwise the nearest pair below it, if close (inside the same object).
    auto it = std::upper_bound(map.data.begin(), map.data.end(), std::make_pair(address, INT32_MAX));
    if (it == map.data.begin()) {
        return address;
    }
    --it;
    if (address - it->first > 0x100) {
        return address;
    }
    return address + static_cast<gaddr>(it->second);
}

gaddr openrac_relocate_code(gaddr address) {
    const int overlay = current_overlay();
    if (overlay == OPENRAC_OVERLAY_EXE) {
        return address;
    }
    std::scoped_lock hold(g_relocate_lock);
    const LevelMap& map = level_map(overlay);
    auto it = map.code.find(address);
    return it == map.code.end() ? address : it->second;
}

void openrac_guest_set_level_programs(void (*load)(int level)) {
    g_level_program_source = load;
}

void openrac_guest_add_level_program(int level, gaddr base, const uint8_t* bytes, uint32_t size) {
    g_level_programs[level].push_back(Chunk{base, std::vector<std::uint8_t>(bytes, bytes + size)});
}

gaddr openrac_relocate_level_data(int from, gaddr address) {
    const int overlay = current_overlay();
    if (overlay == from || overlay == OPENRAC_OVERLAY_EXE || address - kLevelLow >= kLevelHigh - kLevelLow) {
        return address;
    }
    std::scoped_lock hold(g_relocate_lock);
    return relocate_by(level_pair_map(from, overlay), address);
}

gaddr openrac_relocate_level_code(int from, gaddr address) {
    const int overlay = current_overlay();
    if (overlay == from || overlay == OPENRAC_OVERLAY_EXE) {
        return address;
    }
    std::scoped_lock hold(g_relocate_lock);
    const LevelMap& map = level_pair_map(from, overlay);
    auto it = map.code.find(address);
    return it == map.code.end() ? address : it->second;
}

}  // extern "C"

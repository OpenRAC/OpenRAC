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

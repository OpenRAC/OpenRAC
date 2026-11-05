// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The part of a game built from the player's disc (port/tools/hostgen/asmext.py): the functions
// the decompilation still has only in assembly, translated from the disc's own programs where the
// player is (an installed launcher, Android) instead of in the port's build. The build calls each
// through a slot; the part's openrac_asm_register fills them. Loaded before the game runs, from
// --asm-part, or <data>/asm-part/<program>.<so|dylib|dll> where the launcher builds it.

#include "openrac/asm_part.h"

#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>

#include "common/log.h"
#include "mod_library.h"
#include "openrac/guest.h"
#include "program/options.h"
#include "program/program.h"

namespace fs = std::filesystem;

namespace {

fs::path default_part(const openrac::program::Options& o) {
#if defined(_WIN32)
    const char* suffix = ".dll";
#elif defined(__APPLE__)
    const char* suffix = ".dylib";
#else
    const char* suffix = ".so";
#endif
    return o.data / "asm-part" / ("lib" + openrac::program::program_name() + "-part" + suffix);
}

bool g_loaded = false;

// Whether this program calls functions through slots a part fills (cmake/Games.cmake).
#if defined(OPENRAC_ASM_PART_SLOTS)
constexpr bool kHasSlots = true;
#else
constexpr bool kHasSlots = false;
#endif

}  // namespace

namespace openrac::program {

void load_asm_part(const Options& o) {
    const bool named = !o.asm_part.empty();
    // Built with the disc, or with no assembly at all: nothing is called through a slot, and a
    // part left in the data folder (another build's) is not this one's.
    if (!kHasSlots) {
        if (named) {
            log::warn(
                "--asm-part {}: this build has the whole game in it, so the part is not loaded",
                o.asm_part.string()
            );
        }
        return;
    }
    const fs::path path = named ? o.asm_part : default_part(o);
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
        if (named) {
            log::fatalf("--asm-part {}: no such file", path.string());
        }
        return;
    }
    std::string error;
    void* library = mods::library::open(path, error);
    if (library == nullptr) {
        log::fatalf(
            "the part of the game built from your disc ({}) does not load: {}", path.string(), error
        );
    }
    using Register = int (*)();
    auto entry = reinterpret_cast<Register>(mods::library::symbol(library, "openrac_asm_register"));
    if (entry == nullptr) {
        log::fatalf(
            "{} is not a part of the game built from a disc (no openrac_asm_register)",
            path.string()
        );
    }
    const int count = entry();
    if (count < 0) {
        log::fatalf(
            "{} does not fit this game (it asks for what the game does not have): set the "
            "game up from your disc again in the launcher",
            path.string()
        );
    }
    g_loaded = true;
    log::header(std::format("disc part {} ({} functions)", path.filename().string(), count));
}

}  // namespace openrac::program

// A function of the part, called through its slot (asmext.trampoline): the slot, or a stop that
// says what is missing when the part was never built or loaded.
extern "C" void* openrac_asm_external(void* fn, const char* name) {
    if (fn == nullptr) {
        openrac::log::fatalf(
            "{} is in the part of the game built from your disc, which is {}: set the game up from "
            "your disc again in the launcher",
            name,
            g_loaded ? "too old for this game" : "not there"
        );
    }
    return fn;
}

// Where the runtime's globals are, for a part on Android, where they are the game's own
// (guest.h OPENRAC_PART_GLOBALS).
extern "C" void* openrac_asm_global(const char* name) {
#define OPENRAC_PART_ADDRESS(type, field)                                                          \
    if (std::strcmp(name, "openrac_" #field) == 0) {                                               \
        return static_cast<void*>(&openrac_##field);                                               \
    }
    OPENRAC_PART_GLOBALS(OPENRAC_PART_ADDRESS)
#undef OPENRAC_PART_ADDRESS
    return nullptr;
}

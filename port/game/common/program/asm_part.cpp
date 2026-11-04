// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The part of a game built from the player's disc (port/tools/hostgen/asmext.py): the functions
// the decompilation still has only in assembly, translated from the disc's own programs where the
// player is (an installed launcher, Android) instead of in the port's build. The build calls each
// through a slot; the part's openrac_asm_register fills them. Loaded before the game runs, from
// --asm-part, or <data>/asm-part/<program>.<so|dylib|dll> where the launcher builds it.

#include <filesystem>
#include <format>
#include <string>
#include <system_error>

#include "common/log.h"
#include "mod_library.h"
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

}  // namespace

namespace openrac::program {

void load_asm_part(const Options& o) {
    const bool named = !o.asm_part.empty();
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
        log::fatalf("the part of the game built from your disc ({}) does not load: {}", path.string(),
                    error);
    }
    using Register = int (*)();
    auto entry = reinterpret_cast<Register>(mods::library::symbol(library, "openrac_asm_register"));
    if (entry == nullptr) {
        log::fatalf("{} is not a part of the game built from a disc (no openrac_asm_register)",
                    path.string());
    }
    const int count = entry();
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
            name, g_loaded ? "too old for this game" : "not there");
    }
    return fn;
}

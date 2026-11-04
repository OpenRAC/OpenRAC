// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// A tiny real CodeView/PDB executable, also run after relocation by test_crash_symbols.py.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

// DbgHelp requires Windows SDK types to be declared first.
#include <dbghelp.h>

#include "common/crash_symbols.h"
#include "tests/check.h"

__declspec(noinline) int symbol_test_anchor(int value) {
    volatile int result = value + 1;
    return result;
}

// Registered before symbol startup: this runs after its owner has been destroyed.
void late_symbol_check() {
    using namespace openrac::crash::symbols;
    Location location;
    if (std::strcmp(status(), "not checked") != 0 || locate(1, location)) {
        std::fputs("symbol session remained published after cleanup\n", stderr);
        std::_Exit(1);
    }
}

int main(int argc, char**) {
    using namespace openrac::crash::symbols;
    CHECK(std::strcmp(status(), "not checked") == 0);
    Location unavailable;
    CHECK(!locate(1, unavailable));
    CHECK(std::atexit(late_symbol_check) == 0);
    // Model another component owning a DbgHelp session and an existing option.
    SymSetOptions(SymGetOptions() | SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_ANYTHING);
    CHECK(SymInitialize(GetCurrentProcess(), nullptr, FALSE));
    initialize();
    CHECK((SymGetOptions() & SYMOPT_CASE_INSENSITIVE) != 0);
    CHECK((SymGetOptions() & SYMOPT_LOAD_ANYTHING) == 0);
    CHECK(SymCleanup(GetCurrentProcess()));
    std::puts(status());
    Location location;
    if (argc > 1) {
        // A missing PDB remains a usable startup; diagnostics explain the fallback.
        CHECK(std::strstr(status(), "matching local PDB not loaded"));
        locate(reinterpret_cast<std::uintptr_t>(&symbol_test_anchor), location);
        CHECK(location.line == 0);
    } else {
        CHECK(std::strstr(status(), "function and source-line lookup verified"));
        CHECK(locate(reinterpret_cast<std::uintptr_t>(&symbol_test_anchor), location));
        CHECK(std::strstr(location.function, "symbol_test_anchor"));
        CHECK(std::strcmp(location.file, "crash_symbols_test.cpp") == 0);
        CHECK(location.line > 0);
    }
    CHECK(!locate(1, location));
    CHECK(!location.function[0] && !location.file[0] && location.line == 0);
    return openrac::test::result();
}

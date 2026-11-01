// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Local Windows symbols, initialized before installing crash handlers or starting workers.
#pragma once

#include <cstdint>

namespace openrac::crash::symbols {
struct Location {
    char function[256]{};
    char file[128]{};
    std::uint32_t line = 0;
    std::uint64_t displacement = 0;
};

// Loads the executable's local PDB and verifies a known function and line. Idempotent;
// setup may allocate, so call only during startup, never from a fault handler.
void initialize();
// A short startup result, prepared before any fault; contains no symbol payload or paths.
const char* status();
// Best-effort Windows DbgHelp lookup, using the session prepared by initialize().
// Like the existing crash symbol lookup, DbgHelp itself is not async-signal-safe.
bool locate(std::uintptr_t address, Location& out);
}  // namespace openrac::crash::symbols

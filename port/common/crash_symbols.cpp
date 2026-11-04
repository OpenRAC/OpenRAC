// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "common/crash_symbols.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <windows.h>

// DbgHelp requires Windows SDK types to be declared first.
#include <dbghelp.h>

#include "common/crash.h"
#include "common/log.h"

namespace openrac::crash::symbols {
namespace {
// A real, non-inlined code location ensures the startup check requires our PDB,
// rather than succeeding against Windows DLL exports or a merely loaded module.
__declspec(noinline) void startup_symbol_probe() {
    volatile unsigned marker = 1;
    (void)marker;
}

// Startup owns the DbgHelp session; subsequent lookups reuse it without loading
// another session. A distinct handle avoids collisions with mods using DbgHelp.
class SymbolContext;
std::atomic<SymbolContext*>& published_context();

class SymbolContext {
public:
    // Unpublish before releasing the session so late shutdown reports fall back safely.
    ~SymbolContext() {
        published_context().store(nullptr, std::memory_order_release);
        if (m_initialized) {
            SymCleanup(m_process);
        }
        if (m_process) {
            CloseHandle(m_process);
        }
    }

    SymbolContext() = default;
    SymbolContext(const SymbolContext&) = delete;
    SymbolContext& operator=(const SymbolContext&) = delete;
    void initialize();
    bool locate(std::uintptr_t address, Location& out) const;

    const char* status() const { return m_attempted ? m_status.c_str() : "not checked"; }

private:
    HANDLE m_process = nullptr;
    bool m_attempted = false, m_initialized = false;
    Text m_status;
};

SymbolContext& context() {
    static SymbolContext symbols;
    return symbols;
}

// Reporting before configure() must not construct the owner or register its destructor
// from a fault handler. Publish the completed startup session with a lock-free pointer.
std::atomic<SymbolContext*>& published_context() {
    static_assert(std::atomic<SymbolContext*>::is_always_lock_free);
    static std::atomic<SymbolContext*> symbols{nullptr};
    return symbols;
}

bool SymbolContext::locate(std::uintptr_t address, Location& out) const {
    out = {};
    if (!m_initialized) {
        return false;
    }
    alignas(SYMBOL_INFO) std::array<char, sizeof(SYMBOL_INFO) + 256> storage{};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage.data());
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;
    DWORD64 displacement = 0;
    const bool named = SymFromAddr(m_process, address, &displacement, symbol) != FALSE;
    if (named) {
        std::memcpy(
            out.function,
            symbol->Name,
            std::min(std::strlen(symbol->Name), sizeof(out.function) - 1)
        );
        out.displacement = displacement;
    }
    // A line lookup is independent: keep a useful source line even if name lookup fails.
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD column = 0;
    if (SymGetLineFromAddr64(m_process, address, &column, &line) && line.FileName) {
        const char* file = line.FileName;
        for (const char* at = file; *at; ++at) {
            if (*at == '/' || *at == '\\') {
                file = at + 1;
            }
        }
        std::memcpy(out.file, file, std::min(std::strlen(file), sizeof(out.file) - 1));
        out.line = line.LineNumber;
    }
    return named || out.line != 0;
}

void SymbolContext::initialize() {
    if (m_attempted) {
        return;
    }
    m_attempted = true;
    std::array<wchar_t, 32768> path{};
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    if (!length || length >= path.size()) {
        m_status.str("unavailable: executable path error ").dec(GetLastError());
        return;
    }
    wchar_t* separator = nullptr;
    for (auto* at = path.data(); *at; ++at) {
        if (*at == L'/' || *at == L'\\') {
            separator = at;
        }
    }
    if (!separator) {
        m_status.str("unavailable: executable directory missing");
        return;
    }
    // Retain a drive root's separator (C:\\), otherwise remove the filename separator.
    separator[separator == path.data() + 2 && path[1] == L':' ? 1 : 0] = L'\0';
    const HANDLE process = GetCurrentProcess();
    if (!DuplicateHandle(process, process, process, &m_process, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        m_status.str("unavailable: process handle error ").dec(GetLastError());
        return;
    }
    // Search beside the actual executable, independent of the launcher's working
    // directory. Do not add a symbol server or download anything during a crash.
    // Preserve compatible process-wide options, but never accept a different build's PDB.
    SymSetOptions(
        (SymGetOptions() & ~SYMOPT_LOAD_ANYTHING) | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES
        | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS
        | SYMOPT_EXACT_SYMBOLS
    );
    if (!SymInitializeW(m_process, path.data(), TRUE)) {
        m_status.str("unavailable: DbgHelp initialization error ").dec(GetLastError());
        return;
    }
    m_initialized = true;
    Location probe;
    const auto address = reinterpret_cast<std::uintptr_t>(&startup_symbol_probe);
    locate(address, probe);  // Force the executable's deferred PDB load during startup.
    IMAGEHLP_MODULE64 module{};
    module.SizeOfStruct = sizeof(module);
    if (!SymGetModuleInfo64(m_process, address, &module) || module.SymType != SymPdb
        || module.PdbUnmatched) {
        m_status.str(
            "unavailable: matching local PDB not loaded; keep the build's PDB beside the executable"
        );
    } else if (!std::strstr(probe.function, "startup_symbol_probe")) {
        m_status.str("local PDB loaded; function lookup failed");
    } else if (!probe.line || !std::strstr(probe.file, "crash_symbols.cpp")) {
        m_status.str("local PDB loaded; function lookup verified, source lines unavailable");
    } else {
        m_status.str("local PDB loaded; function and source-line lookup verified");
    }
}
}  // namespace

void initialize() {
    auto& symbols = context();
    symbols.initialize();
    published_context().store(&symbols, std::memory_order_release);
    log::info("Windows symbols: {}", symbols.status());
}

const char* status() {
    const auto* symbols = published_context().load(std::memory_order_acquire);
    return symbols ? symbols->status() : "not checked";
}

bool locate(std::uintptr_t address, Location& out) {
    const auto* symbols = published_context().load(std::memory_order_acquire);
    if (!symbols) {
        out = {};
        return false;
    }
    return symbols->locate(address, out);
}
}  // namespace openrac::crash::symbols
#endif

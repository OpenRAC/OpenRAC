// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reports an access to game memory that is not mapped: the game address and
// the kind of access, then ends the program. Anything else is left to the
// system's own handling.

#include <cstdio>
#include <cstdlib>

#include "openrac/memory.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#endif

namespace openrac::runtime {
namespace {

void report(const void* host, const char* what) {
    gaddr address = 0;
    if (Memory::get().contains(host, &address)) {
        std::fprintf(
            stderr, "[error] %s at game address 0x%08X, which is not mapped\n", what, address
        );
    } else {
        std::fprintf(stderr, "[error] %s at host address %p\n", what, host);
    }
    std::fflush(stderr);
}

#if defined(_WIN32)

LONG WINAPI on_exception(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    const auto kind = info->ExceptionRecord->ExceptionInformation[0];
    const auto* host =
        reinterpret_cast<const void*>(info->ExceptionRecord->ExceptionInformation[1]);
    report(host, kind == 1 ? "write" : "read");
    std::_Exit(3);
}

#else

void on_signal(int sig, siginfo_t* info, void*) {
    report(info->si_addr, sig == SIGBUS ? "bus error" : "access");
    // The host functions on the way there (the translated game code is named after the game's).
    void* frames[32];
    const int count = backtrace(frames, 32);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _exit(3);
}

#endif

}  // namespace

void install_crash_handler() {
#if defined(_WIN32)
    AddVectoredExceptionHandler(1, on_exception);
#else
    struct sigaction action {};

    action.sa_sigaction = on_signal;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
#endif
}

}  // namespace openrac::runtime

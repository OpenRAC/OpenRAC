// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The program's fatal faults: an access to game memory that is not mapped (with the game address
// and the kind of access), an illegal instruction, a trap, an arithmetic fault, an abort, and on Windows
// the structured exceptions that are these; and a C++ exception nobody caught. Each writes a crash
// report (common/crash.h) and ends the program with status 3.

#include "common/crash.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include "common/env.h"
#include "openrac/guest.h"
#include "openrac/memory.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <csignal>
#include <windows.h>
#else
#include <csignal>
#include <cstring>
#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/ucontext.h>
#if defined(__linux__)
#include <ucontext.h>
#endif
#endif

namespace openrac::runtime {
namespace {

// The fault's words, built without allocating, then the report; the program ends here.
[[noreturn]] void report(const void* host, const char* what, crash::Kind kind) {
    crash::Text reason;
    gaddr address = 0;

    if (host != nullptr && Memory::get().contains(host, &address)) {
        reason.str(what).str(" at game address ").hex(address, 8).str(", which is not mapped");
    } else if (host != nullptr) {
        reason.str(what).str(" at host address ").hex(reinterpret_cast<std::uintptr_t>(host));
    } else {
        reason.str(what);
    }

    crash::report(reason.c_str(), kind);
    std::_Exit(3);
}

// std::terminate: a C++ exception nobody caught (or a noexcept one broken).
[[noreturn]] void on_terminate() {
    crash::Text reason;
    reason.str("a C++ exception nobody caught");

    if (const std::exception_ptr e = std::current_exception()) {
        try {
            std::rethrow_exception(e);
        } catch (const std::exception& x) {
            reason.str(": ").str(x.what());
        } catch (...) {
        }
    }

    crash::report(reason.c_str(), crash::Kind::Exception);
    std::_Exit(3);
}

#if defined(_WIN32)

// The code the program never catches a fault in: its own image (the .exe) and the libraries it
// loads itself (add_own_library). Read by the handler on any thread, so each range is atomic.
struct CodeRange {
    std::atomic<std::uintptr_t> low{0}, high{0};
};

constexpr int kOwnRanges = 128;
CodeRange g_own[kOwnRanges];

// The image's range from its PE headers: no call into the system in the handler.
void image_range(const void* module, std::uintptr_t& low, std::uintptr_t& high) {
    const auto* base = static_cast<const std::uint8_t*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    low = reinterpret_cast<std::uintptr_t>(base);
    high = low + nt->OptionalHeader.SizeOfImage;
}

bool in_own_code(const void* address) {
    const auto at = reinterpret_cast<std::uintptr_t>(address);
    for (const CodeRange& range : g_own) {
        const std::uintptr_t low = range.low.load(std::memory_order_acquire);
        if (low != 0 && at >= low && at < range.high.load(std::memory_order_acquire)) {
            return true;
        }
    }
    return false;
}

// Names the exception and writes the report; the program ends here.
[[noreturn]] void report_exception(const EXCEPTION_RECORD& record) {
    const DWORD code = record.ExceptionCode;

    if ((code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR)
        && record.NumberParameters >= 2) {
        const auto kind = record.ExceptionInformation[0];
        const auto* host = reinterpret_cast<const void*>(record.ExceptionInformation[1]);
        report(
            host,
            kind == 1   ? "write"
            : kind == 8 ? "execution"
                        : "read",
            crash::Kind::Exception
        );
    }

    const char* what = nullptr;

    switch (code) {
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            what = "illegal instruction";
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            what = "privileged instruction";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            what = "integer division by zero";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            what = "stack overflow";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            what = "misaligned access";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            what = "array bounds exceeded";
            break;
        default: {
            crash::Text text;
            text.str("an unhandled exception, code ").hex(code, 8);
            report(record.ExceptionAddress, text.c_str(), crash::Kind::Exception);
        }
    }

    report(record.ExceptionAddress, what, crash::Kind::Exception);
}

bool is_fault(DWORD code) {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return true;
        default:
            return false;
    }
}

// A first look at every exception, before any handler. Only a hardware fault in the program's own
// code is reported here: the program never catches one. A fault in another library (an overlay
// such as RivaTuner's, which Discord's screen share wakes, or a driver) is often a probe that
// library catches itself, so it goes on; if nothing catches it, on_unhandled reports it.
LONG WINAPI on_exception(EXCEPTION_POINTERS* info) {
    const EXCEPTION_RECORD& record = *info->ExceptionRecord;

    if (!is_fault(record.ExceptionCode) || !in_own_code(record.ExceptionAddress)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    report_exception(record);
}

// Any exception nothing handled.
LONG WINAPI on_unhandled(EXCEPTION_POINTERS* info) {
    report_exception(*info->ExceptionRecord);
}

void on_abort(int) {
    report(nullptr, "the program aborted (abort)", crash::Kind::Signal);
}

#else

// OPENRAC_GUARD (guard_writes): the guarded range and the pages it lies in, read-only while armed.
gaddr g_guard_low = 0, g_guard_high = 0;
std::uint8_t* g_guard_page = nullptr;
std::size_t g_guard_span = 0;
std::uint32_t g_guard_skip = 0;

// After a write the guard let through: the page is read-only again once that one instruction ran
// (x86-64 Linux: the trap flag single-steps it).
void on_step(int, siginfo_t*, void* context) {
#if defined(__linux__) && defined(__x86_64__)
    auto* uc = static_cast<ucontext_t*>(context);
    uc->uc_mcontext.gregs[REG_EFL] &= ~0x100LL;

    if (g_guard_page != nullptr) {
        mprotect(g_guard_page, g_guard_span, PROT_READ);
    }
#else
    (void)context;
#endif
}

// OPENRAC_SOFT_FAULTS: the page and function pairs seen, and the pages opened this tick. Fixed
// tables: the signal handler cannot allocate.
constexpr unsigned kSoftFaults = 512;
SoftFault g_soft_faults[kSoftFaults];
unsigned g_soft_fault_count = 0;
std::uint8_t* g_soft_open[kSoftFaults];
unsigned g_soft_open_count = 0;
bool g_soft_faults_on = false;

// Whether the access that faulted was a write, where the system says.
bool fault_was_write(void* context) {
#if defined(__APPLE__) && defined(__aarch64__)
    return (static_cast<ucontext_t*>(context)->uc_mcontext->__es.__esr & (1u << 6)) != 0;  // WnR
#elif defined(__APPLE__) && defined(__x86_64__)
    return (static_cast<ucontext_t*>(context)->uc_mcontext->__es.__err & 2u) != 0;
#elif defined(__linux__) && defined(__x86_64__)
    return (static_cast<ucontext_t*>(context)->uc_mcontext.gregs[REG_ERR] & 2) != 0;
#else
    (void)context;
    return false;
#endif
}

// The innermost translated function on the stack: the game's are named func_ or FUN_.
const char* faulting_function() {
    void* frames[12];
    const int count = backtrace(frames, 12);

    for (int i = 0; i < count; ++i) {
        Dl_info info{};

        if (dladdr(frames[i], &info) != 0 && info.dli_sname != nullptr) {
            const char* name = info.dli_sname;
            name += name[0] == '_' ? 1 : 0;  // macOS's names

            if (std::strncmp(name, "func_", 5) == 0 || std::strncmp(name, "FUN_", 4) == 0) {
                return name;
            }
        }
    }

    return nullptr;
}

// Opens the page of a game address that is not mapped and counts the access. False when the fault
// is not one of those, or the tables are full.
bool soft_fault(std::uint8_t* at, void* context) {
    gaddr address = 0;

    if (!g_soft_faults_on || !Memory::get().contains(at, &address)
        || g_soft_open_count >= kSoftFaults) {
        return false;
    }

    const auto size = static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    auto* page =
        reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(at) & ~(size - 1));

    if (mprotect(page, size, PROT_READ | PROT_WRITE) != 0) {
        return false;
    }

    g_soft_open[g_soft_open_count++] = page;
    const gaddr game_page = address & ~static_cast<gaddr>(size - 1);
    const char* function = faulting_function();

    for (unsigned i = 0; i < g_soft_fault_count; ++i) {
        SoftFault& f = g_soft_faults[i];

        if (f.page == game_page && f.function == function) {
            ++f.hits;
            f.write = f.write || fault_was_write(context);
            return true;
        }
    }

    if (g_soft_fault_count < kSoftFaults) {
        g_soft_faults[g_soft_fault_count++] = {game_page, fault_was_write(context), function, 1};
    }

    return true;
}

void on_signal(int sig, siginfo_t* info, void* context) {
    auto* at = static_cast<std::uint8_t*>(info->si_addr);

    if (soft_fault(at, context)) {
        return;
    }

    if (g_guard_page != nullptr && at >= g_guard_page && at < g_guard_page + g_guard_span) {
        gaddr address = 0;

        if (Memory::get().contains(at, &address) && address >= g_guard_low && address < g_guard_high
            && g_guard_skip-- == 0) {
            crash::Text reason;
            reason.str("OPENRAC_GUARD: a write into the guarded range, at game address ")
                .hex(address, 8);
            crash::report(reason.c_str(), crash::Kind::Signal);
            _exit(3);
        }

        // Another write to the page: let this one through, then guard again (on_step).
        mprotect(g_guard_page, g_guard_span, PROT_READ | PROT_WRITE);
#if defined(__linux__) && defined(__x86_64__)
        static_cast<ucontext_t*>(context)->uc_mcontext.gregs[REG_EFL] |= 0x100LL;
#else
        (void)context;
#endif
        return;
    }

    // The host functions on the way there are in the report's backtrace (the translated game code
    // is named after the game's).
    report(
        info->si_addr, sig == SIGBUS ? "access (SIGBUS)" : "access (SIGSEGV)", crash::Kind::Signal
    );
}

// The other faults: no game address to give.
void on_fault(int sig, siginfo_t* info, void*) {
    const char* what = "a fatal signal";

    switch (sig) {
        case SIGILL:
            what = "illegal instruction (SIGILL)";
            break;
        case SIGFPE:
            what = "arithmetic fault (SIGFPE)";
            break;
        case SIGABRT:
            what = "the program aborted (SIGABRT)";
            break;
        case SIGTRAP:
            // A trap instruction: __builtin_trap, a failed check the compiler turned into one, or a
            // breakpoint left in (brk on arm64, ud2 and int3 on x86-64 raise SIGILL and SIGTRAP).
            what = "a trap instruction (SIGTRAP)";
            break;
        default:
            break;
    }

    report(sig == SIGABRT ? nullptr : info->si_addr, what, crash::Kind::Signal);
}

#endif

}  // namespace

void rearm_soft_faults() {
#if !defined(_WIN32)
    const auto size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));

    while (g_soft_open_count > 0) {
        mprotect(g_soft_open[--g_soft_open_count], size, PROT_NONE);
    }
#endif
}

const SoftFault* soft_fault_at(unsigned index) {
#if !defined(_WIN32)
    return index < g_soft_fault_count ? &g_soft_faults[index] : nullptr;
#else
    (void)index;
    return nullptr;
#endif
}

void guard_writes(gaddr address, std::uint32_t bytes, std::uint32_t skip) {
#if !defined(_WIN32)
    const auto page = static_cast<std::uintptr_t>(sysconf(_SC_PAGESIZE));
    const auto first = reinterpret_cast<std::uintptr_t>(openrac_guest_base + address) & ~(page - 1);
    const auto last =
        reinterpret_cast<std::uintptr_t>(openrac_guest_base + address + bytes - 1) & ~(page - 1);

    if (g_guard_high == 0) {
        g_guard_skip = skip;
    }

    g_guard_low = address;
    g_guard_high = address + bytes;
    g_guard_page = reinterpret_cast<std::uint8_t*>(first);
    g_guard_span = last - first + page;
    static bool stepping = false;

    if (!stepping) {
        stepping = true;
        struct sigaction step{};
        step.sa_sigaction = on_step;
        step.sa_flags = SA_SIGINFO;
        sigemptyset(&step.sa_mask);
        sigaction(SIGTRAP, &step, nullptr);
    }

    mprotect(g_guard_page, g_guard_span, PROT_READ);
#else
    (void)address;
    (void)bytes;
    (void)skip;
#endif
}

void add_own_library(const void* module) {
#if defined(_WIN32)
    if (module == nullptr) {
        return;
    }
    std::uintptr_t low = 0, high = 0;
    image_range(module, low, high);
    for (CodeRange& range : g_own) {
        std::uintptr_t free = 0;
        if (range.low.load(std::memory_order_relaxed) == low) {
            return;  // already in
        }
        if (range.low.compare_exchange_strong(free, low, std::memory_order_acq_rel)) {
            range.high.store(high, std::memory_order_release);
            return;
        }
    }
    // All taken: its faults are still reported by on_unhandled.
#else
    (void)module;
#endif
}

void remove_own_library(const void* module) {
#if defined(_WIN32)
    const auto low = reinterpret_cast<std::uintptr_t>(module);
    for (CodeRange& range : g_own) {
        if (low != 0 && range.low.load(std::memory_order_relaxed) == low) {
            range.high.store(0, std::memory_order_release);
            range.low.store(0, std::memory_order_release);
        }
    }
#else
    (void)module;
#endif
}

void install_crash_handler() {
    std::set_terminate(on_terminate);
#if defined(_WIN32)
    add_own_library(GetModuleHandleW(nullptr));
    AddVectoredExceptionHandler(1, on_exception);
    SetUnhandledExceptionFilter(on_unhandled);
    std::signal(SIGABRT, on_abort);
#else
    crash::prepare_thread();
    g_soft_faults_on = env::flag(env::Var::SoftFaults);
    struct sigaction action{};

    action.sa_sigaction = on_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    action.sa_sigaction = on_fault;
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGTRAP, &action, nullptr);  // OPENRAC_GUARD replaces it with its single step
#endif
}

}  // namespace openrac::runtime

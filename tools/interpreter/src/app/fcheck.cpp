// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * openrac-fcheck: does a function written in C do what the retail function does?
 *
 *   openrac-fcheck --code FILE@ADDRESS [--code FILE@ADDRESS ...] --entry ADDRESS
 *                  [--returns void|int|long|float] STATE...
 *
 * Each STATE is the machine at the start of a real call of the retail function, written by
 * `openrac-boot --capture`. From it the retail function runs in the interpreter, then, from the
 * same state again, the candidate: the code files (the candidate compiled for the console and
 * placed by tools/fcheck.py in the decompilation) are put into memory and the call starts at
 * `--entry`. What the two leave is compared: main memory and the scratchpad (but for the dead
 * stack below the caller's stack pointer and the candidate's own code), the result, the registers
 * a function must keep, and the order of what they did outside memory (devices, system calls).
 *
 * Exit status 0 when every state compared gives the same, 1 when one differs, 2 on a usage error,
 * 4 when no state could be compared (the retail function did not return from any).
 * It is a tool for writing functions, not part of the game.
 */

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "sys/machine.h"
#include "sys/snapshot.h"

using namespace ps2;

namespace {

/** Where the games' programs start in memory; below are the kernel's area and the candidate. */
constexpr u32 kProgramStart = 0x00100000;

/** The most instructions a call may take before it counts as not returning. */
u64 kLimit = 200'000'000;

/** A piece of candidate code and where it goes. */
struct Code {
    u32 address;
    std::vector<u8> bytes;
};

/** One thing a call did outside memory, in order. */
struct Outside {
    char what;  // 'r' read, 'w' write, 'q' quadword write, 's' system call
    u32 address;  // for 's' the SYSCALL's code
    u64 value, value2;  // for 's' the inputs taken, and the kernel call's number (v1) or 0
    std::array<u32, 8> in{};  // for 's' the argument registers taken, a0-a3 then t0-t3

    bool operator==(const Outside& o) const {
        return what == o.what && address == o.address && value == o.value && value2 == o.value2 && in == o.in;
    }
};

/**
 * What a system call takes from the caller's registers, which is what is compared of it: values
 * the caller merely left in other registers are not its inputs.
 *
 * A kernel call (a SYSCALL of the program's own stubs) takes its number in v1, which the stub
 * sets, and its arguments in a0 onward: the count is the call's, from the kernel's public call
 * table, for the calls the machine answers; a0 alone for any other. A hooked library function is
 * an ordinary call of a C function: its arguments are a0-a3 and t0-t3 (EE GCC's ABI; v0 and v1
 * are results, never arguments), as many as its hooks table line says (`inputs=N`), else a0.
 * No hooked service takes a floating-point argument.
 */
constexpr u32 kHookMark = 0x80000;  // the SYSCALL code's mark of a hooked function (machine.cpp)

int kernel_inputs(int number) {
    switch (number < 0 ? -number : number) {
        case 0x23: case 0x24: case 0x2F: case 0x32: case 0x3E: case 0x70: case 0x7F:
            return 0;  // ExitThread, ExitDeleteThread, GetThreadId, SleepThread, EndOfHeap, GsGetIMR, GetMemorySize
        case 0x11: case 0x13: case 0x22: case 0x29: case 0x2A: case 0x3D: case 0x73: case 0x77: case 0x79:
            return 2;  // Remove{Intc,Dmac}Handler, StartThread, (i)ChangeThreadPriority, SetupHeap,
                       // SetVSyncFlag, SifSetDma, SifSetReg
        case 0x02:
            return 3;  // SetGsCrt
        case 0x10: case 0x12:
            return 4;  // Add{Intc,Dmac}Handler
        case 0x3C:
            return 5;  // SetupThread
        default:
            return 1;
    }
}

/** What a call left. */
struct Result {
    bool returned = false;
    u64 instructions = 0, misaligned = 0;
    u32 lowest_jump = ~0u;
    std::array<Ee::Reg, 32> gpr{};
    std::array<u32, 32> fpr{};
    std::vector<u8> ram, scratchpad;
    std::vector<Outside> outside;
};

const char* const kGpr[32] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2",
                              "t3",   "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5",
                              "s6",   "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};

}  // namespace

/** Two words that hold floats of the same sign and differ by at most `ulps` in the last place. */
bool near_float(u32 a, u32 b, int ulps) {
    if (ulps <= 0 || ((a ^ b) & 0x80000000u)) {
        return false;
    }
    u32 ea = (a >> 23) & 0xFF, eb = (b >> 23) & 0xFF;
    if (ea == 0 || eb == 0) {
        return false;
    }
    long long d = static_cast<long long>(a & 0x7FFFFFFF) - static_cast<long long>(b & 0x7FFFFFFF);
    return d <= ulps && d >= -ulps;
}

int main(int argc, char** argv) {
    std::vector<Code> code;
    std::vector<std::string> states;
    u32 entry = 0;
    std::string returns = "void";
    bool have_entry = false;
    int ulps = 0;
    std::string hooks, disc, coverage;
    u32 cover_size = 0;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--code" && i + 1 < argc) {
            std::string spec = argv[++i];
            std::size_t at = spec.rfind('@');

            if (at == std::string::npos) {
                std::fprintf(stderr, "--code wants FILE@ADDRESS\n");
                return 2;
            }

            std::ifstream f(spec.substr(0, at), std::ios::binary);
            Code c{static_cast<u32>(std::stoul(spec.substr(at + 1), nullptr, 16)),
                   std::vector<u8>(std::istreambuf_iterator<char>(f), {})};

            if (!f.good() && !f.eof()) {
                std::fprintf(stderr, "cannot read %s\n", spec.c_str());
                return 2;
            }

            code.push_back(std::move(c));
        } else if (arg == "--entry" && i + 1 < argc) {
            entry = static_cast<u32>(std::stoul(argv[++i], nullptr, 16));
            have_entry = true;
        } else if (arg == "--ulps" && i + 1 < argc) {
            // Floats that differ by this many units in the last place count as the same (VU0 and
            // the FPU round differently; a hand-written VU0 routine in C cannot always agree).
            ulps = std::stoi(argv[++i]);
        } else if (arg == "--limit" && i + 1 < argc) {
            // Instructions retail may take before a state counts as skipped (it waits for an
            // interrupt, a frame or the disc, which no run here delivers).
            kLimit = std::stoull(argv[++i]);
        } else if (arg == "--hooks" && i + 1 < argc) {
            // The hooks table of the capture run: a hooked function's first word in the states is
            // the SYSCALL that calls its service, which must answer here too (else the rest of the
            // function runs without its first instruction).
            hooks = argv[++i];
        } else if (arg == "--disc" && i + 1 < argc) {
            // The disc image, for the services that read it.
            disc = argv[++i];
        } else if (arg == "--returns" && i + 1 < argc) {
            returns = argv[++i];
        } else if (arg == "--coverage" && i + 1 < argc) {
            // Retail only, no candidate: FILE gets, per entry address, which of its instructions
            // the states run ("ADDRESS WORDS STATES RETURNED HEX", one byte a word).
            coverage = argv[++i];
        } else if (arg == "--cover-size" && i + 1 < argc) {
            cover_size = static_cast<u32>(std::stoul(argv[++i], nullptr, 0));
        } else if (!arg.empty() && arg[0] != '-') {
            states.push_back(arg);
        } else {
            std::fprintf(stderr,
                         "usage: openrac-fcheck --code FILE@ADDRESS [...] --entry ADDRESS "
                         "[--returns void|int|long|float] [--ulps N] [--hooks FILE] [--disc ISO] [--limit N] STATE...\n");
            return 2;
        }
    }

    if (!have_entry || (code.empty() && coverage.empty()) || states.empty()) {
        std::fprintf(stderr, "openrac-fcheck needs --code, --entry and at least one state\n");
        return 2;
    }

    std::vector<Outside>* log = nullptr;
    std::unique_ptr<sys::Machine> machine;

    /*
     * A machine of its own for each run. A call state holds the processors and the memories, not
     * what the kernel and the services remember (semaphores and the next id, handlers, the DMA
     * registers, the sound server's handles): each run starts from a fresh machine, so retail and
     * candidate meet the same. With the capture run's hooks a hooked function's service answers
     * (its first word in the state is the SYSCALL that calls it).
     */
    auto fresh = [&](std::string* error) -> bool {
        machine = std::make_unique<sys::Machine>();
        if (!disc.empty() && !machine->disc.open(disc)) {
            *error = "cannot open the disc image " + disc;
            return false;
        }
        if (!hooks.empty() && !machine->load_hooks(hooks, error)) {
            return false;
        }
        Ee& ee = machine->ee;

        /*
         * No timed events: the machine is not booted, so it has no frame timing, and a call
         * is compared on its own, without interrupts, frames or disc callbacks arriving in it.
         */
        ee.on_event = nullptr;

        // Everything that leaves memory is written down, then done as the machine does it.
        auto syscall = ee.on_syscall;
        auto read = ee.on_read;
        auto write = ee.on_write;
        auto write128 = ee.on_write128;

        sys::Machine* m = machine.get();
        ee.on_syscall = [&ee, &log, syscall, m](u32 c) {
            if (log) {
                Outside o{'s', c, 0, 0};
                int inputs = 1;
                if (!m->hooked_call(c, &inputs)) {
                    o.value2 = ee.gpr[3].lo;
                    inputs = kernel_inputs(static_cast<s32>(ee.gpr[3].lo));
                } else if (inputs < 0) {
                    inputs = 1;
                }
                o.value = static_cast<u64>(inputs);
                for (int n = 0; n < inputs; n++) {
                    o.in[n] = static_cast<u32>(ee.gpr[4 + n].lo);
                }
                log->push_back(o);
            }
            if (syscall) {
                syscall(c);
            }
        };
        ee.on_read = [&log, read](u32 a, unsigned n) -> u64 {
            if (log) {
                log->push_back({'r', a, n, 0});
            }
            return read ? read(a, n) : 0;
        };
        ee.on_write = [&log, write](u32 a, u64 v, unsigned n) {
            if (log) {
                log->push_back({'w', a, v, n});
            }
            if (write) {
                write(a, v, n);
            }
        };
        ee.on_write128 = [&log, write128](u32 a, u64 lo, u64 hi) {
            if (log) {
                log->push_back({'q', a, lo, hi});
            }
            if (write128) {
                write128(a, lo, hi);
            }
        };
        return true;
    };

    std::map<u32, std::vector<u8>> covered;
    std::map<u32, std::pair<int, int>> cover_runs;

    // Runs one call from a state, the retail function or the candidate.
    auto run = [&](const sys::CallState& state, u32 start, bool candidate, u64 limit) {
        Result r;
        std::string error;

        if (!fresh(&error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            std::exit(2);
        }
        Ee& ee = machine->ee;
        state.put(ee, machine->vu0, machine->memory, machine->vif0.micro.data(), machine->vif0.data.data());

        if (candidate) {
            for (const Code& c : code) {
                std::memcpy(machine->memory.ram(c.address), c.bytes.data(), c.bytes.size());
            }
        }

        ee.cover = nullptr;
        if (!candidate && !coverage.empty()) {
            std::vector<u8>& map = covered[start];
            map.resize(cover_size / 4);
            ee.cover = map.data();
            ee.cover_base = start;
            ee.cover_bytes = cover_size;
        }

        ee.event_at = ~u64{0};
        ee.pc = start;
        ee.next_pc = start + 4;
        log = &r.outside;

        ee.lowest_jump = ~0u;
        Ee::Call call = ee.begin_call(start);
        while (!r.returned && r.instructions < limit) {
            r.returned = ee.run_call(1'000'000);
            r.instructions += 1'000'000;
        }
        ee.end_call(call);
        ee.cover = nullptr;

        log = nullptr;
        r.gpr = ee.gpr;
        r.fpr = ee.fpr;
        r.misaligned = ee.misaligned;
        r.lowest_jump = ee.lowest_jump;
        r.ram.assign(machine->memory.ram(0), machine->memory.ram(0) + GuestMemory::kRamBytes);
        r.scratchpad.assign(machine->memory.scratchpad(0),
                            machine->memory.scratchpad(0) + GuestMemory::kScratchpadBytes);
        return r;
    };

    int differing = 0, compared = 0, skipped_in_a_row = 0;

    for (const std::string& path : states) {
        sys::CallState state;
        std::string error;

        if (!state.load(path, &error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 2;
        }

        /*
         * The candidate gets eight times what retail took and at least 20 million instructions:
         * one that loops for ever is told apart in seconds, not after the whole limit.
         */
        // Two states in a row where retail did not return: it waits for what no run delivers.
        if (skipped_in_a_row >= 2) {
            std::printf("%s: SKIPPED (not run: the retail function did not return from the two before)\n", path.c_str());
            continue;
        }
        Result retail = run(state, state.address, false, kLimit);
        if (!coverage.empty()) {
            cover_runs[state.address].first++;
            cover_runs[state.address].second += retail.returned ? 1 : 0;
            skipped_in_a_row = retail.returned ? 0 : skipped_in_a_row + 1;
            continue;
        }
        Result ours = run(state, entry, true, std::min(kLimit, std::max<u64>(20'000'000, retail.instructions * 8)));
        std::vector<std::string> notes;
        char text[256];

        if (!retail.returned) {
            std::printf("%s: SKIPPED (the retail function did not return within %llu instructions)\n",
                        path.c_str(), static_cast<unsigned long long>(kLimit));
            skipped_in_a_row++;
            continue;
        }
        /*
         * Retail jumped below the program, into the kernel's area at the bottom of memory where
         * no code of the game is (the candidate's place): it went astray, as when a call needs
         * what a state does not hold (the second processor's memory) and takes a path that
         * returns to nowhere. Such a run is no reference, though it may come back by chance.
         */
        if (retail.lowest_jump < kProgramStart) {
            std::printf("%s: SKIPPED (the retail function went astray: it jumped to %08x, below the program)\n",
                        path.c_str(), retail.lowest_jump);
            continue;
        }
        skipped_in_a_row = 0;
        compared++;

        if (!ours.returned) {
            notes.push_back("the candidate did not return (an endless loop, or it jumped away)");
        }

        // The result.
        if ((returns == "int" && static_cast<u32>(ours.gpr[2].lo) != static_cast<u32>(retail.gpr[2].lo))
            || (returns == "long" && ours.gpr[2].lo != retail.gpr[2].lo)) {
            std::snprintf(text, sizeof(text), "returns %llx, retail %llx",
                          static_cast<unsigned long long>(ours.gpr[2].lo),
                          static_cast<unsigned long long>(retail.gpr[2].lo));
            notes.push_back(text);
        }
        if (returns == "float" && ours.fpr[0] != retail.fpr[0] && !near_float(ours.fpr[0], retail.fpr[0], ulps)) {
            std::snprintf(text, sizeof(text), "returns float bits %08x, retail %08x", ours.fpr[0],
                          retail.fpr[0]);
            notes.push_back(text);
        }

        // Accesses the console refuses (an address error): more than retail made is a difference.
        if (ours.misaligned > retail.misaligned) {
            std::snprintf(text, sizeof(text), "%llu misaligned loads or stores (the console raises an address error), retail %llu",
                          static_cast<unsigned long long>(ours.misaligned),
                          static_cast<unsigned long long>(retail.misaligned));
            notes.push_back(text);
        }

        // The registers a function keeps for its caller.
        for (int g : {16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30}) {
            if (ours.gpr[g].lo != state.gpr[g].lo) {
                std::snprintf(text, sizeof(text), "does not keep $%s (%llx, was %llx)", kGpr[g],
                              static_cast<unsigned long long>(ours.gpr[g].lo),
                              static_cast<unsigned long long>(state.gpr[g].lo));
                notes.push_back(text);
            }
        }
        for (int f = 20; f < 32; f++) {
            if (ours.fpr[f] != state.fpr[f]) {
                std::snprintf(text, sizeof(text), "does not keep $f%d", f);
                notes.push_back(text);
            }
        }

        // Memory, but for the dead stack and the candidate's own code.
        u32 sp = static_cast<u32>(state.gpr[29].lo) & (GuestMemory::kRamBytes - 1);
        auto skipped = [&](u32 at) {
            if (at < sp && sp - at <= 0x100000) {
                return true;
            }
            for (const Code& c : code) {
                u32 base = c.address & (GuestMemory::kRamBytes - 1);
                if (at + 4 > base && at < base + c.bytes.size()) {
                    return true;
                }
            }
            return false;
        };
        int shown = 0, words = 0;
        for (u32 at = 0; at < GuestMemory::kRamBytes; at += 4) {
            if (std::memcmp(ours.ram.data() + at, retail.ram.data() + at, 4) == 0 || skipped(at)) {
                continue;
            }
            u32 wa, wb;
            std::memcpy(&wa, ours.ram.data() + at, 4);
            std::memcpy(&wb, retail.ram.data() + at, 4);
            /*
             * An address in the candidate's own code (a label's address, a return address kept
             * outside the stack) is the same place as retail's when it is as far from the entry
             * as retail's is from the function's start.
             */
            bool own_code = false;
            for (const Code& c : code) {
                if (wa >= c.address && wa < c.address + c.bytes.size()) {
                    own_code = wa - entry == wb - state.address;
                }
            }
            if (own_code || near_float(wa, wb, ulps)) {
                continue;
            }
            words++;
            if (shown < 16) {
                u32 a, b, was;
                std::memcpy(&a, ours.ram.data() + at, 4);
                std::memcpy(&b, retail.ram.data() + at, 4);
                std::memcpy(&was, state.ram.data() + at, 4);
                std::snprintf(text, sizeof(text), "word at %08x: ours %08x, retail %08x (before %08x)",
                              at, a, b, was);
                notes.push_back(text);
                shown++;
            }
        }
        if (words > shown) {
            std::snprintf(text, sizeof(text), "... %d words of memory differ in all", words);
            notes.push_back(text);
        }
        /*
         * The scratchpad, as memory: a word that holds an address in the candidate's own code
         * as far from the entry as retail's is from the function's start is the same (a return
         * address a callee saves on a stack it keeps in the scratchpad), and so is a float
         * within --ulps.
         */
        if (ours.scratchpad != retail.scratchpad) {
            int spr_words = 0;
            for (u32 at = 0; at < GuestMemory::kScratchpadBytes; at += 4) {
                if (std::memcmp(ours.scratchpad.data() + at, retail.scratchpad.data() + at, 4) == 0) {
                    continue;
                }
                u32 wa, wb;
                std::memcpy(&wa, ours.scratchpad.data() + at, 4);
                std::memcpy(&wb, retail.scratchpad.data() + at, 4);
                bool own_code = false;
                for (const Code& c : code) {
                    if (wa >= c.address && wa < c.address + c.bytes.size()) {
                        own_code = wa - entry == wb - state.address;
                    }
                }
                if (own_code || near_float(wa, wb, ulps)) {
                    continue;
                }
                if (spr_words++ < 4) {
                    std::snprintf(text, sizeof(text), "scratchpad word at %04x: ours %08x, retail %08x", at, wa, wb);
                    notes.push_back(text);
                }
            }
            if (spr_words > 4) {
                std::snprintf(text, sizeof(text), "... %d words of the scratchpad differ in all", spr_words);
                notes.push_back(text);
            }
        }

        /*
         * What they did outside memory, in order. An address in the dead stack (a packet built in
         * a local and handed to the DMA controller) is the compiler's choice of frame: it compares
         * as "a stack address", whatever its offset.
         */
        auto stack_value = [&](u64 v) -> u64 {
            u64 seg = v & ~u64{0x01FFFFFF};
            u32 a = static_cast<u32>(v & 0x01FFFFFF);
            bool mirror = seg == 0 || seg == 0x20000000 || seg == 0x30000000 || seg == 0x80000000;
            return mirror && a < sp && sp - a <= 0x100000 ? 0x57AC0000'00000000ull : v;
        };
        for (std::vector<Outside>* v : {&ours.outside, &retail.outside}) {
            for (Outside& o : *v) {
                if (o.what == 'w' || o.what == 'q') {
                    o.value = stack_value(o.value);
                }
                if (o.what == 'q') {
                    o.value2 = stack_value(o.value2);
                }
                if (o.what == 's') {
                    for (u32& a : o.in) {
                        a = stack_value(a) != a ? 0x57AC57ACu : a;  // "a stack address"
                    }
                }
            }
        }
        if (ours.outside != retail.outside) {
            std::size_t k = 0;
            while (k < ours.outside.size() && k < retail.outside.size() && ours.outside[k] == retail.outside[k]) {
                k++;
            }
            auto describe = [](const std::vector<Outside>& v, std::size_t k) {
                char t[96];
                if (k >= v.size()) {
                    return std::string("nothing more");
                }
                const Outside& o = v[k];
                if (o.what == 's') {
                    std::string s;
                    std::snprintf(t, sizeof(t), "s %08x", o.address);
                    s = t;
                    if (!(o.address & kHookMark)) {
                        std::snprintf(t, sizeof(t), " v1=%llx", static_cast<unsigned long long>(o.value2));
                        s += t;
                    }
                    for (u64 n = 0; n < o.value && n < 8; n++) {
                        std::snprintf(t, sizeof(t), " %s=%x", kGpr[4 + n], o.in[n]);
                        s += t;
                    }
                    return s;
                }
                std::snprintf(t, sizeof(t), "%c %08x %llx %llx", o.what, o.address,
                              static_cast<unsigned long long>(o.value),
                              static_cast<unsigned long long>(o.value2));
                return std::string(t);
            };
            std::snprintf(text, sizeof(text),
                          "outside memory, step %zu: ours [%s], retail [%s] (r read, w write, s system call)",
                          k, describe(ours.outside, k).c_str(), describe(retail.outside, k).c_str());
            notes.push_back(text);
        }

        if (notes.empty()) {
            std::printf("%s: SAME (retail %llu, ours %llu million instructions or less)\n",
                        path.c_str(), static_cast<unsigned long long>(retail.instructions / 1'000'000),
                        static_cast<unsigned long long>(ours.instructions / 1'000'000));
        } else {
            differing++;
            std::printf("%s: DIFFERS\n", path.c_str());
            for (const std::string& n : notes) {
                std::printf("    %s\n", n.c_str());
            }
        }
    }

    if (!coverage.empty()) {
        FILE* out = std::fopen(coverage.c_str(), "w");
        if (!out) {
            std::fprintf(stderr, "cannot write %s\n", coverage.c_str());
            return 2;
        }
        for (const auto& [address, map] : covered) {
            std::fprintf(out, "%08x %zu %d %d ", address, map.size(), cover_runs[address].first,
                         cover_runs[address].second);
            for (u8 hit : map) {
                std::fputc(hit ? '1' : '0', out);
            }
            std::fputc('\n', out);
        }
        std::fclose(out);
        return 0;
    }

    // How many states were compared: none (every one skipped) is not a verdict (exit 4).
    std::printf("compared %d of %zu states\n", compared, states.size());
    return differing ? 1 : compared ? 0 : 4;
}

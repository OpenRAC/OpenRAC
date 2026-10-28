// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * Tests for openrac-fcheck's comparison of system calls: of a call, only the registers the called
 * service takes are compared (a kernel call's number in v1 and its arguments; a hooked library
 * function's argument registers as its hooks table line says), not what the caller left in the
 * others.
 *
 *   openrac-fcheck-tests PATH-OF-openrac-fcheck SCRATCH-DIRECTORY
 *
 * Each case writes a call state of a small hand-assembled "retail" function and a candidate, runs
 * openrac-fcheck on them and checks its exit status.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "sys/snapshot.h"

using namespace ps2;

namespace {

int failures = 0;

// Registers.
enum : u32 { zero = 0, v1 = 3, a0 = 4, a1 = 5, sp = 29, ra = 31 };

u32 addiu(u32 rt, u32 rs, s32 imm) { return (9u << 26) | (rs << 21) | (rt << 16) | (static_cast<u32>(imm) & 0xFFFF); }
u32 sw(u32 rt, s32 off, u32 rs) { return (0x2Bu << 26) | (rs << 21) | (rt << 16) | (static_cast<u32>(off) & 0xFFFF); }
u32 lw(u32 rt, s32 off, u32 rs) { return (0x23u << 26) | (rs << 21) | (rt << 16) | (static_cast<u32>(off) & 0xFFFF); }
u32 jal(u32 target) { return (3u << 26) | ((target >> 2) & 0x3FFFFFF); }
constexpr u32 kJrRa = (31u << 21) | 8, kNop = 0, kSyscall = 0x0C;

constexpr u32 kRetail = 0x00100000, kHooked = 0x00100100, kCandidate = 0x000A0000;
// The first line of the test's hooks table is service 0: its SYSCALL, as a capture state holds it.
constexpr u32 kHookSyscall = ((0x80000u | 0) << 6) | 0x0C;

/** A function that sets the given registers, calls the hooked function (or makes a kernel call), returns. */
std::vector<u32> function(const std::vector<u32>& sets, bool kernel) {
    std::vector<u32> w = {addiu(sp, sp, -16), sw(ra, 0, sp)};
    w.insert(w.end(), sets.begin(), sets.end());
    if (kernel) {
        w.push_back(kSyscall);
    } else {
        w.push_back(jal(kHooked));
        w.push_back(kNop);
    }
    w.insert(w.end(), {lw(ra, 0, sp), addiu(sp, sp, 16), kJrRa, kNop});
    return w;
}

void put(std::vector<u8>& ram, u32 at, const std::vector<u32>& words) {
    std::memcpy(ram.data() + at, words.data(), words.size() * 4);
}

/** Runs openrac-fcheck on RETAIL and CANDIDATE with a hooks table of LINE; true when its exit status is WANT. */
void check(const char* what, const std::string& fcheck, const std::string& dir, const std::string& line,
           const std::vector<u32>& retail, const std::vector<u32>& candidate, int want) {
    sys::CallState s;
    s.address = kRetail;
    s.ram.assign(GuestMemory::kRamBytes, 0);
    s.scratchpad.assign(GuestMemory::kScratchpadBytes, 0);
    s.vu0_micro.assign(4096, 0);
    s.vu0_data.assign(4096, 0);
    s.gpr[sp].lo = 0x01F00000;
    s.gpr[ra].lo = 0x00100F00;
    s.gpr[v1].lo = static_cast<u64>(-1);  // what the caller left
    s.gpr[a1].lo = 0x55;
    put(s.ram, kRetail, retail);
    put(s.ram, kHooked, {kHookSyscall, kNop});
    std::string state = dir + "/f.0.snap", code = dir + "/code.bin", hooks = dir + "/test.hooks";
    if (!s.save(state)) {
        std::printf("FAIL %s: cannot write %s\n", what, state.c_str());
        failures++;
        return;
    }
    std::ofstream(code, std::ios::binary).write(reinterpret_cast<const char*>(candidate.data()),
                                                 static_cast<std::streamsize>(candidate.size() * 4));
    std::ofstream(hooks) << "00100100 " << line << "\n";
    char entry[16];
    std::snprintf(entry, sizeof(entry), "%x", kCandidate);
    std::string cmd = "'" + fcheck + "' --hooks '" + hooks + "' --entry " + entry + " --code '" + code + "@" + entry +
                      "' '" + state + "' > '" + dir + "/out.txt' 2>&1";
    int r = std::system(cmd.c_str());
    int got = WIFEXITED(r) ? WEXITSTATUS(r) : -1;
    if (got != want) {
        std::printf("FAIL %s: exit %d, expected %d (see %s/out.txt)\n", what, got, want, dir.c_str());
        failures++;
    } else {
        std::printf("ok   %s\n", what);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: openrac-fcheck-tests PATH-OF-openrac-fcheck SCRATCH-DIRECTORY\n");
        return 2;
    }
    std::string fcheck = argv[1], dir = argv[2];
    std::system(("mkdir -p '" + dir + "'").c_str());

    // sceCdSync(mode): retail leaves v1 as the caller had it, the candidate's code uses v1 and a1.
    auto retail = function({addiu(a0, zero, 1)}, false);
    check("a hooked call: v1 and unused argument registers are not inputs", fcheck, dir, "sceCdSync inputs=1",
          retail, function({addiu(v1, zero, 0x1234), addiu(a1, zero, 7), addiu(a0, zero, 1)}, false), 0);
    check("a hooked call: a different a0 differs", fcheck, dir, "sceCdSync inputs=1", retail,
          function({addiu(a0, zero, 0)}, false), 1);
    check("a hooked call without inputs=: a0 is compared", fcheck, dir, "sceCdSync", retail,
          function({addiu(a0, zero, 0)}, false), 1);
    check("a hooked call taking two: a different a1 differs", fcheck, dir, "sceCdSync inputs=2", retail,
          function({addiu(a1, zero, 7), addiu(a0, zero, 1)}, false), 1);
    check("a hooked call taking none: a0 is not compared", fcheck, dir, "sceCdSync inputs=0", retail,
          function({addiu(a0, zero, 0)}, false), 0);

    // FlushCache(0), number 0x64 in v1, set by the stub.
    auto flush = function({addiu(v1, zero, 0x64), addiu(a0, zero, 0)}, true);
    check("a kernel call: an argument register it does not take is not compared", fcheck, dir, "sceCdSync",
          flush, function({addiu(a1, zero, 9), addiu(v1, zero, 0x64), addiu(a0, zero, 0)}, true), 0);
    check("a kernel call: its number in v1 is compared", fcheck, dir, "sceCdSync", flush,
          function({addiu(v1, zero, 0x68), addiu(a0, zero, 0)}, true), 1);
    check("a kernel call: its argument is compared", fcheck, dir, "sceCdSync", flush,
          function({addiu(v1, zero, 0x64), addiu(a0, zero, 2)}, true), 1);

    // A retail run that jumps to address 0 (here on its first pass) slides through the empty
    // memory below the program into it again and returns by chance: no reference, skipped.
    u32 beq_a1 = (4u << 26) | (a1 << 21) | 3;  // beq a1, zero, +3 (to the jr ra)
    u32 jr_zero = 8, clear_a1 = (a1 << 11) | 0x2D;  // jr zero; daddu a1, zero, zero
    check("a retail run that jumps below the program is skipped", fcheck, dir, "sceCdSync",
          {beq_a1, kNop, jr_zero, clear_a1, kJrRa, kNop}, {kJrRa, kNop}, 4);

    std::printf("%s\n", failures ? "FAILED" : "all passed");
    return failures ? 1 : 0;
}

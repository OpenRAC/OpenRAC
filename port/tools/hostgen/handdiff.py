#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks the port's hand C for a routine of hand-written assembly against hostgen's translation of
the retail routine, call by call, bit for bit.

    python3 port/tools/hostgen/handdiff.py --ref <gen of a build with the disc> \\
        --cand <gen of a build that has the hand C> func_00282928 [more names] [--trials 2000]

--ref is a hostgen output that translated the routine from its assembly (report.json
"from_assembly"; the disc build's gen before the hand C, or a run with --asm-console off and the
row taken out of hand.tsv); --cand one that translated the port's C for it (hand/hand.tsv). Both
are generated files of a local build, never committed: nothing of the retail code leaves them.

Each side's whole file is compiled as the build compiles it, its symbols prefixed apart (ref_,
cand_), and linked into one test program with the runtime's float and EE code. The program gives
both the same game memory (all 32 MiB of RAM filled with random words, and the scratchpad), the
same stack and the same random arguments, calls one then the other, and compares, bit for bit:
the result, every byte of RAM and of the scratchpad, the hardware register writes
(openrac_ee_sw_dma) in order, and the calls each makes to other functions (by name and order; a
callee returns 0, or for a call through a function pointer, records the address). Arguments are
drawn by parameter type: an integer is a pointer into RAM, a small number or any word; a float any
of a spread of values with zeros, denormals, huge values and the EE's NaN and infinity patterns
among them. --args gives a spec per parameter (p pointer, i small int, w word, f float, or a
number) where the default spread is wrong for a routine.

Exit status 0 when every trial agrees, 1 at the first that does not (said, with its inputs).
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]

DEF = r"^(?P<ret>[A-Za-z_][\w \*]*?)\s*\b{name}\((?P<params>[^)]*)\)\s*\{{"


def find_definition(gen: Path, name: str, prefer_asm: bool) -> tuple[Path, str, list[str]]:
    """The file of gen that defines name, its result type and its parameter types."""
    pat = re.compile(DEF.format(name=re.escape(name)), re.M)
    files = sorted(gen.glob("asm_*.c")) if prefer_asm else []
    files += [p for p in sorted(gen.rglob("*.c")) if "prep" not in p.parts
              and p.name not in ("stubs.c", "functions.c", "libraries.c", "hooks.c") and p not in files]
    for p in files:
        text = p.read_text(errors="replace")
        m = pat.search(text)
        if m and not m.group("ret").strip().startswith(("return", "else")):
            params = [x.strip() for x in m.group("params").split(",") if x.strip() and x.strip() != "void"]
            types = [re.sub(r"\s*\b\w+$", "", x) if re.search(r"[\w\*]\s+\w+$", x) else x for x in params]
            return p, " ".join(m.group("ret").split()), types
    raise SystemExit(f"handdiff: {gen} defines no {name}")


def compile_flags(gen: Path, game: str) -> list[str]:
    return ["-I", str(gen), "-I", str(PORT / "game" / game / "host"), "-I", str(PORT / "game/rac2/host"),
            "-I", str(PORT.parent / "sdk/include"), "-I", str(PORT / "game/common/include"),
            "-I", str(PORT / "runtime/include"), "-I", str(PORT), "-O2", "-std=gnu11",
            "-fno-strict-aliasing", "-fwrapv", "-fsigned-char", "-ffp-contract=off", "-w",
            "-DOPENRAC_HAS_DISC=1", "-fno-pic", "-fno-pie"]


def run(cmd: list[str], cwd: Path | None = None) -> str:
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(f"handdiff: {' '.join(cmd[:3])} ...: {r.stderr[-3000:]}")
    return r.stdout


def undefined(obj: Path) -> set[str]:
    return {l.split()[-1] for l in run(["nm", "-u", str(obj)]).splitlines() if l.strip()}


def defined(obj: Path) -> set[str]:
    out = set()
    for l in run(["nm", "--defined-only", str(obj)]).splitlines():
        parts = l.split()
        if len(parts) == 3 and parts[1] in "TtDdBbRrVvWw":
            out.add(parts[2])
    return out


def ctype_kind(t: str) -> str:
    t = t.replace("const", "").strip()
    if t in ("float", "openrac_f32", "f32"):
        return "f"
    if t in ("double",):
        return "d"
    if "long long" in t or t in ("openrac_u64", "openrac_s64", "uint64_t", "int64_t", "u64", "s64"):
        return "q"
    return "w"


HARNESS = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* The vector unit's registers (runtime/ee.cpp): both sides share them, so they are set alike before
 * each call, filled with random floats, and compared after. */
extern unsigned char openrac_vu0[];
#define VU0_SIZE @VU0SIZE@

typedef uint32_t gaddr;
uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = 0x02000000u;
gaddr openrac_guest_sp, openrac_relocate_low, openrac_relocate_high, openrac_guest_last_entry;
void* openrac_guest_last_fn;

#define LOG_MAX 4096
static char log_[2][LOG_MAX][48];
static int nlog[2], side;
void handdiff_note(const char* what, uint64_t v) {
    if (nlog[side] < LOG_MAX) {
        snprintf(log_[side][nlog[side]++], 48, "%s %llx", what, (unsigned long long)v);
    }
}
void openrac_ee_sw_dma(uint32_t a, uint64_t v) {
    handdiff_note("dma", ((uint64_t)a << 32) | (uint32_t)v);
    memcpy(openrac_guest_base + a, &v, 4);
}
static gaddr fn_called;
static uint64_t through_pointer(void) {
    handdiff_note("callptr", fn_called);
    return 0;
}
void* openrac_guest_function(gaddr a) {
    fn_called = a;
    return (void*)through_pointer;
}
uint64_t openrac_guest_call_regs(gaddr a, const uint64_t* i, const float* f, float* r, int n) {
    (void)i; (void)f; (void)n;
    handdiff_note("callregs", a);
    if (r) *r = 0.0f;
    return 0;
}
gaddr openrac_guest_frame(uint32_t size) {
    openrac_guest_sp = (openrac_guest_sp - size) & ~15u;
    return openrac_guest_sp;
}
gaddr openrac_guest_static(uint32_t size) {
    static gaddr next = 0x01E00000u;
    gaddr at = next;
    next += (size + 15) & ~15u;
    return at;
}
void handdiff_stub(const char* name) { handdiff_note(name, 0); }

static uint64_t rng = 0x9E3779B97F4A7C15ull;
static uint64_t next64(void) {
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return rng;
}
static uint32_t pick_word(char kind) {
    switch (kind) {
    case 'p': return 0x00100000u + ((uint32_t)next64() % 0x01C00000u & ~15u) + (uint32_t)(next64() % 4 == 0 ? (next64() & 15u) : 0);
    case 'i': return (uint32_t)(int32_t)((int64_t)(next64() % 600) - 100);
    case 'w': return (uint32_t)next64();
    default: {
        switch (next64() % 3) {
        case 0: return 0x00100000u + ((uint32_t)next64() % 0x01C00000u & ~15u);
        case 1: return (uint32_t)(int32_t)((int64_t)(next64() % 600) - 100);
        default: return (uint32_t)next64();
        }
    }
    }
}
static float pick_float(void) {
    static const uint32_t special[] = {0, 0x80000000u, 0x3F800000u, 0xBF800000u, 0x00000001u, 0x007FFFFFu,
                                       0x7F800000u, 0xFF800000u, 0x7FFFFFFFu, 0xFFFFFFFFu, 0x7F7FFFFFu,
                                       0x00800000u, 0x3F000000u, 0x40490FDBu};
    uint32_t b;
    switch (next64() % 4) {
    case 0: b = special[next64() % (sizeof special / sizeof *special)]; break;
    case 1: b = (uint32_t)next64(); break;
    default: { float f = (float)((int64_t)(next64() % 2000001) - 1000000) / 1000.0f; memcpy(&b, &f, 4); }
    }
    float f; memcpy(&f, &b, 4); return f;
}

#define RAM 0x02000000u
#define STACK_LOW 0x01EF0000u
#define SPR 0x70000000u
static uint8_t *ram0, *ram1, *spr1;

static void fill(void) {
    uint64_t* w = (uint64_t*)openrac_guest_base;
    for (uint32_t i = 0; i < RAM / 8; ++i) w[i] = next64();
    /* Some floats that look like the game's (small, finite) among the random words. */
    float* f = (float*)openrac_guest_base;
    for (uint32_t i = 0; i < RAM / 4; i += 3) f[i] = (float)((int32_t)(w[i / 2] % 20001) - 10000) / 100.0f;
    uint64_t* s = (uint64_t*)(openrac_guest_base + SPR);
    for (uint32_t i = 0; i < 0x4000 / 8; ++i) s[i] = next64();
    memset(openrac_guest_base + 0x10000000u, 0, 0x10000u); /* the hardware registers read 0: idle */
}

static int differs(const char* what, const uint8_t* a, const uint8_t* b, uint32_t base, uint32_t n) {
    if (memcmp(a, b, n) == 0) return 0;
    for (uint32_t i = 0; i < n; i += 4) {
        if (memcmp(a + i, b + i, 4)) {
            uint32_t x, y; memcpy(&x, a + i, 4); memcpy(&y, b + i, 4);
            printf("  %s differs at %08x: ref %08x, cand %08x\n", what, base + i, x, y);
            return 1;
        }
    }
    return 1;
}

@DECLS@

int main(int argc, char** argv) {
    int trials = argc > 1 ? atoi(argv[1]) : 1000;
    if (argc > 2) rng ^= strtoull(argv[2], 0, 0);
    openrac_guest_base = mmap(0, 0x100000000ull, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    ram0 = malloc(RAM); ram1 = malloc(RAM); spr1 = malloc(0x4000);
    if (openrac_guest_base == MAP_FAILED || !ram0 || !ram1) { puts("no memory"); return 2; }
    for (int t = 0; t < trials; ++t) {
        fill();
        memcpy(ram0, openrac_guest_base, RAM);
        uint8_t spr0[0x4000]; memcpy(spr0, openrac_guest_base + SPR, 0x4000);
        uint8_t vu0[VU0_SIZE], vu1[VU0_SIZE];
        for (int i = 0; i < VU0_SIZE / 4; ++i) { float f = pick_float(); memcpy(openrac_vu0 + 4 * i, &f, 4); }
        { const float one[4] = {0.0f, 0.0f, 0.0f, 1.0f}; memcpy(openrac_vu0, one, 16); } /* vf0 */
        memcpy(vu0, openrac_vu0, VU0_SIZE);
@ARGS@
        side = 0; nlog[0] = nlog[1] = 0; openrac_guest_sp = 0x01F00000u;
        memset(openrac_guest_base + 0x10000000u, 0, 0x10000u);
@CALLREF@
        memcpy(ram1, openrac_guest_base, RAM); memcpy(spr1, openrac_guest_base + SPR, 0x4000);
        memcpy(vu1, openrac_vu0, VU0_SIZE); memcpy(openrac_vu0, vu0, VU0_SIZE);
        memcpy(openrac_guest_base, ram0, RAM); memcpy(openrac_guest_base + SPR, spr0, 0x4000);
        memset(openrac_guest_base + 0x10000000u, 0, 0x10000u);
        side = 1; openrac_guest_sp = 0x01F00000u;
@CALLCAND@
        int bad = 0;
@CMPRES@
        /* The stack below the caller's is dead once the call returns: each side's frame is its own. */
        memset(ram1 + STACK_LOW, 0, 0x01F00000u - STACK_LOW);
        memset(openrac_guest_base + STACK_LOW, 0, 0x01F00000u - STACK_LOW);
        bad |= differs("RAM", ram1, openrac_guest_base, 0, RAM);
        if (!getenv("HANDDIFF_VU0_DEAD")) bad |= differs("VU0", vu1, openrac_vu0, 0, VU0_SIZE);
        bad |= differs("scratchpad", spr1, openrac_guest_base + SPR, SPR, 0x4000);
        if (nlog[0] != nlog[1]) { printf("  %d side effects against %d\n", nlog[0], nlog[1]); bad = 1; }
        for (int i = 0; i < nlog[0] && i < nlog[1]; ++i)
            if (strcmp(log_[0][i], log_[1][i])) { printf("  effect %d: ref %s, cand %s\n", i, log_[0][i], log_[1][i]); bad = 1; break; }
        if (bad) {
            printf("@NAME@: trial %d differs; arguments:", t);
@PRINTARGS@
            printf("\n");
            return 1;
        }
    }
    printf("@NAME@: %d trials, all the same\n", trials);
    return 0;
}
"""

VU0_SIZE = 636  # sizeof(openrac_vu0_state) (runtime/include/openrac/ee_ops.h)
ARG_C = {"w": "uint32_t", "q": "uint64_t", "f": "float", "d": "double"}


def harness_text(name: str, ret: str, kinds: list[str], specs: list[str]) -> str:
    decl_params = ", ".join(ARG_C[k] for k in kinds) or "void"
    rk = "v" if ret == "void" else ctype_kind(ret)
    rt = "void" if rk == "v" else ARG_C[rk]
    decls = f"{rt} ref_{name}({decl_params});\n{rt} cand_{name}({decl_params});\n"
    args, prints = [], []
    for i, (k, s) in enumerate(zip(kinds, specs)):
        if k in "fd":
            val = s if s not in ("", "f") else "pick_float()"
            args.append(f"        {ARG_C[k]} a{i} = {val};")
            prints.append(f'            {{ float x = a{i}; uint32_t b; memcpy(&b, &x, 4); printf(" %08x", b); }}')
        else:
            val = f"pick_word('{s or '?'}')" if s in ("", "p", "i", "w") else s
            if k == "q":
                val = f"(uint64_t)(int64_t)(int32_t){val}"
            args.append(f"        {ARG_C[k]} a{i} = {val};")
            prints.append(f'            printf(" %llx", (unsigned long long)a{i});')
    call = ", ".join(f"a{i}" for i in range(len(kinds)))
    if rk == "v":
        cref, ccand, cmp = f"        ref_{name}({call});", f"        cand_{name}({call});", ""
    else:
        cref = f"        {rt} r0 = ref_{name}({call});"
        ccand = f"        {rt} r1 = cand_{name}({call});"
        cmp = ("        if (memcmp(&r0, &r1, sizeof r0)) { printf(\"  the results differ\\n\"); bad = 1; }"
               if rk in "fd" else
               "        if ((uint32_t)r0 != (uint32_t)r1) { printf(\"  the results differ: %llx, %llx\\n\", "
               "(unsigned long long)r0, (unsigned long long)r1); bad = 1; }")
    return (HARNESS.replace("@VU0SIZE@", str(VU0_SIZE)).replace("@DECLS@", decls).replace("@ARGS@", "\n".join(args))
            .replace("@CALLREF@", cref).replace("@CALLCAND@", ccand).replace("@CMPRES@", cmp)
            .replace("@PRINTARGS@", "\n".join(prints)).replace("@NAME@", name))


def build_runtime(work: Path) -> list[Path]:
    objs = []
    for src in ("runtime/ps2float.cpp", "runtime/ee.cpp", "common/ps2_float.cpp", "common/env.cpp",
                "common/log.cpp"):
        o = work / (Path(src).name + ".o")
        run(["clang++", "-std=c++20", "-O2", "-fno-pic", "-I", str(PORT / "runtime/include"), "-I", str(PORT),
             "-c", str(PORT / src), "-o", str(o)])
        objs.append(o)
    # log.cpp's fatal path reports a crash: the test has none to report.
    stub = work / "crash_stub.cpp"
    stub.write_text('#include "common/crash.h"\n#include <cstdlib>\n'
                    'namespace openrac::crash { const char* report(const char*, Kind) { std::abort(); } }\n')
    o = work / "crash_stub.o"
    run(["clang++", "-std=c++20", "-O1", "-fno-pic", "-I", str(PORT), "-c", str(stub), "-o", str(o)])
    objs.append(o)
    return objs


def check(name: str, args: argparse.Namespace, work: Path, runtime: list[Path]) -> bool:
    ref_file, ret, ref_types = find_definition(args.ref, name, True)
    cand_file, cret, cand_types = find_definition(args.cand, name, False)
    if len(ref_types) != len(cand_types):
        print(f"{name}: {len(ref_types)} parameters in the translation, {len(cand_types)} in the hand C")
        return False
    kinds = [ctype_kind(t) for t in ref_types]
    specs = (args.args.split(",") if args.args else [""] * len(kinds)) + [""] * len(kinds)
    objs = []
    for side, gen, f in (("ref", args.ref, ref_file), ("cand", args.cand, cand_file)):
        o = work / f"{side}_{name}.o"
        run(["clang", *compile_flags(gen, args.game_id), "-c", str(f), "-o", str(o)])
        run(["objcopy", f"--prefix-symbols={side}_", str(o)])
        objs.append(o)
    h = work / f"main_{name}.c"
    h.write_text(harness_text(name, ret, kinds, specs[:len(kinds)]))
    ho = work / f"main_{name}.o"
    run(["clang", "-O1", "-fno-pic", "-w", "-c", str(h), "-o", str(ho)])
    have = set().union(*(defined(o) for o in [ho, *runtime]))
    stubs, defsyms = [], []
    need = set().union(*(undefined(o) for o in [*objs, *runtime]))
    for sym in sorted(need):
        bare = re.sub(r"^(ref|cand)_", "", sym)
        if sym in have or (bare == sym and not sym.startswith(("openrac_", "func_", "FUN_", "D_"))):
            continue  # the C and C++ libraries
        if bare != sym and bare in have:
            defsyms.append(f"-Wl,--defsym={sym}={bare}")
        elif re.match(r"(D_|B_|g_|s_)", bare) or bare.startswith("openrac_part_"):
            stubs.append(f"char {sym}[4096] __attribute__((aligned(16)));")
        else:
            stubs.append(f"unsigned long long {sym}(void) {{ extern void handdiff_stub(const char*); "
                         f"handdiff_stub(\"{bare}\"); return 0; }}")
    s = work / f"stubs_{name}.c"
    s.write_text("\n".join(stubs) + "\n")
    so = work / f"stubs_{name}.o"
    run(["clang", "-O1", "-fno-pic", "-w", "-c", str(s), "-o", str(so)])
    exe = work / f"handdiff_{name}"
    run(["clang++", "-no-pie", "-o", str(exe), str(ho), str(so), *map(str, objs), *map(str, runtime),
         *defsyms, "-lm"])
    env = dict(os.environ, **({"HANDDIFF_VU0_DEAD": "1"} if args.vu0_dead else {}))
    r = subprocess.run([str(exe), str(args.trials), str(args.seed)], capture_output=True, text=True, env=env)
    sys.stdout.write(r.stdout + r.stderr)
    return r.returncode == 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--ref", type=Path, required=True, help="gen of a build that translated the routine's assembly")
    ap.add_argument("--cand", type=Path, required=True, help="gen of a build with the port's hand C")
    ap.add_argument("--game-id", default="rac2-ntsc")
    ap.add_argument("--trials", type=int, default=2000)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--args", help="per parameter: p, i, w, f, or a C value, comma-separated")
    ap.add_argument("--vu0-dead", action="store_true",
                    help="leave the vector unit's registers out of the comparison: only for a routine whose "
                         "registers no later code reads (say why where its verdict is kept)")
    ap.add_argument("--keep", type=Path, help="keep the test programs in this directory")
    ap.add_argument("names", nargs="+")
    args = ap.parse_args(argv)
    work = args.keep or Path(tempfile.mkdtemp(prefix="handdiff-"))
    work.mkdir(parents=True, exist_ok=True)
    try:
        runtime = build_runtime(work)
        bad = [n for n in args.names if not check(n, args, work, runtime)]
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    if bad:
        print(f"handdiff: {len(bad)} of {len(args.names)} differ: {' '.join(bad)}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

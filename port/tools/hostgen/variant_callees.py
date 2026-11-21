#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""What each copy of a folded level function calls, where the copies call different functions.

Up Your Arsenal's catalogue (config/levels/functions.tsv) folds the copies of a function by a
fingerprint that leaves address fields out, so a wrapper of one jal is one name however its copies
differ: func_L01_003B2DC8 calls func_L01_00459AF8 in Veldin, but its copies among Florana's class
updates call one that removes the moby they were given. hostgen reaches such a call through
openrac_call_in_copy (the running copy's own target), with the registers the caller has; the caller's
signature must then take every argument register any copy's callee reads, and hand back the result
kind they all give. This lists, for each such call of a wrapper (64 bytes or less), every callee name the copies reach and one place
of each, for hostgen's signature check (asm.signature_check: hostgen.json asm "variant_callees"):

    caller  callee  level  place  word  passes  result

level, place: a copy of the caller that calls this callee, word: the call's index in the caller.
passes: the argument registers ($a0-$a7 as a0..a7, $f12-$f19 as f12..f19) the caller's code writes
nowhere before the call (nor in its delay slot): what it hands on from its own caller. result: v0
and/or f0 when the code after the call writes no such register before it returns: the callee's result
is the caller's ("-" for neither).

    python3 port/tools/hostgen/variant_callees.py --source games/rac3/decomp \\
        --split port/game/rac3-pal/tables/split_places.tsv --out port/game/rac3-pal/tables/variant_callees.tsv
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from copycalls import elf_sections, jal_target, words  # noqa: E402

HEADER = ("# caller\tcallee\tlevel\tplace\tword\tpasses\tresult"
          " -- port/tools/hostgen/variant_callees.py\n")
INT_ARGS = {4 + k: f"a{k}" for k in range(8)}       # $a0-$a3, then $t0-$t3 as a4-a7
FLOAT_ARGS = {12 + k: f"f{12 + k}" for k in range(8)}
JR_RA = 0x03E00008
MAX_WRAPPER = 64  # bytes


def written(w: int) -> tuple[set[int], set[int]]:
    """The general and FPU registers an instruction writes (as far as passing arguments on goes)."""
    op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    gpr: set[int] = set()
    fpr: set[int] = set()
    if op in (0x00, 0x1C):
        funct = w & 0x3F
        if not (op == 0 and funct in (0x08, 0x0C, 0x0D, 0x0F, 0x11, 0x13, 0x18, 0x19, 0x1A, 0x1B)):
            gpr.add(rd)
    elif op == 0x03:
        gpr.add(31)
    elif op in (0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x18, 0x19, 0x1A, 0x1B, 0x1E,
                0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x37):
        gpr.add(rt)
    elif op in (0x31, 0x35):
        fpr.add(rt)
    elif op == 0x11:
        if rs in (0x00, 0x02):      # mfc1, cfc1
            gpr.add(rt)
        elif rs == 0x04:            # mtc1
            fpr.add(rd)
        elif rs in (0x10, 0x14):    # .s and .w arithmetic and conversions
            fpr.add((w >> 6) & 31)
    elif op == 0x12 and rs in (0x01, 0x02):  # qmfc2, cfc2
        gpr.add(rt)
    gpr.discard(0)
    return gpr, fpr


def passes(code: tuple[int, ...], call: int) -> str:
    """The argument registers the code writes nowhere before the call at word `call` (its delay
    slot included): what the call hands on from the function's own caller."""
    gpr: set[int] = set()
    fpr: set[int] = set()
    for w in code[:call + 2]:
        g, f = written(w) if w != code[call] else (set(), set())
        gpr |= g
        fpr |= f
    regs = [n for r, n in INT_ARGS.items() if r not in gpr] + [n for r, n in FLOAT_ARGS.items() if r not in fpr]
    return " ".join(regs) or "-"


def branch(w: int) -> bool:
    """Whether an instruction branches or jumps (anything but straight on)."""
    op = w >> 26
    return op in (0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17) \
        or (op == 0 and (w & 0x3F) in (0x08, 0x09)) or (op == 0x11 and ((w >> 21) & 31) == 0x08)


def result(code: tuple[int, ...], call: int) -> str:
    """v0 and/or f0 when the code goes straight from the call to its return (no branch, at most
    eight words) writing neither, its delay slot included: the callee's result is the function's.
    "-" otherwise."""
    gpr: set[int] = set()
    fpr: set[int] = set()
    for at in range(call + 2, min(len(code), call + 10)):
        w = code[at]
        if w == JR_RA:
            if at + 1 < len(code):
                g, f = written(code[at + 1])
                gpr |= g
                fpr |= f
            out = [n for r, n in ((2, "v0"),) if r not in gpr] + (["f0"] if 0 not in fpr else [])
            return " ".join(out) or "-"
        if branch(w):
            return "-"
        g, f = written(w)
        gpr |= g
        fpr |= f
    return "-"


def level_programs(levels: Path) -> dict[int, list[tuple[int, bytes]]]:
    """Each level's program from the decompilation's split of the disc (baserom/levels/NN_name.elf)."""
    return {int(p.name[:2]): elf_sections(p) for p in sorted(levels.glob("*.elf")) if p.name[:2].isdigit()}


def catalogue(path: Path) -> list[tuple[str, int, list[tuple[int, int]]]]:
    out = []
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[6].split(",") if p]
        out.append((cols[0], int(cols[2]), places))
    return out


def apply_split(table: list, split: Path | None) -> list:
    """The catalogue with split_places.tsv applied (program.read_split_places): the places a variant
    takes leave their function and are a caller of their own under the variant's name."""
    if split is None or not split.exists():
        return table
    moved: dict[tuple[int, int], str] = {}
    variants: dict[str, tuple[int, list]] = {}
    sizes = {name: size for name, size, _ in table}
    for line in split.read_text().splitlines():
        cols = line.split("	")
        if line.startswith("#") or len(cols) < 3 or cols[0] not in sizes:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[2].split(",") if p]
        variants.setdefault(cols[1], (sizes[cols[0]], []))[1].extend(places)
        moved.update({place: cols[1] for place in places})
    out = [(name, size, [p for p in places if p not in moved]) for name, size, places in table]
    out += [(name, size, places) for name, (size, places) in variants.items()]
    return out


def rows(source: Path, split: Path | None = None) -> list[tuple[str, str, int, int, int, str, str]]:
    table = apply_split(catalogue(source / "config/levels/functions.tsv"), split)
    names = {place: name for name, _, places in table for place in places}
    programs = level_programs(source / "baserom/levels")
    out = []
    for caller, size, places in table:
        size &= ~3
        # Wrappers: a function of a few instructions hands its registers on by design; in a longer
        # one an argument register left unwritten before a call is the EE's leftover.
        if len(places) < 2 or size < 8 or size > MAX_WRAPPER:
            continue
        copies = [(level, address, words(programs[level], address, size))
                  for level, address in places if level in programs]
        copies = [c for c in copies if c[2] is not None]
        if len(copies) < 2:
            continue
        for i in range(size // 4):
            targets = [(level, address, jal_target(w[i])) for level, address, w in copies]
            if any(t is None for _, _, t in targets):
                continue
            called: dict[str, tuple[int, int]] = {}
            for level, address, t in targets:
                callee = names.get((level, t), f"?{t:08X}")
                called.setdefault(callee, (level, address))
            if len(called) < 2:
                continue
            home = copies[0][2]
            how = (passes(home, i), result(home, i))
            for callee, (level, address) in sorted(called.items()):
                out.append((caller, callee, level, address, i, *how))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--split", type=Path, help="the game's tables/split_places.tsv, applied to the catalogue")
    args = ap.parse_args()
    found = rows(args.source, args.split)
    text = HEADER + "".join(f"{c}\t{e}\t{lv:02d}\t{pl:08X}\t{w}\t{p}\t{r}\n" for c, e, lv, pl, w, p, r in found)
    args.out.write_text(text)
    print(f"variant_callees: {len({r[0] for r in found})} callers, {len(found)} rows", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())

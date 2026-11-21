#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""NTSC-U's address of a PAL level's global, from the same instruction of the same function.

The shared rac3 host C names a level's own data by PAL's address in that level (render.c's
Veldin tables, the profile's per-level camera and moby globals, tools.c's mode words), written
RAC3_LADDR(level, 0x...u). rac3-decomp's NTSC-U tables name the core programs' globals only
(config/us/names.tsv). This finds a level global's NTSC-U address as release_carry.py finds Going
Commando's ("the same instruction of the paired function"): every function of that PAL level
whose NTSC-U twin is the same code (config/us/map.tsv `match` or `asm`, the same size) is read
from both discs' level programs; where a PAL `lui`/`addiu` (or a load or store) forms the address,
the NTSC-U twin's instruction at the same offset gives NTSC-U's. Each answer is written into
port/game/rac3-ntsc/release/release.json's `level_addresses` with its evidence (the function, the
offset and how many places agree), for release_carry_rac3.py, which needs no disc.

    python3 port/tools/hostgen/rac3_level_pairs.py --source games/rac3/decomp [--write] L01:001E1068 ...

With no addresses it pairs every RAC3_LADDR() of the shared host C and the profile.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]
RELEASE = PORT / "game/rac3-ntsc/release/release.json"
LADDR = re.compile(r"\bRAC3_LADDR\(\s*(\d+)\s*,\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
# Instructions that take a base register and a signed 16-bit offset (loads, stores, addiu, ...).
IMM_OPS = {0x09, 0x0D, 0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2B, 0x31, 0x35, 0x37, 0x39, 0x3D,
           0x3F, 0x1E, 0x1F}


def elf_segments(path: Path) -> list[tuple[int, bytes]]:
    d = path.read_bytes()
    ph, n = struct.unpack_from("<I", d, 28)[0], struct.unpack_from("<H", d, 44)[0]
    out = []
    for k in range(n):
        _t, off, va, _pa, fs, _ms = struct.unpack_from("<6I", d, ph + 32 * k)
        out.append((va, d[off:off + fs]))
    return out


def words(segs: list[tuple[int, bytes]], address: int, size: int) -> list[int] | None:
    for va, b in segs:
        if va <= address and address + size <= va + len(b):
            return list(struct.unpack_from(f"<{size // 4}I", b, address - va))
    return None


def formed(code: list[int], gp: int) -> dict[int, list[int]]:
    """Address -> the offsets (in words) of the instructions that complete it (lui + imm, or an
    offset from $gp)."""
    out: dict[int, list[int]] = defaultdict(list)
    hi: dict[int, int] = {}
    for i, w in enumerate(code):
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op == 0x0F:
            hi[rt] = (w & 0xFFFF) << 16
            continue
        imm = w & 0xFFFF
        imm = imm - 0x10000 if imm & 0x8000 else imm
        if op in IMM_OPS and rs == 28:
            out[(gp + imm) & 0xFFFFFFFF].append(i)
        elif op in IMM_OPS and rs in hi:
            out[(hi[rs] + imm) & 0xFFFFFFFF].append(i)
        # What the instruction writes no longer holds the upper half.
        if op in (0x00, 0x1C):
            hi.pop((w >> 11) & 31, None)
        elif op in (0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x1A, 0x1B, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                    0x26, 0x27, 0x37, 0x1E):
            hi.pop(rt, None)
        elif op in (0x02, 0x03):
            hi.clear()
    return out


def level_file(folder: Path, level: int) -> Path | None:
    return next(iter(sorted(folder.glob(f"{level:02d}_*.elf"))), None)


def catalogue(path: Path) -> dict[str, tuple[int, dict[int, int]]]:
    out = {}
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7:
            continue
        places = {}
        for item in cols[6].split(","):
            level, _, address = item.partition(":")
            if address and level.isdigit():
                places[int(level)] = int(address, 16)
        out[cols[0]] = (int(cols[2]) if cols[2].isdigit() else 0, places)
    return out


def level_gp(source: Path, version: str, level: int) -> int:
    for line in (source / "config/gp.tsv").read_text().splitlines():
        cols = line.split("\t")
        if len(cols) >= 3 and cols[0] == version and cols[1] == f"level_{level:02d}":
            return int(cols[2], 16)
    return 0x001DC8B0


def pair(source: Path, level: int, targets: list[int]) -> dict[int, tuple[int, str]]:
    pal_seg = elf_segments(level_file(source / "baserom/levels", level))
    us_seg = elf_segments(level_file(source / "baserom/us/levels", level))
    pal_cat = catalogue(source / "config/levels/functions.tsv")
    us_cat = catalogue(source / "config/us/levels/functions.tsv")
    twins = {}
    for line in (source / "config/us/map.tsv").read_text().splitlines():
        cols = line.split("\t")
        if not line.startswith("#") and len(cols) >= 7 and cols[1] == "levels" and cols[6] in ("match", "asm"):
            twins.setdefault(cols[0], cols[2])
    want = set(targets)
    pal_gp, us_gp = level_gp(source, "pal", level), level_gp(source, "us", level)
    votes: dict[int, Counter] = defaultdict(Counter)
    every: dict[int, Counter] = defaultdict(Counter)  # every address the same code forms, both discs
    where: dict[tuple[int, int], str] = {}
    for name, us_name in twins.items():
        size, places = pal_cat.get(name, (0, {}))
        us_size, us_places = us_cat.get(us_name, (0, {}))
        if level not in places or level not in us_places or size != us_size or size < 8:
            continue
        a, b = words(pal_seg, places[level], size), words(us_seg, us_places[level], size)
        if a is None or b is None:
            continue
        fa, fb = formed(a, pal_gp), formed(b, us_gp)
        # the same code: the same opcodes word for word
        if any((x >> 26) != (y >> 26) for x, y in zip(a, b)):
            continue
        by_offset = {i: addr for addr, offs in fb.items() for i in offs}
        for address, offs in fa.items():
            for i in offs:
                if i in by_offset and 0x00100000 <= address < 0x02000000:
                    every[address][by_offset[i]] += 1
        for target in want:
            for i in fa.get(target, ()):
                if i in by_offset:
                    votes[target][by_offset[i]] += 1
                    where.setdefault((target, by_offset[i]), f"{name} (NTSC-U {us_name}) +0x{4 * i:X}")
    out = {}
    # The level program's entry (its ELF header's): the other disc's entry.
    pal_entry = struct.unpack_from("<I", level_file(source / "baserom/levels", level).read_bytes(), 24)[0]
    us_entry = struct.unpack_from("<I", level_file(source / "baserom/us/levels", level).read_bytes(), 24)[0]
    if pal_entry in want:
        out[pal_entry] = (us_entry, f"the level program's entry (its ELF header: PAL 0x{pal_entry:08X}, "
                                    f"NTSC-U 0x{us_entry:08X})")
    # Unanimous pairs of every address the same code forms, for an address no instruction forms
    # itself: between the nearest below and above it, when both moved by as much (and are near).
    known = sorted((a, c.most_common(1)[0][0]) for a, c in every.items() if len(c) == 1)
    starts = [a for a, _ in known]
    for target in want:
        if target in votes or target in out:
            continue
        import bisect
        k = bisect.bisect_left(starts, target)
        if 0 < k < len(known):
            (lo, lo_us), (hi_, hi_us) = known[k - 1], known[k]
            if lo_us - lo == hi_us - hi_ and hi_ - lo <= 0x2000:
                out[target] = ((target + lo_us - lo) & 0xFFFFFFFF,
                               f"between the same code's 0x{lo:08X} and 0x{hi_:08X}, both moved by "
                               f"{lo_us - lo:+#x} (NTSC-U 0x{lo_us:08X}, 0x{hi_us:08X})")
    for target, counter in votes.items():
        (value, n), *rest = counter.most_common()
        total = sum(counter.values())
        if n * 2 <= total:
            continue  # no majority: not taken
        out[target] = (value, f"the same instruction of the same code: {where[(target, value)]}, "
                              f"{n} of {total} places agree")
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, required=True, help="rac3-decomp with both discs' level dumps")
    ap.add_argument("--write", action="store_true", help="write the answers into release.json")
    ap.add_argument("addresses", nargs="*", help="Lnn:address, PAL's")
    args = ap.parse_args(argv)
    todo: dict[int, set[int]] = defaultdict(set)
    for item in args.addresses:
        level, _, address = item.partition(":")
        todo[int(level.lstrip("L"))].add(int(address, 16))
    if not args.addresses:
        files = [*(PORT / "game/rac3-pal/host").glob("*.c"), PORT / "game/rac3-pal/profile.cpp"]
        for path in files:
            for m in LADDR.finditer(path.read_text()):
                todo[int(m.group(1))].add(int(m.group(2), 16))
    hand = json.loads(RELEASE.read_text())
    rows = hand.setdefault("level_addresses", {"about": "NTSC-U's address of a PAL level's global the "
                           "shared host C writes as RAC3_LADDR(level, address): Lnn:PAL's address, NTSC-U's, "
                           "the evidence (rac3_level_pairs.py)."})
    missing = []
    for level, targets in sorted(todo.items()):
        found = pair(args.source.resolve(), level, sorted(targets))
        for target in sorted(targets):
            key = f"L{level:02d}:{target:08X}"
            if target in found:
                value, why = found[target]
                print(f"{key} -> {value:08X}  {why}")
                rows[key] = {"is": f"{value:08X}", "why": why}
            else:
                print(f"{key}: no instruction of the same code forms it")
                missing.append(key)
    if args.write:
        RELEASE.write_text(json.dumps(hand, indent=2) + "\n")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())

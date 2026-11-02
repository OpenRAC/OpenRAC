#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The catalogue's folded functions whose copies call different functions.

The catalogue's fingerprint leaves address fields out, so a wrapper of one jal is "the same
function" in every level whatever it calls: Going Commando's func_L00_003E4948 calls
func_L00_003A6C30 in level 0's build and func_L05_0041CFE8 in level 5, and the port, translating
one copy, ran level 0's moby update on level 5's moby. This reads every place's words from the
retail programs and lists each jal that differs between places, for hostgen to call through
openrac_call_in_copy (hostgen.json "call_copies"). One row per caller and callee name:

    caller  callee  home  canon  size  own  exe

home, canon: a level holding the caller and that call, and the caller's place there. size: the
caller's, in bytes. own: the callee's place in home. exe: the jal's target in the executable's copy
of the caller, or 0.

    python3 port/tools/hostgen/copycalls.py --source games/rac2/ntsc \\
        --out port/game/rac2-ntsc/call_copies.tsv
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


def elf_sections(path: Path) -> list[tuple[int, bytes]]:
    data = path.read_bytes()
    table = struct.unpack_from("<I", data, 28)[0]
    count = struct.unpack_from("<H", data, 44)[0]
    out = []
    for i in range(count):
        kind, offset, vaddr, _, size = struct.unpack_from("<5I", data, table + i * 32)
        if kind == 1:
            out.append((vaddr, data[offset:offset + size]))
    return out


def level_sections(folder: Path) -> list[tuple[int, bytes]]:
    manifest = json.loads((folder / "manifest.json").read_text())
    out = []
    for section in manifest["sections"]:
        path = folder / f"{section['name']}.bin"
        if path.exists():
            out.append((section["address"], path.read_bytes()))
    return out


def words(sections: list[tuple[int, bytes]], address: int, size: int) -> tuple[int, ...] | None:
    for start, data in sections:
        if start <= address and address + size <= start + len(data):
            return struct.unpack_from(f"<{size // 4}I", data, address - start)
    return None


def jal_target(word: int) -> int | None:
    return (word & 0x03FFFFFF) << 2 if word >> 26 == 3 else None


def rows(source: Path) -> list[tuple[str, str, int, int, int, int, int]]:
    baserom = source / "baserom"
    table: list[tuple[str, str, int, list[tuple[int, int]]]] = []
    names: dict[tuple[int, int], str] = {}
    for line in (source / "config/levels/functions.tsv").read_text().splitlines():
        if line.startswith("#"):
            continue
        cols = line.split("\t")
        if len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        table.append((cols[0], cols[1], int(cols[2]), places))
        for place in places:
            names[place] = cols[0]
    programs = {int(d.name[:2]): level_sections(d) for d in sorted((baserom / "levels").iterdir())
                if d.is_dir() and d.name[:2].isdigit()}
    exe = elf_sections(baserom / "SCUS_972.68")
    out = []
    for caller, kind, size, places in table:
        if len(places) < 2 or size < 8:
            continue
        size &= ~3
        copies = [(level, address, words(programs[level], address, size))
                  for level, address in places if level in programs]
        copies = [c for c in copies if c[2] is not None]
        own_exe = words(exe, int(caller[-8:], 16), size) if kind == "boot" else None
        found: dict[str, tuple[int, int, int, int]] = {}
        for i in range(size // 4):
            targets = {(level, address): jal_target(w[i]) for level, address, w in copies}
            called = {names.get((level, t), f"?{t:X}") for (level, _), t in targets.items() if t is not None}
            if len(called) < 2:
                continue
            exe_target = jal_target(own_exe[i]) if own_exe else None
            for (level, address), t in targets.items():
                name = names.get((level, t)) if t is not None else None
                if name is not None and name not in found:
                    found[name] = (level, address, t, exe_target or 0)
        for name, (level, address, target, exe_target) in sorted(found.items()):
            out.append((caller, name, level, address, size, target, exe_target))
    return out


COMPLETES = (0x09, 0x0D, 0x31, 0x39, 0x37, 0x3F, 0x1E, 0x1F)


def formed(code: tuple[int, ...], gp: int) -> dict[int, int]:
    """The addresses a function forms, by a lui and its completion or from $gp: index of the
    completing word -> address."""
    upper: dict[int, tuple[int, int]] = {}
    out = {}
    for i, w in enumerate(code):
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if op == 0x0F:
            upper[rt] = (i, w & 0xFFFF)
        elif rs == 28 and rt != 28 and op != 0x0D and (op in COMPLETES or 0x20 <= op <= 0x2B):
            low = w & 0xFFFF
            out[i] = (gp + (low - 0x10000 if low & 0x8000 else low)) & 0xFFFFFFFF
        elif rs in upper and (op in COMPLETES or 0x20 <= op <= 0x2B):
            low = w & 0xFFFF
            if op == 0x0D:
                out[i] = (upper[rs][1] << 16) | low
            else:
                out[i] = ((upper[rs][1] << 16) + (low - 0x10000 if low & 0x8000 else low)) & 0xFFFFFFFF
    return out


def data_conflicts(source: Path, low: int, high: int, gp: int) -> list[tuple[str, int]]:
    """Functions and the level globals they form that do not pair with one address of another level:
    what is one field of a record in one level is two in another (level 0's 0x22144C is both
    0x221D34 and 0x221D4C in Feltzin), so pairing all functions gives one of them the wrong one."""
    baserom = source / "baserom"
    programs = {int(d.name[:2]): level_sections(d) for d in sorted((baserom / "levels").iterdir())
                if d.is_dir() and d.name[:2].isdigit()}
    pairs: dict[tuple[int, int, int], set[int]] = {}
    users: dict[tuple[int, int], set[str]] = {}
    small: set[tuple[str, int]] = set()
    for line in (source / "config/levels/functions.tsv").read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        size = int(cols[2]) & ~3
        if len(places) < 2 or size < 8:
            continue
        if not cols[0].startswith("func_L"):
            continue  # the executable's names: hostgen relocates those from the executable
        home = int(cols[0].split("_")[1][1:])
        codes = {lv: words(programs[lv], a, size) for lv, a in places if lv in programs}
        if codes.get(home) is None:
            continue
        canon = formed(codes[home], gp)
        if size <= 64 and any(low <= address < high for address in canon.values()):
            # An accessor: pairing also reads a base and an offset in other functions, which in
            # another level's record is another field (level 0's 0x22144C pairs as 0x221D4C through
            # its neighbours, where Feltzin's accessor reads 0x221D34).
            small.update((cols[0], a) for a in canon.values() if low <= a < high)
        for level, code in codes.items():
            if level == home or code is None:
                continue
            mine = formed(code, gp)
            for i, address in canon.items():
                if low <= address < high and i in mine:
                    pairs.setdefault((home, level, address), set()).add(mine[i])
                    users.setdefault((home, address), set()).add(cols[0])
    out = small
    for (home, _, address), seen in pairs.items():
        if len(seen) > 1:
            out |= {(user, address) for user in users[(home, address)]}
    return sorted(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--data", type=Path, help="also write the functions that take globals from their copy")
    parser.add_argument("--config", type=Path, help="the game's hostgen.json")
    args = parser.parse_args()
    found = rows(args.source)
    lines = ["# Written by port/tools/hostgen/copycalls.py from the retail programs; do not edit.",
             "# caller\tcallee\thome\tcanon\tsize\town\texe"]
    lines += [f"{a}\t{b}\t{c}\t{d:08X}\t{e}\t{f:08X}\t{g:08X}" for a, b, c, d, e, f, g in found]
    args.out.write_text("\n".join(lines) + "\n")
    print(f"{len(found)} calls of {len({r[0] for r in found})} functions")
    if args.data:
        config = json.loads(args.config.read_text())
        low, high = config["relocate"][:2] if isinstance(config["relocate"], list) else (config["relocate"]["low"], config["relocate"]["high"])
        names = data_conflicts(args.source, int(str(low), 0), int(str(high), 0), int(config["asm"]["gp"], 0))
        args.data.write_text("# Written by port/tools/hostgen/copycalls.py from the retail programs; do not edit.\n"
                             + "".join(f"{n}\t{a:08X}\n" for n, a in names))
        print(f"{len(names)} uses, by {len({n for n, _ in names})} functions, of globals that do not pair")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""NTSC-U's places of PAL's multiplayer executable's functions and globals (I5BOOTN.ELF).

rac3-pal's multiplayer program (port/game/rac3-pal/mp) is translated from the disc's I5BOOTN.ELF
alone: rac3-decomp has no tables for it. Its tables and the shared host C name it by PAL's
addresses (mp/hostgen.json's roots and signatures, mp/libraries.tsv, the host C's mp_func_ names,
RAC3_MADDR()). This pairs each with NTSC-U's from both discs' executables:

- a function: the same instructions (jumps' targets, upper halves and offsets masked) at the only
  place in NTSC-U's executable they occur; where they occur more than once (the kernel's stubs),
  the one at the distance the nearest uniquely paired function moved by;
- a global (RAC3_MADDR): the same instruction of a paired function forms it on both discs
  (lui and an offset, or $gp and an offset: PAL's $gp 0x00247FC0, NTSC-U's 0x00248140, as
  config/gp.tsv gives level 39's, the program the executable runs its menus in).

Writes port/game/rac3-ntsc/release/mp_pairs.json, each pair with its evidence, for
release_carry_rac3.py, which needs no disc.

    python3 port/tools/hostgen/rac3_mp_pairs.py --source games/rac3/decomp
"""
from __future__ import annotations

import argparse
import bisect
import json
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import rac3_level_pairs as lp  # noqa: E402

PORT = Path(__file__).resolve().parents[2]
PRIMARY = PORT / "game/rac3-pal"
OUT = PORT / "game/rac3-ntsc/release/mp_pairs.json"
DISCS = {"pal": "baserom/SCES_524.56.iso", "us": "baserom/SCUS_973.53.iso"}
GP = {"pal": 0x00247FC0, "us": 0x00248140}
WINDOW = 16  # words compared to pair a function
MADDR = re.compile(r"\bRAC3_MADDR\(\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
MP_NAME = re.compile(r"\b(?:mp_)?func_([0-9A-F]{8})\b")


def iso_file(iso: Path, name: str) -> bytes:
    """A file of the disc's root directory."""
    with iso.open("rb") as f:
        def sectors(n: int, count: int = 1) -> bytes:
            f.seek(n * 2048)
            return f.read(2048 * count)
        root = sectors(16)[156:190]
        lba, size = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
        d = sectors(lba, (size + 2047) // 2048)
        i = 0
        while i < len(d):
            n = d[i]
            if n == 0:
                i = (i // 2048 + 1) * 2048
                continue
            if d[i + 33:i + 33 + d[i + 32]].decode(errors="replace").split(";")[0] == name:
                at, length = struct.unpack_from("<I", d, i + 2)[0], struct.unpack_from("<I", d, i + 10)[0]
                f.seek(at * 2048)
                return f.read(length)
            i += n
    raise FileNotFoundError(f"{iso}: no {name}")


def program(source: Path, version: str) -> tuple[list[tuple[int, bytes]], int]:
    """The executable's loaded records (address, bytes) and its entry, from its WAD payload."""
    sys.path.insert(0, str(source / "tools"))
    import unpack_wad  # rac3-decomp's
    data = iso_file(source / DISCS[version], "I5BOOTN.ELF")
    payload = unpack_wad.decompress(data, unpack_wad.find_wad(data))
    segs, at, entry, records = [], 0, 0, []
    while at + 16 <= len(payload):
        address, size, kind, entry = struct.unpack_from("<4I", payload, at)
        if size > len(payload) - at - 16:
            break
        if kind == 1:
            segs.append((address, payload[at + 16:at + 16 + size]))
        if kind in (1, 8):
            records.append((address, size))
        at += 16 + size
    program.records[version] = records
    return segs, entry


program.records = {}


def mask(w: int) -> int:
    op = w >> 26
    if op in (2, 3):
        return w & 0xFC000000
    if op == 0x0F or op in lp.IMM_OPS or op in (0x08, 0x0A, 0x0B, 0x0C, 0x0E):
        return w & 0xFFFF0000
    return w


class Pairer:
    def __init__(self, pal, us):
        self.pal, self.us = pal, us
        self.masked = {va: [mask(x) for x in struct.unpack(f"<{len(b) // 4}I", b[:len(b) // 4 * 4])]
                       for va, b in us}

    def hits(self, address: int) -> list[int]:
        words = lp.words(self.pal, address, 4 * WINDOW)
        if words is None:
            return []
        key = [mask(x) for x in words]
        out = []
        for va, m in self.masked.items():
            for i in range(len(m) - WINDOW):
                if m[i] == key[0] and m[i:i + WINDOW] == key:
                    out.append(va + 4 * i)
        return out

    def functions(self, names: list[int]) -> dict[int, tuple[int | None, str]]:
        found = {a: self.hits(a) for a in names}
        unique = sorted((a, h[0]) for a, h in found.items() if len(h) == 1)
        starts = [a for a, _ in unique]
        out: dict[int, tuple[int | None, str]] = {}
        for a, h in found.items():
            if len(h) == 1:
                out[a] = (h[0], "the same instructions, the only place they occur")
                continue
            k = bisect.bisect_left(starts, a)
            near = min((unique[i] for i in (k - 1, k) if 0 <= i < len(unique)), key=lambda p: abs(p[0] - a),
                       default=None)
            want = a + (near[1] - near[0]) if near else None
            if near and want in h:
                out[a] = (want, f"the same instructions ({len(h)} places), at the distance the nearest "
                                f"paired function 0x{near[0]:08X} moved by ({near[1] - near[0]:+#x})")
            else:
                out[a] = (None, "no place with the same instructions" if not h else
                          f"{len(h)} places with the same instructions, none at a paired neighbour's distance")
        return out

    def data(self, functions: dict[int, int], targets: set[int]) -> dict[int, tuple[int, str]]:
        votes: dict[int, Counter] = defaultdict(Counter)
        where: dict[tuple[int, int], str] = {}
        for a, b in functions.items():
            pw, uw = lp.words(self.pal, a, 1024), lp.words(self.us, b, 1024)
            if pw is None or uw is None:
                continue
            n = next((i + 2 for i, w in enumerate(pw) if w == 0x03E00008), len(pw))  # to its jr ra
            fa, fb = lp.formed(pw[:n], GP["pal"]), lp.formed(uw[:n], GP["us"])
            by = {i: x for x, offs in fb.items() for i in offs}
            for t in targets:
                for i in fa.get(t, ()):
                    if i in by:
                        votes[t][by[i]] += 1
                        where.setdefault((t, by[i]), f"func 0x{a:08X} (NTSC-U 0x{b:08X}) +0x{4 * i:X}")
        out = {}
        for t, c in votes.items():
            (v, n), total = c.most_common(1)[0], sum(c.values())
            if n * 2 > total:
                out[t] = (v, f"the same instruction of the same code: {where[(t, v)]}, {n} of {total} places agree")
        return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, default=PORT.parent / "games/rac3/decomp")
    args = ap.parse_args(argv)
    source = args.source.resolve()
    pal, pal_entry = program(source, "pal")
    us, us_entry = program(source, "us")
    cfg = json.loads((PRIMARY / "mp/hostgen.json").read_text())
    text = json.dumps({k: cfg.get(k) for k in ("entry", "roots", "from_assembly")}) + json.dumps(
        list(cfg.get("asm", {}).get("signatures", {})))
    text += "\n".join(line.split("\t")[0] for line in (PRIMARY / "mp/libraries.tsv").read_text().splitlines()
                      if not line.startswith("#"))
    host = "".join(p.read_text() for p in sorted((PRIMARY / "host").glob("*.c")))
    names = {int(m.group(1), 16) for m in MP_NAME.finditer(text)}
    names |= {int(m.group(1), 16) for m in re.finditer(r"\bmp_func_([0-9A-F]{8})\b", host)}
    names = sorted(a for a in names if 0x00100000 <= a < 0x01000000)
    pairer = Pairer(pal, us)
    functions = pairer.functions(names)
    functions[pal_entry] = (us_entry, "the executable's entry (its payload's records)")
    # The executable's own front end (the payload's record at 0x0027B100): its first function on
    # NTSC-U, the same as PAL's but for the order of two register saves, starts the record, where
    # PAL's record has another function before it.
    if functions.get(0x0027B1C0, (None,))[0] is None:
        functions[0x0027B1C0] = (0x0027B200, "the front end's entry: NTSC-U's record 0x0027B200 starts with the "
                                             "same function (its register saves in another order)")
    targets = {int(m.group(1), 16) for m in MADDR.finditer(host)}
    paired = {a: b for a, (b, _) in functions.items() if b is not None}
    data = pairer.data(paired, targets - set(paired))
    # A record's start or end: the same record of NTSC-U's payload (the records come in one order).
    for (pa, psize), (ua, usize) in zip(program.records["pal"], program.records["us"]):
        for t in targets:
            if t == pa and t not in data:
                data[t] = (ua, f"the start of the payload's record at PAL 0x{pa:08X} (NTSC-U 0x{ua:08X})")
            if t == pa + psize and t not in data:
                data[t] = (ua + usize, f"the end of the payload's record at PAL 0x{pa:08X} (NTSC-U 0x{ua:08X}, "
                                       f"0x{usize:X} bytes)")
    # Level 39's functions the executable's tables and host C name (the menus' program it loads):
    # the same instructions in both discs' level 39 programs, for those rac3-decomp's map does not
    # pair (release_carry_rac3.py takes the map's first).
    l39 = sorted({int(m.group(1), 16) for m in re.finditer(r"\bfunc_L39_([0-9A-F]{8})\b", text + host)})
    level_pairer = Pairer(lp.elf_segments(lp.level_file(source / "baserom/levels", 39)),
                          lp.elf_segments(lp.level_file(source / "baserom/us/levels", 39)))
    out_l39 = level_pairer.functions(l39)
    out = {"about": "PAL's multiplayer executable's (I5BOOTN.ELF) functions and globals the port names, and "
                    "NTSC-U's, with the evidence: written by port/tools/hostgen/rac3_mp_pairs.py from both "
                    "discs; do not edit.",
           "functions": {f"func_{a:08X}": {"is": f"func_{b:08X}" if b else None, "why": why}
                         for a, (b, why) in sorted(functions.items())},
           "addresses": {f"0x{a:08X}": {"is": f"0x{v:08X}", "why": why} for a, (v, why) in sorted(data.items())},
           "level39": {f"func_L39_{a:08X}": {"is": f"func_L39_{b:08X}" if b else None, "why": why}
                       for a, (b, why) in sorted(out_l39.items())}}
    missing = [t for t in sorted(targets) if t not in data and t not in paired]
    for t in missing:
        out["addresses"][f"0x{t:08X}"] = {"is": None, "why": "no instruction of the same code forms it"}
    OUT.write_text(json.dumps(out, indent=1) + "\n")
    bad = [k for k, v in out["functions"].items() if v["is"] is None] + [k for k, v in out["addresses"].items()
                                                                          if v["is"] is None]
    print(f"{OUT.name}: {len(out['functions'])} functions, {len(out['addresses'])} addresses; not paired: "
          f"{', '.join(bad) or 'none'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

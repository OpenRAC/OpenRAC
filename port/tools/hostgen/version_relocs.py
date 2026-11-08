#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The values a rac1-decomp release's check proved, as release.py reads a build's (placed.json).

rac1-decomp builds every release of Ratchet & Clank from one C (games/rac1/decomp/docs/VERSIONS.md): its
names state PAL v2.00's addresses, and `tools/version_check.py --version K` places each C function
where release K has its counterpart, resolves every relocation in it (a call, a %hi/%lo pair, a
$gp offset) to the primary's address, translates that to K's and compares the result with K's
bytes. A function whose check says EXACT has therefore had every relocated field it holds proven
against the release's bytes: the symbol the field names is at that address in K's program.

This runs that check, in a built checkout of rac1-decomp (the primary's build, the levels' report
objects, `tools/version_src.py build --version K`, and K's dumps in baserom/<K>/), and writes, for
each program of K (the executable, each level), every symbol an EXACT function's relocations
named and the address K's bytes gave it:

    python3 port/tools/hostgen/version_relocs.py --source <rac1-decomp> --version pal_v1 --out build/rac1-pal1-relocs

<out>/boot/placed.json and <out>/NN/placed.json: {"values": {name: "0x..."}, "conflicts": {...}},
the form release.py reads for Going Commando's builds. A name two EXACT functions of one program
saw at two addresses (two copies of one function in a level, each called by its own callers) keeps
the address most fields gave, and the others are listed under "conflicts".
"""
from __future__ import annotations

import argparse
import collections
import json
import os
import re
import sys
from pathlib import Path

# An address name's stated address (its last eight digits).
ADDRESS = re.compile(r"(?:func|D|jtbl)_(?:[A-Z][A-Z0-9]{0,3}_)?(?:L\d\d_)?([0-9A-F]{8})")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, required=True, help="a built checkout of rac1-decomp")
    ap.add_argument("--version", required=True, help="the release's key (pal_v1, jp)")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args(argv)
    out = args.out.resolve()
    source = args.source.resolve()
    sys.path.insert(0, str(source / "tools"))
    os.chdir(source)
    import version_check as vc  # noqa: E402  (rac1-decomp's)
    import versions  # noqa: E402

    v = versions.get(args.version)
    seen: dict[str, dict[str, collections.Counter]] = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    placed: dict[str, dict[str, collections.Counter]] = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    fielded: dict[str, dict[str, collections.Counter]] = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    checked = collections.Counter()
    original = vc.check_one

    def program_of(sp) -> str:
        return "boot" if sp == "boot" else f"{int(sp):02d}"

    def check_one(v_, table, name, src, entry, show=False):
        result = original(v_, table, name, src, entry, show)
        checked[result[0]] += 1
        if result[0] != "match":
            return result
        # The program the check placed it in: its first place where the primary has it too.
        mine = sorted(entry["places"], key=lambda p: (-1 if p[0] == "boot" else p[0], p[1]))
        theirs = {p[0] for p in vc.primary_places().get(name, [])}
        sp, xa = next((p for p in mine if p[0] in theirs), mine[0])
        prog = program_of(sp)
        placed[prog][name][xa] += 1
        for kind, _offsets, sym, value, delta in getattr(vc.relocate, "fields", []):
            if not sym or sym.startswith("."):
                continue
            if delta == 0:
                # The field holds the symbol's own address: the release's bytes state it.
                seen[prog][sym][value & 0xFFFFFFFF] += 1
                continue
            # A field at an offset from the symbol (a record's member): the bytes state the member's
            # address. The symbol's own is that less the offset the C wrote, which the primary's
            # address of the member less the symbol's stated address gives.
            stated = ADDRESS.fullmatch(sym)
            member = table.inverse(sp, value & 0xFFFFFFFF) if stated else None
            if member is not None:
                fielded[prog][sym][(value - (member - int(stated.group(1), 16))) & 0xFFFFFFFF] += 1
            else:
                fielded[prog][sym][(value - delta) & 0xFFFFFFFF] += 1
        return result

    vc.check_one = check_one
    rows, counts = vc.run(v)
    out.mkdir(parents=True, exist_ok=True)
    total = 0
    for prog in sorted(set(seen) | set(placed) | set(fielded)):
        values, conflicts, members = {}, {}, []
        for sym, cnt in fielded[prog].items():
            if sym not in seen[prog]:
                # Only reached at an offset: the symbol's address rests on the C's offset as well.
                seen[prog][sym] = cnt
                members.append(sym)
        for sym, cnt in sorted(seen[prog].items()):
            (best, _), *rest = cnt.most_common()
            values[sym] = f"{best:#010x}"
            if rest:
                conflicts[sym] = [f"{a:#010x}" for a, _ in rest]
        for name, cnt in sorted(placed[prog].items()):
            values.setdefault(name, f"{cnt.most_common(1)[0][0]:#010x}")
        d = out / prog
        d.mkdir(exist_ok=True)
        (d / "placed.json").write_text(json.dumps({
            "about": f"rac1-decomp's check of {v.key} ({v.label}): every symbol an EXACT function of this "
                     "program names in a relocated field, and the address the release's bytes gave it "
                     "(port/tools/hostgen/version_relocs.py).",
            "values": values, "conflicts": conflicts, "by_member": sorted(members)}, indent=1) + "\n")
        total += len(values)
    print(f"version_relocs: {v.key}: {dict(counts)}; {total} values in {len(set(seen) | set(placed))} programs -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

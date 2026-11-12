#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Up Your Arsenal's NTSC-U release view of rac3-decomp (release.py's table, for rac3-ntsc).

rac3-decomp holds both discs in one tree (games/rac3/decomp/docs/build/versions.md): its C in src/ is
named by PAL's addresses (`func_0039B470`, `D_001DBC58`), and NTSC-U's own C, where the code
differs, is in src/us/ with names of its own (`func_U_0037D1A8`, `D_U_00227600`, NTSC-U
addresses). Its tables say where every name is on NTSC-U, without building anything:

- config/us/map.tsv: each PAL function, its program, and its NTSC-U name and place; its status
  (`match`: the PAL C is NTSC-U's code; `differs`: NTSC-U's own code is there, its C in src/us/
  when written; `none`: NTSC-U has no such function; `asm`, `near`, `nomatch`);
- config/us/names.tsv: the NTSC-U value of every other name the core programs' C uses (globals,
  their per-caller aliases `D_00142430_0039B470`, short names `D_142430`), one per program, as
  rac3-decomp's NTSC-U build read them from the disc's bytes.

From them this writes port/game/rac3-ntsc/release_names.json, the release table hostgen reads
("release" and "derived" in hostgen.json; release.py):

- every PAL function name renamed to its NTSC-U name; one NTSC-U does not have gets the `PAL`
  mark (`func_PAL_00123C20`: no address, so nothing reaches it); src/us/'s `func_U_`/`D_U_`
  names lose their mark;
- a PAL definition whose code is not NTSC-U's (status other than `match`) left out ("gone"),
  in its own program's files only ("gone_in": one PAL name can be a function of the executable
  and of the front end): NTSC-U's own C (src/us/) or its assembly takes its place;
- every plain global renamed to its NTSC-U address; the front end (frontbin, its own program
  over part of the executable) gets its own names per unit where its value differs from the
  executable's ("units");
- the names that state no address of their own (aliases, short names) as values;
- a name the C uses that neither table places (only C the NTSC-U build leaves as assembly
  uses it) gets the `PAL` mark, so what uses it becomes a stub instead of reading PAL's address.

    python3 port/tools/hostgen/release_rac3.py --game port/game/rac3-ntsc/hostgen.json \\
        --source games/rac3/decomp
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import release  # noqa: E402

PROGRAMS = ("boot_elf", "i5bootn", "frontbin")  # the executable's names first; frontbin's per unit
FRONT = ("src/frontbin", "src/us/frontbin", "nonmatching/frontbin")
# The resident engine core: the same addresses on both discs (rac3-decomp docs/versions.md).
RESIDENT = 0x1D5680
PLAIN = re.compile(r"^(func|D)_(?:([A-Z][A-Z0-9]{0,3})_)?([0-9A-F]{8})$")


def read_map(source: Path) -> list[list[str]]:
    rows = []
    for line in (source / "config/us/map.tsv").read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 7 or cols[1] not in PROGRAMS:
            continue
        rows.append(cols)
    return rows


def read_names(source: Path) -> dict[str, dict[str, int]]:
    out: dict[str, dict[str, int]] = {p: {} for p in PROGRAMS}
    for line in (source / "config/us/names.tsv").read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 3:
            continue
        out[cols[0]][cols[1]] = int(cols[2], 16)
    return out


def key(name: str) -> str | None:
    m = release.NAME.fullmatch(name)
    if not m:
        return None
    return release.core(m.group(1), int(m.group(3)) if m.group(3) else None, int(m.group(4), 16), m.group(2))


def units_of(source: Path, roots: list[str], skip: list[str]) -> dict[str, str]:
    """Every C file and header hostgen reads -> its program (frontbin or the executable)."""
    out = {}
    for root in roots:
        for path in sorted((source / root).rglob("*")):
            if path.suffix not in (".c", ".h") or not path.is_file():
                continue
            rel = path.relative_to(source).as_posix()
            if any(rel == s or rel.startswith(s.rstrip("/") + "/") for s in skip):
                continue
            out[rel] = "frontbin" if rel.startswith(tuple(f + "/" for f in FRONT)) else "boot_elf"
    return out


def build(source: Path, cfg: dict) -> dict:
    evidence: Counter = Counter()
    names: dict[str, str] = {}
    front: dict[str, str] = {}  # frontbin's own, where they differ from the executable's
    gone: list[str] = []
    gone_in: dict[str, list[str]] = {}

    def leave_out(pal: str, program: str) -> None:
        if pal not in gone:
            gone.append(pal)
        gone_in.setdefault(pal, []).extend(f"{root}/{program}/" for root in ("src", "src/us", "nonmatching"))
    rows = read_map(source)
    for pal, program, us, _place, _size, state, status, *_ in rows:
        k = key(pal)
        if k is None:
            continue
        new = us if us != "-" else pal.replace("func_", "func_PAL_", 1)
        table = front if program == "frontbin" else names
        if k in table:
            # A second row of one name: a near miss beside its twin's row, or another program of
            # the executable (i5bootn); the first is the function's place.
            if table[k] != new:
                evidence["function: a second row of one name (the first kept)"] += 1
            if state == "C" and status != "match":
                leave_out(pal, program)
            continue
        table[k] = new
        evidence[f"function: {status}"] += 1
        if state == "C" and status != "match":
            leave_out(pal, program)
    values: dict[str, dict[str, int]] = read_names(source)

    def plain_value(program: str, name: str) -> int | None:
        return values[program].get(name)

    data_names: dict[str, str] = {}
    data_front: dict[str, str] = {}
    for program in PROGRAMS:
        for name, value in values[program].items():
            m = PLAIN.match(name)
            if not m or m.group(1) != "D":
                continue
            new = f"D_{value:08X}"
            k = key(name)
            if program == "frontbin":
                data_front[k] = new
            elif k not in data_names:
                data_names[k] = new
            elif data_names[k] != new:
                evidence["global: two programs of the executable disagree (the first kept)"] += 1
    for k, new in data_names.items():
        if k not in names:
            names[k] = new
            evidence["global: renamed to its NTSC-U address"] += 1
    for k, new in data_front.items():
        if names.get(k) != new:
            front.setdefault(k, new)
            evidence["global: the front end's own address"] += 1
    # src/us/'s own names, with NTSC-U addresses already.
    us_own = set()
    for path in sorted((source / "src/us").rglob("*.c")):
        for m in release.NAME.finditer(path.read_text(errors="replace")):
            if m.group(2) == "U":
                us_own.add(m.group(0))
    for name in sorted(us_own):
        k = key(name)
        if k not in names:
            names[k] = name.replace("_U_", "_", 1)
            evidence["src/us: its own name, without the mark"] += 1

    # The names the C reads, and what it reads that nothing places on NTSC-U.
    roots = sorted(set(cfg.get("sources", ["src"]) + cfg.get("includes", ["include"])) - {"."})
    if cfg.get("candidates"):
        roots += ["nonmatching/boot_elf", "nonmatching/frontbin"]  # the candidates' files
    units = units_of(source, roots, cfg.get("skip", []))
    used: dict[str, set[str]] = {"boot_elf": set(), "frontbin": set()}
    gone_set = set(gone)  # (a name gone in one program only is still placed in the other)
    for rel, program in units.items():
        if not rel.endswith(".c"):
            continue  # a header's declarations are read only where a C file uses them
        for m in release.NAME.finditer((source / rel).read_text(errors="replace")):
            used[program].add(m.group(0))
    for program, words in used.items():
        mine = front if program == "frontbin" else {}
        for word in sorted(words):
            if word in gone_set:
                continue
            k = key(word)
            if k is None or k in mine or k in names:
                continue
            m = release.NAME.fullmatch(word)
            if m.group(2) or m.group(3) or int(m.group(4), 16) < RESIDENT:
                continue  # a tag or a level of its own, or the resident core: the same on both discs
            names[k] = word.replace("_", "_PAL_", 1)
            evidence["no NTSC-U place (marked)"] += 1

    table = release.Release({"names": names})
    unit_names = {}
    for rel, program in units.items():
        if program != "frontbin":
            continue
        own = {}
        text = (source / rel).read_text(errors="replace")
        for word in sorted({m.group(0) for m in release.NAME.finditer(text)}):
            k = key(word)
            if k in front:
                own[word] = front[k]
        if own:
            unit_names[rel] = own
    table.units = unit_names

    # Names with no address of their own (aliases, short names): their values, in the release's names.
    derived: dict[str, str] = {}
    for program in PROGRAMS:
        unit = next((u for u, p in units.items() if p == ("frontbin" if program == "frontbin" else "boot_elf")), None)
        for name, value in values[program].items():
            if PLAIN.match(name):
                continue
            renamed = table.text(name, unit if program == "frontbin" else None)
            text = f"0x{value:08X}"
            if renamed in derived and derived[renamed] != text:
                if program == "frontbin":
                    evidence["value: the executable's kept over the front end's"] += 1
                    continue
            derived[renamed] = text
            evidence["value: a name with no address of its own"] += 1
    return {"names": names, "units": unit_names, "gone": sorted(gone),
            "gone_in": {k: sorted(set(v)) for k, v in sorted(gone_in.items())}, "values": derived,
            "evidence": dict(sorted(evidence.items()))}


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game", type=Path, required=True, help="port/game/rac3-ntsc/hostgen.json")
    ap.add_argument("--source", type=Path, required=True, help="rac3-decomp (games/rac3/decomp)")
    args = ap.parse_args(argv)
    cfg = json.loads(args.game.read_text())
    source = args.source.resolve()
    parts = build(source, cfg)
    try:
        commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=source, capture_output=True, text=True,
                                timeout=15).stdout.strip() or None
    except (OSError, subprocess.TimeoutExpired):
        commit = None
    out = {
        "about": "The NTSC-U view of rac3-decomp: each PAL address name and NTSC-U's name for it, the PAL "
                 "definitions NTSC-U has other code for, and the values of the names with no address of "
                 "their own. Written by port/tools/hostgen/release_rac3.py from config/us/map.tsv and "
                 "config/us/names.tsv; do not edit.",
        "source": {"commit": commit, "version": "us", "table": "config/us/map.tsv, config/us/names.tsv"},
        **{k: parts[k] for k in ("evidence", "names", "units", "gone", "gone_in", "values")},
    }
    path = args.game.parent / cfg["release"]["table"]
    path.write_text(json.dumps(out, indent=1, sort_keys=False) + "\n")
    print(f"{path}: {len(parts['names'])} names, {len(parts['units'])} units, {len(parts['gone'])} gone, "
          f"{len(parts['values'])} values")
    for k, v in parts["evidence"].items():
        print(f"  {v:6d}  {k}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

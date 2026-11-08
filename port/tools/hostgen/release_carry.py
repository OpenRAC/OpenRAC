#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""A release's port tables, carried from the primary release's (release.py).

The primary release's port (port/game/rac2-ntsc) names what it knows by v1.01's functions and
addresses: the functions the host replaces, the library entry points, the level hooks, the
folded copies, the copies of the draw lists. This writes the same tables for another release
(port/game/rac2-pal, rac2-ntsc2) in that release's names, from:

- the release table (release.py build): a function of the same code has its new name there;
- the decompilation's carry (`build/<version>/carry.json`, tools/carry.py): a function the
  release changed at the same size and offsets (60 against 50 Hz) is paired with the v1.01
  function it was;
- versionmap's map (port/tools/versionmap): a function between the same unchanged neighbours
  (`near`) or reached by the same call of shared code (`called`), and a global formed by the
  same instruction of the same code (`exact`) or between two that moved alike (`between`).

Everything that rests on less than the same code is listed in the carry's report
(`release_carry.json` beside the tables), with how it was paired, so it can be read again:
a host replacement of a changed function may need the release's own fields.

    python3 port/tools/hostgen/release_carry.py --game port/game/rac2-pal/hostgen.json \\
        --primary port/game/rac2-ntsc/hostgen.json --source <the built decompilation> --map <versionmap>
"""
from __future__ import annotations

import argparse
import bisect
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import release as rel  # noqa: E402

FUNC = re.compile(r"(?<![\w])func_(?:L(\d\d)_)?([0-9A-F]{8})((?:_\w+)?)(?![\w])")


class Pairing:
    """Where the release has what v1.01 has."""

    def __init__(self, source: Path, version: str, table: rel.Release, vmap: dict | None, carry: dict | None):
        self.table, self.vmap, self.version, self.source = table, vmap, version, source
        info = rel.RELEASES[version]
        self.us_boot = rel.read_boot(source / "config/boot/functions.tsv")
        self.us_levels = rel.read_levels(source / "config/levels/functions.tsv")
        self.r_boot = rel.read_boot(source / info["config"] / "boot/functions.tsv")
        self.r_levels = rel.read_levels(source / info["config"] / "levels/functions.tsv")
        self.us_entry = {n: a for n, a, _ in self.us_boot}
        self.us_size = {n: s for n, _, s in self.us_boot}
        self.us_size.update({n: s for n, _, s, _ in self.us_levels})
        self.us_places = {n: p for n, _, _, p in self.us_levels}
        self.r_places = {table.name(n): p for n, _, _, p in self.r_levels}
        self.r_entry = {table.name(n): a for n, a, _ in self.r_boot}
        self.r_size = {table.name(n): s for n, _, s in self.r_boot}
        self.r_size.update({table.name(n): s for n, _, s, _ in self.r_levels})
        self.r_exe_at = {a: table.name(n) for n, a, _ in self.r_boot}
        self.r_level_at = {}
        for n, _, _, places in self.r_levels:
            for p in places:
                self.r_level_at.setdefault(p, table.name(n))
        self.us_level_rows: dict[int, list[tuple[int, int, str]]] = {}
        for n, _, s, places in self.us_levels:
            for lv, a in places:
                self.us_level_rows.setdefault(lv, []).append((a, s, n))
        for rows in self.us_level_rows.values():
            rows.sort()
        # tools/carry.py's pairs: the release's function -> the v1.01 functions it was ("changed").
        self.was: dict[str, tuple[str, str]] = {}
        for part in ("boot", "levels"):
            for name, row in (carry or {}).get(part, {}).items():
                how = row.get("how")
                hows = how if isinstance(how, list) else [how]
                was = row.get("was")
                was = was if isinstance(was, list) else [was] if was else []
                # The carry's own names are its run's; the committed catalogue's row at the same place
                # is the release's function.
                m = FUNC.fullmatch(name.replace(f"_{rel.RELEASES[version]['tag']}_", "_", 1))
                if not re.search(r"[0-9A-F]{8}$", name):
                    continue
                if part == "boot":
                    address = row.get("address", int(name[-8:], 16))
                    here = self.r_exe_at.get(address)
                else:
                    lv = int(m.group(1)) if m and m.group(1) else None
                    here = self.r_level_at.get((lv, int(name[-8:], 16))) if lv is not None else None
                if here is None:
                    continue
                for w in was:
                    if "changed" in hows and w:
                        self.was.setdefault(w, (here, "changed"))
        self.report: dict[str, dict] = {}
        self.kinds: Counter = Counter()
        self.by_hand: dict[str, str] = {}

    def read_by_hand(self, path: Path) -> None:
        """release_pairs.tsv beside the release's hostgen.json: pairs checked by hand where nothing
        above pairs them (v1.01 name, the release's name in the release's names, the evidence)."""
        if not path.is_file():
            return
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 3:
                continue
            self.by_hand[cols[0]] = cols[1]

    def _starts(self, level: int | None) -> set[int]:
        """v1.01's catalogue places in the executable (None) or a level."""
        if level is None:
            return set(self.us_entry.values())
        return {a for a, _, _ in self.us_level_rows.get(level, [])}

    # ---- functions

    def us_place(self, name: str) -> tuple[str, int] | None:
        """(versionmap program, address) of a v1.01 function."""
        m = FUNC.fullmatch(name)
        if m is None:
            return None
        if m.group(1):
            lv = int(m.group(1))
            places = self.us_places.get(name.removesuffix(m.group(3)), [])
            a = next((a for l, a in places if l == lv), int(m.group(2), 16))
            return f"level:{lv:02d}", a
        base = name.removesuffix(m.group(3))
        return "boot", self.us_entry.get(base, int(m.group(2), 16))

    def function(self, name: str, why: str = "") -> str | None:
        """The release's name for v1.01's function NAME (a suffix kept), or None."""
        m = FUNC.fullmatch(name)
        if m is None:
            return name
        suffix = m.group(3)
        base = name[:len(name) - len(suffix)] if suffix else name
        new = self.table.name(base)
        how = None
        if base in self.by_hand:
            new, how = self.by_hand[base], "by hand"
        elif "_V1_" not in new and (new in self.r_entry or new in self.r_places):
            how = "same"
        elif base in self.was:
            new, how = self.was[base]
        elif self.vmap is not None and (at := self.us_place(base)) is not None:
            hit = self.vmap["programs"].get(at[0], {}).get("functions", {}).get(f"{at[1]:08X}")
            if hit:
                address = int(hit[0], 16)
                lv = None if at[0] == "boot" else int(at[0][6:])
                found = self.r_exe_at.get(address) if lv is None else self.r_level_at.get((lv, address))
                if found is not None and lv is None and int(found[-8:], 16) != address and base not in self.us_entry:
                    # v1.01's name is an entry, not a row of its catalogue (a fill word in front), and
                    # the release's row is entered there too: the entry's own name.
                    found = rel.core("func", None, address)
                if found is None and at[1] not in self._starts(lv):
                    # An entry inside a function (the linker's fill word before it), found by its
                    # instructions or by the calls that reach it: its address.
                    found = rel.core("func", lv, address)
                if found is not None:
                    new, how = found, hit[1]
        if how is None:
            self.kinds["function: not found"] += 1
            self.report.setdefault("unpaired", {})[name] = why
            return None
        self.kinds[f"function: {how}"] += 1
        if how != "same":
            self.report.setdefault("functions", {})[name] = [new + suffix, how, why]
        return new + suffix

    # ---- globals

    def data(self, address: int, program: str = "boot", why: str = "") -> int | None:
        """The release's address of v1.01's global at ADDRESS (in PROGRAM: "boot", "level:NN")."""
        level_part = program != "boot" and address >= rel.RELEASES["us"]["level_base"]
        name = rel.core("D", int(program[6:]) if level_part else None, address)
        new = self.table.names.get(name)
        if new and "_V1_" not in new:
            self.kinds["global: the release's build"] += 1
            self.last = "the release's build"
            return int(new[-8:], 16)
        if self.vmap is not None:
            keys, values = self._data_index(program)
            at = bisect.bisect_right(keys, address)
            if at and keys[at - 1] == address:
                self.kinds["global: exact"] += 1
                self.last = "versionmap: exact"
                return values[at - 1]
            got = self.aligned(program).get(address)
            if got is not None:
                self.kinds["global: aligned"] += 1
                self.last = "the same instruction of the paired function"
                return got
            below = (keys[at - 1], values[at - 1]) if at else None
            above = (keys[at], values[at]) if at < len(keys) else None
            if below and above and below[1] - below[0] == above[1] - above[0]:
                self.kinds["global: between"] += 1
                self.last = "versionmap: between"
                self.report.setdefault("globals", {})[f"{program}:{address:08X}"] = [
                    f"{address + below[1] - below[0]:08X}", "between", why]
                return address + below[1] - below[0]
        self.kinds["global: not found"] += 1
        self.report.setdefault("unpaired", {})[f"{program}:{address:08X}"] = why
        return None

    def code(self, version: str, program: str):
        """A program's loaded sections, [(address, bytes)]: the executable's, or a level's dump."""
        import copycalls
        cache = self.__dict__.setdefault("_code", {})
        key = (version, program)
        if key not in cache:
            info = rel.RELEASES[version]
            base = self.source / info["baserom"]
            if program == "boot":
                exe = {"us": "SCUS_972.68", "pal": "SCES_516.07", "us2": "SCUS_972.68"}[version]
                cache[key] = copycalls.elf_sections(base / exe)
            else:
                lv = program[6:]
                folder = next((d for d in sorted((base / "levels").iterdir()) if d.name[:2] == lv), None)
                cache[key] = copycalls.level_sections(folder) if folder else []
        return cache[key]

    def aligned(self, program: str) -> dict[int, int]:
        """v1.01's globals formed by an instruction of a function versionmap pairs (the same, or
        changed between unchanged neighbours) and the release's formed by the same instruction:
        where every such pair agrees."""
        import copycalls
        import difflib
        cache = self.__dict__.setdefault("_aligned", {})
        if program in cache:
            return cache[program]
        votes: dict[int, Counter] = {}
        gp_us, gp_r = rel.RELEASES["us"]["gp"], rel.RELEASES[self.version]["gp"]
        us_code, r_code = self.code("us", program), self.code(self.version, program)
        for a, row in (self.vmap or {}).get("programs", {}).get(program, {}).get("functions", {}).items():
            if row[1] not in ("same", "near") or not row[2] or not row[3]:
                continue
            mine = copycalls.words(us_code, int(a, 16), row[2] & ~3)
            theirs = copycalls.words(r_code, int(row[0], 16), row[3] & ~3)
            if mine is None or theirs is None:
                continue
            fa, fb = copycalls.formed(mine, gp_us), copycalls.formed(theirs, gp_r)
            if len(mine) == len(theirs):
                pairs = [(i, i) for i in fa]
            else:
                sm = difflib.SequenceMatcher(None, [copycalls.masked(w) >> 26 for w in mine],
                                             [copycalls.masked(w) >> 26 for w in theirs], autojunk=False)
                index = {}
                for tag, i1, i2, j1, j2 in sm.get_opcodes():
                    if tag == "equal":
                        index.update(zip(range(i1, i2), range(j1, j2)))
                pairs = [(i, index[i]) for i in fa if i in index]
            for i, j in pairs:
                if j in fb and (mine[i] >> 26) == (theirs[j] >> 26):
                    votes.setdefault(fa[i], Counter())[fb[j]] += 1
        out = {}
        for x, c in votes.items():
            y, n = c.most_common(1)[0]
            if n == sum(c.values()):
                out[x] = y
        cache[program] = out
        return out

    def _data_index(self, program: str):
        cache = getattr(self, "_index", {})
        if program not in cache:
            d = self.vmap["programs"].get(program, {}).get("data", {})
            keys = sorted(int(k, 16) for k in d)
            cache[program] = (keys, [int(d[f"{k:08X}"], 16) for k in keys])
            self._index = cache
        return cache[program]

    def code_place(self, level: int, address: int, why: str = "") -> int | None:
        """The release's address of a place inside a v1.01 level function (a copy entered late)."""
        rows = self.us_level_rows.get(level, [])
        i = bisect.bisect_right(rows, (address, 1 << 40, "")) - 1
        if i < 0 or not rows[i][0] <= address < rows[i][0] + rows[i][1]:
            return None
        start, _, name = rows[i]
        new = self.function(name, why)
        if new is None:
            return None
        place = next((a for l, a in self.r_places.get(new, []) if l == level), None)
        return None if place is None else place + address - start


def carry_list(p: Pairing, names: list[str], why: str) -> list[str]:
    out = []
    for n in names:
        new = p.function(n, why)
        if new is not None and new not in out:
            out.append(new)
    return out


def level_hooks(p: Pairing, carried: list[str]) -> list[str]:
    """The release's level hooks: the empty functions of its executable each level's program fills.
    They are alike, so the calls that reach them, not their order, say which is which; the release
    has more of them (PAL: 33 for 31). All the 8-byte functions of the run that holds the carried
    ones, in address order."""
    rows = sorted((a, s, n) for n, a, s in ((p.table.name(n), a, s) for n, a, s in p.r_boot))
    at = {n: i for i, (_, _, n) in enumerate(rows)}
    found = sorted(at[n] for n in carried if n in at)
    if not found:
        return carried
    first, last = found[0], found[-1]
    while first > 0 and rows[first - 1][1] == 8 and rows[first - 1][0] + 8 == rows[first][0]:
        first -= 1
    while last + 1 < len(rows) and rows[last + 1][1] == 8 and rows[last][0] + 8 == rows[last + 1][0]:
        last += 1
    hooks = [n for a, s, n in rows[first:last + 1] if s == 8]
    p.report["level_hooks"] = {"from": f"{rows[first][0]:08X}", "to": f"{rows[last][0]:08X}", "count": len(hooks),
                               "carried": len(found)}
    return hooks


def carry_config(p: Pairing, cfg: dict, version: str) -> dict:
    """The release's hostgen.json from the primary release's."""
    info = rel.RELEASES[version]
    out = dict(cfg)
    names = {"pal": "PAL", "us2": "NTSC-U, v2.00"}
    out["id"] = {"pal": "rac2-pal", "us2": "rac2-ntsc2"}[version]
    out["title"] = f"Ratchet & Clank: Going Commando ({names[version]})"
    out["serial"] = {"pal": "SCES_516.07", "us2": "SCUS_972.68"}[version]
    out["frame_rate"] = 50 if version == "pal" else 60
    out["entry"] = p.function(cfg["entry"], "entry")
    out["ps2_float"] = carry_list(p, cfg.get("ps2_float", []), "ps2_float")
    other = {"pal": "src/us2", "us2": "src/pal"}[version]
    out["skip"] = [other if s in ("src/pal", "src/us2") else s for s in cfg.get("skip", [])]
    out["skip"] = list(dict.fromkeys(out["skip"]))
    out["symbols"] = []
    out["derived"] = {"table": "release_names.json"}
    out["release"] = {"version": version, "table": "release_names.json"}
    out["places"] = f"{info['config']}/levels/functions.tsv"
    out["executable_functions"] = f"{info['config']}/boot/functions.tsv"
    if "formed_as" in cfg:
        formed = {}
        primary = json.loads((p.primary_dir / cfg["derived"]["table"]).read_text())
        for name, address in cfg["formed_as"].items():
            # The sum the code forms, less the name's value: the same C adds the same, so the release's
            # sum is its own value plus that.
            was = primary.get("values", {}).get(name) or (primary.get("levels", {}).get(name) or [None, None])[1]
            mine = p.table.table.get("values", {}).get(name) or (p.table.table.get("levels", {}).get(name) or [None, None])[1]
            new = None
            if was is not None and mine is not None:
                new = int(mine, 16) + int(address, 16) - int(was, 16)
                p.report.setdefault("globals", {})[name] = [f"{new:08X}", "its value plus what the C adds", "formed_as"]
            else:
                m = re.match(r"ref_L(\d\d)_", name)
                new = p.data(int(address, 16), f"level:{m.group(1)}" if m else "boot", f"formed_as {name}")
            if new is not None:
                formed[name] = f"0x{new:08X}"
        out["formed_as"] = formed
    # By the decompilation's names (read_candidates compares the table's rows before the release view):
    # the primary's, and those the release table finds naming what the release does not have.
    out["candidates_left_out"] = {**cfg.get("candidates_left_out", {}),
                                  **p.table.table.get("candidates_left_out", {})}
    calls = {}
    for key, table in cfg.get("calls_as", {}).items():
        k2 = key if re.fullmatch(r"L\d\d", key) else p.function(key, "calls_as")
        if k2 is None:
            continue
        row = {}
        for a, b in table.items():
            a2 = p.table.name(a) if FUNC.fullmatch(a) else a
            b2 = p.function(b, f"calls_as {key}")
            if b2 is not None:
                row[a2] = b2
        calls[k2] = row
    out["calls_as"] = calls
    more = {}
    for name, text in cfg.get("more_places", {}).items():
        new = p.function(name, "more_places")
        places = []
        for item in text.split(","):
            lv, a = item.split(":")
            got = p.code_place(int(lv), int(a, 16), f"more_places {name}")
            if got is not None:
                places.append(f"{lv}:{got:08X}")
        if new and places:
            more[new] = ",".join(places)
    out["more_places"] = more
    units = {}
    for unit, table in cfg.get("unit_aliases", {}).items():
        own = p.table.units.get(unit, {})
        row = {}
        for a, b in table.items():
            if a in own:
                continue        # the release table names it already, from the release's build
            a2 = p.table.name(a)
            b2 = p.function(b, f"unit_aliases {unit}")
            if b2 is not None and a2 != b2:
                row[a2] = b2
        if row:
            units[unit] = row
    out["unit_aliases"] = units
    out["relocate"] = [info["level_base"], cfg["relocate"][1]]
    out["level_hooks"] = level_hooks(p, carry_list(p, cfg.get("level_hooks", []), "level_hooks"))
    out["host_functions"] = carry_list(p, cfg.get("host_functions", []), "host_functions")
    out["from_assembly"] = carry_list(p, cfg.get("from_assembly", []), "from_assembly")
    out["roots"] = carry_list(p, cfg.get("roots", []), "roots")
    out["returns_64"] = carry_list(p, cfg.get("returns_64", []), "returns_64")
    if "asm" in cfg:
        asm = dict(cfg["asm"])
        asm["signatures"] = {n2: sig for n, sig in cfg["asm"].get("signatures", {}).items()
                             if (n2 := (p.function(n, "asm.signatures") if FUNC.fullmatch(n) else n))}
        asm["gp"] = f"0x{info['gp']:08X}"
        asm["retail"] = {"exe": f"{info['baserom']}/{out['serial']}", "dump": f"{info['baserom']}/levels"}
        out["asm"] = asm
    out.pop("about", None)
    return {"about": "Written by port/tools/hostgen/release_carry.py from port/game/rac2-ntsc/hostgen.json "
                     "and the release table; do not edit by hand (release_carry.json says how each name was paired).",
            **out}


PROTO = re.compile(r"^(?P<ret>[\w ]+?)\s*\b(?P<name>func_\w+)\((?P<params>[^)]*)\);$", re.M)


def add_signatures(p: Pairing, new_cfg: dict, cfg: dict, libraries: str, protos: str) -> None:
    """asm.signatures for the functions the port's host C replaces or wraps (libraries.tsv's host and
    wrap rows, host_functions) where this release has its own code, as v1.01's C declares them: the
    release's are translated from their assembly, and its callers' declarations need not be the
    host's (`int f()` where the host defines `void f(gaddr)`)."""
    known = {m.group("name"): m for m in PROTO.finditer(protos)}
    # Only for what this release has as assembly alone (asm.signatures is for those): its markers,
    # less what its C defines, in the release's names.
    other = {"pal": "src/us2", "us2": "src/pal"}[p.version]
    marked, defined = set(), set()
    for path in (p.source / "src").rglob("*.c"):
        if path.relative_to(p.source).as_posix().startswith(other + "/"):
            continue
        text = path.read_text(errors="replace")
        marked.update(p.table.name(n) for n in rel.INCLUDED.findall(text))
        defined.update(p.table.name(n) for n in rel.DEFINED.findall(text))
    asm_only = marked - defined
    names = [row.split("\t")[0] for row in libraries.splitlines()
             if not row.startswith("#") and len(row.split("\t")) > 3 and row.split("\t")[3] in ("host", "wrap")]
    names += cfg.get("host_functions", [])
    sigs = new_cfg.setdefault("asm", {}).setdefault("signatures", {})
    for name in names:
        if name not in p.report.get("functions", {}) or name not in known:
            continue                    # the same code: v1.01's C is the release's
        new = p.function(name, "signature")
        m = known[name]
        params = [] if m.group("params").strip() in ("", "void") else [
            re.sub(r"\s+a\d+$", "", x.strip()) for x in m.group("params").split(",")]
        params = ["void *" if x == "gaddr" else x for x in params]
        ret = "void *" if m.group("ret").strip() == "gaddr" else m.group("ret").strip()
        if new is not None and new in asm_only:
            sigs.setdefault(new, f"{ret} ({', '.join(params) or 'void'})")
            p.kinds["signature: v1.01's"] += 1


def carry_libraries(p: Pairing, text: str) -> str:
    lines = ["# This release's copy of port/game/rac2-ntsc/libraries.tsv, written by port/tools/hostgen/release_carry.py:",
             "# the same rows by this release's names (release_carry.json says how each was paired); do not edit.",
             "# The primary's notes follow."]
    for line in text.splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 2:
            lines.append(line)
            continue
        new = p.function(cols[0], f"libraries.tsv {cols[2] if len(cols) > 2 else ''}")
        if new is None:
            lines.append(f"# not in this release: {line}")
            continue
        lines.append("\t".join([new] + cols[1:]))
    return "\n".join(lines) + "\n"


def carry_split_places(p: Pairing, text: str) -> str:
    lines = ["# This release's copy of port/game/rac2-ntsc/split_places.tsv, by its names (release_carry.py).", "#"]
    for line in text.splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 3:
            lines.append(line)
            continue
        variant, parent = p.function(cols[0], "split_places"), p.function(cols[1], "split_places")
        if variant is None or parent is None:
            lines.append(f"# not in this release: {line}")
            continue
        places = ",".join(f"{lv:02d}:{a:08X}" for lv, a in p.r_places.get(variant, []))
        lines.append("\t".join([variant, parent, places] + cols[3:]))
    return "\n".join(lines) + "\n"


HOST_FUNC = re.compile(r"\bfunc_(?:L\d\d_)?[0-9A-F]{8}((?:_\w+)?)\b")
HOST_ADDR = re.compile(r"\bRAC2_ADDR\(\s*0x([0-9A-F]{8})u\s*\)")
HOST_LADDR = re.compile(r"\bRAC2_LADDR\(\s*(\d\d)\s*,\s*0x([0-9A-F]{8})u\s*\)")
HOST_SKIP = re.compile(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', re.S)


def code_text(text: str) -> str:
    """TEXT without comments and string literals."""
    return HOST_SKIP.sub(" ", text)


def exe_bytes(path: Path, address: int, size: int) -> bytes | None:
    import struct
    data = path.read_bytes()
    table, count = struct.unpack_from("<I", data, 28)[0], struct.unpack_from("<H", data, 44)[0]
    for i in range(count):
        kind, offset, vaddr, _, filesz = struct.unpack_from("<5I", data, table + i * 32)
        if kind == 1 and vaddr <= address and address + size <= vaddr + filesz:
            return data[offset + address - vaddr:offset + address - vaddr + size]
    return None


def exe_size(path: Path) -> int:
    """The executable's loaded bytes from 0x00100000, to a page (its first program header's end)."""
    import struct
    data = path.read_bytes()
    table = struct.unpack_from("<I", data, 28)[0]
    _, _, vaddr, _, _, memsz = struct.unpack_from("<6I", data, table)
    end = vaddr + memsz
    return ((end + 0xFFF) & ~0xFFF) - 0x00100000


HOST_K = re.compile(r"\bRAC2_K\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)u?\s*,\s*((?:func|D)_\w+)\s*\)")
IMMEDIATE_OPS = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x18, 0x19, 0x1E, 0x1F, 0x20, 0x21, 0x23, 0x24,
                 0x25, 0x27, 0x28, 0x29, 0x2B, 0x31, 0x37, 0x39, 0x3F}


def constants(p: Pairing, code: dict[str, str], version: str) -> list[str]:
    """The shared C's RAC2_K(name, value, where) in this release: the immediate the same instruction
    of `where`'s pair here has (where v1.01's has value), or, for a table D_..., the word at the same
    index of the release's table."""
    import copycalls
    import difflib
    found = {}
    for t in code.values():
        for m in HOST_K.finditer(t):
            found.setdefault(m.group(1), (int(m.group(2), 0), m.group(3)))
    if not found:
        return []
    out = ["/* Constants of the game's code (RAC2_K): the same instruction's in this release. */"]
    by_hand = {}
    path = p.out_dir / "release_constants.tsv"
    if path.is_file():
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if not line.startswith("#") and len(cols) >= 3:
                by_hand[cols[0]] = (int(cols[1], 0), cols[2])
    us_boot, r_boot = p.code("us", "boot"), p.code(version, "boot")
    for name, (value, where) in sorted(found.items()):
        new, how = None, None
        if name in by_hand:
            new, how = by_hand[name][0], f"by hand: {by_hand[name][1]}"
        elif where.startswith("D_"):
            table = int(where[-8:], 16)
            mine = copycalls.words(us_boot, table, 256) or ()
            k = next((i for i, w in enumerate(mine) if w == value), None)
            there = p.data(table, "boot", f"RAC2_K {name}")
            theirs = copycalls.words(r_boot, there, 256) if there is not None else None
            if k is not None and theirs:
                new, how = theirs[k], f"word {k} of the table at 0x{there:08X} (v1.01's 0x{table:08X})"
        else:
            pair = p.function(where, f"RAC2_K {name}")
            if pair is not None and where in p.us_entry and pair in p.r_entry:
                a, b = p.us_entry[where], p.r_entry[pair]
                mine = copycalls.words(us_boot, a, p.us_size[where] & ~3)
                theirs = copycalls.words(r_boot, b, p.r_size[pair] & ~3)
                if mine and theirs:
                    sm = difflib.SequenceMatcher(None, [w >> 26 or w & 0x3F for w in mine],
                                                 [w >> 26 or w & 0x3F for w in theirs], autojunk=False)
                    seen = Counter()
                    for tag, i1, i2, j1, j2 in sm.get_opcodes():
                        if tag != "equal":
                            continue
                        for i, j in zip(range(i1, i2), range(j1, j2)):
                            x, y = mine[i], theirs[j]
                            if x >> 26 in IMMEDIATE_OPS and x >> 26 == y >> 26 and x & 0xFFFF == value & 0xFFFF:
                                seen[(value & ~0xFFFF) | (y & 0xFFFF)] += 1
                    if len(seen) == 1:
                        new = next(iter(seen))
                        how = f"{where} as {pair}: {sum(seen.values())} instructions"
        if new is None:
            out.append(f"/* RAC2_K_{name}: not found */")
            p.report.setdefault("unpaired", {})[f"RAC2_K {name}"] = f"0x{value:X} in {where}"
            continue
        out.append(f"#define RAC2_K_{name} 0x{new:X} /* v1.01 0x{value:X}; {how} */")
    return out


def host_table(p: Pairing, shared: Path, primary_host: Path, source: Path, version: str) -> str:
    """The release's rac2_version.h for the shared host C (port/game/rac2/host/rac2.h)."""
    info = rel.RELEASES[version]
    texts = {f.name: f.read_text() for f in sorted(shared.glob("*.[ch]"))}
    code = {n: code_text(t) for n, t in texts.items()}
    lines = []
    funcs: dict[str, set[str]] = {}
    for t in code.values():
        for m in HOST_FUNC.finditer(t):
            base = m.group(0)[:len(m.group(0)) - len(m.group(1))] if m.group(1) else m.group(0)
            funcs.setdefault(base, set()).add(m.group(1))
    # Functions: v1.01's name and this release's, a table the build renames the shared C with
    # (release_host.py: one pass, so that no name is renamed twice, as macros would).
    renames = ["# The shared host C's function names (v1.01's) and this release's. Written by",
               "# port/tools/hostgen/release_carry.py (release_carry.json says how each was paired); do not edit."]
    for base in sorted(funcs):
        new = p.function(base, "host C")
        if new is None:
            renames.append(f"# {base}: not in this release")
            continue
        new = "func" + new[3:] if new.startswith("FUN_") else new
        if new == base:
            continue
        for suffix in sorted(funcs[base]):
            renames.append(f"{base}{suffix}\t{new}{suffix}")
    p.host_names = "\n".join(renames) + "\n"
    addrs, laddrs = set(), set()
    for t in code.values():
        addrs.update(int(m.group(1), 16) for m in HOST_ADDR.finditer(t))
        laddrs.update((int(m.group(1)), int(m.group(2), 16)) for m in HOST_LADDR.finditer(t))
    us_exe = source / "baserom/SCUS_972.68"
    r_exe = source / info["baserom"] / {"pal": "SCES_516.07", "us2": "SCUS_972.68"}[version]
    starts = {a: n for n, a in p.us_entry.items()}
    lines.append("")
    lines.append("/* Addresses of the executable's memory. */")
    for a in sorted(addrs):
        new, how = None, None
        if a in starts or a - 8 in starts or a - 4 in starts:
            name = starts.get(a) or rel.core("func", None, a)
            got = p.function(name if a in starts else rel.core("func", None, a), f"host C 0x{a:08X}")
            if got is not None:
                new, how = int(got[-8:], 16), "code"
        if new is None and 0x00100000 <= a < 0x00115220:
            # The VU microcode the game uploads (.vutext): the same bytes at the same place?
            mine, theirs = exe_bytes(us_exe, a, 64), exe_bytes(r_exe, a, 64)
            if mine is not None and mine == theirs:
                new, how = a, "the same microcode at the same place"
        if new is None:
            new = p.data(a, "boot", f"host C 0x{a:08X}")
            how = p.last
        if new is None:
            lines.append(f"/* RAC2_ADDR_0x{a:08X}u: not found */")
            continue
        lines.append(f"#define RAC2_ADDR_0x{a:08X}u 0x{new:08X}u /* {how} */")
    if laddrs:
        lines.append("")
        lines.append("/* Addresses of a level's program. */")
    for lv, a in sorted(laddrs):
        new = p.data(a, f"level:{lv:02d}", f"host C level {lv:02d} 0x{a:08X}")
        if new is None:
            lines.append(f"/* RAC2_LADDR_{lv:02d}_0x{a:08X}u: not found */")
            continue
        lines.append(f"#define RAC2_LADDR_{lv:02d}_0x{a:08X}u 0x{new:08X}u /* {p.last} */")
    lines += constants(p, code, version)
    names = {"pal": "PAL (SCES-51607)", "us2": "USA v2.00 (SCUS-97268, Greatest Hits)"}[version]
    head = [
        "/* Written by port/tools/hostgen/release_carry.py from the shared host C and this release's maps. Do not edit. */",
        "/* SPDX-License-Identifier: GPL-3.0-or-later",
        " * Copyright (c) 2026 the OpenRAC contributors",
        " *",
        f" * {names}: its names and addresses for the shared host C (port/game/rac2/host/rac2.h), v1.01's",
        " * names and addresses as this release has them. Written by port/tools/hostgen/release_carry.py",
        " * from the shared C, the release table and versionmap's map; do not edit. */",
        "#ifndef OPENRAC_RAC2_VERSION_H",
        "#define OPENRAC_RAC2_VERSION_H",
        "",
        "#define RAC2_ADDR(a) RAC2_ADDR_##a",
        "#define RAC2_LADDR(n, a) RAC2_LADDR_##n##_##a",
        "#define RAC2_K(name, value, where) RAC2_K_##name",
        "",
        f"#define RAC2_LEVEL_BASE 0x{info['level_base']:08X}u /* a level's program replaces the executable's memory from here */",
        f"#define RAC2_GP 0x{info['gp']:08X}u         /* the executable's $gp */",
        f"#define RAC2_EXE_SIZE 0x{exe_size(r_exe):08X}u   /* the executable's loaded bytes from 0x00100000, to a page */",
        "",
        '#include "rac2_addresses.h"',
        "",
    ]
    return "\n".join(head + lines + ["", "#endif /* OPENRAC_RAC2_VERSION_H */", ""])


def addresses_header(p: Pairing, primary: str, version: str) -> str:
    """The release's rac2_addresses.h: the primary's names with this release's addresses."""
    out = []
    for line in primary.splitlines():
        m = re.match(r"(#define RAC2_\w+\s+)0x([0-9A-F]{8})u(.*)$", line)
        if m:
            new = p.data(int(m.group(2), 16), "boot", f"rac2_addresses.h {m.group(1).split()[1]}")
            line = f"{m.group(1)}0x{new:08X}u{m.group(3)}" if new is not None else f"/* not found: {line} */"
        elif re.match(r"\s+0x([0-9A-F]{8})u", line) and out and out[-1].rstrip().endswith("\\"):
            mm = re.match(r"(\s+)0x([0-9A-F]{8})u(.*)$", line)
            new = p.data(int(mm.group(2), 16), "boot", "rac2_addresses.h")
            line = f"{mm.group(1)}0x{new:08X}u{mm.group(3)}" if new is not None else line
        out.append(line)
    text = ("/* Written by port/tools/hostgen/release_carry.py from rac2-ntsc's rac2_addresses.h. Do not edit. */\n"
            + "\n".join(out) + "\n")
    return text.replace("(games/rac2/decomp/symbol_addrs/usa/symbol_addrs.txt)",
                        "(games/rac2/decomp/symbol_addrs/usa/symbol_addrs.txt), at this release's addresses "
                        "(port/tools/hostgen/release_carry.py)")


def profile_table(p: Pairing, profile: str, version: str) -> str:
    """The release's table for port/game/rac2/profile.cpp: v1.01's kNtsc, each address as this
    release has it, with how it was found."""
    block = profile[profile.index("constexpr Release kNtsc{"):]
    block = block[:block.index("\n};\n") + 4]
    ident = {"pal": "kPal", "us2": "kNtsc2"}[version]
    out = []
    section = None
    level = -1
    for line in block.splitlines():
        if line.startswith("constexpr Release kNtsc{"):
            out.append(f"constexpr Release {ident}{{")
            section = "boot"
            continue
        if line.strip() == "{":
            section = "levels"
            out.append(line)
            continue
        if section == "levels" and line.strip() == "},":
            section = "rest"
            out.append(line)
            continue
        if section == "levels":
            level += 1
        program = f"level:{level:02d}" if section == "levels" else "boot"
        notes = []

        def one(m: re.Match) -> str:
            a = int(m.group(1), 16)
            new = p.data(a, program, f"profile {program} 0x{a:08X}")
            if new is None:
                notes.append(f"0x{a:08X} not found")
                return m.group(0)
            notes.append(p.last)
            return f"0x{new:08X}{m.group(2)}"
        line = re.sub(r"\b0x([0-9A-Fa-f]{8})(u?)\b", one, line)
        if notes and section != "levels":
            kinds = sorted(set(notes))
            line = line.split("//")[0].rstrip() + f"  // {', '.join(kinds)}"
        elif notes:
            kinds = sorted(set(notes))
            line = line.rstrip() + f" ({', '.join(kinds)})"
        out.append(line)
    names = {"pal": "PAL (SCES-51607)", "us2": "USA v2.00 (SCUS-97268, Greatest Hits)"}[version]
    return "\n".join([
        "// Written by port/tools/hostgen/release_carry.py from profile.cpp's kNtsc and this release's maps. Do not edit.",
        "// SPDX-License-Identifier: GPL-3.0-or-later",
        "// Copyright (c) 2026 the OpenRAC contributors",
        "//",
        f"// {names}: port/game/rac2/profile.cpp's table (Release) for this disc, v1.01's addresses as this",
        "// release has them. Written by port/tools/hostgen/release_carry.py; do not edit.",
        "",
        "namespace openrac::game::rac2 {",
        "namespace {",
        "",
        *out,
        "",
        "}  // namespace",
        "}  // namespace openrac::game::rac2",
        "",
    ])


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game", type=Path, required=True, help="the release's hostgen.json (its directory is written)")
    ap.add_argument("--primary", type=Path, required=True, help="the primary release's hostgen.json")
    ap.add_argument("--source", type=Path, required=True, help="the built decompilation (release.py build's)")
    ap.add_argument("--map", type=Path, help="versionmap.py's map from v1.01 to the release")
    ap.add_argument("--version", required=True, choices=["pal", "us2"])
    ap.add_argument("--primary-protos", type=Path,
                    help="the primary's generated game_protos.h (port/build/<preset>/games/rac2-ntsc/gen): the "
                         "signatures of the functions the host C replaces or wraps that this release changed")
    args = ap.parse_args(argv)
    out_dir = args.game.parent
    cfg = json.loads(args.primary.read_text())
    table = rel.Release.load(out_dir / "release_names.json")
    carry_path = args.source / rel.RELEASES[args.version]["build"] / "carry.json"
    carry = json.loads(carry_path.read_text()) if carry_path.is_file() else None
    vmap = json.loads(args.map.read_text()) if args.map else None
    p = Pairing(args.source, args.version, table, vmap, carry)
    p.read_by_hand(out_dir / "release_pairs.tsv")
    p.primary_dir = args.primary.parent
    p.out_dir = out_dir
    new_cfg = carry_config(p, cfg, args.version)
    primary = args.primary.parent
    (out_dir / "libraries.tsv").write_text(carry_libraries(p, (primary / "libraries.tsv").read_text()))
    extra = out_dir / "release_extra.json"
    if extra.is_file():
        for key, rows in json.loads(extra.read_text()).items():
            if key == "from_assembly":
                new_cfg["from_assembly"] = list(dict.fromkeys(new_cfg.get("from_assembly", []) + list(rows)))
    if args.primary_protos:
        add_signatures(p, new_cfg, cfg, (primary / "libraries.tsv").read_text(), args.primary_protos.read_text())
    args.game.write_text(json.dumps(new_cfg, indent=2) + "\n")
    (out_dir / "split_places.tsv").write_text(carry_split_places(p, (primary / "split_places.tsv").read_text()))
    host = out_dir / "host"
    host.mkdir(exist_ok=True)
    shared = primary.parent / "rac2" / "host"
    (host / "rac2_addresses.h").write_text(addresses_header(p, (primary / "host/rac2_addresses.h").read_text(), args.version))
    (host / "rac2_version.h").write_text(host_table(p, shared, primary / "host", args.source, args.version))
    (host / "rac2_names.tsv").write_text(p.host_names)
    profile_dir = primary.parent / "rac2"
    (profile_dir / f"profile_{new_cfg['id']}.h").write_text(
        profile_table(p, (profile_dir / "profile.cpp").read_text(), args.version))
    report = {"about": "How release_carry.py paired what the primary release's port names: what rests on less "
                       "than the same code, and what it found nowhere.", "counts": dict(sorted(p.kinds.items())),
              **p.report}
    (out_dir / "release_carry.json").write_text(json.dumps(report, indent=1, sort_keys=True) + "\n")
    print(f"release_carry: {out_dir}: " + ", ".join(f"{k} {v}" for k, v in sorted(p.kinds.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())

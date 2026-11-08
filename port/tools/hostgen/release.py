#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Another release of a game whose decompilation names everything by one release's addresses.

Going Commando's decompilation builds three releases from one C (games/rac2/decomp/docs/VERSIONS.md):
USA v1.01, whose addresses every name states (`func_0027F538`, `D_001A7A18`, `func_L05_0041DF58`),
and PAL and USA v2.00, whose programs hold the same code elsewhere. Its build reads each name's
value in another release from that release's bytes, and the release's own functions are named
for it (`func_PAL_...`, `D_US2_L00_...`, at the release's addresses).

hostgen, and every table of the port, knows a function or a global by the address its name
states. So the port of another release reads the decompilation through a table that renames
every address name to the release's address, the "release view": `func_0027F538` is
`func_0027F3D8` in the PAL port, `func_PAL_002907D0` is `func_002907D0`, a v1.01 function the
release does not have is `func_V1_0028E458` (no address, so nothing reaches it). Then the
release's port is what v1.01's is, on the release's addresses, and the same rules hold.

The table is written from a built checkout of the decompilation, beside the release's
hostgen.json (`"release": {"table": ...}`, the same file as its `"derived"` table):

    python3 port/tools/hostgen/release.py build --game port/game/rac2-pal/hostgen.json \\
        --source <rac2-decomp built with --version pal and for v1.01> [--map <versionmap>]

Where each name's new address comes from (the table's "evidence" counts them):

- a function: the release's catalogues (`config/<version>/boot/functions.tsv` and
  `levels/functions.tsv`), which name each of its functions by the v1.01 function whose code it
  is (tools/carry.py); a twin entered away from its name keeps that distance;
- a global: the value the release's own build gave the name (its `build/<version>/*/placed.json`,
  every relocation checked against the retail bytes), else, for a name no placed C uses, the
  address versionmap pairs it with by the same instruction of the same code (`exact` only).

A name with neither is not renamed to a guess: it gets the `V1` mark, and what uses it stays a
stub, which hostgen's report lists.

Short names (`func_2F15C8`, an address in the program of the file's level, and kseg0 spellings
`func_80338398`) are renamed per file, from the value that level's build gave them.
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
import derived as derived_mod  # noqa: E402

# An address name: func_, FUN_ or D_, a release tag, a level, eight digits; a suffix after it stays.
NAME = re.compile(r"(?<![\w])(func|FUN|D)_(?:(?!L\d\d_)([A-Z][A-Z0-9]{0,3})_)?(?:L(\d\d)_)?([0-9A-Fa-f]{8})(?![0-9A-Za-z])")
# A short name: six digits, or kseg0's 80 and six (an address in the program of the file's level).
SHORT = re.compile(r"(?<![\w])func_(80)?([0-9A-F]{6})(?![0-9A-Za-z_])")
# Either, for a file with short names of its own.
ANY = re.compile(r"(?<![\w])func_(?:80)?[0-9A-F]{6}(?![0-9A-Za-z_])|" + NAME.pattern)
DEFINED = re.compile(r"^[A-Za-z_][\w \t\*]*?\b(func_(?:[A-Z][A-Z0-9]{0,3}_)?(?:L\d\d_)?[0-9A-F]{8})\s*\([^;{}]*\)\s*\{", re.M)
INCLUDED = re.compile(r"^[ \t]*(?:INCLUDE_ASM|ASM_FUNC)\([^,]*,\s*(func_(?:[A-Z][A-Z0-9]{0,3}_)?(?:L\d\d_)?[0-9A-F]{8})\s*\)", re.M)
UNIT_LEVEL = re.compile(r"(?:^|/)(?:L|levels/)(\d\d)_")
DEAD = "V1"

# The releases tools/levels.py builds: catalogues, build, dumps; where a level's program starts; $gp.
RELEASES = {
    "us": {"config": "config", "build": "build", "baserom": "baserom", "tag": None,
           "level_base": 0x1A7C80, "gp": 0x1AEFF0},
    "pal": {"config": "config/pal", "build": "build/pal", "baserom": "baserom/pal", "tag": "PAL",
            "level_base": 0x1A7D00, "gp": 0x1AF070},
    "us2": {"config": "config/us2", "build": "build/us2", "baserom": "baserom/us2", "tag": "US2",
            "level_base": 0x1A7C80, "gp": 0x1AEFF0},
}


def core(kind: str, level: int | None, address: int, tag: str | None = None) -> str:
    kind = "func" if kind == "FUN" else kind
    return f"{kind}_{tag + '_' if tag else ''}{f'L{level:02d}_' if level is not None else ''}{address:08X}"


def read_boot(path: Path) -> list[tuple[str, int, int]]:
    rows = []
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 3:
            continue
        rows.append((cols[0], int(cols[1], 16), int(cols[2])))
    return rows


def read_levels(path: Path) -> list[tuple[str, str, int, list[tuple[int, int]]]]:
    rows = []
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 6:
            continue
        places = [(int(p[:2]), int(p[3:], 16)) for p in cols[5].split(",") if p]
        rows.append((cols[0], cols[1], int(cols[2]), places))
    return rows


def placed_values(build: Path) -> dict[str, dict[str, int]]:
    """Program ("boot", "00", ...) -> {name: value} from a build's placed.json files."""
    out = {}
    for path in sorted(build.glob("*/placed.json")):
        name = path.parent.name
        program = "boot" if name == "boot" else name[:2] if name[:2].isdigit() else None
        if program is None:
            continue
        values = json.loads(path.read_text()).get("values", {})
        out[program] = {k: int(v, 0) for k, v in values.items() if isinstance(v, str)}
    return out


class Release:
    """The table: renames and the derived values in the release's names."""

    def __init__(self, table: dict):
        self.table = table
        self.names: dict[str, str] = table.get("names", {})
        self.units: dict[str, dict[str, str]] = table.get("units", {})
        self.gone: list[str] = table.get("gone", [])
        # Where a name is gone, when only in some of the tree (rac3: one PAL name can be a function
        # of the executable and of the front end, gone in one only): name -> directory prefixes.
        self.gone_in: dict[str, list[str]] = table.get("gone_in", {})

    @classmethod
    def load(cls, path: Path) -> "Release":
        return cls(json.loads(Path(path).read_text()))

    # The release's own definitions (rac1-decomp's src/versions/<key>/<shared file>/<function>.c):
    # shared file (its path under the decompilation) -> [(function, the definition's text)].
    splices: dict[str, list[tuple[str, str]]] = {}
    tag: str | None = None

    def read_splices(self, source: Path, directory: str, tag: str | None) -> None:
        """The release's definitions under DIRECTORY (src/versions/<key>), each standing in for the
        shared file's definition of its function, as the release's build compiles it."""
        root = source / directory
        out: dict[str, list[tuple[str, str]]] = {}
        for path in sorted(root.rglob("*.c")) if root.is_dir() else []:
            shared = (Path("src") / path.relative_to(root).parent).as_posix() + ".c"
            out.setdefault(shared, []).append((path.stem, path.read_text(encoding="utf-8", errors="surrogateescape")))
        self.splices, self.tag = out, tag

    def splice(self, text: str, unit: str, readable: dict[str, list[str]] | None = None) -> str:
        """UNIT's text with the release's definitions in place of the shared ones (rac1-decomp's
        tools/version_src.py spliced()): one for a function of the shared file takes the place of
        its definition, found by the function's name or a readable name the names header gives it;
        one for a function only the release has (func_<tag>_...) goes at the end."""
        import prep
        spans, tail = [], []
        for name in self.gone:
            # A definition of PAL v2.00's the release has no function for: left out (its lines stay).
            if name in self.gone_in and not unit.startswith(tuple(self.gone_in[name])):
                continue
            spellings = [name] + (readable or {}).get(name, [])
            for spelling in spellings + [f"{x}_r" for x in spellings]:
                span = prep._definition(text, spelling) if spelling in text else None
                if span:
                    text = text[:span[0]] + "\n" * text.count("\n", span[0], span[1]) + text[span[1]:]
                    break
        for name, body in self.splices.get(unit, []):
            if self.tag and name.startswith(f"func_{self.tag}_"):
                tail.append(body.rstrip("\n"))
                continue
            span = None
            # rac1-decomp's spellings of a definition: its name, a readable one, either with the `_r`
            # its C gives a function defined under an assembler name (`int func_00227100_r(...)
            # __asm__("func_00227100")`).
            spellings = [name] + (readable or {}).get(name, [])
            for spelling in spellings + [f"{x}_r" for x in spellings]:
                span = prep._definition(text, spelling)
                if span:
                    break
            if span is None:
                raise ValueError(f"{unit}: the release's definition of {name} has no definition there to replace")
            spans.append((span, body.rstrip("\n")))
        for (start, end), body in sorted(spans, reverse=True):
            text = text[:start] + body + text[end:]
        return text + "".join(f"\n{body}\n" for body in tail)

    def name(self, symbol: str) -> str:
        """SYMBOL in the release's names (a suffix kept: func_00280B90_7)."""
        return NAME.sub(self._one, symbol)

    def _one(self, m: re.Match) -> str:
        key = core(m.group(1), int(m.group(3)) if m.group(3) else None, int(m.group(4), 16), m.group(2))
        new = self.names.get(key)
        if new is None:
            return m.group(0)
        return "FUN" + new[4:] if m.group(1) == "FUN" and new.startswith("func_") else new

    def text(self, text: str, unit: str | None = None) -> str:
        """A file's text in the release's names: every address name, in code, strings (assembler
        names) and comments alike, and the unit's own short names."""
        own = self.units.get(unit or "", {})
        if not own:
            return NAME.sub(self._one, text)

        # One pass, so that a name the unit's table gives (already the release's) is not renamed again.
        def one(m: re.Match) -> str:
            word = m.group(0)
            if word in own:
                return own[word]
            named = NAME.fullmatch(word)
            return self._one(named) if named else word
        return ANY.sub(one, text)


def configured(source: Path, config_dir: Path, cfg: dict) -> Release | None:
    """The release table hostgen.json names ("release": {"table": ...}), or None for a primary release."""
    r = cfg.get("release")
    if not r:
        return None
    path = config_dir / r["table"]
    if not path.is_file():
        raise FileNotFoundError(f"{cfg.get('id')}: no release table {path}: write it with port/tools/hostgen/release.py build")
    table = Release.load(path)
    if r.get("splice"):
        table.read_splices(source, r["splice"], RAC1_RELEASES.get(r["version"], {}).get("tag"))
    return table


# ---------------------------------------------------------------- building the table

def build(source: Path, version: str, versionmap: Path | None, uses: Counter) -> dict:
    us, rel = RELEASES["us"], RELEASES[version]
    tag = rel["tag"]
    evidence: Counter = Counter()
    names: dict[str, str] = {}

    us_boot = read_boot(source / us["config"] / "boot/functions.tsv")
    us_levels = read_levels(source / us["config"] / "levels/functions.tsv")
    r_boot = read_boot(source / rel["config"] / "boot/functions.tsv")
    r_levels = read_levels(source / rel["config"] / "levels/functions.tsv")
    us_entry = {n: a for n, a, _ in us_boot}

    def stated(name: str) -> int:
        return int(name[-8:], 16)

    def own(name: str) -> bool:
        return f"_{tag}_" in name

    def strip(name: str) -> str:
        return name.replace(f"_{tag}_", "_", 1)

    # Functions of the release's executable.
    r_boot_names = set()
    plain: dict[str, int] = {}
    absent: set[str] = set()
    for name, address, _ in r_boot:
        r_boot_names.add(name)
        if own(name):
            new = strip(name)
            evidence["function: the release's own"] += 1
        else:
            m = NAME.fullmatch(name)
            if m is None:
                # A name of the symbol table (_sysbitFlush): the catalogue's address, as a symbol.
                evidence["function: by its own name"] += 1
                plain[name] = address
                continue
            if name not in us_entry:
                raise SystemExit(f"release: {name} of the release's executable is not a v1.01 function")
            # A twin entered away from its name (leftovers in front): the same distance here.
            new = core("func", None, address + stated(name) - us_entry[name])
            evidence["function: carried (same code)"] += 1
        names[name] = new
    # Functions of the levels (and executable functions the levels hold only).
    for name, kind, _, places in r_levels:
        if name in names:
            continue
        m = NAME.fullmatch(name)
        if m is None:
            continue
        level = int(m.group(3)) if m.group(3) else None
        if level is None:
            # An executable function the release changed whose level copies stayed v1.01's: here a
            # level function, named for its first place.
            level, address = places[0]
            evidence["function: v1.01's level copies of a changed executable function"] += 1
        else:
            address = next((a for lv, a in places if lv == level), None)
            if address is None:
                level, address = places[0]
            evidence["function: the release's own" if own(name) else "function: carried (same code)"] += 1
        names[name] = core("func", level, address)
    # Where the release's catalogues place each function (their names, before renaming).
    level_base_r = rel["level_base"]
    r_level_place = {}
    for n, _, _, places in r_levels:
        for pl in places:
            r_level_place.setdefault(pl, n)
    r_boot_at = {a: n for n, a, _ in r_boot}
    pending: dict[str, str] = {}
    # Globals and other names: the release's build's values for v1.01's names, program by program.
    r_values = placed_values(source / rel["build"])
    us_values = placed_values(source / us["build"])
    # v1.01's functions the release's catalogues do not have: where the release's C takes one's
    # address, the value its build read (the release's changed function there); else no address.
    for name, *_ in us_boot + [(n, 0, 0) for n, *_ in us_levels]:
        if name in names or not (m := NAME.fullmatch(name)):
            continue
        home = f"{int(m.group(3)):02d}" if m.group(3) else "boot"
        value = r_values.get(home, {}).get(name)
        absent.add(name)
        if value is not None:
            names[name] = core("func", int(m.group(3)) if m.group(3) else None, value)
            evidence["function: the release's build (a changed function's address)"] += 1
            continue
        if home == "boot":
            # Only a level's C names it: the release's function at the place that level's build read
            # (a level's copy of it, or the executable's).
            found = None
            for program in sorted(p for p in r_values if p != "boot"):
                v = r_values[program].get(name)
                if v is None:
                    continue
                lv = int(program)
                row = r_level_place.get((lv, v)) if v >= level_base_r else r_boot_at.get(v)
                if row is not None:
                    found = row
                    break
            if found is not None:
                pending[name] = found
                evidence["function: the release's build (a level's, the release's function there)"] += 1
                continue
        names[name] = name.replace("func_", f"func_{DEAD}_", 1)
        evidence["function: not in the release (marked)"] += 1
    vmap = json.loads(versionmap.read_text()) if versionmap else None

    def mapped(program: str, address: int) -> int | None:
        if vmap is None:
            return None
        key = "boot" if program == "boot" else f"level:{program}"
        hit = vmap["programs"].get(key, {}).get("data", {}).get(f"{address:08X}")
        return int(hit, 16) if hit else None

    level_base = us["level_base"]

    def mapped_function(program: str, address: int) -> int | None:
        if vmap is None:
            return None
        key = "boot" if program == "boot" else f"level:{program}"
        hit = vmap["programs"].get(key, {}).get("functions", {}).get(f"{address:08X}")
        return int(hit[0], 16) if hit else None

    # Every other address name the C uses or the builds placed: functions the catalogues do not
    # have (an entry inside one: func_00283208), globals, and names spelled with another release's
    # tag (PAL's C carried from v2.00's says D_US2_...): their value in the release's build.
    cores: set[str] = set()
    for program, values in list(r_values.items()) + list(us_values.items()):
        for sym in values:
            m = NAME.match(sym)
            if m:
                cores.add(core(m.group(1), int(m.group(3)) if m.group(3) else None, int(m.group(4), 16), m.group(2)))
    for sym in uses:
        m = NAME.match(sym)
        if m:
            cores.add(core(m.group(1), int(m.group(3)) if m.group(3) else None, int(m.group(4), 16), m.group(2)))
    for name in sorted(cores - set(names)):
        m = NAME.fullmatch(name)
        kind, tagged = m.group(1), m.group(2)
        level, address = (int(m.group(3)) if m.group(3) else None), int(m.group(4), 16)
        what = "function" if kind == "func" else "global"
        if tagged == tag:
            names[name] = strip(name)
            evidence[f"{what}: the release's own"] += 1
            continue
        if tagged in (DEAD,):
            continue
        home = f"{level:02d}" if level is not None else "boot"
        value, how, at = None, None, level
        if name in r_values.get(home, {}):
            value, how = r_values[home][name], f"{what}: the release's build"
        elif not tagged and (v := (mapped(home, address) if kind == "D" else mapped_function(home, address))) is not None:
            value, how = v, f"{what}: versionmap ({'exact' if kind == 'D' else 'paired'})"
        else:
            # Used only where another program placed it (a level's C): the first level whose build
            # did (level_data has each level's own).
            for program in sorted(p for p in r_values if p != "boot"):
                if name in r_values[program]:
                    value, how = r_values[program][name], f"{what}: the release's build (a level's)"
                    if kind == "func" and level is None and value >= level_base:
                        at = int(program)     # a level's copy: a function of that level
                    break
        if value is None:
            names[name] = name.replace(f"{'func' if kind == 'func' else 'D'}_", f"{'func' if kind == 'func' else 'D'}_{DEAD}_", 1)
            evidence[f"{what}: no value (marked)"] += 1
            continue
        names[name] = core(kind, at, value)
        evidence[how] += 1
    # Names given as another catalogue row's (above): that row's name in the release.
    for k, v in pending.items():
        names[k] = names.get(v, v)
    # Two v1.01 names of one function here (the release merged two of v1.01's alike empty hooks; a
    # level's short spelling and its catalogue name): one file can declare both, with other types,
    # so all but the catalogue's take another spelling of the same address (FUN_..., F<n>_func_...),
    # which hostgen reads as that function.
    catalogued = {n for n, _, _ in r_boot} | {n for n, _, _, _ in r_levels}
    groups: dict[str, list[str]] = {}
    for k, v in names.items():
        if v.startswith("func_") and f"_{DEAD}_" not in v:
            groups.setdefault(v, []).append(k)
    for v, ks in groups.items():
        if len(ks) < 2:
            continue
        ks.sort(key=lambda k: (k not in catalogued, k))
        m = NAME.fullmatch(v)
        for i, k in enumerate(ks[1:]):
            if i == 0:
                names[k] = "FUN" + v[4:]
            elif m.group(3) is None:
                names[k] = f"F{i:08x}_{v}"
            else:
                continue
            evidence["function: another spelling of a function with two v1.01 names"] += 1
    return {"names": dict(sorted((k, v) for k, v in names.items() if k != v)), "evidence": dict(sorted(evidence.items())),
            "r_values": r_values, "us_values": us_values, "r_levels": r_levels, "us_levels": us_levels,
            "r_boot": r_boot, "us_boot": us_boot, "plain": plain, "absent": absent}


# ---------------------------------------------------------------- Ratchet & Clank's releases

# rac1-decomp (games/rac1/decomp/docs/VERSIONS.md) builds PAL v1.00 and NTSC-J from PAL v2.00's C. Each
# release's catalogue (config/versions/<key>/functions.tsv) names every function by its PAL v2.00
# counterpart, or func_<tag>_... for code only it has, with its places; its own definitions
# (src/versions/<key>/) stand in for the shared ones where its code differs; and its check
# (tools/version_check.py) proves every relocation of every function against its bytes, which
# version_relocs.py writes out as the values release.py reads for Going Commando's builds.
MAIN_SEGMENT = 0x15F000     # where PAL v2.00's main segment (the part a level replaces) starts
RAC1_RELEASES = {
    "pal_v1": {"game": "rac1", "tag": "E1", "dead": "V2", "label": "PAL v1.00"},
    "jp": {"game": "rac1", "tag": "J", "dead": "V2", "label": "NTSC-J"},
}


def read_rows(path: Path) -> list[dict]:
    """A rac1-decomp version catalogue (config/versions/<key>/functions.tsv): name, kind, size,
    fingerprint, counterpart, method, similarity, same, places (boot:ADDR or NN:ADDR)."""
    rows = []
    for line in path.read_text().splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 9:
            continue
        places = []
        for item in cols[8].split(","):
            prog, _, addr = item.partition(":")
            if addr:
                places.append(("boot" if prog == "boot" else int(prog), int(addr, 16)))
        rows.append({"name": cols[0], "kind": cols[1], "size": int(cols[2]), "fp": cols[3],
                     "counterpart": None if cols[4] == "-" else cols[4], "same": cols[7] == "yes",
                     "places": places})
    return rows


def build_rac1(source: Path, version: str, relocs: Path, versionmap: Path | None, uses: Counter) -> dict:
    """The release table of a rac1-decomp release: build()'s, from its catalogue and its check."""
    info = RAC1_RELEASES[version]
    tag, dead = info["tag"], info["dead"]
    evidence: Counter = Counter()
    names: dict[str, str] = {}
    how: dict[str, str] = {}
    primary = {r["name"]: r for r in read_rows(source / "config/versions/pal/functions.tsv")}
    rows = read_rows(source / "config/versions" / version / "functions.tsv")
    r_values = placed_values(relocs)

    def own(name: str) -> bool:
        return name.startswith((f"func_{tag}_", f"D_{tag}_"))

    def strip(name: str) -> str:
        return name.replace(f"_{tag}_", "_", 1)

    def home_of(m: re.Match):
        return int(m.group(3)) if m.group(3) else "boot"

    def mark(name: str) -> str:
        return re.sub(r"^(func|D)_", rf"\1_{dead}_", name)

    def put(name: str, new: str, why: str) -> None:
        names[name] = new
        how[name] = why
        evidence[why] += 1

    # Functions: the release's catalogue places each by its PAL v2.00 name.
    for r in rows:
        name = r["name"]
        if own(name):
            put(name, strip(name), "function: the release's own")
            continue
        m = NAME.fullmatch(name)
        if m is None:
            continue
        home, stated = home_of(m), int(m.group(4), 16)
        mine = sorted(a for p, a in r["places"] if p == home)
        theirs = sorted(a for p, a in primary.get(name, {}).get("places", []) if p == home)
        why = "function: carried (same code)" if r["same"] else "function: carried (the release's own C for it)"
        if mine:
            # The k-th place of PAL v2.00's in that program is the k-th here (version_check.py's table);
            # a name stated away from its place keeps the distance.
            k = theirs.index(stated) if stated in theirs else 0
            base = theirs[k] if theirs else stated
            put(name, core("func", m.group(3) and int(m.group(3)), mine[min(k, len(mine) - 1)] + stated - base), why)
        elif r["places"]:
            # Here only in other programs (an executable function only the levels hold): a function of
            # the first of them, as release.build() names one.
            level, address = next((p, a) for p, a in sorted(r["places"], key=lambda p: (p[0] == "boot", p)) if p != "boot")
            put(name, core("func", level, address), "function: only in other programs here")
    vmap = json.loads(versionmap.read_text()) if versionmap else None
    between = None
    if versionmap:
        sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "versionmap"))
        from versionmap import Map
        between = Map(versionmap)
    # PAL v2.00's functions the release does not have: what the release's check placed for the name
    # (a relocation to it, through its counterpart here), else marked.
    absent, gone = set(), set()
    r_at = {}
    for r in rows:
        for p in r["places"]:
            r_at.setdefault(p, r)
    for name in primary:
        if name in names or not (m := NAME.fullmatch(name)):
            continue
        absent.add(name)
        home = "boot" if m.group(3) is None else f"{int(m.group(3)):02d}"
        value = r_values.get(home, {}).get(name)
        if value is None and home == "boot":
            # Reached only from the levels' checked C (a callback a level's code stores): the
            # executable's function whose copy in that level its relocations reached.
            for p in sorted(x for x in r_values if x != "boot"):
                v = r_values[p].get(name)
                row = r_at.get((int(p), v)) if v is not None else None
                if v is not None and v < MAIN_SEGMENT:
                    value = v
                elif row is not None:
                    value = next((a for q, a in row["places"] if q == "boot"), None)
                if value is not None:
                    break
        if value is None and between is not None:
            # No relocation of the release's checked C reaches it: versionmap's pair of the same
            # instructions, where the release's catalogue counts them under another copy (below).
            hit = between.function(int(m.group(4), 16), "boot" if home == "boot" else f"level:{home}")
            if hit is not None and hit[1] == "same":
                cand = r_at.get((home if home == "boot" else int(home), hit[0]))
                if cand is not None and cand["fp"] == primary[name]["fp"]:
                    value = hit[0]
        at = r_at.get((home if home == "boot" else int(home), value)) if value is not None else None
        if at is not None and at["fp"] == primary[name]["fp"]:
            # The same code here, which the release's catalogue counts under another copy's name
            # (an executable function and its level copies folded into one row): its own function.
            put(name, core("func", m.group(3) and int(m.group(3)), value),
                "function: carried (same code; the release's catalogue names it by another copy)")
        elif value is not None:
            # The release has the code PAL v2.00's C calls by this name elsewhere (its own function,
            # or one it changed): the calls go there, spelled FUN_, as hostgen reads another name
            # of a function; PAL v2.00's definition is not the release's (Release.splice drops it).
            new = core("func", m.group(3) and int(m.group(3)), value)
            put(name, "FUN" + new[4:], "function: the release's check (its counterpart here)")
            gone.add(name)
        else:
            put(name, mark(name), "function: not in the release (marked)")

    def mapped(program: str, address: int, kind: str) -> int | None:
        if vmap is None:
            return None
        key = "boot" if program == "boot" else f"level:{program}"
        if kind == "D":
            hit = vmap["programs"].get(key, {}).get("data", {}).get(f"{address:08X}")
            if isinstance(hit, list):
                hit = hit[0] if len(hit) > 1 and hit[1] == "exact" else None
            return int(hit, 16) if isinstance(hit, str) else None
        hit = vmap["programs"].get(key, {}).get("functions", {}).get(f"{address:08X}")
        return int(hit[0], 16) if hit and hit[1] == "same" else None

    # Every other address name the C uses or the check placed: globals, entries inside functions,
    # the release's own data (D_E1_...): the check's value, else versionmap's exact pair.
    cores: set[str] = set()
    for values in r_values.values():
        cores.update(values)
    cores.update(uses)
    for sym in sorted(cores - set(names)):
        m = NAME.fullmatch(sym)
        if m is None:
            continue
        kind, tagged = m.group(1), m.group(2)
        what = "function" if kind == "func" else "global"
        if tagged == tag:
            put(sym, strip(sym), f"{what}: the release's own")
            continue
        if tagged:
            continue
        level, address = (int(m.group(3)) if m.group(3) else None), int(m.group(4), 16)
        home = f"{level:02d}" if level is not None else "boot"
        value, why, at = r_values.get(home, {}).get(sym), f"{what}: the release's check", level
        if value is None and (value := mapped(home, address, "D" if kind == "D" else "func")) is not None:
            why = f"{what}: versionmap ({'exact' if kind == 'D' else 'same'})"
        if value is None:
            # Used only where another program's check placed it (a level's C naming the executable's).
            for program in sorted(p for p in r_values if p != "boot"):
                if sym in r_values[program] and (level is not None or r_values[program][sym] < 0x15F000):
                    value, why = r_values[program][sym], f"{what}: the release's check (a level's)"
                    break
        if value is None and kind == "D" and between is not None:
            # Between two neighbours that moved alike (versionmap's `between`): inferred, listed.
            hit = between.data(address, "boot" if home == "boot" else f"level:{home}")
            if hit is not None and hit[1] == "between":
                value, why = hit[0], "global: versionmap (between)"
        if value is None:
            put(sym, mark(sym), f"{what}: no value (marked)")
            continue
        put(sym, core(kind, at, value), why)
    # The release's own datum that a PAL v2.00 name reaches too (the release's own C names it D_E1_...,
    # the shared C by PAL v2.00's name): one datum, two declarations that may differ in type, so the
    # release's own takes a spelling of its own of the same address (D_..._E1, a view hostgen reads
    # at that address).
    taken = Counter(v for k, v in names.items() if not own(k))
    for k, v in list(names.items()):
        if own(k) and k.startswith("D_") and taken[v]:
            names[k] = f"{v}_{tag}"
            evidence["global: the release's own, spelled apart from a PAL v2.00 name of it"] += 1
    return {"names": dict(sorted((k, v) for k, v in names.items() if k != v)), "evidence": dict(sorted(evidence.items())),
            "how": how, "absent": absent, "gone": sorted(gone), "rows": rows, "primary": primary, "r_values": r_values}


def units(source: Path, version: str, release: Release, parts: dict, sources: list[str]) -> tuple[dict, Counter]:
    """Each file's short names in the release's names: the function at the value that level's
    build gave the name (the release's, else v1.01's renamed)."""
    tag = RELEASES[version]["tag"]
    evidence: Counter = Counter()

    def at(rows_levels, boot_rows, level: int, address: int) -> str | None:
        for name, _, _, places in rows_levels:
            if (level, address) in places:
                return name
        for name, a, _ in boot_rows:
            if a == address:
                return name
        return None
    r_at = {}
    for name, _, _, places in parts["r_levels"]:
        for p in places:
            r_at.setdefault(p, name)
    us_at = {}
    for name, _, _, places in parts["us_levels"]:
        for p in places:
            us_at.setdefault(p, name)
    r_exe = {a: n for n, a, _ in parts["r_boot"]}
    r_place_of = {}
    for (l, a), n in r_at.items():
        r_place_of.setdefault((l, release.name(n)), a)
    r_exe_names = {release.name(n): a for n, a, _ in parts["r_boot"]}
    us_exe = {a: n for n, a, _ in parts["us_boot"]}
    out: dict[str, dict[str, str]] = {}
    for d in sources:
        for path in sorted((source / d).rglob("*.c")):
            rel = path.relative_to(source).as_posix()
            unit_level = UNIT_LEVEL.search(rel)
            level = f"{int(unit_level.group(1)):02d}" if unit_level else "boot"
            mine = rel.startswith(f"src/{version}/")
            text = path.read_text(errors="replace")
            # A v1.01 function the release does not have, defined here (as C or as its assembly's
            # marker): no address in this file, though the release's C elsewhere may take the address
            # its build read for that name (the release's own function there).
            for name in DEFINED.findall(text) + INCLUDED.findall(text):
                if name in parts["absent"] and f"_{DEAD}_" not in release.name(name):
                    out.setdefault(rel, {})[name] = name.replace("func_", f"func_{DEAD}_", 1)
                    evidence["function: v1.01's definition of one the release does not have"] += 1
            if unit_level is None:
                continue
            found = {m.group(0) for m in SHORT.finditer(text)}
            found |= set(re.findall(r"(?<![\w])func_80[0-9A-F]{6}(?![0-9A-Za-z_])", text))
            lv = int(level)
            for short in sorted(found):
                # The same spelling at the release's address of that function in this level's program,
                # so hostgen reads it as it reads v1.01's (hostgen.json "level_short_names").
                value = parts["r_values"].get(level, {}).get(short)
                how = "short name: the release's build"
                if value is None and not mine:
                    old = parts["us_values"].get(level, {}).get(short)
                    name = (us_at.get((lv, old)) or us_exe.get(old)) if old is not None else None
                    renamed = release.name(name) if name else None
                    value = r_place_of.get((lv, renamed)) if renamed else None
                    if value is None and renamed and renamed in r_exe_names and old is not None and old < 0x1000000:
                        value = r_exe_names[renamed]
                    how = "short name: v1.01's build, the same function's place here"
                if value is None or value >= 0x1000000:
                    evidence["short name: left as it is"] += 1
                    continue
                new = f"func_80{value:06X}" if short.startswith("func_80") and len(short) == 13 else f"func_{value:06X}"
                evidence[how] += 1
                if new != short:
                    out.setdefault(rel, {})[short] = new
    return out, evidence


def derived_values(parts: dict, release: Release, level_base: int) -> dict:
    """The derived table (derived.py's form) in the release's names: every value the release's build
    gave, the executable's as values, a level's where it moved."""
    values = {release.name(k): v for k, v in parts["r_values"].get("boot", {}).items()}
    for k, v in parts["plain"].items():
        values.setdefault(k, v)
    # A named global only levels' C uses (g_pHeroMoby): one value in every level below the levels'
    # programs is the executable's datum.
    seen: dict[str, set[int]] = {}
    for program, vals in parts["r_values"].items():
        if program != "boot":
            for k, v in vals.items():
                if NAME.match(k) is None and not k.startswith(("ref_", "r1_", "func_", "FUN_")) \
                        and not re.match(r"F[0-9a-f]{8}_", k):
                    seen.setdefault(k, set()).add(v)
    for k, vs in seen.items():
        if k not in values and len(vs) == 1 and next(iter(vs)) < level_base:
            values[k] = next(iter(vs))
    levels: dict[str, list] = {}
    level_data: dict[str, dict[str, str]] = {}
    for program, vals in sorted(parts["r_values"].items()):
        if program == "boot":
            continue
        lv = int(program)
        moved = {}
        for k, v in vals.items():
            name = release.name(k)
            if name.startswith(("ref_", "r1_func_")) or re.fullmatch(r"F[0-9a-f]{8}_(?:FUN|func)_[0-9A-F]{8}", name):
                if name not in values:
                    own = re.match(r"(?:ref|r1_func)_L(\d\d)_", name)
                    if name not in levels or (own is not None and int(own.group(1)) == lv):
                        levels[name] = [lv, f"{v:#010x}"]
                continue
            m = NAME.match(name)
            if m and m.group(1) == "D":
                stated = int(m.group(4), 16)
                if v != stated or (m.group(0) == name and stated >= level_base and name not in values):
                    moved[name] = f"{v:#010x}"
            elif name.startswith(("D_", "r1_D_")) or (m is None and not name.startswith(("func_", "FUN_"))):
                if name in values and int(values[name]) != v:
                    moved[name] = f"{v:#010x}"
                elif name not in values and not name.startswith(("func_", "FUN_")):
                    moved[name] = f"{v:#010x}"
        if moved:
            level_data[str(lv)] = dict(sorted(moved.items()))
    return {"values": {k: f"{v:#010x}" for k, v in sorted(values.items())},
            "levels": dict(sorted(levels.items())), "level_data": level_data}


def used_tokens(source: Path, sources: list[str], skip: list[str] = ()) -> Counter:
    out: Counter = Counter()
    for d in sources:
        for p in (source / d).rglob("*"):
            rel = p.relative_to(source).as_posix()
            if any(rel == s or rel.startswith(s.rstrip("/") + "/") for s in skip):
                continue
            if p.suffix in (".c", ".h") and p.is_file():
                out.update(m.group(0) for m in NAME.finditer(p.read_text(errors="replace")))
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build", help="write the release's table from a built decompilation")
    b.add_argument("--game", type=Path, required=True, help="the release's hostgen.json")
    b.add_argument("--source", type=Path, required=True,
                   help="the decompilation, built for v1.01 and for the release (build/, build/<version>/)")
    b.add_argument("--map", type=Path, help="versionmap.py's map from v1.01 to the release (globals no placed C uses)")
    b.add_argument("--relocs", type=Path,
                   help="Ratchet & Clank: version_relocs.py's output for the release (what its check proved)")
    n = sub.add_parser("name", help="print names in the release's names")
    n.add_argument("--game", type=Path, required=True)
    n.add_argument("names", nargs="+")
    args = ap.parse_args(argv)
    cfg = json.loads(args.game.read_text())
    table_path = args.game.parent / cfg["release"]["table"]
    if args.cmd == "name":
        r = Release.load(table_path)
        for x in args.names:
            print(x, r.name(x))
        return 0
    version = cfg["release"]["version"]
    source = args.source.resolve()
    roots = sorted(set(cfg.get("sources", ["src"]) + cfg.get("includes", ["include"]) + ["nonmatching"]))
    uses = used_tokens(source, roots)
    if version in RAC1_RELEASES:
        # What hostgen leaves out (src/data: the port reads the executable's data from the disc).
        uses = used_tokens(source, roots, cfg.get("skip", []))
        if args.relocs is None:
            ap.error("a Ratchet & Clank release needs --relocs (version_relocs.py's output)")
        # The release's own definitions are its C too (hostgen splices them in: "release" "splice").
        uses.update(used_tokens(source, [f"src/versions/{version}"]))
        parts = build_rac1(source, version, args.relocs.resolve(), args.map, uses)
        release = Release({"names": parts["names"]})
        unit_names, unit_evidence = {}, Counter()
        derived = {"values": {}, "levels": {}, "level_data": {}}
        DEAD_NOW = RAC1_RELEASES[version]["dead"]
        source_note = {"commit": None, "version": version, "table": "version_relocs.py (rac1-decomp's tools/version_check.py)"}
    else:
        parts = build(source, version, args.map, uses)
        release = Release({"names": parts["names"]})
        unit_names, unit_evidence = units(source, version, release, parts, cfg.get("sources", ["src"]))
        release.units = unit_names
        derived = derived_values(parts, release, RELEASES[version]["level_base"])
        DEAD_NOW = DEAD
        source_note = {"commit": None, "version": version, "table": f"{RELEASES[version]['build']}/*/placed.json"}
    try:
        commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=source, capture_output=True, text=True,
                                timeout=15).stdout.strip() or None
    except (OSError, subprocess.TimeoutExpired):
        commit = None
    # The decompilation's candidates (C for functions it has as assembly) for functions this release
    # does not have, or that name what it does not: the release's assembly is translated instead.
    left_out = {}
    table_path_c = source / cfg.get("candidates", "nonmatching/functional.tsv")
    if table_path_c.is_file():
        for line in table_path_c.read_text().splitlines():
            if not line.strip() or line.startswith("#"):
                continue
            cname, _, crel = line.partition("\t")
            cname, cpath = cname.strip(), source / crel.strip()
            if f"_{DEAD_NOW}_" in release.name(cname):
                left_out[cname] = "not in this release"
                continue
            if cpath.is_file():
                text = release.text(cpath.read_text(errors="replace"))
                marked = sorted(set(re.findall(rf"\b(?:func|D)_{DEAD_NOW}_\w+", text)))
                if marked:
                    left_out[cname] = f"names what this release does not have ({', '.join(marked[:3])})"
    unresolved = sorted({release.name(t) for t in uses if t.startswith(("D_", "func_"))
                         and f"_{DEAD_NOW}_" in release.name(t) and uses[t]})
    table = {
        "about": "The release view of the decompilation: each v1.01 address name and the release's name for it, "
                 "and the values the release's build derived, in those names. Written by "
                 "port/tools/hostgen/release.py build; do not edit.",
        "source": {**source_note, "commit": commit},
        "evidence": {**parts["evidence"], **dict(sorted(unit_evidence.items()))},
        "names": parts["names"],
        "units": unit_names,
        **derived,
        # The derived names the C uses that this release's build gave no value (their C is v1.01's
        # only, or left as assembly here): derived.py's check tells them from a stale table.
        "unresolved": (sorted(derived_mod.used_names(source, cfg) - set(derived["values"]) - set(derived["levels"]))
                       if cfg.get("derived") else []),
        "candidates_left_out": dict(sorted(left_out.items())),
        "marked_in_use": unresolved,
    }
    if parts.get("gone"):
        # PAL v2.00's definitions the release has no function for (another of its own is called).
        table["gone"] = parts["gone"]
    if parts.get("how"):
        # How each name rests (Ratchet & Clank's releases: release_rac1.py lists what is inferred).
        table["how"] = {k: v for k, v in sorted(parts["how"].items()) if parts["names"].get(k, k) != k
                        or not v.startswith("function: carried (same code)")}
    table_path.write_text(json.dumps(table, indent=1) + "\n")
    print(f"release: {table_path}: {len(table['names'])} names, {len(unit_names)} files with short names, "
          f"{len(table['values'])} values; {len(unresolved)} marked names the C uses")
    for k, v in table["evidence"].items():
        print(f"  {k}: {v}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

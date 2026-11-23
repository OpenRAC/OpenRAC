#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Up Your Arsenal's NTSC-U port tables, carried from PAL's (rac3-pal is the primary release).

rac3-pal's port names what it knows by PAL's functions and addresses: the host replacements and
wraps (libraries.tsv), the entry and roots, the functions read from their assembly, the shared
host C (port/game/rac3-pal/host). This writes rac3-ntsc's in NTSC-U's names, as release_carry.py
does for Going Commando's releases, from what rac3-decomp proves:

- config/us/map.tsv: each PAL function, its program (boot_elf, frontbin, i5bootn, levels) and its
  NTSC-U name; `match` (the same code), `differs` (NTSC-U's own code in that place), `none`;
- release/release_names.json (release_rac3.py): every global's NTSC-U address;
- release/release.json (by hand): the release's facts and the pairs and addresses checked by hand,
  each with its evidence.

It writes, in port/game/rac3-ntsc:

- hostgen.json: rac3-pal's, renamed, with release.json's own keys over it;
- libraries.tsv: rac3-pal's rows, the symbol renamed; a row whose function NTSC-U does not have is
  left out (listed in release_carry.json);
- host/rac3_names.tsv: the shared host C's function names (PAL's) and NTSC-U's
  (tools/hostgen/release_host.py writes the shared C with them, cmake/Games.cmake);
- host/rac3_addresses.h: each RAC3_ADDR(0x...u) the shared host C and profile write, at NTSC-U's
  address (the global of that address renamed, else release.json's `addresses`);
- release/release_carry.json: what rests on less than the same code: the host functions and wraps
  whose NTSC-U twin is other code (`differs`), the rows left out, the addresses not found, and the
  literal addresses the shared host C writes outside RAC3_ADDR() whose global NTSC-U has elsewhere.

    python3 port/tools/hostgen/release_carry_rac3.py --source games/rac3/decomp
"""
from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]
PRIMARY = PORT / "game/rac3-pal"
RELEASE = PORT / "game/rac3-ntsc"

# A function or global name of the decompilation: func_0038EBC8, func_L01_00436F60,
# func_003A8F90__frontbin (frontbin's own), func_0038EBC8__game (the game's body of a wrap), D_001D89B8.
NAME = re.compile(r"\b(func|D)_(?:(L\d\d)_)?([0-9A-F]{8})(?:__(\w+))?(?![0-9A-Za-z_])")
ADDR = re.compile(r"\bRAC3_ADDR\(\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
LADDR = re.compile(r"\bRAC3_LADDR\(\s*(\d+)\s*,\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
HOST_DEF = re.compile(r"^((?:unsigned |signed )?(?:void|int|float|gaddr|unsigned int|long long|unsigned long long|"
                      r"uint32_t|int32_t)) (func_L\d\d_[0-9A-F]{8})\(([^)]*)\)\s*\{", re.M)
MP_TOKEN = re.compile(r"\bmp_func_(?:L\d\d_)?[0-9A-F]{8}(?:__\w+)?\b")
MADDR = re.compile(r"\bRAC3_MADDR\(\s*0x([0-9A-Fa-f]{1,8})u?\s*\)")
ALIAS = re.compile(r"(func_[0-9A-F]{8})(_[0-9A-F]{8})")
ALIAS_TOKEN = re.compile(r"\bfunc_[0-9A-F]{8}_[0-9A-F]{8}(?![0-9A-Za-z_])")
RESIDENT_END = 0x001D5680  # below: the engine core, at the same addresses on both discs
CORE_TEXT_END = 0x0013D420  # the engine core's code ends here (core.text 0x116F80-0x13D420)
LITERAL = re.compile(r"\b0x(00[1-3][0-9A-Fa-f]{5})u?\b")  # an address of the executable's memory
SHARED_SUFFIXES = ("game",)  # a wrap's own body: the same program as the plain name


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


class Names:
    """PAL's function and global names -> NTSC-U's (map.tsv, release_names.json, release.json)."""

    def __init__(self, source: Path, table: dict, hand: dict):
        self.boot: dict[str, str] = {}
        self.front: dict[str, str] = {}
        self.levels: dict[str, str] = {}
        self.status: dict[tuple[str, str], str] = {}
        for line in (source / "config/us/map.tsv").read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 7:
                continue
            pal, program, us, _place, _size, _state, status = cols[:7]
            which = {"frontbin": self.front, "levels": self.levels}.get(program, self.boot)
            kind = "frontbin" if program == "frontbin" else "levels" if program == "levels" else "boot"
            if pal in which and (us == "-" or not which[pal].startswith("func_PAL_")):
                continue  # a second row of one name: the first with a twin is its place
            which[pal] = us if us != "-" else pal.replace("func_", "func_PAL_", 1)
            self.status[(kind, pal)] = status
        # The levels' copies of frontbin's functions (the catalogues' "frontbin" rows) are named after
        # their first place (hostgen's place_kinds_as_levels): PAL's func_L01_00436F60 is the row of
        # frontbin's func_003A0760, NTSC-U's that of frontbin's twin, at its own first place.
        def first_places(path: Path) -> dict[str, str]:
            out = {}
            if not path.is_file():
                return out
            for line in path.read_text().splitlines():
                cols = line.split("\t")
                if line.startswith("#") or len(cols) < 7 or cols[1] != "frontbin":
                    continue
                level, _, address = cols[6].split(",")[0].partition(":")
                if address:
                    out[cols[0]] = f"func_L{int(level):02d}_{int(address, 16):08X}"
            return out
        us_first = first_places(source / "config/us/levels/functions.tsv")
        # Every name NTSC-U's level functions go by in hostgen: its catalogue's rows and the levels'
        # copies of frontbin's functions after their first place.
        self.us_level_names = set(us_first.values())
        us_catalogue = source / "config/us/levels/functions.tsv"
        if us_catalogue.is_file():
            self.us_level_names |= {line.split("\t")[0] for line in us_catalogue.read_text().splitlines()
                                    if not line.startswith("#")}
        for row, first in first_places(source / "config/levels/functions.tsv").items():
            twin = self.front.get(row)
            if first not in self.levels and twin in us_first:
                self.levels[first] = us_first[twin]
                self.status[("levels", first)] = self.status.get(("frontbin", row))
        self.data = {k: v for k, v in table["names"].items() if k.startswith("D_")}
        self.pairs = {k: v["is"] for k, v in hand.get("pairs", {}).items() if not k.startswith("about")}

    def function(self, name: str) -> tuple[str, str | None]:
        """NAME's NTSC-U spelling and its status (None: not a function of map.tsv)."""
        alias = ALIAS.fullmatch(name)
        if alias:
            # A per-caller spelling (func_003D3158_003C1558): the function renamed, the caller's
            # place kept, as the release view writes it (release.py).
            new, status = self.function(alias.group(1))
            return new + alias.group(2), status
        m = NAME.fullmatch(name)
        if not m:
            return name, None
        kind, level, address, suffix = m.groups()
        base = f"{kind}_{level + '_' if level else ''}{address}"
        if kind == "D":
            return (self.pairs.get(name) or self.data.get(base, base)) + (f"__{suffix}" if suffix else ""), "data"
        if name in self.pairs:
            return self.pairs[name], "hand"
        if level:
            table, which = self.levels, "levels"
        elif suffix is not None and suffix.split("__")[0] == "frontbin":
            table, which = self.front, "frontbin"
        else:
            table, which = self.boot, "boot"
        if base not in table:
            # A PAL function map.tsv has no row for, outside the resident core (which keeps PAL's
            # addresses): its PAL spelling is another function's on NTSC-U (PAL's func_0038D0A8 is
            # not NTSC-U's func_0038D0A8, the twin of PAL's func_0038D160), so it is PAL's own.
            if level or int(address, 16) >= RESIDENT_END:
                return name.replace("func_", "func_PAL_", 1), "none"
            return name, None
        new = table[base]
        return new + (f"__{suffix}" if suffix else ""), self.status.get((which, base))

    def own_functions(self, source: Path) -> list[str]:
        """NTSC-U's inventory functions of the executable and frontbin that are no PAL function's
        twin and hold none (config/us/inventory.tsv against map.tsv and the hand pairs)."""
        path = source / "config/us/inventory.tsv"
        if not path.is_file():
            return []
        named = {"boot_elf": set(), "frontbin": set()}
        for program, table in (("boot_elf", self.boot), ("frontbin", self.front)):
            for new in table.values():
                m = NAME.fullmatch(new)
                if m and "PAL" not in new:
                    named[program].add(int(m.group(3), 16))
        for pal, new in self.pairs.items():
            m = NAME.fullmatch(new)
            if m:
                named["frontbin" if pal.endswith("__frontbin") else "boot_elf"].add(int(m.group(3), 16))
        out = []
        for line in path.read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 5 or cols[0] not in named or cols[4] != "function":
                continue
            start, size = int(cols[2], 16), int(cols[3])
            if any(start <= a < start + size for a in named[cols[0]]):
                continue
            out.append(f"func_{start:08X}" + ("__frontbin" if cols[0] == "frontbin" else ""))
        return out

    def gone_twins(self, table: dict) -> list[str]:
        """NTSC-U's names of the PAL definitions release_names.json leaves out ("gone"), in the
        executable and frontbin: NTSC-U's own code there, translated from the disc."""
        out = []
        for pal in table.get("gone", []):
            places = table.get("gone_in", {}).get(pal, ["src/boot_elf/"])
            for program, suffix in (("boot_elf", ""), ("frontbin", "__frontbin")):
                if not any(p.rstrip("/").endswith(program) for p in places):
                    continue
                new, _status = self.function(pal + suffix)
                if NAME.fullmatch(new) and "PAL" not in new:
                    out.append(new)
        return sorted(set(out))

    def text(self, text: str) -> str:
        return NAME.sub(lambda m: self.function(m.group(0))[0], text)


# NTSC-U's own multiplayer maps: split-screen builds of PAL's 44 and 45, which PAL does not have.
MP_OWN_MAPS = {54: 44, 55: 45}


class MpNames:
    """The multiplayer executable's names (port/game/rac3-pal/mp, the host C's mp_func_): its own
    functions paired by release/mp_pairs.json (rac3_mp_pairs.py), level 39's by their places
    (the menus' program the executable loads, named after its own places there, not the
    catalogue's first place)."""

    def __init__(self, source: Path, names: Names):
        path = RELEASE / "release/mp_pairs.json"
        self.pairs = json.loads(path.read_text()) if path.is_file() else {"functions": {}, "addresses": {}}
        self.functions = {k: v["is"] for k, v in self.pairs["functions"].items() if v["is"]}

        def rows(path: Path) -> dict[str, tuple[str, dict[int, int]]]:
            out = {}
            for line in path.read_text().splitlines() if path.is_file() else []:
                cols = line.split("\t")
                if line.startswith("#") or len(cols) < 7:
                    continue
                places = {}
                for item in cols[6].split(","):
                    level, _, address = item.partition(":")
                    if address and level.isdigit():
                        places[int(level)] = int(address, 16)
                out[cols[0]] = (cols[1], places)
            return out
        pal_rows = rows(source / "config/levels/functions.tsv")
        us_rows = rows(source / "config/us/levels/functions.tsv")
        # Level 39's and the maps' (40 and up, the same numbers on both discs) places, PAL's -> NTSC-U's.
        self.levels: dict[int, dict[int, int]] = {}
        for pal, (kind, places) in pal_rows.items():
            twin = names.front.get(pal) if kind == "frontbin" else names.levels.get(pal)
            if not twin or twin not in us_rows:
                continue
            for level, address in places.items():
                if level >= 39 and level in us_rows[twin][1]:
                    self.levels.setdefault(level, {})[address] = us_rows[twin][1][level]
        self.l39 = self.levels.get(39, {})
        # Every PAL place of a twinned row -> NTSC-U's places of that row (own_map).
        self.us_places: dict[tuple[int, int], dict[int, int]] = {}
        for pal, (kind, places) in pal_rows.items():
            twin = names.front.get(pal) if kind == "frontbin" else names.levels.get(pal)
            if twin and twin in us_rows:
                for level, address in places.items():
                    self.us_places[(level, address)] = us_rows[twin][1]

    def own_map(self, name: str, us_level: int) -> str | None:
        """PAL's func_L<pal>_ADDR as NTSC-U's own map us_level spells the same row (NTSC-U's maps 54
        and 55 are split-screen builds of 44 and 45, which PAL does not have)."""
        m = re.fullmatch(r"func_L(\d\d)_([0-9A-F]{8})(__\w+)?", name)
        if not m:
            return None
        pal_level, address = int(m.group(1)), int(m.group(2), 16)
        places = self.us_places.get((pal_level, address))
        if places is None and (pal_level, address - 0x484) in self.us_places:
            base = self.us_places[(pal_level, address - 0x484)].get(us_level)
            return f"func_L{us_level}_{base + 0x484:08X}{m.group(3) or ''}" if base is not None else None
        if places is None or us_level not in places:
            return None
        return f"func_L{us_level}_{places[us_level]:08X}{m.group(3) or ''}"

    def function(self, name: str) -> str | None:
        """NAME (func_..., without the mp_ prefix) as NTSC-U's executable spells it, or None."""
        m = NAME.fullmatch(name)
        if not m or m.group(1) != "func":
            return None
        _kind, level, address, suffix = m.groups()
        tail = f"__{suffix}" if suffix else ""
        if level == "L39":
            new = self.l39.get(int(address, 16))
            if new is None:  # rac3-decomp's map has no twin: the same instructions (mp_pairs.json)
                row = self.pairs.get("level39", {}).get(f"func_L39_{address}") or {}
                return row["is"] + tail if row.get("is") else None
            return f"func_L39_{new:08X}{tail}"
        if level and int(level[1:]) > 39:
            places = self.levels.get(int(level[1:]), {})
            new = places.get(int(address, 16))
            if new is None and int(address, 16) - 0x484 in places:
                # A map's second entry (rac3-pal/mp/maps.py), inside the catalogue's row of its
                # entry: the same distance past NTSC-U's entry.
                new = places[int(address, 16) - 0x484] + 0x484
            return f"func_{level}_{new:08X}{tail}" if new is not None else None
        if level:
            return None
        new = self.functions.get(f"func_{address}")
        return new + tail if new else None


def absent(name: str, names: Names) -> bool:
    """A function name NTSC-U's program has no such function for (PAL's own, or a level copy
    NTSC-U's levels do not have)."""
    if name.startswith("func_PAL_"):
        return True
    plain = re.fullmatch(r"func_([0-9A-F]{6,8})", name)
    if plain and CORE_TEXT_END <= int(plain.group(1), 16) < RESIDENT_END:
        # Past the engine core's code and below the levels: the multiplayer executable's
        # (I5BOOTN.ELF: PAL's func_00193590 and func_1488A8, which level 39's shared C calls), loaded over the boot
        # program's data. NTSC-U's port does not translate that executable.
        return True
    if re.fullmatch(r"func_L\d\d_[0-9A-F]{8}_\w+", name) and not name.endswith(("__game", "__frontbin")):
        # A level copy's own spelling of PAL's (func_L39_006AE8F8_006AE3D8, after the place it is
        # called from): no function of NTSC-U's catalogue.
        return name not in names.us_level_names
    m = NAME.fullmatch(name)
    return bool(m) and m.group(1) == "func" and (
        "_PAL_" in name or (m.group(2) is not None and name.split("__")[0] not in names.us_level_names))


def leave_out_absent(value, names: Names, report: dict, where: str = ""):
    """VALUE without the keys and list items that name a function NTSC-U does not have."""
    if isinstance(value, dict):
        out = {}
        for k, v in value.items():
            if absent(k, names):
                report.setdefault(where or "hostgen.json", []).append(k)
                continue
            out[k] = leave_out_absent(v, names, report, f"{where}.{k}" if where else k)
        return out
    if isinstance(value, list):
        kept = [v for v in value if not (isinstance(v, str) and absent(v, names))]
        report_list = [v for v in value if isinstance(v, str) and absent(v, names)]
        if report_list:
            report.setdefault(where, []).extend(report_list)
        return [leave_out_absent(v, names, report, where) for v in kept]
    return value


def one_name(text: str) -> bool:
    return bool(NAME.fullmatch(text) or ALIAS.fullmatch(text))


def rename_json(value, names: Names):
    """VALUE with every key and string that is one name renamed (prose left as it is)."""
    if isinstance(value, dict):
        return {(names.function(k)[0] if one_name(k) else k): rename_json(v, names) for k, v in value.items()}
    if isinstance(value, list):
        return [rename_json(v, names) for v in value]
    if isinstance(value, str) and one_name(value):
        return names.function(value)[0]
    return value


def merge(base: dict, over: dict) -> dict:
    out = dict(base)
    for k, v in over.items():
        if k.startswith("about") or k.endswith("_why"):
            continue
        if v is None:
            out.pop(k, None)
        elif isinstance(v, dict) and isinstance(out.get(k), dict) and not v.get("replace"):
            out[k] = merge(out[k], v)
        elif (isinstance(v, list) and isinstance(out.get(k), list) and v
              and all(isinstance(x, str) and x.startswith("func_") for x in v)):
            # A list of function names (from_assembly): the release's added to the carried ones.
            # Any other list (sources, skip) replaces the carried one.
            out[k] = out[k] + [x for x in v if x not in out[k]]
        else:
            out[k] = {kk: vv for kk, vv in v.items() if kk != "replace"} if isinstance(v, dict) else v
    return out


def host_files() -> list[Path]:
    return sorted([*(PRIMARY / "host").glob("*.c"), *(PRIMARY / "host").glob("*.h"), PRIMARY / "profile.cpp"])


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--source", type=Path, default=PORT.parent / "games/rac3/decomp",
                    help="rac3-decomp (default: games/rac3/decomp)")
    ap.add_argument("--check", action="store_true",
                    help="write nothing: fail when a table differs from what it would be written as")
    args = ap.parse_args(argv)
    source = args.source.resolve()
    stale: list[str] = []

    def put(name: str, text: str) -> None:
        path = RELEASE / name
        if not args.check:
            path.parent.mkdir(parents=True, exist_ok=True)
        if args.check:
            if not path.is_file() or path.read_text() != text:
                stale.append(name)
            return
        path.write_text(text)
    hand = json.loads((RELEASE / "release/release.json").read_text())
    table = json.loads((RELEASE / "release/release_names.json").read_text())
    names = Names(source, table, hand)
    report: dict = {"about": "What rac3-ntsc's carried tables rest on besides the same code, written by "
                             "port/tools/hostgen/release_carry_rac3.py; do not edit.",
                    "differs": {}, "left_out": {}, "addresses_not_found": [], "hand_addresses": {}}

    # hostgen.json: PAL's, renamed, then the release's own keys.
    primary = json.loads((PRIMARY / "hostgen.json").read_text())
    carried = rename_json(copy.deepcopy(primary), names)
    # The candidates left out are named as the release view names the candidates' files: a PAL
    # name with no map row keeps its spelling there.
    if "candidates_left_out" in primary:
        carried["candidates_left_out"] = {
            (lambda new: k if new.startswith("func_PAL_") else new)(names.function(k)[0]) if one_name(k) else k: v
            for k, v in primary["candidates_left_out"].items()}
    report["hostgen_left_out"] = {}
    kept = carried.pop("candidates_left_out", None)
    carried = leave_out_absent(carried, names, report["hostgen_left_out"])
    if kept is not None:
        carried["candidates_left_out"] = kept
    out = {"about": "Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/hostgen.json, "
                    "config/us/map.tsv and release/release.json; do not edit by hand (release_carry.json "
                    "says what rests on less than the same code)."}
    out.update(merge(carried, hand.get("hostgen", {})))
    # release.json's own rows too: a level name NTSC-U's catalogue does not have is no function of
    # its program (a row written from a build that read PAL's level spellings).
    own_rows = {k: out[k] for k in ("asm", "from_assembly") if k in out}
    own_rows = leave_out_absent(own_rows, names, report["hostgen_left_out"])
    out.update(own_rows)
    # NTSC-U's own functions of the executable and frontbin, which no PAL function is the twin of
    # (the Options page's PROGRESSIVE SCAN and LANGUAGE actions, 0x003B6430 and 0x003B64D8, which
    # its items table points at): roots, translated from the disc's instructions. An inventory row
    # holding a paired function after a remnant (0x003F0140 and its 0x003F0168) is that function's.
    own = names.own_functions(source)
    report["own_functions"] = own
    # The twins of PAL definitions the release view leaves out (release_names.json "gone": NTSC-U's
    # code differs there): roots too. One no C calls by name has no stub for the assembly stage to
    # write, so a call through a pointer table finds no function: level 27's and 28's load runs
    # NTSC-U's 0x003BE710 (PAL's func_003BE800) from the loader's state table at 0x0031C910, and
    # without it the pack's WAD destination is never set ("WAD buffer lies outside game RAM").
    gone = names.gone_twins(table)
    report["gone_roots"] = gone
    out["roots"] = list(dict.fromkeys([*out.get("roots", []), *own, *gone]))
    # The levels' functions the shared host C defines (wraps and replacements): PAL's prototype is
    # its C's, NTSC-U's would be its registers' (its levels are translated from the disc), and the
    # two clash where the host C defines it. The host C's own prototype is the assembly signature.
    signatures = out.setdefault("asm", {}).setdefault("signatures", {})
    # (Not the libraries' host rows: hostgen takes no assembly signature for a function the port
    # answers itself.)
    host_rows = {names.function(c[0])[0] for c in (line.split("\t") for line in
                 (PRIMARY / "libraries.tsv").read_text().splitlines())
                 if not c[0].startswith("#") and len(c) > 3 and c[3] == "host"}
    for path in host_files():
        if path.suffix != ".c":
            continue
        for m in HOST_DEF.finditer(strip_comments(path.read_text())):
            ret, name, params = m.group(1), m.group(2), m.group(3)
            new, _status = names.function(name)
            if absent(new, names) or new in signatures or new in host_rows:
                continue
            types = [] if params.strip() in ("", "void") else [
                re.sub(r"\s*\b\w+$", "", p.strip()) or p.strip() for p in params.split(",")]
            signatures[new] = f"{ret} ({', '.join(t.replace('gaddr', 'unsigned int') for t in types) or 'void'})"
    put("hostgen.json", json.dumps(out, indent=2) + "\n")

    # checks/result_allow.tsv: PAL's rows (caller, callee, why) in NTSC-U's names.
    if primary.get("result_allow"):
        allow = ["# Written by port/tools/hostgen/release_carry_rac3.py from rac3-pal's table, in NTSC-U's names;",
                 "# do not edit."]
        for line in (PRIMARY / primary["result_allow"]).read_text().splitlines():
            cols = line.split("\t")
            if line.startswith("#") or len(cols) < 3:
                if line.startswith("#"):
                    allow.append(line)
                continue
            new = [names.function(c)[0] if one_name(c) else c for c in cols[:2]]
            if any(absent(c, names) for c in new):
                continue
            allow.append("\t".join([*new, *cols[2:]]))
        put(primary["result_allow"], "\n".join(allow) + "\n")

    # libraries.tsv: the rows renamed.
    lines = (PRIMARY / "libraries.tsv").read_text().splitlines()
    rows = ["# Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/libraries.tsv in",
            "# NTSC-U's names (config/us/map.tsv); do not edit. The notes are PAL's, its addresses as PAL has them.",
            "# Rows of a function NTSC-U does not have are left out (release/release_carry.json)."]
    drop = set(hand.get("libraries_left_out", {}))
    for line in lines:
        if line.startswith("#") or not line.strip():
            if line.startswith("# symbol"):
                rows.append(line)
            continue
        cols = line.split("\t")
        new, status = names.function(cols[0])
        if cols[0] in drop:
            report["left_out"][cols[0]] = hand["libraries_left_out"][cols[0]]
            continue
        if new.startswith("func_PAL_") or status == "none" or absent(new, names):
            report["left_out"][cols[0]] = f"NTSC-U has no such function ({status})"
            continue
        if status not in ("match", "asm", None, "data", "hand"):
            report["differs"][cols[0]] = {"is": new, "status": status, "port": cols[3] if len(cols) > 3 else ""}
        cols[0] = new
        rows.append("\t".join(cols))
    put("libraries.tsv", "\n".join(rows) + "\n")

    # host/rac3_names.tsv: the shared host C's function names.
    used: dict[str, str] = {}
    for path in host_files():
        if path.suffix == ".cpp":
            continue
        text = strip_comments(path.read_text())
        # A per-caller spelling (func_003CB798_003C3C18, a function's place of its own in the
        # catalogue) is no NAME token: renamed as names.function renames it (ALIAS).
        tokens = [m.group(0) for m in NAME.finditer(text) if m.group(1) == "func"]
        tokens += [m.group(0) for m in ALIAS_TOKEN.finditer(text)]
        for token in tokens:
            new, status = names.function(token)
            if new != token:
                used[token] = new
                if status not in ("match", "asm", "hand"):
                    report["differs"].setdefault(token, {"is": new, "status": status, "port": "host C"})
    mp = MpNames(source, names)
    for path in host_files():
        if path.suffix == ".cpp":
            continue
        for m in MP_TOKEN.finditer(strip_comments(path.read_text())):
            new = mp.function(m.group(0)[3:])
            if new is None:
                report["mp_not_paired"] = sorted({*report.get("mp_not_paired", []), m.group(0)})
            elif "mp_" + new != m.group(0):
                used[m.group(0)] = "mp_" + new
    if not args.check:
        (RELEASE / "host").mkdir(exist_ok=True)
    put("host/rac3_names.tsv", 
        "# The shared host C's function names (PAL's, port/game/rac3-pal/host) and NTSC-U's. Written by\n"
        "# port/tools/hostgen/release_carry_rac3.py from config/us/map.tsv; do not edit.\n"
        + "".join(f"{k}\t{v}\n" for k, v in sorted(used.items())))

    # host/rac3_addresses.h: every RAC3_ADDR the shared C and the profile write.
    addresses = {}
    hand_addresses = {k: v for k, v in hand.get("addresses", {}).items() if not k.startswith("about")}
    for path in host_files():
        for m in ADDR.finditer(strip_comments(path.read_text())):
            pal = int(m.group(1), 16)
            key = f"0x{pal:08X}u"
            if key in addresses:
                continue
            hand_row = hand_addresses.get(f"0x{pal:08X}")
            if hand_row:
                addresses[key] = (int(hand_row["is"], 16), "release.json: " + hand_row["why"])
                report["hand_addresses"][f"0x{pal:08X}"] = hand_row
                continue
            found = None
            for kind in ("D", "func"):
                new, status = names.function(f"{kind}_{pal:08X}")
                if new != f"{kind}_{pal:08X}" or (status is not None and status != "data"):
                    if "PAL" not in new:
                        found = (int(new.split("_")[-1], 16), f"{kind}_{pal:08X} ({'map.tsv ' + status if kind == 'func' else 'names.tsv'})")
                        break
                elif kind == "D" and f"D_{pal:08X}" in names.data:
                    found = (pal, f"D_{pal:08X} (names.tsv, the same address)")
                    break
                elif kind == "func" and status == "match":
                    found = (pal, f"func_{pal:08X} (map.tsv, the same place)")
                    break
            if found is None:
                report["addresses_not_found"].append(f"0x{pal:08X}")
                addresses[key] = (None, "not known on NTSC-U")
            else:
                addresses[key] = found
    # Each RAC3_LADDR(level, 0x...u): a level's own global, paired by rac3_level_pairs.py.
    level_rows = {k: v for k, v in hand.get("level_addresses", {}).items() if not k.startswith("about")}
    level_addresses = {}
    for path in host_files():
        for m in LADDR.finditer(strip_comments(path.read_text())):
            level, pal = int(m.group(1)), int(m.group(2), 16)
            row = level_rows.get(f"L{level:02d}:{pal:08X}")
            key = f"{level}_0x{pal:08X}u"
            if row:
                level_addresses[key] = (int(row["is"], 16), row["why"])
            else:
                level_addresses[key] = (None, "not paired (rac3_level_pairs.py)")
                report["addresses_not_found"].append(f"L{level:02d}:{pal:08X}")
    head = ("/* Written by port/tools/hostgen/release_carry_rac3.py; do not edit. */\n"
            "/* SPDX-License-Identifier: GPL-3.0-or-later\n"
            " * Copyright (c) 2026 the OpenRAC contributors\n"
            " *\n"
            " * NTSC-U's address for each PAL address the shared host C writes as RAC3_ADDR(0x...u) (rac3_version.h):\n"
            " * the global or function of that address renamed (config/us/names.tsv, config/us/map.tsv), else\n"
            " * release/release.json's `addresses`. One not known is 0: what reads it reads nothing of the game's. */\n"
            "#ifndef OPENRAC_RAC3_ADDRESSES_H\n#define OPENRAC_RAC3_ADDRESSES_H\n\n")
    # Each one's evidence on a line of its own above it: the defines stay within the line length.
    body = "".join(f"/* {v[1]} */\n#define RAC3_ADDR_{k} {('0x%08Xu' % v[0]) if v[0] is not None else '0u'}\n"
                   for k, v in sorted(addresses.items()))
    maddr = {}
    for path in host_files():
        for m in MADDR.finditer(strip_comments(path.read_text())):
            pal = int(m.group(1), 16)
            row = mp.pairs["addresses"].get(f"0x{pal:08X}") or {}
            fn = mp.functions.get(f"func_{pal:08X}")
            hand_mp = hand.get("mp_addresses", {}).get(f"0x{pal:08X}")
            if hand_mp:
                maddr[f"0x{pal:08X}u"] = (int(hand_mp["is"], 16), "release.json: " + hand_mp["why"])
            elif row.get("is"):
                maddr[f"0x{pal:08X}u"] = (int(row["is"], 16), row["why"])
            elif fn:
                maddr[f"0x{pal:08X}u"] = (int(fn[5:13], 16),
                                          "the function's pair: " + mp.pairs["functions"][f"func_{pal:08X}"]["why"])
            else:
                maddr[f"0x{pal:08X}u"] = (None, "not paired (rac3_mp_pairs.py)")
                report["addresses_not_found"].append(f"mp:{pal:08X}")
    body += "\n/* The multiplayer executable's (release/mp_pairs.json). */\n" + "".join(
        f"/* {v[1]} */\n#define RAC3_MADDR_{k} {('0x%08Xu' % v[0]) if v[0] is not None else '0u'}\n"
        for k, v in sorted(maddr.items()))
    body += "\n/* Levels' own globals (release.json's level_addresses). */\n" + "".join(
        f"/* {v[1]} */\n#define RAC3_LADDR_{k} {('0x%08Xu' % v[0]) if v[0] is not None else '0u'}\n"
        for k, v in sorted(level_addresses.items()))
    put("host/rac3_addresses.h", head + body + "\n#endif /* OPENRAC_RAC3_ADDRESSES_H */\n")

    # Literal addresses outside RAC3_ADDR() whose global NTSC-U has elsewhere: the shared C reads
    # PAL's there on NTSC-U too. Listed, and printed, for whoever wrote them to wrap.
    report["unwrapped"] = {}
    for path in host_files():
        if path.suffix == ".cpp":
            continue  # the profile holds each disc's table (kPal, kUs)
        text = ADDR.sub("", re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', strip_comments(path.read_text())))
        for m in LITERAL.finditer(text):
            pal = int(m.group(1), 16)
            new = names.data.get(f"D_{pal:08X}")
            if new and new != f"D_{pal:08X}" and "_PAL_" not in new:
                report["unwrapped"].setdefault(path.relative_to(PORT).as_posix(), []).append(
                    f"0x{pal:08X} (NTSC-U's {new})")
    # mp/: the multiplayer executable's program, rac3-pal's in NTSC-U's names.
    if (PRIMARY / "mp/hostgen.json").is_file() and mp.functions:
        def mp_json(value):
            if isinstance(value, dict):
                return {(mp.function(k) if NAME.fullmatch(k) else k): mp_json(v)
                        for k, v in value.items() if not (NAME.fullmatch(k) and mp.function(k) is None)}
            if isinstance(value, list):
                return [mp_json(v) for v in value
                        if not (isinstance(v, str) and NAME.fullmatch(v) and mp.function(v) is None)]
            if isinstance(value, str) and NAME.fullmatch(value):
                return mp.function(value) or value
            return value
        pal_mp = json.loads((PRIMARY / "mp/hostgen.json").read_text())
        mp_cfg = mp_json(pal_mp)
        # NTSC-U's own maps (MP_OWN_MAPS): what PAL's hostgen.json says for the map each is a build
        # of, in that map's spelling.
        if pal_mp.get("level_copies"):
            for us_level, pal_level in MP_OWN_MAPS.items():
                for key in ("retail", ):
                    mp_cfg["asm"][key]["levels"] = sorted({*mp_cfg["asm"][key]["levels"], us_level})
                mp_cfg["asm"]["discover_levels"] = sorted({*mp_cfg["asm"]["discover_levels"], us_level})
                for key in ("level_copies", "inner_extents"):
                    if key in mp_cfg:
                        mp_cfg[key]["levels"] = sorted({*mp_cfg[key]["levels"], us_level})
                for key in ("roots", "inner_entries"):
                    own = [mp.own_map(n, us_level) for n in pal_mp.get(key, [])
                           if n.startswith(f"func_L{pal_level}_")]
                    mp_cfg[key] = [*mp_cfg.get(key, []), *[n for n in own if n]]
                sigs = mp_cfg["asm"].setdefault("copy_signatures", {})
                for n, sig in pal_mp["asm"].get("copy_signatures", {}).items():
                    if n.startswith(f"func_L{pal_level}_"):
                        own = mp.own_map(n, us_level)
                        if own:
                            sigs[own] = sig
        mp_out = {"about": "Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/mp/hostgen.json "
                           "and release/mp_pairs.json; do not edit by hand."}
        mp_out.update(merge(mp_cfg, hand.get("mp_hostgen", {})))
        put("mp/hostgen.json", json.dumps(mp_out, indent=2) + "\n")
        # host/mp_maps.h: each multiplayer map's places of the functions its host C stands for
        # (rac3-pal/mp/maps.py), NTSC-U's. A map whose places are not all known is left out, its
        # libraries rows with it.
        maps_lines = ["/* SPDX-License-Identifier: GPL-3.0-or-later",
                      " * Copyright (c) 2026 the OpenRAC contributors",
                      " *",
                      " * Written by port/tools/hostgen/release_carry_rac3.py from rac3-pal's (port/game/rac3-pal/mp/maps.py):",
                      " * do not edit by hand. Each multiplayer map's places of the functions mp/maps.py CARRIED lists,",
                      " * NTSC-U's. */",
                      "/* clang-format off */"]
        maps_left = set()
        own_lines: list[str] = []
        for line in (PRIMARY / "host/mp_maps.h").read_text().splitlines() if (PRIMARY / "host/mp_maps.h").is_file() else []:
            m = re.fullmatch(r"(MP_MAP\w*)\((\d+), (.*)\)", line)
            if not m:
                continue
            level = int(m.group(2))
            new_places = [mp.function(f"func_L{level}_{a}") for a in m.group(3).split(", ")]
            if any(p is None for p in new_places):
                maps_left.add(level)
                continue
            maps_lines.append(f"{m.group(1)}({level}, " + ", ".join(p.split("_")[-1] for p in new_places) + ")")
            for us_level, pal_level in MP_OWN_MAPS.items():
                if level == pal_level:
                    own = [mp.own_map(f"func_L{level}_{a}", us_level) for a in m.group(3).split(", ")]
                    if all(own):
                        own_lines.append(f"{m.group(1)}({us_level}, " + ", ".join(p.split("_")[-1] for p in own) + ")")
                    else:
                        maps_left.add(us_level)
        maps_lines += own_lines
        maps_lines = [line for line in maps_lines
                      if not (re.match(r"MP_MAP\w*\((\d+),", line)
                              and int(re.match(r"MP_MAP\w*\((\d+),", line).group(1)) in maps_left)]
        if maps_left:
            report["mp_maps_left_out"] = sorted(maps_left)
        put("host/mp_maps.h", "\n".join(maps_lines) + "\n/* clang-format on */\n")
        mp_rows = ["# Written by port/tools/hostgen/release_carry_rac3.py from port/game/rac3-pal/mp/libraries.tsv in",
                   "# NTSC-U's names (release/mp_pairs.json); do not edit. The notes are PAL's."]
        for line in (PRIMARY / "mp/libraries.tsv").read_text().splitlines():
            if line.startswith("#") or not line.strip():
                if line.startswith("# symbol"):
                    mp_rows.append(line)
                continue
            cols = line.split("\t")
            new = mp.function(cols[0])
            mm = re.match(r"func_L(\d\d)_", cols[0])
            if mm and int(mm.group(1)) in maps_left:
                new = None
            if new is None:
                report["left_out"]["mp:" + cols[0]] = "no NTSC-U pair (release/mp_pairs.json)"
                continue
            mp_rows.append("\t".join([new, *cols[1:]]))
        for us_level, pal_level in MP_OWN_MAPS.items():
            if us_level in maps_left:
                continue
            for line in (PRIMARY / "mp/libraries.tsv").read_text().splitlines():
                cols = line.split("\t")
                if line.startswith("#") or not cols[0].startswith(f"func_L{pal_level}_"):
                    continue
                own = mp.own_map(cols[0], us_level)
                if own is None:
                    report["left_out"][f"mp:{cols[0]} as map {us_level}"] = "no NTSC-U place"
                    continue
                cols[2] = cols[2].replace(f"Map{pal_level}", f"Map{us_level}")
                cols[4] = cols[4].replace(f"map {pal_level}'s", f"map {us_level}'s (NTSC-U's, a build of map {pal_level})")
                mp_rows.append("\t".join([own, *cols[1:]]))
        put("mp/libraries.tsv", "\n".join(mp_rows) + "\n")
    report["differs"] = dict(sorted(report["differs"].items()))
    put("release/release_carry.json", json.dumps(report, indent=1) + "\n")
    print(f"rac3-ntsc: {len(rows) - 4} library rows ({len(report['left_out'])} left out), {len(used)} host names, "
          f"{len(addresses)} addresses ({len(report['addresses_not_found'])} not found), "
          f"{len(report['differs'])} on other code")
    for file, found in report["unwrapped"].items():
        print(f"  {file}: not in RAC3_ADDR(), elsewhere on NTSC-U: {', '.join(dict.fromkeys(found))}")
    if stale:
        print(f"rac3-ntsc: {', '.join(stale)} not what release_carry_rac3.py writes from rac3-pal's tables "
              f"and the decompilation: run it (port/game/rac3-ntsc/README.md)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

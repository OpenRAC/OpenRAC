#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""A Ratchet & Clank release's port, carried from PAL v2.00's (port/game/rac1-pal).

rac1-decomp builds PAL v1.00 and NTSC-J from PAL v2.00's C (games/rac1/decomp/docs/VERSIONS.md), so a
release's port is rac1-pal's read through the release view (release.py, its "release" table):
every address name of the decompilation renamed to the release's address, the release's own
definitions spliced into their shared files. What the port itself knows by PAL v2.00's names and
addresses is carried here, into port/game/rac1-<release>/:

| File | |
|---|---|
| `hostgen.json` | rac1-pal's, every function and address the release's, the places from the release's catalogue |
| `libraries.tsv`, `split_places.tsv` | rac1-pal's rows by the release's names and places |
| `hand/` | rac1-pal's hand C and its tables, in the release's names |
| `host/*.c` | rac1-pal's own host C, in the release's names and addresses |
| `host/rac1_names.tsv` | the shared host C's function names in the release (release_host.py) |
| `host/rac1_version.h` | the shared host C's addresses in the release (RAC1_ADDR) and rac1-pal's table, carried |
| `release_carry.json` | what rests on less than the release's check or the same instruction of the same code |

A function's name comes from the release table (its catalogue, the release's check); an address
from the release's check (version_relocs.py: the value the release's bytes gave the name the C
uses at that address), else versionmap's `exact` pair, else its `between` or `guess`, listed.

    python3 port/tools/hostgen/release_rac1.py --game port/game/rac1-pal1 --primary port/game/rac1-pal \\
        --map build/versionmap/rac1-pal--rac1-pal1.json
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "versionmap"))
import release as rel  # noqa: E402
from versionmap import Map  # noqa: E402

ROOT = HERE.parents[2]
FUNC = re.compile(r"(?<![\w])func_(?:L(\d\d)_)?([0-9A-F]{8})(?![0-9A-Za-z])")
PLACE = re.compile(r"\b(\d\d):([0-9A-F]{8})\b")
LITERAL = re.compile(r"\b0x(00[0-9A-Fa-f]{6})([uU]?)\b")
ADDR = re.compile(r"\bRAC1_ADDR\(\s*0x([0-9A-F]{8})u\s*\)")
LADDR = re.compile(r"\bRAC1_LADDR\(\s*(\d\d)\s*,\s*0x([0-9A-F]{8})u\s*\)")
OMIT = re.compile(r"\bRAC1_OMIT\(\s*(\w+)\s*\)")
MAIN = 0x15F000          # where PAL v2.00's main segment (the part a level replaces) starts


class Carry:
    """Where the release has what PAL v2.00 has, and how sure that is."""

    def __init__(self, table: rel.Release, raw: dict, vmap: Map, primary_rows: list[dict], rows: list[dict]):
        self.table, self.raw, self.map = table, raw, vmap
        self.how: dict[str, str] = raw.get("how", {})
        self.report: dict[str, dict] = {"functions": {}, "addresses": {}, "dropped": {}}
        # PAL v2.00's places and the release's, by name (the k-th of a program is the k-th).
        self.p_places = {r["name"]: r["places"] for r in primary_rows}
        self.r_places = {r["name"]: r["places"] for r in rows}
        self.p_at: dict[tuple, str] = {}
        for r in primary_rows:
            for p in r["places"]:
                self.p_at.setdefault(p, r["name"])
        # Every address an address name of the release table states, per program: what the release's
        # check (or versionmap's exact pair) gave the name the C uses there.
        self.by_address: dict[tuple[str, int], tuple[int, str]] = {}
        # The table lists a name only where the release's differs; the evidence covers the others.
        pairs = {**{k: k for k in self.how if k not in table.names}, **table.names}
        for name, new in pairs.items():
            m, n = rel.NAME.fullmatch(name), rel.NAME.fullmatch(new)
            if not m or not n or m.group(2) or n.group(2):
                continue
            prog = f"{int(m.group(3)):02d}" if m.group(3) else "boot"
            self.by_address.setdefault((prog, int(m.group(4), 16)), (int(n.group(4), 16), self.how.get(name, "")))

    # -- functions

    def function(self, name: str, why: str, calls: bool = False) -> str | None:
        """NAME (a function of PAL v2.00, any suffix kept) in the release, or None (none there).
        A function the release does not have whose callers its check sends to another (FUN_...) is
        that one for a call (CALLS), and none for a table, which would bind or replace it."""
        new = self.table.name(name)
        core = FUNC.search(name)
        key = core.group(0) if core else name
        how = self.how.get(key, "function: unchanged" if new == name else "")
        if "(marked)" in how or "_V2_" in new:
            self.report["dropped"][name] = f"{why}: not in the release"
            return None
        if new.startswith("FUN_"):
            if not calls:
                self.report["dropped"][name] = f"{why}: not in the release (its callers reach {new})"
                return None
            new = "func_" + new[4:]
        if not how.startswith("function: carried (same code)") and how not in ("function: unchanged",):
            self.report["functions"][name] = {"as": new, "how": how, "for": why}
        return new

    def place(self, level: int, address: int, why: str) -> int | None:
        """PAL v2.00's function place LEVEL:ADDRESS in the release: the same place of the same
        function there (its catalogue), else versionmap's pair."""
        name = self.p_at.get((level, address))
        if name is not None:
            theirs = sorted(a for p, a in self.p_places[name] if p == level)
            new = self.table.name(name)
            mine = sorted(a for p, a in self.r_places.get(name, self.r_places.get(new, [])) if p == level)
            if address in theirs and len(mine) == len(theirs):
                return mine[theirs.index(address)]
        hit = self.map.function(address, f"level:{level:02d}")
        if hit is not None:
            if hit[1] != "same":
                self.report["functions"][f"{level:02d}:{address:08X}"] = {"as": f"{level:02d}:{hit[0]:08X}",
                                                                        "how": f"versionmap ({hit[1]})", "for": why}
            return hit[0]
        self.report["dropped"][f"{level:02d}:{address:08X}"] = f"{why}: no place in the release"
        return None

    # -- addresses

    def address(self, address: int, level: int | None, why: str) -> int | None:
        """PAL v2.00's address (in LEVEL's program, or the executable's) in the release."""
        if level is not None and address >= MAIN:
            # The level's own main segment: the executable's names there are another program's.
            programs = [f"{level:02d}"]
        elif level is not None:
            programs = ["boot", f"{level:02d}"]
        else:
            programs = ["boot"]
        for prog in programs:
            hit = self.by_address.get((prog, address))
            if hit is not None:
                if not hit[1].endswith("the release's check") and "function: carried" not in hit[1]:
                    self.report["addresses"][self._key(address, level)] = {"as": f"0x{hit[0]:08X}", "how": hit[1], "for": why}
                return hit[0]
        for prog in programs:
            key = "boot" if prog == "boot" else f"level:{prog}"
            fn = self.map.function(address, key)
            if fn is not None and fn[1] == "same":
                return fn[0]
            hit = self.map.data(address, key)
            if hit is not None:
                if hit[1] != "exact":
                    self.report["addresses"][self._key(address, level)] = {"as": f"0x{hit[0]:08X}", "how": f"versionmap ({hit[1]})", "for": why}
                return hit[0]
        self.report["dropped"][self._key(address, level)] = f"{why}: no address in the release"
        return None

    @staticmethod
    def _key(address: int, level: int | None) -> str:
        return f"{level:02d}:0x{address:08X}" if level is not None else f"0x{address:08X}"


# ---------------------------------------------------------------- the tables

def carry_names(c: Carry, names: list[str], why: str) -> list[str]:
    out = []
    for n in names:
        new = c.function(n, why)
        if new is not None:
            out.append(new)
    return out


def carry_config(c: Carry, cfg: dict, version: str, out_id: str, title: str, serial: str, rate: int,
                 gp: int, main: int, top: int) -> dict:
    new = json.loads(json.dumps(cfg))
    new.update({"id": out_id, "title": title, "serial": serial, "frame_rate": rate})
    new["entry"] = c.function(cfg["entry"], "entry")
    new["places"] = f"config/versions/{version}/functions.tsv"
    new["release"] = {"version": version, "table": "release_names.json", "splice": f"src/versions/{version}"}
    # Names by address, and the FUN_ spelling the release table gives a PAL v2.00 function the
    # release has not, whose callers reach another (release.py).
    new["names"] = {"code": [r"^func_(?:L(?P<overlay>\d\d)_)?(?P<addr>[0-9A-Fa-f]{6,8})(?:_\w*)?$",
                             r"^FUN_(?:L(?P<overlay>\d\d)_)?(?P<addr>[0-9A-Fa-f]{8})$"],
                    "data": [r"^(?:D|func)_(?:L(?P<overlay>\d\d)_)?(?P<addr>[0-9A-Fa-f]{6,8})(?:_\w*)?$"]}
    # Every section from the main segment on (rac1-pal: 0x15F000..0x250000): the release's own.
    new["relocate"] = [main, top]
    for key in ("level_hooks", "roots", "host_functions", "returns_64", "ps2_float"):
        if key in cfg:
            new[key] = carry_names(c, cfg[key], key)
    if "spilled_locals" in cfg:
        new["spilled_locals"] = {c.function(k, "spilled_locals"): v for k, v in cfg["spilled_locals"].items()
                                 if c.function(k, "spilled_locals")}
    if "expanded_target_table_addresses" in cfg:
        new["expanded_target_table_addresses"] = [
            f"0x{c.address(int(a, 16), None, 'expanded_target_table_addresses'):08X}"
            for a in cfg["expanded_target_table_addresses"]]
    asm = new.get("asm")
    if asm:
        asm["in_place"] = carry_names(c, asm.get("in_place", []), "asm in_place")
        asm["shadows"] = {k: c.function(v, f"asm shadow {k}") for k, v in asm.get("shadows", {}).items()}
        asm["gp"] = f"0x{gp:08X}"
        exe = cfg["asm"]["retail"]["exe"].split("/")[-1] if version == "pal_v1" else "SCPS_150.37"
        asm["retail"] = {"exe": f"baserom/{version}/{exe}", "dump": f"baserom/{version}/overlays"}
        # The primary's assembly is PAL v2.00's: the release's is read from its own programs.
        asm["dirs"] = []
    return new


def carry_libraries(c: Carry, text: str) -> str:
    out = []
    for line in text.splitlines():
        cols = line.split("\t")
        if line.startswith("#") or not line.strip() or not cols[0].startswith("func_"):
            out.append(line)
            continue
        new = c.function(cols[0], f"libraries.tsv {cols[0]}")
        if new is None:
            continue
        out.append("\t".join([new] + cols[1:]))
    return "\n".join(out) + "\n"


def carry_split_places(c: Carry, text: str) -> str:
    out = []
    for line in text.splitlines():
        cols = line.split("\t")
        if line.startswith("#") or len(cols) < 3:
            out.append(line)
            continue
        name = c.function(cols[0], f"split_places {cols[0]}")
        variant = c.function(cols[1], f"split_places {cols[0]} variant")
        places = []
        for item in cols[2].split(","):
            m = PLACE.fullmatch(item.strip())
            if not m:
                continue
            at = c.place(int(m.group(1)), int(m.group(2), 16), f"split_places {cols[0]}")
            if at is not None:
                places.append(f"{m.group(1)}:{at:08X}")
        if name is None or variant is None or not places:
            continue
        rest = [c.function(x, f"split_places {cols[0]} executable") or x for x in cols[3:4]] + cols[4:]
        out.append("\t".join([name, variant, ",".join(places)] + rest))
    return "\n".join(out) + "\n"


def carry_c(c: Carry, text: str, why: str) -> str:
    """C of the port in the release's names (every address name) and addresses (literals in the
    game's ranges, RAC1_ADDR/RAC1_LADDR left to rac1_version.h)."""
    text = c.table.text(text)

    def literal(m: re.Match) -> str:
        address = int(m.group(1), 16)
        if not 0x100000 <= address < 0x2000000:
            return m.group(0)
        new = c.address(address, None, why)
        return f"0x{new:08X}{m.group(2)}" if new is not None else m.group(0)
    def level_literal(m: re.Match) -> str:
        new = c.address(int(m.group(2), 16), int(m.group(1)), why)
        return f"OPENRAC_LDATA({m.group(1)}, 0x{new:08X}u)" if new is not None else m.group(0)
    # A level's datum (OPENRAC_LDATA(level, address)): in that level's program.
    text = re.sub(r"\bOPENRAC_LDATA\(\s*(\d+)\s*,\s*0x(00[0-9A-Fa-f]{6})[uU]?\s*\)", level_literal, text)
    # Only outside RAC1_ADDR (the table maps those), the levels' data above, and comments.
    parts = re.split(r"(/\*.*?\*/|//[^\n]*|RAC1_L?ADDR\([^)]*\)|OPENRAC_LDATA\([^)]*\))", text, flags=re.S)
    return "".join(p if i % 2 else LITERAL.sub(literal, p) for i, p in enumerate(parts))


def host_names(c: Carry, shared: list[Path]) -> str:
    """The shared host C's function names (as release_host.py reads them): every func_ token, but
    those inside RAC1_OMIT(), which the version table answers with PAL v2.00's names."""
    found: set[str] = set()
    for path in shared:
        text = OMIT.sub("", path.read_text())
        found.update(m.group(0) for m in re.finditer(r"\bfunc_\w+\b", text))
    rows = ["# The shared host C's function names (PAL v2.00's) and this release's. Written by",
            "# port/tools/hostgen/release_rac1.py (release_carry.json says how each rests); do not edit."]
    for name in sorted(found):
        if not FUNC.match(name):
            continue
        new = c.function(name, "shared host C", calls=True)
        if new is not None and new != name:
            rows.append(f"{name}\t{new}")
    return "\n".join(rows) + "\n"


def version_header(c: Carry, primary_header: str, shared: list[Path], label: str, serial_text: str,
                   save_folder: str, vtbl: list[int], extra: dict[str, str], omit: dict[str, str],
                   added: dict[str, list[str]]) -> str:
    """rac1-pal's rac1_version.h for the release: its hand part carried (each address the release's),
    and every RAC1_ADDR the shared host C uses."""
    # The addresses the shared C uses in this release: hostnames.py's reading (code outside
    # comments, and outside what the table leaves out with RAC1_OMIT).
    import hostnames
    omitted = set(re.findall(r"#define RAC1_OMIT_(\w+) 1", primary_header)) | set(omit)
    keys, _ = hostnames.used(shared, omitted)
    used: dict[str, tuple[int | None, int]] = {}
    for key in keys:
        m = re.fullmatch(r"RAC1_ADDR_(?:L(\d\d)_)?0x([0-9A-F]{8})u", key)
        if m:
            used[key] = (int(m.group(1)) if m.group(1) else None, int(m.group(2), 16))
    head = primary_header.split("#define RAC1_ADDR(a) a", 1)[0]
    body = primary_header.split("#define RAC1_LADDR(n, a) a", 1)[1]
    head = head.replace("Ratchet & Clank (PAL, SCES-50916): the names and addresses of the shared host C\n"
                        " * (port/game/rac1/host, rac1.h) in this version. The shared C is written with games/rac1's names\n"
                        " * and addresses, which are this version's own: nothing is renamed.",
                        f"Ratchet & Clank ({label}, {serial_text}): the addresses of the shared host C (port/game/rac1/host,\n"
                        " * rac1.h) in this release, carried from rac1-pal's table by port/tools/hostgen/release_rac1.py\n"
                        " * (release_carry.json says what rests on less than the release's check). The function names are\n"
                        " * host/rac1_names.tsv's (release_host.py).")
    out = [head.rstrip("\n"), "",
           "gaddr rac1_ship_destination_text(int id);",
           "#define RAC1_DESTINATION_TEXT(id) rac1_ship_destination_text(id)", ""]

    def fix(m: re.Match) -> str:
        address = int(m.group(1), 16)
        new = c.address(address, None, "rac1_version.h")
        return f"0x{new:08X}u" if new is not None else m.group(0)
    vtbl_rows = ",\n        ".join(", ".join(f"0x{a:08X}u" for a in vtbl[i:i + 7]) for i in range(0, len(vtbl), 7))
    body = re.sub(r"static const unsigned addresses\[\] = \{.*?\};",
                  lambda _: "static const unsigned addresses[] = {\n        " + vtbl_rows + ",\n    };", body, flags=re.S)
    vtbl_at = body.index("static const unsigned addresses[]")
    vtbl_end = body.index("};", vtbl_at)
    body = re.sub(r"\b0x(00[0-9A-F]{6})u\b", fix, body[:vtbl_at]) + body[vtbl_at:vtbl_end] + \
        re.sub(r"\b0x(00[0-9A-F]{6})u\b", fix, body[vtbl_end:])
    body = body.replace('"BESCES-50916RATCHET"', f'"{save_folder}"')
    for k, v in extra.items():
        body = re.sub(rf"#define {k} .*", f"#define {k} {v}", body)
    body = body.replace("#endif /* OPENRAC_RAC1_VERSION_H */", "").rstrip("\n")
    out.append(body.lstrip("\n"))
    for name, why in omit.items():
        out += [f"/* {why} */", f"#define RAC1_OMIT_{name} 1"]
    for name, (value, why) in added.items():
        out += [f"/* {why} */", f"#define {name} {value}"]
    out += ["", "/* ---- generated below ---- */", "",
            "/* Addresses: games/rac1's, this version's (RAC1_LADDR: in level NN's program). */",
            "#define RAC1_ADDR(a) RAC1_ADDR_##a", "#define RAC1_LADDR(n, a) RAC1_ADDR_L##n##_##a"]
    for key in sorted(used):
        level, address = used[key]
        new = c.address(address, level, f"shared host C {key}")
        if new is None:
            continue
        out.append(f"#define {key} 0x{new:08X}u")
    out += ["", "#endif /* OPENRAC_RAC1_VERSION_H */", ""]
    return "\n".join(out)


def profile_table(c: Carry, game_state: str, profile: str, out_id: str, label: str, serial_text: str) -> str:
    """game_state.cpp's PAL tables (kRac1Pal, kRac1PalLevels) and profile.cpp's kPalExecutable for
    the release: each address the release's, with how it rests."""
    ident = "".join(part.capitalize() for part in re.split(r"[^A-Za-z0-9]", out_id.removeprefix("rac1-")))
    row = re.compile(r"\{(0x[0-9A-F]+), (0x[0-9A-F]+), (0x[0-9A-F]+), (0x[0-9A-F]+), (0x[0-9A-F]+), ([0-9.]+f), "
                     r"(0x[0-9A-F]+), (0x[0-9A-F]+)\}")
    boot = row.search("{" + game_state.split("const GameAddresses kRac1Pal{", 1)[1])
    levels_text = game_state.split("const GameAddresses kRac1PalLevels[19] = {", 1)[1].split("};", 1)[0]
    levels = row.findall(levels_text)
    exe = re.search(r"kPalExecutable\{(0x[0-9A-F]+)u, (0x[0-9A-F]+)u, (0x[0-9A-F]+)u, (0x[0-9A-F]+)u\}", profile)

    def carry_row(values, level: int | None) -> tuple[str, set[str]]:
        out, hows = [], set()
        for i, v in enumerate(values):
            if i == 5:
                out.append(v)
                continue
            before = len(c.report["addresses"]) + len(c.report["dropped"])
            new = c.address(int(v, 16), level, f"profile {'boot' if level is None else f'level {level}'}")
            hows.add("inferred" if len(c.report["addresses"]) + len(c.report["dropped"]) != before else "proven")
            out.append(f"0x{(new or 0):08X}")
        return "{" + ", ".join(out) + "}", hows
    lines = [f"// Written by port/tools/hostgen/release_rac1.py from game_state.cpp's and profile.cpp's PAL tables. Do not edit.",
             "// SPDX-License-Identifier: GPL-3.0-or-later",
             "// Copyright (c) 2026 the OpenRAC contributors",
             "//",
             f"// {label} ({serial_text}): the profile's tables for this disc, PAL v2.00's addresses as this release",
             "// has them (the release's check, or versionmap's exact pair; release_carry.json lists any other).",
             "#pragma once", "", "namespace openrac::game::rac1 {", ""]
    text, hows = carry_row(boot.groups(), None)
    lines.append(f"inline constexpr GameAddresses kRac1{ident}{text};  // {', '.join(sorted(hows))}")
    lines.append(f"inline constexpr GameAddresses kRac1{ident}Levels[19] = {{")
    for lv, values in enumerate(levels):
        text, hows = carry_row(values, lv)
        lines.append(f"    {text},  // {lv} ({', '.join(sorted(hows))})")
    lines.append("};")
    ev = []
    for v in exe.groups():
        new = c.address(int(v, 16), None, "profile executable")
        ev.append(f"0x{(new or 0):08X}u")
    lines += ["", f"// The executable's addresses (profile.cpp's ExecutableAddresses).",
              f"#define OPENRAC_RAC1_{ident.upper()}_EXECUTABLE {{{', '.join(ev)}}}", "",
              "}  // namespace openrac::game::rac1", ""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game", type=Path, required=True, help="the release's folder (port/game/rac1-pal1)")
    ap.add_argument("--primary", type=Path, required=True, help="port/game/rac1-pal")
    ap.add_argument("--map", type=Path, required=True, help="versionmap's map from rac1/pal to the release")
    ap.add_argument("--settings", type=Path, help="the release's own facts (JSON): id, version, title, serial, ...")
    args = ap.parse_args(argv)
    game, primary = args.game.resolve(), args.primary.resolve()
    settings = json.loads((args.settings or game / "release.json").read_text())
    version = settings["version"]
    cfg = json.loads((primary / "hostgen.json").read_text())
    source = (ROOT / cfg["source"]).resolve()
    raw = json.loads((game / "release_names.json").read_text())
    table = rel.Release(raw)
    vmap = Map(args.map)
    c = Carry(table, raw, vmap, rel.read_rows(source / "config/versions/pal/functions.tsv"),
              rel.read_rows(source / "config/versions" / version / "functions.tsv"))
    # hostgen.json
    new_cfg = carry_config(c, cfg, version, settings["id"], settings["title"], settings["serial"],
                           settings["frame_rate"], int(settings["gp"], 16), int(settings["main"], 16),
                           int(settings["top"], 16))
    # The symbols the decompilation's build names otherwise (config/port_symbols.txt): each the
    # release's name, at the address that name states.
    symbols = []
    for table_name in cfg.get("symbols", []):
        out_lines = []
        for line in (source / table_name).read_text().splitlines():
            m = re.match(r"^(\w+) = 0x[0-9A-Fa-f]+;(.*)$", line)
            if not m:
                out_lines.append(line)
                continue
            new = table.name(m.group(1))
            n = rel.NAME.fullmatch(new)
            if n is None or n.group(2):
                c.report["dropped"][m.group(1)] = f"{table_name}: not in the release"
                continue
            out_lines.append(f"{new} = 0x{n.group(4).upper()};{m.group(2)}")
        name = Path(table_name).name
        (game / name).write_text("\n".join(out_lines) + "\n")
        symbols.append(os.path.relpath(game.resolve() / name, source))  # read relative to the source
    new_cfg["symbols"] = symbols
    (game / "hostgen.json").write_text(json.dumps(new_cfg, indent=2) + "\n")
    # libraries.tsv, split_places.tsv
    (game / "libraries.tsv").write_text(carry_libraries(c, (primary / "libraries.tsv").read_text()))
    (game / "split_places.tsv").write_text(carry_split_places(c, (primary / "split_places.tsv").read_text()))
    # hand/: the tables and the C, in the release's names
    hand = game / "hand"
    if hand.exists():
        shutil.rmtree(hand)
    hand.mkdir()
    for path in sorted((primary / "hand").iterdir()):
        if path.suffix == ".tsv" and path.name in ("hand.tsv", "overrides.tsv"):
            (hand / path.name).write_text(carry_libraries(c, path.read_text()))
        elif path.suffix == ".c":
            (hand / path.name).write_text(carry_c(c, path.read_text(), f"hand/{path.name}"))
    # host/: rac1-pal's own host C, the shared C's names and addresses
    host = game / "host"
    host.mkdir(exist_ok=True)
    for path in sorted((primary / "host").glob("*.c")):
        (host / path.name).write_text(carry_c(c, path.read_text(), f"host/{path.name}"))
    shared = sorted(p for d in cfg.get("host_shared", []) for p in (primary / d).resolve().glob("*.[ch]"))
    (host / "rac1_names.tsv").write_text(host_names(c, shared))
    # Each level's vtbl record: where the release's own programs put it (its programs.tsv, from the disc).
    vtbl = [int(cols[2], 16) for cols in (line.split("\t") for line in
            (source / "config/versions" / version / "programs.tsv").read_text().splitlines())
            if len(cols) > 2 and cols[0].startswith("level_") and cols[1] == "vtbl"]
    (host / "rac1_version.h").write_text(version_header(
        c, (primary / "host/rac1_version.h").read_text(), shared, settings["label"], settings["serial_text"],
        settings["save_folder"], vtbl, settings.get("defines", {}), settings.get("omit", {}),
        settings.get("defines_extra", {})))
    # The profile's tables (port/game/rac1/profile_<id>.h).
    rac1_dir = primary.parent / "rac1"
    (rac1_dir / f"profile_{settings['id']}.h").write_text(profile_table(
        c, (rac1_dir / "game_state.cpp").read_text(), (rac1_dir / "profile.cpp").read_text(), settings["id"],
        settings["label"], settings["serial_text"]))
    # What rests on less than the release's check or the same code.
    report = {"about": "What the carry from rac1-pal rests on besides the release's check and the same "
                       "instructions of the same code (port/tools/hostgen/release_rac1.py): functions by "
                       "another kind of evidence, addresses by versionmap's between or guess, and what the "
                       "release has no counterpart of, left out.",
              **{k: dict(sorted(v.items())) for k, v in c.report.items()},
              "counts": {k: len(v) for k, v in c.report.items()}}
    (game / "release_carry.json").write_text(json.dumps(report, indent=1) + "\n")
    print(f"release_rac1: {game.name}: " + ", ".join(f"{k} {len(v)}" for k, v in c.report.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())

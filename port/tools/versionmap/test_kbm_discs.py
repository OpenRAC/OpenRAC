# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""The built-in keyboard and mouse knows every version of every game it is built into.

Each version folder (port/game/<game>-<version>) of a game with a keyboard_mouse folder needs that
version's entry in the game's address table, or the keyboard and mouse does not load there. Ratchet
& Clank's carried releases (a release/ folder) must hold the PAL disc's entry as the release view
carries it: data through release_names.json, else release/data_moves.tsv, else the release's
rac1_version.h; functions by their release names. No disc is needed.
"""

import json
import re
import unittest
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]
GAMES = PORT / "game"

# kbm_disc's fields after the version, in order: (name, kind). Kind "boot" is the executable's
# data, "level" level 0's program's, "function" a function's name.
RAC1_FIELDS = [
    ("hero", "boot"),
    ("quick_select", "boot"),
    ("boot_mode", "boot"),
    ("page_menu", "boot"),
    ("dialog", "boot"),
    ("map_screen", "function"),
    ("level_mode", "level"),
    ("pad_sticks", "boot"),
    ("tilt_invert", "boot"),
    ("cameras", "level"),
    ("fast_cos", "function"),
    ("fast_sin", "function"),
    ("camera_look_along", "function"),
    ("grid_update", "function"),
    ("list_update", "function"),
    ("grid_icon_size", "boot"),
    ("font_bank", "boot"),
    ("font_table", "boot"),
    ("vendor", "level"),
    ("map_state", "boot"),
]


def versions_of(game):
    """The versions built of `game`: the suffixes of its port/game/<game>-<version> folders."""
    return sorted(p.name[len(game) + 1 :] for p in GAMES.glob(f"{game}-*") if p.is_dir())


def rac1_discs():
    """discs.h's entries: version to its fields (ints and names), in RAC1_FIELDS' order."""
    text = (GAMES / "rac1/keyboard_mouse/discs.h").read_text()
    table = text[text.index("k_kbm_discs[] = {") :]
    out = {}
    for body in re.findall(r"\{\s*(\"[^}]*)\}", table):
        values = [v.strip() for v in re.sub(r"/\*.*?\*/", "", body, flags=re.S).split(",")]
        values = [v for v in values if v]
        version = values[0].strip('"')
        fields = []
        for v in values[1:]:
            fields.append(v.strip('"') if v.startswith('"') else int(v.rstrip("uU"), 16))
        out[version] = fields
    return out


def rac2_discs():
    """keyboard_mouse.cpp's kDiscs versions."""
    text = (GAMES / "rac2/keyboard_mouse/keyboard_mouse.cpp").read_text()
    table = text[text.index("kDiscs = {{") : text.index("}};", text.index("kDiscs = {{"))]
    return set(re.findall(r"\{\"([a-z0-9_]+)\"", table))


def data_moves(release):
    """release/data_moves.tsv: program to its (first, last, move) runs."""
    runs = {}
    for line in (release / "release/data_moves.tsv").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        program, first, last, move, _ = line.split("\t")
        runs.setdefault(program, []).append((int(first, 16), int(last, 16), int(move, 16)))
    return runs


def carried(release, kind, address, names, moves, version_h):
    """The PAL disc's `address` (`kind`) in `release`, or None when nothing carries it."""
    name = ("D_L00_%08X" if kind == "level" else "D_%08X") % address
    target = names.get(name, "")
    if re.fullmatch(r"D_(L00_)?[0-9A-F]{8}", target):
        return int(target[-8:], 16)
    for first, last, move in moves.get("level:00" if kind == "level" else "boot", []):
        if first <= address <= last:
            return address + move
    macro = ("RAC1_LADDR_00_0x%08Xu" if kind == "level" else "RAC1_ADDR_0x%08Xu") % address
    found = re.search(rf"#define {macro} (0x[0-9A-Fa-f]+)u", version_h)
    return int(found.group(1), 16) if found else None


class KeyboardMouseDiscs(unittest.TestCase):
    def test_every_rac1_version_has_addresses(self):
        discs = rac1_discs()
        for version in versions_of("rac1"):
            with self.subTest(version=version):
                self.assertIn(version, discs, f"rac1/keyboard_mouse/discs.h has no {version} entry")
                self.assertEqual(len(discs[version]), len(RAC1_FIELDS))

    def test_every_rac2_version_has_addresses(self):
        discs = rac2_discs()
        for version in versions_of("rac2"):
            with self.subTest(version=version):
                self.assertIn(version, discs, f"rac2/keyboard_mouse kDiscs has no {version} entry")

    def test_rac1_releases_carry_the_pal_entry(self):
        discs = rac1_discs()
        pal = discs["pal"]
        for release in sorted(GAMES.glob("rac1-*/release")):
            release = release.parent
            version = release.name[len("rac1-") :]
            names = json.loads((release / "release/release_names.json").read_text())["names"]
            moves = data_moves(release)
            version_h = (release / "host/rac1_version.h").read_text()
            for (field, kind), primary, have in zip(RAC1_FIELDS, pal, discs[version]):
                with self.subTest(version=version, field=field):
                    if kind == "function":
                        self.assertEqual(have, names.get(primary, primary))
                    else:
                        self.assertEqual(
                            have, carried(release, kind, primary, names, moves, version_h)
                        )


if __name__ == "__main__":
    unittest.main()

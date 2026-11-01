#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every wrap and hook of rac1-pal has its row on rac1-ntsc, or is listed here with why it has none.

The shared host C (port/game/rac1/host) is built for both discs, and its addresses come from
rac1_version.h, which hostnames.py checks. What neither sees is a libraries.tsv row left out: the
wrap then never runs on that disc, and nothing fails until a player gets there. A new wrap must be
carried, or listed here (and in docs/port/MODS.md when a mod would notice).
"""
import re
import unittest
from pathlib import Path

GAME = Path(__file__).resolve().parents[2] / "game"

NOT_ON_NTSC = {
    # The memory card page's MODS badges: this disc shows them in the launcher only.
    "func_00224C30",
    # rac1_version.h: the boot loop's picture goes through draw_bootImage's wrap instead.
    "func_001F7680",
    # Custom map destinations on the ship's list: PAL's until a map is carried.
    "func_00222848",
    "func_0021BB90",
    "func_L00_00290030",
    # Enhanced Novalis water: named in rac1-pal/host/water.c; a look, nothing shared.
    "func_L01_002FE498",
    "func_L01_002F74D0",
    "func_L01_002F7528",
    "func_L01_002FFF00",
    "func_L01_002B9198",
    "func_L00_002A5158",
    "func_L01_002635C8",
    "func_L01_0030AFD0",
}


def rows(version):
    path = GAME / version / "libraries.tsv"
    lines = path.read_text(encoding="utf-8").splitlines()
    return [line.split("\t") for line in lines if line.strip() and not line.startswith("#")]


class Twins(unittest.TestCase):
    def test_every_wrap_and_hook_is_carried(self):
        ntsc = rows("rac1-ntsc")
        names = {row[2] for row in ntsc}
        carried = {(pal, row[3]) for row in ntsc for pal in re.findall(r"PAL (\w+)", " ".join(row[4:]))}
        missing = {
            row[0]
            for row in rows("rac1-pal")
            if row[3] in ("wrap", "hook")
            and (row[0], row[3]) not in carried
            and not (row[3] == "hook" and row[2] in names)
        }
        self.assertEqual(missing - NOT_ON_NTSC, set(), "PAL rows with no twin on NTSC")
        self.assertEqual(NOT_ON_NTSC - missing, set(), "listed here, but carried now")


if __name__ == "__main__":
    unittest.main()

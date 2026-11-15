# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""parity.py on a small made-up game: no disc, no decomp."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import parity  # noqa: E402

HEAD = "# name\tkind\tsize\tfp\tcounterpart\tmethod\tsimilarity\tsame\tplaces\n"


class Parity(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.saved = dict(parity.GAMES)
        parity.GAMES["toy"] = {"source": "src", "programs": {"toy-a": "a", "toy-b": "b"}}
        # Version b moved everything 0x100 down; f2 has a level copy in level 3.
        cats = {"a": [("func_00100000", 64, "boot:00100000"), ("func_00100100", 64, "boot:00100100,03:00300000")],
                "b": [("func_00100000", 64, "boot:00100F00"), ("func_00100100", 64, "boot:00101000,03:00300100")]}
        for version, entries in cats.items():
            path = self.root / "src/config/versions" / version / "functions.tsv"
            path.parent.mkdir(parents=True)
            path.write_text(HEAD + "".join(f"{n}\texe\t{s}\t0\t{n}\tx\t1\tyes\t{p}\n" for n, s, p in entries))
        for program in ("toy-a", "toy-b"):
            (self.root / "port/game" / program).mkdir(parents=True)
            (self.root / "port/game" / program / "hostgen.json").write_text(json.dumps({"source": "src"}))
        (self.root / "port/game/toy").mkdir()

    def tearDown(self):
        parity.GAMES.clear()
        parity.GAMES.update(self.saved)
        self.tmp.cleanup()

    def tables(self, a, b, reasons=""):
        (self.root / "port/game/toy-a/libraries.tsv").write_text(a)
        (self.root / "port/game/toy-b/libraries.tsv").write_text(b)
        (self.root / "port/game/toy/parity.tsv").write_text(reasons)
        return parity.check("toy", root=self.root)

    def test_alike_is_quiet(self):
        self.assertEqual(self.tables("func_00100000\tx\tf1\thost\nfunc_L03_00300000\tx\tf2\twrap\n",
                                     "func_00100F00\tx\tf1\thost\nfunc_L03_00300100\tx\tf2\twrap\n"), [])

    def test_a_row_another_version_lacks(self):
        errors = self.tables("func_00100000\tx\tf1\twrap\n", "")
        self.assertEqual(len(errors), 1)
        self.assertIn("func_00100000 (boot) runs differently", errors[0])

    def test_a_row_at_the_wrong_address(self):
        # Carried unchanged: in b, 0x00100000 is no function's start.
        errors = self.tables("func_00100000\tx\tf1\thost\n", "func_00100000\tx\tf1\thost\n")
        self.assertTrue(any("binds boot:00100000, at no function" in e for e in errors), errors)
        errors = self.tables("", "func_L03_00300120\tx\tf2\thost\n")
        self.assertTrue(any("inside func_00100100" in e for e in errors), errors)

    def test_a_reason_and_a_stale_reason(self):
        why = "func_00100000\tboot\ttoy-b\tb has no use for it\n"
        self.assertEqual(self.tables("func_00100000\tx\tf1\thost\n", "", why), [])
        errors = self.tables("func_00100000\tx\tf1\thost\n", "func_00100F00\tx\tf1\thost\n", why)
        self.assertEqual(len(errors), 1)
        self.assertIn("no longer differ", errors[0])


    def test_an_omit_covers_every_place(self):
        # b leaves the shared host C of f2 out (RAC1_OMIT): none of f2's places is compared there.
        header = self.root / "port/game/toy-b/host/rac1_version.h"
        header.parent.mkdir()
        header.write_text("/* b runs its own C */\n#define RAC1_OMIT_func_00100100 1\n")
        self.assertEqual(parity.omits(header), {"func_00100100": "b runs its own C"})
        self.assertEqual(self.tables("func_00100100\tx\tf2\thost\nfunc_L03_00300000\tx\tf2c\thost\n", ""), [])

    def test_a_split_variant_is_read_at_its_own_places(self):
        # b's split_places.tsv keeps a's name for the variant, with b's places.
        (self.root / "port/game/toy-a/split_places.tsv").write_text("func_00100100\tfunc_L03_00300000\t03:00300000\n")
        (self.root / "port/game/toy-b/split_places.tsv").write_text("func_00100100\tfunc_L03_00300000\t03:00300100\n")
        row = "func_L03_00300000\tx\tv\thost\n"
        self.assertEqual(self.tables(row, row), [])
        errors = self.tables(row, "")
        self.assertEqual(len(errors), 1)
        self.assertIn("func_00100100 (03) runs differently", errors[0])

if __name__ == "__main__":
    unittest.main()

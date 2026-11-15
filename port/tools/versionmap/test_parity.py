# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""parity.py on a small made-up game: no disc, no decomp."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import data_moves  # noqa: E402
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
        header = self.root / "port/game/toy-b/host/toy_version.h"
        header.parent.mkdir()
        header.write_text("/* b runs its own C */\n#define TOY_OMIT_func_00100100 1\n")
        self.assertEqual(parity.omits(header, "toy"), {"func_00100100": "b runs its own C"})
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

    def test_a_row_with_another_c_type(self):
        a = "func_00100000\tx\tf1\twrap\tnote\tint (gaddr menu)\n"
        self.assertEqual(self.tables(a, "func_00100F00\tx\tf1\twrap\tnote\tint (unsigned int)\n"), [])
        b = "func_00100F00\tx\tf1\twrap\tnote\tvoid (gaddr, int)\n"
        errors = self.tables(a, b)
        self.assertEqual(len(errors), 1)
        self.assertIn("another C type in toy-b void (u32, int)", errors[0])
        self.assertEqual(self.tables(a, b, "func_00100000\tsig\ttoy-b\tb's decomp declares it so\n"), [])

    def test_a_data_address_with_nothing_behind_it(self):
        b = self.root / "port/game/toy-b"
        (b / "host").mkdir()
        (b / "release_names.json").write_text(json.dumps({
            "how": {"D_L17_00160000": "x", "D_L17_00160100": "x", "D_00130000": "x"},
            "names": {"D_L17_00160000": "D_L17_0015FF00", "D_L17_00160100": "D_L17_00160000"}}))
        header = b / "host/rac1_version.h"
        # Named, between neighbours that moved alike, and unchanged in the executable: all fine.
        header.write_text("#define RAC1_ADDR_L17_0x00160000u 0x0015FF00u\n"
                          "#define RAC1_ADDR_L17_0x00160080u 0x0015FF80u\n"
                          "#define RAC1_ADDR_0x00130000u 0x00130000u\n")
        self.assertEqual(self.tables("", ""), [])
        header.write_text("#define RAC1_ADDR_L17_0x00160080u 0x0015FF90u\n#define RAC1_ADDR_0x00130000u 0x00130010u\n")
        errors = self.tables("", "")
        self.assertEqual(len(errors), 2, errors)
        self.assertIn("L17:00160080: no name there", errors[0] + errors[1])
        self.assertIn("boot:00130000: release_names.json places it at 00130000", errors[0] + errors[1])
        why = "L17:00160080\tdata\ttoy-b\tchecked by hand\nboot:00130000\tdata\ttoy-b\tchecked by hand\n"
        self.assertEqual(self.tables("", "", why), [])

    def test_the_map_checks_named_data_too(self):
        b = self.root / "port/game/toy-b"
        (b / "host").mkdir()
        # rac1-decomp named the wrong datum: the check gave L07:00208820 its neighbour's place.
        (b / "release_names.json").write_text(json.dumps({
            "how": {"D_L07_00208820": "x"}, "names": {"D_L07_00208820": "D_L07_001D3300"}}))
        vmap = {"programs": {"level:07": {"data": {"001D36C0": "001D3300", "00208820": "00208460",
                                                   "00208830": "00208470"}}}}
        (b / "data_moves.tsv").write_text(data_moves.write(vmap, "toy-b"))
        header = b / "host/rac1_version.h"
        header.write_text("#define RAC1_ADDR_L07_0x00208820u 0x001D3300u\n")
        errors = self.tables("", "")
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("data_moves.tsv) places it at 00208460", errors[0])
        # The map alone vouches for an unnamed address inside one of its runs.
        (b / "release_names.json").write_text(json.dumps({"how": {}, "names": {}}))
        header.write_text("#define RAC1_ADDR_L07_0x00208828u 0x00208468u\n")
        self.assertEqual(self.tables("", ""), [])

    def test_data_moves_runs(self):
        vmap = {"programs": {"boot": {"data": {"00100000": "00100100", "00100010": "00100110",
                                               "00100020": "00100000"}}}}
        self.assertEqual(data_moves.runs(vmap), [("boot", 0x100000, 0x100010, 0x100, 2),
                                                 ("boot", 0x100020, 0x100020, -0x20, 1)])
        path = self.root / "moves.tsv"
        path.write_text(data_moves.write(vmap, "toy"))
        table = data_moves.read(path)
        self.assertEqual(data_moves.moved(table, "boot", 0x100008), 0x100108)
        self.assertEqual(data_moves.moved(table, "boot", 0x100020), 0x100000)
        self.assertIsNone(data_moves.moved(table, "boot", 0x100018))
        self.assertIsNone(data_moves.moved(table, "level:03", 0x100008))

    def test_a_catalogue_of_an_executable_and_levels(self):
        # rac2-decomp's form: the executable's (name, address, size) and the levels' places.
        boot, levels = self.root / "boot.tsv", self.root / "levels.tsv"
        boot.write_text("# name\taddress\tsize\nfunc_00100000\t00100F00\t64\n")
        levels.write_text("# name\tkind\tsize\tfingerprint\tlevels\tplaces (level:address)\n"
                          "func_L03_00300000\tshared\t32\t0\t2\t03:00300100,04:00400100\n")
        cat = parity.Catalogue([boot, levels])
        self.assertEqual(cat.at[("boot", 0x00100F00)], "func_00100000")
        self.assertEqual(cat.at[("04", 0x00400100)], "func_L03_00300000")
        self.assertEqual(cat.inside("04", 0x00400110), "func_L03_00300000")


if __name__ == "__main__":
    unittest.main()

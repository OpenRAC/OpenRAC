# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""release.py and release_carry.py: another release's names for the primary's, and the port tables
carried with them."""
import json
import sys
import tempfile
import unittest
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import release  # noqa: E402
import release_carry  # noqa: E402

REPO = HERE.parent.parent.parent


class ReleaseNames(unittest.TestCase):
    def setUp(self):
        self.r = release.Release({
            "names": {"func_00282DD8": "func_00283698", "func_00283698": "func_00283F58",
                      "D_001A7A18": "D_001A7AA0", "func_L05_0041DF58": "func_L05_00422CA8",
                      "func_0026F730": "FUN_0026F5A0", "func_PAL_002907D0": "func_002907D0"},
            "units": {"src/levels/14_x/L14_a.c": {"func_2F5D68": "func_2F7338",
                                                 "func_00283698": "func_V1_00283698"}},
        })

    def test_a_name_keeps_its_suffix(self):
        self.assertEqual(self.r.name("func_00282DD8_7"), "func_00283698_7")
        self.assertEqual(self.r.name("func_00282DD8__game"), "func_00283698__game")
        self.assertEqual(self.r.name("D_001A7A18_sda"), "D_001A7AA0_sda")

    def test_names_it_does_not_know_stay(self):
        self.assertEqual(self.r.name("func_00115200"), "func_00115200")
        self.assertEqual(self.r.name("r1_func_00282DD8"), "r1_func_00282DD8")
        self.assertEqual(self.r.name("F391de845_FUN_00282DD8"), "F391de845_FUN_00282DD8")

    def test_another_spelling_of_one_function(self):
        self.assertEqual(self.r.name("func_0026F730"), "FUN_0026F5A0")
        self.assertEqual(self.r.name("FUN_00282DD8"), "FUN_00283698")

    def test_text_renames_once(self):
        # A name the release gives (func_00283698) is not renamed again as v1.01's of that spelling.
        text = 'func_00282DD8(a); __asm__("func_00282DD8"); D_001A7A18 = 1; func_L05_0041DF58();'
        self.assertEqual(self.r.text(text),
                         'func_00283698(a); __asm__("func_00283698"); D_001A7AA0 = 1; func_L05_00422CA8();')

    def test_a_files_own_names(self):
        text = "func_2F5D68(x); func_00283698(y); func_00282DD8(z);"
        self.assertEqual(self.r.text(text, "src/levels/14_x/L14_a.c"),
                         "func_2F7338(x); func_V1_00283698(y); func_00283698(z);")
        # Another file has none of them: v1.01's names only.
        self.assertEqual(self.r.text(text, "src/game/other.c"),
                         "func_2F5D68(x); func_00283F58(y); func_00283698(z);")

    def test_the_tag_is_not_a_level(self):
        m = release.NAME.fullmatch("func_L00_002A3B80")
        self.assertEqual((m.group(2), m.group(3)), (None, "00"))
        m = release.NAME.fullmatch("func_PAL_L01_002BEC60")
        self.assertEqual((m.group(2), m.group(3)), ("PAL", "01"))


class Building(unittest.TestCase):
    """A small decompilation: two programs of each release, catalogues and placed values."""

    def write(self, root: Path, files: dict):
        for name, text in files.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text if isinstance(text, str) else json.dumps(text))

    def test_names_from_catalogues_and_values(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            boot = "# name\taddress\tsize\n"
            levels = "# name\tkind\tsize\tfingerprint\tlevels\tplaces\n"
            self.write(root, {
                "config/boot/functions.tsv": boot + "func_00200000\t00200000\t16\nfunc_00200010\t00200018\t16\n"
                                            "func_00200030\t00200030\t8\n",
                "config/levels/functions.tsv": levels + "func_L00_002B0000\tlevel\t8\tx\t1\t00:002B0000\n",
                "config/pal/boot/functions.tsv": boot + "func_00200000\t00200080\t16\nfunc_00200010\t00200098\t16\n"
                                                "func_PAL_002000B0\t002000B0\t12\n",
                "config/pal/levels/functions.tsv": levels + "func_L00_002B0000\tlevel\t8\tx\t1\t00:002B0100\n",
                "build/boot/placed.json": {"values": {"D_001A7A18": "0x001a7a18"}},
                "build/pal/boot/placed.json": {"values": {"D_001A7A18": "0x001a7aa0", "func_00200030": "0x002000b0"}},
                "build/pal/00_x/placed.json": {"values": {"D_L00_001B0000": "0x001b0040"}},
                "src/game/a.c": "void func_00200030(void) {}\nint D_001A7A18;\nextern int D_L00_001B0000;\n",
            })
            parts = release.build(root, "pal", None, Counter(["D_L00_001B0000"]))
            names = parts["names"]
            self.assertEqual(names["func_00200000"], "func_00200080")
            # A twin entered away from its name keeps the distance (name 0x200010, entry 0x200018).
            self.assertEqual(names["func_00200010"], "func_00200090")
            self.assertEqual(names["func_PAL_002000B0"], "func_002000B0")
            self.assertEqual(names["func_L00_002B0000"], "func_L00_002B0100")
            self.assertEqual(names["D_001A7A18"], "D_001A7AA0")
            self.assertEqual(names["D_L00_001B0000"], "D_L00_001B0040")
            # v1.01's function the release changed: the address the release's C took, another
            # spelling of the release's own function there.
            self.assertEqual(names["func_00200030"], "FUN_002000B0")
            self.assertIn("func_00200030", parts["absent"])


class Carried(unittest.TestCase):
    def test_the_hook_run(self):
        table = release.Release({"names": {"func_0026F720": "func_0026F588", "func_0026F728": "func_0026F590"}})

        class P:
            pass
        p = P()
        p.table = table
        p.report = {}
        p.r_boot = [("func_PAL_0026F578", 0x26F578, 8), ("func_PAL_0026F580", 0x26F580, 8),
                    ("func_0026F720", 0x26F588, 8), ("func_0026F728", 0x26F590, 8), ("func_X", 0x26F598, 64)]
        table.names["func_PAL_0026F578"] = "func_0026F578"
        table.names["func_PAL_0026F580"] = "func_0026F580"
        hooks = release_carry.level_hooks(p, ["func_0026F588", "func_0026F590"])
        self.assertEqual(hooks, ["func_0026F578", "func_0026F580", "func_0026F588", "func_0026F590"])


class Committed(unittest.TestCase):
    """The releases' tables in port/game: what the shared host C needs, each of them has."""

    def test_each_release_table_answers_the_shared_c(self):
        shared = REPO / "port/game/rac2/host"
        code = {f.name: release_carry.code_text(f.read_text()) for f in shared.glob("*.[ch]")}
        wanted_addr = {m.group(1) for t in code.values() for m in release_carry.HOST_ADDR.finditer(t)}
        wanted_k = {m.group(1) for t in code.values() for m in release_carry.HOST_K.finditer(t)}
        for game in ("rac2-pal", "rac2-ntsc2"):
            header = (REPO / "port/game" / game / "host/rac2_version.h").read_text()
            with self.subTest(game=game):
                missing = sorted(a for a in wanted_addr if f"#define RAC2_ADDR_0x{a}u " not in header)
                self.assertEqual(missing, [], "addresses the shared C uses and the table lacks")
                missing = sorted(k for k in wanted_k if f"#define RAC2_K_{k} " not in header)
                self.assertEqual(missing, [], "constants the shared C uses and the table lacks")

    def test_release_configs_name_their_tables(self):
        for game, version in (("rac2-pal", "pal"), ("rac2-ntsc2", "us2")):
            cfg = json.loads((REPO / "port/game" / game / "hostgen.json").read_text())
            with self.subTest(game=game):
                self.assertEqual(cfg["release"], {"version": version, "table": "release_names.json"})
                self.assertEqual(cfg["derived"], {"table": "release_names.json"})
                self.assertTrue((REPO / "port/game" / game / "release_names.json").is_file())
                self.assertEqual(cfg["host_shared"], ["../rac2/host"])


if __name__ == "__main__":
    unittest.main()

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

    def test_a_level_files_names_of_its_level(self):
        # In an Oozla file func_00312B68 is level 1's copy of func_002A9640 (the primary's level
        # catalogue), not the executable's address it states: it is that function in the release.
        r = release.Release({"names": {"func_002A9640": "func_002AA0A8", "func_00312B68": "func_00313FD8",
                                       "func_L01_003BD178": "func_L01_003BF000"}})
        r.level_local = {(1, 0x00312B68): "func_002A9640", (1, 0x003BD178): "func_L01_003BD178"}
        r.release_places = {"func_002A9640": {0: 0x0031EA18, 1: 0x00314908}}
        r.exe_starts = {0x002A9640}
        r.level_base = 0x001A7C80
        text = "func_00312B68(e0); func_002A9640(e1);"
        # Its place in the release's level 1, spelled with the level, as hostgen reads the primary's.
        self.assertEqual(r.text(text, "src/levels/01_oozla/L01_003CC930.c"),
                         "func_L01_00314908_local(e0); func_002AA0A8(e1);")
        # A function of the level the release names on its own: its place there.
        self.assertEqual(r.text("func_003BD178();", "src/levels/01_oozla/L01_a.c"), "func_L01_003BF000_local();")
        # Elsewhere (another level, the executable's files) the name is what it states.
        self.assertEqual(r.text(text, "src/levels/02_x/L02_a.c"), "func_00313FD8(e0); func_002AA0A8(e1);")
        self.assertEqual(r.text(text, "src/game/other.c"), "func_00313FD8(e0); func_002AA0A8(e1);")

    def test_a_files_mark_is_its_definition_only(self):
        # The file defines v1.01's func_002CA070 (marked: the release has its own) and calls it from
        # another function: the call is the release's function.
        r = release.Release({"names": {"func_002CA070": "FUN_002CAEA0"},
                             "units": {"src/game/002C7748.c": {"func_002CA070": "func_V1_002CA070"}}})
        text = 'INCLUDE_ASM("asm/game", func_002CA070);\nvoid func_002CAA58(void)\n{\n    func_002CA070();\n}\n'
        self.assertEqual(r.text(text, "src/game/002C7748.c"),
                         'INCLUDE_ASM("asm/game", func_V1_002CA070);\nvoid func_002CAA58(void)\n{\n    FUN_002CAEA0();\n}\n')

    def test_six_digits_in_a_file_of_the_executable(self):
        # func_2802E0 in src/game/ is the executable's func_002802E0 (as hostgen reads it): renamed
        # as that name; in a level's file six digits are that level's, renamed by the file's table.
        r = release.Release({"names": {"func_002802E0": "func_00280C98"}})
        self.assertEqual(r.text("func_2802E0(m);", "src/game/00288D88-g8.c"), "func_00280C98(m);")
        self.assertEqual(r.text("func_2802E0(m);", "src/levels/01_oozla/L01_a.c"), "func_2802E0(m);")
        self.assertEqual(r.text("func_2E5E58(m);", "src/game/00288D88-g8.c"), "func_2E5E58(m);")

    def test_the_tag_is_not_a_level(self):
        m = release.NAME.fullmatch("func_L00_002A3B80")
        self.assertEqual((m.group(2), m.group(3)), (None, "00"))
        m = release.NAME.fullmatch("func_PAL_L01_002BEC60")
        self.assertEqual((m.group(2), m.group(3)), ("PAL", "01"))


class GoneIn(unittest.TestCase):
    """A definition left out in part of the tree only (rac3: one PAL name, two programs)."""

    def test_only_where_it_is_gone(self):
        r = release.Release({"names": {}, "gone": ["func_00382498"],
                             "gone_in": {"func_00382498": ["src/boot_elf/"]}})
        text = "int func_00382498(int a) {\n    return a;\n}\n"
        self.assertNotIn("return a", r.splice(text, "src/boot_elf/text/382380.c"))
        self.assertEqual(r.splice(text, "src/frontbin/382380.c"), text)


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


class Hooks(unittest.TestCase):
    """pair_stubs: the level hooks by the calls that reach them, not by their order."""

    @staticmethod
    def segment(base: int, words: list[int]) -> list[tuple[int, bytes]]:
        return [(base, b"".join(w.to_bytes(4, "little") for w in words))]

    def test_the_callers_say_which_hook(self):
        jr, nop, zero = release.JR_RA, 0, 0x0000102D

        def jal(t):
            return (3 << 26) | (t >> 2)

        def li_a0(v):
            return 0x24040000 | v
        # v1.01: hooks at 0x100 (a result), 0x108, 0x110 and 0x118 (none); callers at 0x200.
        us = self.segment(0x100, [jr, zero, jr, nop, jr, nop, jr, nop] + [0] * 56
                          + [li_a0(1), jal(0x100), nop, li_a0(2), jal(0x108), nop, jr, nop]
                          + [li_a0(3), jal(0x118), nop, jr, nop, 0, 0, 0])
        us_boot = [("func_00000100", 0x100, 8), ("func_00000108", 0x108, 8), ("func_00000110", 0x110, 8),
                   ("func_00000118", 0x118, 8), ("func_00000200", 0x200, 32), ("func_00000220", 0x220, 20)]
        # The release: one more hook first, so each is 8 bytes on; its catalogue pairs them by
        # address (func_00000108 at 0x108), which is another hook.
        rel = self.segment(0x100, [jr, nop, jr, zero, jr, nop, jr, nop, jr, nop] + [0] * 54
                           + [li_a0(1), jal(0x108), nop, li_a0(2), jal(0x110), nop, jr, nop]
                           + [li_a0(3), jal(0x120), nop, jr, nop, 0, 0, 0])
        r_boot = [("func_00000100", 0x100, 8), ("func_00000108", 0x108, 8), ("func_00000110", 0x110, 8),
                  ("func_00000118", 0x118, 8), ("func_X_00000120", 0x120, 8), ("func_00000200", 0x200, 32),
                  ("func_00000220", 0x220, 20)]
        pairs = release.pair_stubs(us, rel, us_boot, r_boot)
        # 0x110 has no caller: between 0x108 and 0x118, which both moved 8 bytes on.
        self.assertEqual(pairs, {"func_00000100": 0x108, "func_00000108": 0x110,
                                 "func_00000110": 0x118, "func_00000118": 0x120})

    def test_a_hook_of_another_kind_is_not_paired(self):
        jr, nop, zero = release.JR_RA, 0, 0x0000102D
        us = self.segment(0x100, [jr, zero] + [0] * 62 + [(3 << 26) | (0x100 >> 2), nop, jr, nop])
        rel = self.segment(0x100, [jr, nop] + [0] * 62 + [(3 << 26) | (0x100 >> 2), nop, jr, nop])
        boot = [("func_00000100", 0x100, 8), ("func_00000200", 0x200, 16)]
        self.assertEqual(release.pair_stubs(us, rel, boot, boot), {})


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


class Rac1Release(unittest.TestCase):
    """Ratchet & Clank's releases: rac1-decomp's catalogues and its check (release.build_rac1)."""

    HEAD = "# name\tkind\tsize\tfingerprint\tcounterpart\tmethod\tsimilarity\tsame\tplaces\n"

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.source = root / "decomp"
        (self.source / "config/versions/pal").mkdir(parents=True)
        (self.source / "config/versions/pal_v1").mkdir(parents=True)
        (self.source / "config/versions/pal/functions.tsv").write_text(self.HEAD + "".join([
            "func_00200000\texe\t8\tf\tfunc_00200000\tfingerprint\t1.00\tyes\tboot:00200000\n",
            "func_L00_00300000\tshared\t8\tf\tfunc_L00_00300000\tfingerprint\t1.00\tyes\t00:00300000,01:00310000\n",
            "func_00200100\texe\t8\tf\tfunc_00200100\tfingerprint\t1.00\tyes\tboot:00200100\n",
        ]))
        (self.source / "config/versions/pal_v1/functions.tsv").write_text(self.HEAD + "".join([
            "func_00200000\texe\t8\tf\tfunc_00200000\tfingerprint\t1.00\tyes\tboot:001FFF00\n",
            "func_L00_00300000\tshared\t8\tf\tfunc_L00_00300000\tfingerprint\t1.00\tno\t00:002FFF00,01:0030FF00\n",
            "func_E1_00201000\texe\t8\tf\t-\t-\t-\tno\tboot:00201000\n",
        ]))
        self.relocs = root / "relocs"
        (self.relocs / "boot").mkdir(parents=True)
        (self.relocs / "boot/placed.json").write_text(json.dumps(
            {"values": {"D_00150000": "0x0014FF00", "func_00200000": "0x001FFF00"}}))

    def tearDown(self):
        self.tmp.cleanup()

    def test_names(self):
        uses = Counter({"D_00150000": 1, "D_00160000": 1, "D_E1_00150100": 1})
        parts = release.build_rac1(self.source, "pal_v1", self.relocs, None, uses)
        names = parts["names"]
        self.assertEqual(names["func_00200000"], "func_001FFF00")
        self.assertEqual(names["func_L00_00300000"], "func_L00_002FFF00")
        self.assertEqual(names["func_E1_00201000"], "func_00201000")
        # PAL v2.00's function the release does not have, and a global nothing proves: marked.
        self.assertEqual(names["func_00200100"], "func_V2_00200100")
        self.assertEqual(names["D_00160000"], "D_V2_00160000")
        # The release's check proves one global; the release's own keeps its address.
        self.assertEqual(names["D_00150000"], "D_0014FF00")
        self.assertEqual(names["D_E1_00150100"], "D_00150100")
        self.assertEqual(parts["how"]["func_L00_00300000"], "function: carried (the release's own C for it)")

    def test_one_letter_tag(self):
        m = release.NAME.fullmatch("func_J_001F6B60")
        self.assertEqual((m.group(2), m.group(4)), ("J", "001F6B60"))

    def test_splice(self):
        table = release.Release({"names": {}})
        table.splices = {"src/a.c": [("func_00200000", "int read_x(void) { return 2; }\n"),
                                     ("func_E1_00201000", "void func_E1_00201000(void) {}\n")]}
        table.tag = "E1"
        text = "int other(void) { return 0; }\nint read_x(void) {\n    return 1;\n}\nint after;\n"
        out = table.splice(text, "src/a.c", {"func_00200000": ["read_x"]})
        self.assertIn("int read_x(void) { return 2; }\nint after;", out)
        self.assertTrue(out.rstrip().endswith("void func_E1_00201000(void) {}"))
        with self.assertRaises(ValueError):
            table.splice("int nothing;\n", "src/a.c", {})


class Rac1Carry(unittest.TestCase):
    """release_rac1.py: rac1-pal's tables in a release's names."""

    def setUp(self):
        import release_rac1
        self.mod = release_rac1
        raw = {"names": {"func_00200000": "func_001FFF00", "func_00200100": "func_V2_00200100",
                         "func_00200200": "FUN_00201000", "D_00150000": "D_0014FF00"},
               "how": {"func_00200100": "function: not in the release (marked)",
                       "func_00200200": "function: the release's check (its counterpart here)",
                       "D_00150000": "global: the release's check"}}

        class NoMap:
            def function(self, *a):
                return None

            def data(self, *a, **k):
                return None
        self.c = release_rac1.Carry(release.Release(raw), raw, NoMap(), [], [])

    def test_libraries(self):
        text = "# head\nfunc_00200000\tgame\tX\thost\nfunc_00200100\tgame\tY\thost\nfunc_00200200\tgame\tZ\twrap\n"
        out = self.mod.carry_libraries(self.c, text)
        # The function the release has not, and the one whose callers reach another, are left out.
        self.assertEqual(out, "# head\nfunc_001FFF00\tgame\tX\thost\n")
        self.assertEqual(len(self.c.report["dropped"]), 2)

    def test_c_literals(self):
        text = "x = GREF(int, 0x00150000u); /* 0x00150000 */ y = RAC1_ADDR(0x00150000u);"
        self.assertEqual(self.mod.carry_c(self.c, text, "t"),
                         "x = GREF(int, 0x0014FF00u); /* 0x00150000 */ y = RAC1_ADDR(0x00150000u);")

    def test_a_call_reaches_the_counterpart(self):
        self.assertEqual(self.c.function("func_00200200", "t", calls=True), "func_00201000")

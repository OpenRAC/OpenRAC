# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""RAC3 multiplayer's build inputs, checked without a disc or compiler.

    python3 -m unittest discover -s port/tools/hostgen -p test_rac3_mp_config.py

The map wrappers are C macros expanded with each region's generated address
tables. Check their exported names together, before a full game link is needed.
"""
from collections import Counter
from importlib import util
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest import mock

import asmc
import hostgen
import program

GAMES = Path(__file__).resolve().parents[2] / "game"
WRAPPERS = (("mp_maps", "MP_MAP"), ("mp_fixes", "MP_FIX"))
MAPS_SPEC = util.spec_from_file_location("rac3_maps_config", GAMES / "rac3-pal/mp/maps.py")
MAPS = util.module_from_spec(MAPS_SPEC)
# Match the script's CLI import context: maps.py imports its sibling fixes.py.
with mock.patch("sys.path", [str(GAMES / "rac3-pal/mp"), *sys.path]):
    MAPS_SPEC.loader.exec_module(MAPS)


def map_exports(source: str, header: str, macro: str) -> list[str]:
    """Expand only the public function names defined by one map macro."""
    logical_lines = source.replace("\\\n", "")
    definition = re.search(r"^#define " + macro + r"\(([^)]*)\)([^\n]*)", logical_lines, re.MULTILINE)
    if definition is None:
        raise ValueError(f"no definition of {macro}")
    parameters = [part.strip() for part in definition[1].split(",")]
    fields = re.findall(r"\bmp_func_L##m##_##(\w+)\([^;{}]*\)\s*\{", definition[2])
    if not fields:
        raise ValueError(f"{macro} defines no map wrappers")
    exports = []
    for row in re.finditer(r"^" + macro + r"\(([^)]*)\)", header, re.MULTILINE):
        arguments = [part.strip() for part in row[1].split(",")]
        values = dict(zip(parameters, arguments, strict=True))
        exports.extend(f"mp_func_L{values['m']}_{values[field]}" for field in fields)
    if not exports:
        raise ValueError(f"no rows of {macro}")
    return exports


class Rac3MultiplayerBuildConfig(unittest.TestCase):
    def test_protocol_password_error_keeps_its_wire_value_without_a_windows_name(self):
        # winsock2.h defines ERROR_WRONG_PASSWORD as 1323, but the server sends reason 9.
        source = (GAMES / "rac3-pal/host/mp_net.c").read_text(encoding="utf-8")
        block = re.search(r"enum\s*\{\s*(ERROR_PARTY_NOT_FOUND.*?)\};", source, re.DOTALL)
        names = [name.strip() for name in block[1].split(",") if name.strip()]
        self.assertEqual(names[9], "HUB_ERROR_WRONG_PASSWORD")
        self.assertNotIn("ERROR_WRONG_PASSWORD", names)

    def test_both_regions_load_their_multiplayer_configuration(self):
        # Loading resolves asm.wide relative to this file, not the PAL program's directory.
        self.addCleanup(program.set_name_prefix, program.NAME_PREFIX)
        for region in ("pal", "ntsc"):
            with self.subTest(region=region):
                config = hostgen.load_config(GAMES / f"rac3-{region}/mp/hostgen.json")
                self.assertEqual(config["prefix"], "mp_")
                asmc.parse_wide(config["asm"].get("wide"))

    def test_multiplayer_wraps_exist_without_the_disc(self):
        # Multiplayer has no C units: its wrappers must also exist in a disc-less build.
        self.addCleanup(program.set_name_prefix, program.NAME_PREFIX)
        with tempfile.TemporaryDirectory() as temp:
            for region in ("pal", "ntsc"):
                with self.subTest(region=region):
                    folder = GAMES / f"rac3-{region}/mp"
                    config = hostgen.load_config(folder / "hostgen.json")
                    compiled = program.Program()
                    compiled.wrap = hostgen.wrapped(folder, config)
                    compiled.ps2_float = set(config.get("ps2_float", []))
                    compiled.spilled_locals = config.get("spilled_locals", {})
                    hostgen.settle_signatures(compiled, config, Path(temp), {})
                    self.assertTrue(compiled.wrap)
                    self.assertEqual(sorted(compiled.wrap - compiled.functions.keys()), [])

    def test_map_macro_references_exist_without_the_disc(self):
        # HUD's TEX0 getter and VU0's grid relink are calls, not host replacements: maps.py's
        # "ref" entries, wherever they sit in their macro rows.
        ref_columns = {
            "HUD": [n + 1 for n, row in enumerate(MAPS.CARRIED_HUD) if row[3] == "ref"],
            "VU0": [n + 1 for n, row in enumerate(MAPS.CARRIED_VU0) if not row[1]],
        }
        self.addCleanup(program.set_name_prefix, program.NAME_PREFIX)
        with tempfile.TemporaryDirectory() as temp:
            for region in ("pal", "ntsc"):
                with self.subTest(region=region):
                    folder = GAMES / f"rac3-{region}/mp"
                    config = hostgen.load_config(folder / "hostgen.json")
                    compiled = program.Program()
                    compiled.ps2_float = set()
                    compiled.spilled_locals = {}
                    hostgen.settle_signatures(compiled, config, Path(temp), {})
                    references = set()
                    header = (folder.parent / "host/mp_maps.h").read_text(encoding="utf-8")
                    for row in re.finditer(r"^MP_MAP_(HUD|VU0)\(([^)]*)\)", header, re.MULTILINE):
                        fields = [part.strip() for part in row[2].split(",")]
                        references.update(f"func_L{fields[0]}_{fields[n]}" for n in ref_columns[row[1]])
                    source = (GAMES / "rac3-pal/host/mp_fixes.c").read_text(encoding="utf-8").replace("\\\n", "")
                    definition = re.search(r"^#define MP_FIX\(([^)]*)\)", source, re.MULTILINE)
                    parameters = [part.strip() for part in definition[1].split(",")]
                    columns = [parameters.index(name) for name in ("delete_moby", "delete_light")]
                    header = (folder.parent / "host/mp_fixes.h").read_text(encoding="utf-8")
                    for row in re.finditer(r"^MP_FIX\(([^)]*)\)", header, re.MULTILINE):
                        fields = [part.strip() for part in row[1].split(",")]
                        references.update(f"func_L{fields[0]}_{fields[column]}" for column in columns)
                    self.assertTrue(references)
                    self.assertEqual(sorted(references - compiled.functions.keys()), [])

    def test_map_wrappers_have_one_owner_in_both_regions(self):
        # NTSC-U compiles PAL's host C with its own generated address headers.
        for region, maps in (("pal", range(40, 54)), ("ntsc", range(40, 56))):
            with self.subTest(region=region):
                exports = []
                for stem, macro in WRAPPERS:
                    source = (GAMES / f"rac3-pal/host/{stem}.c").read_text()
                    header = (GAMES / f"rac3-{region}/host/{stem}.h").read_text()
                    names = map_exports(source, header, macro)
                    self.assertEqual({int(name.split("_")[2][1:]) for name in names}, set(maps))
                    exports.extend(names)
                duplicates = {name: count for name, count in Counter(exports).items() if count > 1}
                self.assertEqual(duplicates, {})


class MapReferenceRoots(unittest.TestCase):
    def setUp(self):
        spec = util.spec_from_file_location("rac3_mp_maps", GAMES / "rac3-pal/mp/maps.py")
        self.maps = util.module_from_spec(spec)
        self.enterContext(mock.patch("sys.path", [str(GAMES / "rac3-pal/mp"), *sys.path]))
        spec.loader.exec_module(self.maps)
        self.signatures = {f"func_L{level}_{place}": signature
                           for level in (40, 41) for place, signature in
                           (("00100000", "unsigned long long (int)"), ("00100010", "void (int, int)"),
                            ("00100020", "int (int)"))}
        rows = {name: {level: place for level in (40, 41)} for name, place in
                (("tex0", "00100000"), ("grid", "00100010"), ("delete", "00100020"))}
        for name, value in (
            ("MAPS", (40, 41)), ("CARRIED", []),
            ("CARRIED_UI", [("tex0", "", "", "ref"), ("cos", "", "", "ref")]),
            ("CARRIED_HUD", [("tex0", "", "", "ref")]),
            ("CARRIED_VU0", [("cos", "native"), ("grid", "")]),
            ("FIXED", [("delete", "", "", "ref")]),
            ("catalogue", lambda source: rows),
            ("map_signatures", lambda source, cfg: self.signatures),
        ):
            self.enterContext(mock.patch.object(self.maps, name, value))

    def test_references_are_typed_roots_but_host_replacements_are_not(self):
        config = {"roots": ["entry"], "asm": {"copy_signatures": {}}}
        self.maps.refresh_references(config, Path("unused"))
        self.assertEqual(config["roots"], ["entry", *self.signatures])
        self.assertEqual(config["asm"]["copy_signatures"], self.signatures)
        self.maps.refresh_references(config, Path("unused"))
        self.assertEqual(config["roots"], ["entry", *self.signatures])

    def test_an_unverified_reference_does_not_change_the_config(self):
        self.signatures.clear()
        config = {"roots": ["entry"], "asm": {"copy_signatures": {}}}
        with self.assertRaisesRegex(SystemExit, "need verified signatures"):
            self.maps.refresh_references(config, Path("unused"))
        self.assertEqual(config, {"roots": ["entry"], "asm": {"copy_signatures": {}}})


class MapExports(unittest.TestCase):
    SOURCE = (
        "#define MP_FIX(m, coll, delete_moby) \\\n"
        "    int mp_func_L##m##_##coll##__game(int a0); \\\n"
        "    int mp_func_L##m##_##coll(int a0) { \\\n"
        "        return call(mp_func_L##m##_##coll##__game, mp_func_L##m##_##delete_moby); \\\n"
        "    }\n"
    )

    def test_only_definitions_are_exports(self):
        self.assertEqual(map_exports(self.SOURCE, "MP_FIX(48, 004429B8, 0046C1D0)\n", "MP_FIX"),
                         ["mp_func_L48_004429B8"])

    def test_stale_generated_macro_arguments_are_rejected(self):
        for row in ("MP_FIX(48, 004429B8)", "MP_FIX(48, 004429B8, 0046C1D0, 0050C348)"):
            with self.subTest(row=row), self.assertRaises(ValueError):
                map_exports(self.SOURCE, row, "MP_FIX")

    def test_missing_macro_or_rows_are_rejected(self):
        for source, header in (("", "MP_FIX(48, 004429B8, 0046C1D0)"), (self.SOURCE, "")):
            with self.subTest(source=source, header=header), self.assertRaises(ValueError):
                map_exports(source, header, "MP_FIX")


if __name__ == "__main__":
    unittest.main()

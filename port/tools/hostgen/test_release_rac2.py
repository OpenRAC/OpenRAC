# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""release_rac2.py: the hook names of a Going Commando release's own C, checked without a disc."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import release  # noqa: E402
import release_rac2  # noqa: E402


class HookUnitsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.source, self.game = root / "decomp", root / "game"
        for path, text in {
            # v1.01's hooks and the release's catalogue, which has func_0026F790 at another place
            # and lacks func_0026F730 (the release's build names that one).
            "config/boot/functions.tsv": "func_0026F730\t0026F730\t8\nfunc_0026F790\t0026F790\t8\n",
            "config/pal/boot/functions.tsv": "func_0026F790\t0026F5F8\t8\n",
            "include/names.h": "#define stub_empty_5 func_0026F730\n#define stub_empty_17 func_0026F790\n",
            "src/pal/game/a.c": "void a(void) { stub_empty_17(0); stub_empty_5(); }\n",
            "src/game/b.c": "void b(void) { stub_empty_17(0); }\n",
        }.items():
            (self.source / path).parent.mkdir(parents=True, exist_ok=True)
            (self.source / path).write_text(text)
        (self.game / "release").mkdir(parents=True)
        (self.game / "hostgen.json").write_text(json.dumps({
            "source": str(self.source), "sources": ["src"], "aliases": ["include/names.h"],
            "release": {"version": "pal", "table": "release/release_names.json"}}))

    def tearDown(self):
        self.tmp.cleanup()

    def check(self, units):
        # pair_stubs paired func_0026F790 with the hook at 0x0026F600 by its callers.
        table = {"names": {"func_0026F790": "func_0026F600", "func_0026F730": "func_0026F598"},
                 "hooks": ["func_0026F730", "func_0026F790"], "units": units}
        (self.game / "release/release_names.json").write_text(json.dumps(table))
        return release_rac2.check(self.game, self.source)

    def test_the_catalogues_name_for_a_readable_one(self):
        self.assertEqual(release.catalogue_hooks(self.source, "pal", ["func_0026F730", "func_0026F790"]),
                         {"func_0026F790": "func_0026F5F8"})
        units = {"src/pal/game/a.c": {"stub_empty_17": "func_0026F5F8", "stub_empty_5": "FUN_0026F5A0"}}
        self.assertEqual(self.check(units), [])

    def test_a_missing_entry_fails(self):
        # The release's C would call the hook its callers pair, 8 bytes off (PAL levels 17 and 19).
        problems = self.check({"src/pal/game/a.c": {"stub_empty_5": "FUN_0026F5A0"}})
        self.assertEqual(len(problems), 1)
        self.assertIn("stub_empty_17 has no unit entry", problems[0])

    def test_a_wrong_or_left_over_entry_fails(self):
        problems = self.check({"src/pal/game/a.c": {"stub_empty_17": "func_0026F600", "stub_empty_5": "FUN_0026F5A0",
                                                    "func_0026F790": "func_0026F5F8"},
                               "src/pal/game/gone.c": {"stub_empty_17": "func_0026F5F8"}})
        self.assertEqual(len(problems), 3, problems)
        self.assertIn("func_0026F790 -> func_0026F5F8 is left over", problems[0])
        self.assertIn("stub_empty_17 -> func_0026F600, the catalogue calls it func_0026F5F8", problems[1])
        self.assertIn("gone.c: hook entries left over", problems[2])


if __name__ == "__main__":
    unittest.main()

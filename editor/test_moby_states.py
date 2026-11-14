"""moby_states.py: the packed levels' moby start states, by node name (after_pack)."""

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

import moby_states

# A states program as the port's writes its file: two mobys of the packed table, the second dead.
FAKE = """#!{python}
import json, sys
args = sys.argv[1:]
out = args[args.index("--moby-states") + 1]
assert args[args.index("--mods") + 1].endswith("packed")
json.dump({{"format": 1, "level": int(args[args.index("--level") + 1]), "ticks": 300,
           "mobys": [{{"key": 0, "class": 11, "state": 1, "hidden": 0, "dead": 0}},
                     {{"key": 1, "class": 500, "state": 0, "hidden": 0, "dead": 1}}]}}, open(out, "w"))
"""


def level(project: Path, number: int) -> Path:
    folder = project / f"level_{number:02}" / "levels" / f"level_{number:02}"
    folder.mkdir(parents=True)
    (folder / "level.json").write_text(json.dumps({"level": number, "version": "ntsc"}))
    return folder


@unittest.skipIf(os.name == "nt", "the fake program is a script")
class AfterPack(unittest.TestCase):
    def test_states_name_their_nodes_and_stale_ones_go(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            program = temp / "states"
            program.write_text(FAKE.format(python=sys.executable))
            program.chmod(0o755)
            project = temp / "project"
            packed, untouched = level(project, 5), level(project, 6)
            # Level 6 was packed with edits before and is packed as the disc has it now.
            (untouched / "moby_states.json").write_text(json.dumps({"packed": True, "mobys": []}))
            lines = moby_states.after_pack(program, temp, temp, temp / "packed", project,
                                           {5: (packed, ["Vendor_0001", "Crate"])})
            self.assertEqual(lines, ["level 05: moby start states updated"])
            result = json.loads((packed / "moby_states.json").read_text())
            self.assertTrue(result["packed"])
            self.assertEqual(result["names"], ["Vendor_0001", "Crate"])
            self.assertEqual([m["name"] for m in result["mobys"]], ["Vendor_0001", "Crate"])
            self.assertFalse((untouched / "moby_states.json").exists())

    def test_a_failing_program_says_so_and_packs_on(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            program = temp / "states"
            program.write_text("#!/bin/sh\necho broken >&2\nexit 3\n")
            program.chmod(0o755)
            folder = level(temp / "project", 5)
            lines = moby_states.after_pack(program, temp, temp, temp / "packed", temp / "project",
                                           {5: (folder, ["Vendor_0001"])})
            self.assertEqual(len(lines), 1)
            self.assertIn("moby start states failed", lines[0])
            self.assertIn("broken", lines[0])
            self.assertFalse((folder / "moby_states.json").exists())

    def test_a_disc_level_keeps_its_file(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = level(Path(temp) / "project", 5)
            (folder / "moby_states.json").write_text(json.dumps({"mobys": []}))
            moby_states.after_pack(Path(temp) / "none", Path(temp), Path(temp), Path(temp), Path(temp) / "project", {})
            self.assertTrue((folder / "moby_states.json").exists())


if __name__ == "__main__":
    unittest.main()

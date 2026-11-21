#!/usr/bin/env python3
"""release_carry_rac3.py's merge of rac3-ntsc's release.json "hostgen" keys over rac3-pal's carried
hostgen.json: dicts merge, a list of function names gets the release's added, another list is replaced, a value replaces, null removes."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from release_carry_rac3 import merge  # noqa: E402


class Merge(unittest.TestCase):
    def test_dicts_merge_and_lists_of_names_add(self):
        carried = {"asm": {"signatures": {"func_a": "int (int)"}, "enabled": True},
                   "from_assembly": ["func_a", "func_b"]}
        over = {"asm": {"signatures": {"func_c": "float (void)"}, "signatures_why": "evidence"},
                "from_assembly": ["func_b", "func_d"], "from_assembly_why": "evidence"}
        self.assertEqual(merge(carried, over), {
            "asm": {"signatures": {"func_a": "int (int)", "func_c": "float (void)"}, "enabled": True},
            "from_assembly": ["func_a", "func_b", "func_d"]})

    def test_values_and_other_lists_replace_and_null_removes(self):
        out = merge({"frame_rate": 50, "places": "x.tsv", "sources": ["src/a", "src/levels"]},
                    {"frame_rate": 60, "places": None, "sources": ["src/a", "src/us"]})
        self.assertEqual(out, {"frame_rate": 60, "sources": ["src/a", "src/us"]})


if __name__ == "__main__":
    unittest.main()

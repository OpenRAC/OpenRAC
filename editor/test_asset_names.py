"""The Assets dock's readable names (addons/openrac_library/names/<game>.tsv): one name per kind and
class, never a bare number or a placeholder, and the header credits where they come from.

    python3 -m unittest test_asset_names
"""

import re
import unittest
from pathlib import Path

NAMES = Path(__file__).parent / "addons" / "openrac_library" / "names"
KINDS = ("mobys", "ties", "shrubs")


class AssetNames(unittest.TestCase):
    def test_each_class_has_one_readable_name(self):
        for path in sorted(NAMES.glob("*.tsv")):
            seen = set()

            for line in path.read_text(encoding="utf-8").splitlines():
                if line.startswith("#") or not line:
                    continue

                kind, number, name = line.split("\t")
                self.assertIn(kind, KINDS, line)
                self.assertTrue(number.isdigit(), line)
                self.assertNotIn((kind, number), seen, line)
                seen.add((kind, number))
                self.assertEqual(name, name.strip(), line)
                self.assertTrue(name[:1].isupper(), line)
                # No "moby 12", "prop 10", "UNKNOWN" or code names.
                self.assertIsNone(re.search(r"(?i)\b(moby|tie|shrub|prop|class)\s*\d|unknown|_", name), line)

    def test_rac1_credits_its_sources(self):
        head = (NAMES / "rac1.tsv").read_text(encoding="utf-8")[:800]

        for source in ("Lombyte", "Wrench", "Replanetizer"):
            self.assertIn(source, head)


if __name__ == "__main__":
    unittest.main()

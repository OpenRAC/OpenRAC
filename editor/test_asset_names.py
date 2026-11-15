"""The Assets dock's readable names (addons/openrac_library/names/<game>.tsv): one name per kind and
class in short snake_case, never a bare number or a placeholder, and the header credits where they come from.

    python3 -m unittest test_asset_names
"""

import re
import unittest
from pathlib import Path

NAMES = Path(__file__).parent / "addons" / "openrac_library" / "names"
KINDS = ("mobys", "ties", "shrubs")


def wrapped(name: str, line: int = 17) -> list[str]:
    """NAME broken after underscores as the dock shows it (dock.gd wrapped())."""
    lines = [""]
    words = name.split("_")
    for i, word in enumerate(words):
        word += "_" if i < len(words) - 1 else ""
        if len(lines[-1] + word) <= line or not lines[-1]:
            lines[-1] += word
        else:
            lines.append(word)
    return lines


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
                # Short snake_case: small_turret.
                self.assertRegex(name, r"^[a-z0-9]+(_[a-z0-9]+){0,2}$", line)
                # Fits the dock's two lines under the picture.
                lines = wrapped(name)
                self.assertLessEqual(len(lines), 2, line)
                self.assertLessEqual(max(map(len, lines)), 17, line)
                # No "moby_12", "prop_10" or "unknown".
                self.assertIsNone(re.search(r"(^|_)(moby|tie|shrub|prop|class)_?\d|unknown", name), line)

    def test_rac1_credits_its_sources(self):
        head = (NAMES / "rac1.tsv").read_text(encoding="utf-8")[:800]

        for source in ("Lombyte", "Wrench", "Replanetizer"):
            self.assertIn(source, head)


if __name__ == "__main__":
    unittest.main()

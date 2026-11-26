"""progress_line.py: the tools say how far along they are in lines the launcher reads
(launcher/src/lib/progress.ts)."""

import io
import re
import unittest
from pathlib import Path

import progress_line

HERE = Path(__file__).resolve().parent
LAUNCHER = HERE.parent / "launcher" / "src" / "lib" / "progress.ts"


def said(out: str) -> list[dict]:
    return [p for p in (progress_line.parse(line) for line in out.splitlines()) if p is not None]


class Format(unittest.TestCase):
    def test_a_count_and_its_level(self):
        self.assertEqual(progress_line.text("read", 3, 18, 4), "openrac-progress: read 3/18 4")
        self.assertEqual(progress_line.text("pack", 0, 2), "openrac-progress: pack 0/2")
        self.assertEqual(progress_line.text("project"), "openrac-progress: project")

    def test_reads_back(self):
        for line in ("openrac-progress: read 3/18 4", "openrac-progress: pack 0/2", "openrac-progress: project"):
            parsed = progress_line.parse(line)
            self.assertIsNotNone(parsed)
            again = progress_line.text(parsed["step"], parsed["done"], parsed["total"], parsed["level"])
            self.assertEqual(again, line)
        self.assertIsNone(progress_line.parse("level 03: 12 meshes placed"))
        self.assertIsNone(progress_line.parse("openrac-progress: dance 1/2"))

    def test_refuses_what_the_launcher_could_not_read(self):
        with self.assertRaises(ValueError):
            progress_line.text("dance", 1, 2)
        with self.assertRaises(ValueError):
            progress_line.text("read", 3, 2)

    def test_counted_says_zero_then_each_item_as_it_ends(self):
        out = io.StringIO()
        seen = []
        for level in progress_line.counted("pack", [3, 7], lambda i: i, stream=out):
            seen.append(level)
            if level == 3:
                continue
        self.assertEqual(seen, [3, 7])
        self.assertEqual([(p["done"], p["total"], p["level"]) for p in said(out.getvalue())],
                         [(0, 2, None), (1, 2, 3), (2, 2, 7)])

    def test_every_step_is_one_the_launcher_names(self):
        source = LAUNCHER.read_text()
        self.assertIn(f'"{progress_line.LINE}"', source)
        named = set(re.findall(r'^\s+"?([a-z]+)"?: "', source[source.index("STEP_WORDS"):], re.M))
        self.assertLessEqual(set(progress_line.STEPS), named)


if __name__ == "__main__":
    unittest.main()

"""progress_line.py: the tools say how far along they are in lines the launcher reads
(launcher/src/lib/progress.ts), and Export the levels again (`godot --again`) carries every level."""

import io
import re
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

import extract
import progress_line
from test_extract import fake_godot

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


class ToolsSayIt(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.dest = Path(self.temp.name) / "levels" / "pal"

    def tearDown(self):
        self.temp.cleanup()

    def scene(self, i: int) -> Path:
        return self.dest / f"level_{i:02}" / "levels" / f"level_{i:02}" / f"level_{i:02}.tscn"

    def test_again_backs_up_every_level_and_carries_each_with_a_count(self):
        schema = extract.GODOT_SCHEMA
        extract.export_godot(self.dest, [0, 1], True, True, fake_godot(schema))
        self.scene(1).write_text("[gd_scene format=3]\n; moved a crate\n")
        carried = []

        def carry(i, old, new, backup):
            carried.append(i)
            return {"level": i, "carried": [], "not_carried": [], "mobys": 0, "failed": False, "backup": str(backup)}
        out = io.StringIO()
        with redirect_stdout(out):
            words = extract.export_godot(self.dest, [0, 1], True, True, fake_godot(schema), carry=carry, again=True)
        self.assertIn("2 written again", words)
        self.assertEqual(carried, [0, 1])
        backups = list((self.dest.parent / "pal-backups").glob("*/level_01/levels/level_01/level_01.tscn"))
        self.assertEqual(len(backups), 1)
        self.assertIn("moved a crate", backups[0].read_text())
        steps = [(p["step"], p["done"], p["total"], p["level"]) for p in said(out.getvalue())]
        self.assertEqual(steps, [("carry", 0, 2, None), ("carry", 1, 2, 0), ("carry", 2, 2, 1)])

    def test_without_again_a_current_level_is_kept(self):
        schema = extract.GODOT_SCHEMA
        extract.export_godot(self.dest, [0], True, True, fake_godot(schema))
        out = io.StringIO()
        with redirect_stdout(out):
            words = extract.export_godot(self.dest, [0], True, True, fake_godot(schema), carry=lambda *a: {})
        self.assertIn("1 level(s) kept", words)
        self.assertEqual(said(out.getvalue()), [])


if __name__ == "__main__":
    unittest.main()

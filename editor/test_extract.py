"""Output safety: new directories under assets/ or build/, published whole."""

from pathlib import Path
import tempfile
import unittest

from extract import ROOT, output_path, publish
from formats import FormatError


class OutputTests(unittest.TestCase):
    def test_output_must_be_new_and_ignored(self):
        with self.assertRaises(FormatError):
            output_path(ROOT / "docs" / "extracted")
        with self.assertRaises(FormatError):
            output_path(ROOT / "assets" / ".." / "extracted")
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            with self.assertRaises(FormatError):
                output_path(Path(temp))
            escape = Path(temp) / "escape"
            escape.symlink_to(ROOT / "docs", target_is_directory=True)
            with self.assertRaises(FormatError):
                output_path(escape / "extracted")

    def test_publish_is_all_or_nothing(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as temp:
            dest = Path(temp) / "out"

            def fail(directory):
                (directory / "partial").write_text("x")
                raise FormatError("stop")

            with self.assertRaises(FormatError):
                publish(dest, fail)
            self.assertEqual(list(Path(temp).iterdir()), [])
            publish(dest, lambda directory: (directory / "done").write_text("x"))
            self.assertEqual([p.name for p in dest.iterdir()], ["done"])


if __name__ == "__main__":
    unittest.main()

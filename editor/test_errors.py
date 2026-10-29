"""errors.py: every way a tool stops says a code the launcher maps (launcher/src/lib/toolErrors.ts)."""

import errno
import re
import struct
import subprocess
import sys
import unittest
from pathlib import Path

import errors
from formats import FormatError

HERE = Path(__file__).resolve().parent
MAPPING = HERE.parent / "launcher" / "src" / "lib" / "toolErrors.ts"


def raised_codes() -> set[str]:
    """Every code="..." the editor's tools raise, and every code=... errors.py says itself."""
    found = set()
    for path in HERE.glob("*.py"):
        if path.name.startswith("test_"):
            continue
        found |= set(re.findall(r'code="([a-z0-9-]+)"', path.read_text()))
    found |= set(re.findall(r'return "([a-z0-9-]+)"', (HERE / "errors.py").read_text()))
    return found


class Codes(unittest.TestCase):
    def test_a_format_error_says_its_own_code_or_disc_data(self):
        self.assertEqual(errors.code_of(FormatError("x", code="project-outdated-gameplay")), "project-outdated-gameplay")
        self.assertEqual(errors.code_of(FormatError("range 0x0+0x10 exceeds 0x4 bytes")), "disc-data")
        self.assertEqual(errors.code_of(struct.error("unpack requires a buffer")), "disc-data")

    def test_file_errors_by_their_kind(self):
        self.assertEqual(errors.code_of(OSError(errno.ENOSPC, "No space left on device")), "disk-full")
        self.assertEqual(errors.code_of(PermissionError(errno.EACCES, "Permission denied")), "file-locked")
        self.assertEqual(errors.code_of(FileNotFoundError(errno.ENOENT, "No such file")), "file-missing")
        self.assertEqual(errors.code_of(OSError(errno.EIO, "I/O error")), "file-error")
        self.assertEqual(errors.code_of(KeyError("x")), "internal")

    def test_every_code_the_tools_raise_is_mapped_by_the_launcher(self):
        mapping = MAPPING.read_text()
        known = set(re.findall(r'^\s*"?([a-z0-9-]+)"?:\s*\{', mapping, re.M))
        missing = sorted(raised_codes() - known)
        self.assertEqual(missing, [], f"add these codes to {MAPPING.name}")

    def test_extract_says_the_code_line_before_its_message(self):
        done = subprocess.run([sys.executable, str(HERE / "extract.py"), "survey", str(HERE / "no-such-disc.iso")],
                              capture_output=True, text=True, check=False)
        self.assertEqual(done.returncode, 1)
        lines = done.stderr.splitlines()
        self.assertEqual(lines[0], f"{errors.LINE}file-missing")
        self.assertTrue(lines[1].startswith("extract: "))


class ProjectFormat(unittest.TestCase):
    def test_the_launcher_reads_what_the_editor_writes(self):
        import json
        import disc
        import godot
        stated = json.loads((HERE / "project_format.json").read_text())
        self.assertEqual(godot.SCHEMA, stated["schema"])
        self.assertEqual(stated["gameplay"], {"rac1/pal": disc.PAL.gameplay})


if __name__ == "__main__":
    unittest.main()

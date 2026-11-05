# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Offline regression tests for the BC6-only Compressonator extraction: unittest discover -s tools."""

import io
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import unittest


MODULE = Path(__file__).resolve().parents[1] / "port/cmake/Compressonator.cmake"
FILES = (
    "cmp_core/shaders/bc6_common_encoder.h", "cmp_core/shaders/bc6_encode_kernel.cpp",
    "cmp_core/shaders/bc6_encode_kernel.h", "cmp_core/shaders/bcn_common_api.h",
    "cmp_core/shaders/common_def.h", "cmp_core/source/cmp_core.h",
    "cmp_core/source/cmp_math_func.h", "cmp_core/source/cmp_math_vec4.h", "license/corelicense.txt",
)


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
class Subset(unittest.TestCase):
    def setUp(self):
        """Supply a synthetic archive through FetchContent's offline source override."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.archive = self.root / "archive"
        self.archive.mkdir()
        commit = re.search(r"set\(_compressonator_commit ([0-9a-f]+)\)", MODULE.read_text())[1]
        self.prefix = "compressonator-" + commit
        self.subset = self.archive / self.prefix
        script = self.root / "CMakeLists.txt"
        script.write_text(
            'cmake_minimum_required(VERSION 3.24)\nproject(CompressonatorSubset NONE)\n'
            'set(FETCHCONTENT_SOURCE_DIR_OPENRAC_BC6 "' + self.archive.as_posix() + '")\n'
            'include("' + MODULE.as_posix() + '")\n')
        self.command = ["cmake", "-S", str(self.root), "-B", str(self.root / "build"), "-G", "Ninja"]
        self.write_archive(FILES)

    def write_archive(self, files):
        """Include unrelated applications and Git history that must never be extracted."""
        with tarfile.open(self.archive / "compressonator.tar.gz", "w:gz") as archive:
            for name in [*files, "applications/gui.cpp", ".git/objects/history"]:
                data = name.encode()
                entry = tarfile.TarInfo(self.prefix + "/" + name)
                entry.size = len(data)
                archive.addfile(entry, io.BytesIO(data))

    def run_extraction(self, check=True):
        """Exercise the production CMake module without networking or a compiler."""
        return subprocess.run(self.command, cwd=self.root, capture_output=True, text=True, check=check)

    def test_only_bc6_dependencies_and_licence_are_extracted(self):
        """Keep the exact required upstream bytes and exclude unrelated source and Git history."""
        self.run_extraction()
        found = {file.relative_to(self.subset).as_posix() for file in self.subset.rglob("*") if file.is_file()}
        self.assertEqual(set(FILES), found)
        for name in FILES:
            self.assertEqual(name.encode(), (self.subset / name).read_bytes())
        original = self.subset / FILES[0]
        old_time = 1_000_000_000
        os.utime(original, ns=(old_time, old_time))
        self.run_extraction()
        self.assertEqual(old_time, original.stat().st_mtime_ns)

    def test_incomplete_restored_subset_is_repaired(self):
        """A cache missing a required header is repaired from the verified archive."""
        self.run_extraction()
        header = self.subset / "cmp_core/source/cmp_core.h"
        header.unlink()
        self.run_extraction()
        self.assertEqual(b"cmp_core/source/cmp_core.h", header.read_bytes())

    def test_missing_required_archive_file_fails_configure(self):
        """Do not accept an archive that silently omits a required header or licence."""
        self.write_archive(FILES[:-1])
        result = self.run_extraction(check=False)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("license/corelicense.txt", result.stderr)

    def test_existing_offline_source_override_is_preserved(self):
        """Developer-provided source checkouts remain usable without an archive download."""
        source = self.root / "custom"
        source.mkdir()
        self.command.insert(1, "-DFETCHCONTENT_SOURCE_DIR_OPENRAC_COMPRESSONATOR=" + source.as_posix())
        self.run_extraction()
        self.assertFalse(self.subset.exists())


if __name__ == "__main__":
    unittest.main()

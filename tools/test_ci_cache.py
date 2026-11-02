# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""CI preparation isolation and a real metadata-only CMake restore: unittest discover -s tools."""

import copy
import contextlib
import io
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

import ci_cache


class Identity(unittest.TestCase):
    def test_environment_changes_invalidate_configure_results(self):
        identity = {"workspace": "/work", "tools": [{"path": "/clang", "sha256": "a"}],
                    "python": {"path": "/python", "version": "3.12"},
                    "environment": {"ImageVersion": "1"}, "packages": ["sdk=1"]}
        original = ci_cache.fingerprint(identity)
        self.assertEqual(original, ci_cache.fingerprint(dict(reversed(list(identity.items())))))
        for field, changed in (("workspace", "/other"), ("tools", [{"path": "/clang", "sha256": "b"}]),
                               ("python", {"path": "/python", "version": "3.13"}),
                               ("environment", {"ImageVersion": "2"}), ("packages", ["sdk=2"])):
            variant = copy.deepcopy(identity)
            variant[field] = changed
            self.assertNotEqual(original, ci_cache.fingerprint(variant), field)

    def test_compiler_replacement_changes_identity_even_with_same_version(self):
        with tempfile.TemporaryDirectory() as temporary:
            program = Path(temporary) / "compiler"
            program.write_bytes(b"first compiler")
            with mock.patch("ci_cache.shutil.which", return_value=str(program)), \
                    mock.patch("ci_cache.output", return_value="same advertised version"):
                first = ci_cache.tool_identity("compiler")
                # Each job is a new process; simulate replacing a tool before the next job.
                ci_cache.file_digest.cache_clear()
                program.write_bytes(b"changed compiler")
                self.assertNotEqual(first, ci_cache.tool_identity("compiler"))
            ci_cache.file_digest.cache_clear()

    def test_missing_tool_fails_before_restore(self):
        with mock.patch("ci_cache.shutil.which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "not found"):
                ci_cache.tool_identity("missing")

    def test_missing_identity_leaves_cache_disabled(self):
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary) / "outputs"
            with mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}), \
                    mock.patch("ci_cache.installed_identity", side_effect=RuntimeError("no SDK identity")), \
                    contextlib.redirect_stdout(io.StringIO()) as log:
                ci_cache.main()
            self.assertIn("Skipping CI preparation cache", log.getvalue())
            self.assertFalse(out.exists())

    def test_runner_changes_affect_collected_identity(self):
        with mock.patch("ci_cache.tool_identity", return_value={"version": "same compiler"}), \
                mock.patch("ci_cache.system_packages", return_value=["sdk=1"]), \
                mock.patch.dict(os.environ, {"OPENRAC_CLANG": "/tools/clang", "ImageVersion": "one"}):
            first = ci_cache.fingerprint(ci_cache.installed_identity())
            with mock.patch.dict(os.environ, {"ImageVersion": "two"}):
                self.assertNotEqual(first, ci_cache.fingerprint(ci_cache.installed_identity()))
            with mock.patch("ci_cache.Path.cwd", return_value=Path("/another-workspace")):
                self.assertNotEqual(first, ci_cache.fingerprint(ci_cache.installed_identity()))


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja") and shutil.which("clang"),
                     "CMake, Ninja and Clang required")
@unittest.skipIf(os.name == "nt" and not os.environ.get("INCLUDE"),
                 "Configure the Windows compiler environment before running the CMake fixture")
class ConfigureRestore(unittest.TestCase):
    def test_metadata_reuses_probes_but_rebuilds_changed_sources(self):
        """Restore no objects/Ninja files; retain a probe and build/test the updated program."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, build, saved = root / "source", root / "build", root / "saved"
            source.mkdir()
            (source / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.24)
project(CacheRestore C)
include(CheckCSourceCompiles)
check_c_source_compiles("int main(void) { return 0; }" WORKING_COMPILER)
if(NOT WORKING_COMPILER)
  message(FATAL_ERROR "Compiler probe failed")
endif()
enable_testing()
add_executable(probe main.c)
add_test(NAME actual_program COMMAND probe)
''')
            (source / "main.c").write_text("int main(void) { return 1; }\n")
            configure = ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                         "-DCMAKE_C_COMPILER=" + shutil.which("clang")]
            run = lambda command: subprocess.run(command, capture_output=True, text=True, check=True)
            first = run(configure)
            self.assertIn("Performing Test WORKING_COMPILER", first.stdout)
            run(["cmake", "--build", str(build)])
            saved.mkdir()
            shutil.copy2(build / "CMakeCache.txt", saved / "CMakeCache.txt")
            # The action restores only CMake's versioned compiler metadata, not target directories.
            for directory in (build / "CMakeFiles").glob("[0-9]*"):
                shutil.copytree(directory, saved / "CMakeFiles" / directory.name,
                                ignore=shutil.ignore_patterns("*.o", "*.obj", "*.pdb"))
            self.assertEqual(build.resolve().parent, root.resolve())
            shutil.rmtree(build)
            shutil.copytree(saved, build)
            self.assertFalse((build / "build.ninja").exists())
            self.assertFalse(list(build.glob("probe*")))
            self.assertFalse(list(build.rglob("*.o")))
            self.assertFalse(list(build.rglob("*.obj")))
            (source / "main.c").write_text("int main(void) { return 0; }\n")
            second = run(configure)
            self.assertNotIn("Performing Test WORKING_COMPILER", second.stdout)
            rebuilt = run(["cmake", "--build", str(build)])
            self.assertIn("main.c", rebuilt.stdout)
            run(["ctest", "--test-dir", str(build), "--output-on-failure", "--no-tests=error"])


if __name__ == "__main__":
    unittest.main()

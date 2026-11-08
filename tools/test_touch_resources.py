# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Editing an existing touch icon or action set must regenerate its embedded bytes."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
class TouchResourcesTest(unittest.TestCase):
    def test_existing_resource_edits_reconfigure_the_build(self):
        # Keep the small fixture recoverable, including when a build fails.
        root = Path(tempfile.mkdtemp(prefix="openrac-touch-resources-"))
        source = root / "source"
        source.mkdir()
        build = root / "build"
        cmake = Path(__file__).resolve().parents[1] / "port/cmake/Games.cmake"
        function = re.search(
            r"^function\(openrac_touch_files out\)\n.*?^endfunction\(\)",
            cmake.read_text(), re.MULTILINE | re.DOTALL,
        ).group()
        (source / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.24)\nproject(touch_resources NONE)\n'
            'set(OPENRAC_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game")\n'
            + function + '\nopenrac_touch_files(resources)\n'
            'add_custom_target(resources ALL DEPENDS "${resources}")\n'
        )
        files = [source / "game/common/touch/icons/wrench.svg",
                 source / "game/common/touch/sets/common.json"]
        for file in files:
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text("initial")

        def run(*args):
            subprocess.run(["cmake", *map(str, args)], check=True, capture_output=True, text=True)

        def encoded(text):
            return "".join(f"0x{byte:02x}," for byte in text.encode())

        run("-S", source, "-B", build, "-G", "Ninja")
        run("--build", build)
        output = build / "generated/openrac_touch_files.cpp"
        self.assertIn(encoded("initial"), output.read_text())
        for index, file in enumerate(files):
            payload = f"edited resource {index}"
            file.write_text(payload)
            # No explicit configure: normal builds must notice edits to existing files.
            run("--build", build)
            self.assertIn(encoded(payload), output.read_text(), file.name)


if __name__ == "__main__":
    unittest.main()

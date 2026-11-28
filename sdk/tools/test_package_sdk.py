# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Tests of package_sdk.py: python3 -m unittest discover -s sdk/tools"""
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import package_sdk  # noqa: E402


class PackageSdk(unittest.TestCase):
    def test_version_is_the_interfaces(self):
        api = package_sdk.interface()
        major, minor = (int(x) for x in api.split("."))
        self.assertEqual(package_sdk.version(), "dev")
        self.assertEqual(package_sdk.version(release=f"{api}.7"), f"{api}.7")
        if minor > 0:  # a dev release: a patch of the last stable minor, with the open minor's headers
            self.assertEqual(package_sdk.version(release=f"{major}.{minor - 1}.3"), f"{major}.{minor - 1}.3")
        for bad in (f"{api}", f"{api}.x", f"{major + 1}.0.0", f"{major - 1}.0.0", f"{major}.{minor + 1}.0",
                    f"{api}.1-dev"):
            self.assertRaises(SystemExit, package_sdk.version, release=bad)

    @unittest.skipUnless(shutil.which("cmake"), "no cmake")
    def test_cmake_takes_the_headers_version(self):
        # OpenRACModConfigVersion.cmake, as find_package(OpenRACMod <version>) runs it.
        major, minor = (int(x) for x in package_sdk.interface().split("."))
        cases = {"": True, f"{major}.0": True, f"{major}.{minor}": True, f"{major}.{minor + 1}": False,
                 f"{major + 1}.0": False, f"{major - 1}.{minor}": False}
        version_file = package_sdk.SDK / "cmake" / "OpenRACModConfigVersion.cmake"
        for want, ok in cases.items():
            with tempfile.TemporaryDirectory() as d:
                script = Path(d) / "probe.cmake"
                parts = want.split(".") if want else ["", ""]
                script.write_text(
                    f'set(PACKAGE_FIND_VERSION "{want}")\nset(PACKAGE_FIND_VERSION_MAJOR "{parts[0]}")\n'
                    f'include("{version_file.as_posix()}")\n'
                    'message("${PACKAGE_VERSION} ${PACKAGE_VERSION_COMPATIBLE}")\n')
                out = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True).stderr.split()
                self.assertEqual(out[0], f"{major}.{minor}", want)
                self.assertEqual(out[1] == "TRUE", ok, want)

    def test_the_archive_says_its_version(self):
        # VERSION and the templates' sdk.lock name the release the archive is, whatever the tree says.
        v = package_sdk.interface() + ".7"
        with tempfile.TemporaryDirectory() as a:
            archive = package_sdk.write(Path(a), release=v)
            self.assertEqual(archive.name, f"openrac-mod-sdk-{v}.zip")
            z = zipfile.ZipFile(archive)
            top = f"openrac-mod-sdk-{v}/"
            self.assertEqual(z.read(top + "VERSION").decode(), v + "\n")
            for template in package_sdk.TEMPLATES:
                lock = json.loads(z.read(f"{top}{template}/sdk.lock"))
                self.assertEqual(lock["version"], v)
                self.assertEqual(lock["url"], package_sdk.URL.format(v=v))
                self.assertEqual(lock["sha256"], "")
                self.assertIn("about", lock)

    def test_archive_is_whole_and_the_same_bytes_each_time(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            one = package_sdk.write(Path(a))
            two = package_sdk.write(Path(b))
            self.assertEqual(one.read_bytes(), two.read_bytes())
            top = f"openrac-mod-sdk-{package_sdk.version()}/"
            names = zipfile.ZipFile(one).namelist()
            for need in ["include/openrac/openrac_mod.h", "include/openrac/games/rac1.h",
                         "cmake/OpenRACModConfig.cmake", "cmake/OpenRACModConfigVersion.cmake",
                         "tools/package_mod.py", "template/openrac-mod.json", "README.md",
                         "docs/MODS.md", "LICENSE", "VERSION", "template/.github/workflows/build.yml",
                         "template/tools/fetch_sdk.py", "template/sdk.lock", "template/.gitignore", "template/.gitattributes",
                         "template/LICENSE", "catalogue/rac1/behaviour.json", "include/openrac/mod.hpp",
                         "include/openrac/games/rac1.hpp", "template/src/mod.cpp", "tests/mod_hpp_test.cpp",
                         "tests/CMakeLists.txt",
                         "template-script/openrac-mod.json", "template-script/main.luau",
                         "template-script/sdk.lock", "template-script/tools/fetch_sdk.py",
                         "template-script/.github/workflows/build.yml", "template-script/.gitattributes",
                         "docs/README.md", "docs/getting-started.md", "docs/reference/c-api.md",
                         "docs/reference/hooks-rac1.md", "bindings/rac1.ids.lock"]:
                self.assertIn(top + need, names)
            self.assertFalse([n for n in names if "__pycache__" in n or "/build/" in n])
            sha = (Path(a) / (one.name + ".sha256")).read_text().split()[0]
            self.assertEqual(sha, hashlib.sha256(one.read_bytes()).hexdigest())


    def test_the_templates_share_their_files(self):
        # One workflow and one fetch_sdk.py for every mod repository (MODS.md, sections 10 and 11).
        sdk = package_sdk.SDK
        for name in package_sdk.SHARED:
            self.assertEqual((sdk / "template" / name).read_bytes(), (sdk / "template-script" / name).read_bytes(),
                             f"sdk/template-script/{name} is a copy of sdk/template/{name}: change the source, "
                             "then copy it over")

    def test_types_go_in_when_made(self):
        names = set(package_sdk.files())
        for f in sorted((package_sdk.SDK / "types").glob("*.d.luau")):
            self.assertIn(f"types/{f.name}", names)

if __name__ == "__main__":
    unittest.main()

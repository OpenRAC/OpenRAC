# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Checks Android tool selection, native-library packaging, NDK discovery and Python downloads.
Uses synthetic files and a fake network, without an SDK, native build or emulator."""
import contextlib
import hashlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import build_android  # noqa: E402

NDK_VARIABLES = ("ANDROID_NDK_HOME", "NDK_HOME", "ANDROID_NDK_ROOT", "ANDROID_NDK_LATEST_HOME",
                 "ANDROID_HOME", "ANDROID_SDK_ROOT")


class Response(io.BytesIO):
    """What urllib.request.urlopen gives, as a context manager."""

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False


class BuildAndroidTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.dir = Path(tmp.name)

    def make_ndk(self, path: Path) -> Path:
        (path / "build/cmake").mkdir(parents=True)
        (path / "build/cmake/android.toolchain.cmake").write_text("")
        return path

    def test_build_requests_the_unified_baker(self):
        for enabled in (False, True):
            defines = [f"-DOPENRAC_PROBE_BAKER={'ON' if enabled else 'OFF'}"]
            with self.subTest(enabled=enabled), \
                    mock.patch.object(build_android, "baker_defines", return_value=defines), \
                    mock.patch.object(build_android.proc, "run") as run:
                self.assertEqual(build_android.build_port(["rac1-ntsc"], self.dir, self.dir), enabled)
                command = run.call_args.args[0]
                for target in ("openrac-enhanced-baker",):
                    self.assertEqual(target in command, enabled)

    def test_a_game_partly_in_assembly_has_a_part_and_is_built_without_its_disc(self):
        self.assertTrue(build_android.has_part("rac2-ntsc"))
        self.assertFalse(build_android.has_part("rac1-ntsc"))
        with mock.patch.object(build_android.proc, "run") as run:
            build_android.build_port(["rac2-ntsc"], self.dir, self.dir, baker=False)
        configure = run.call_args_list[0].args[0]
        self.assertFalse([a for a in configure if "RETAIL_IMAGE" in str(a)])

    def test_llvm_is_built_once_for_its_version_and_options(self):
        ndk = self.make_ndk(self.dir / "ndk")
        llvm = self.dir / "llvm"
        (llvm / "llvm-project/llvm").mkdir(parents=True)
        (llvm / "llvm-project/llvm/CMakeLists.txt").write_text("")

        def fake(cmd, **kw):
            if cmd[:2] == ["cmake", "--build"]:
                for tool in build_android.PART_TOOLS:
                    (llvm / "build/bin").mkdir(parents=True, exist_ok=True)
                    (llvm / "build/bin" / tool).write_bytes(b"")
        with mock.patch.object(build_android, "LLVM_ANDROID", llvm), \
                mock.patch.object(build_android.proc, "run", side_effect=fake) as run:
            self.assertEqual(build_android.build_llvm(ndk), llvm / "build")
            self.assertEqual(run.call_count, 2)
            configure = run.call_args_list[0].args[0]
            self.assertIn("-DLLVM_TARGETS_TO_BUILD=AArch64", configure)
            build_android.build_llvm(ndk)
            self.assertEqual(run.call_count, 2)

    def native_tree(self):
        """Creates fake build outputs, including SDL and the NDK runtime, without compiling."""
        build, app, ndk, python = (self.dir / name for name in ("build", "app", "ndk", "python"))
        names = ["libopenrac-rac1-ntsc.so", "libopenrac-rac1-ntsc-cli.so",
                 "libopenrac-python.so", "libSDL3.so", "libopenrac-enhanced-baker.so"]
        names += [f"lib{name}.so" for name in build_android.ADRENOTOOLS_HOOKS]
        build.mkdir()
        for name in names:
            (build / name).write_bytes(name.encode())
        java = build / "android-project/app/src/main/java/org/libsdl"
        java.mkdir(parents=True)
        (java / "SDLActivity.java").write_text("synthetic SDL source")
        runtime = ndk / "toolchains/llvm/prebuilt/test/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so"
        runtime.parent.mkdir(parents=True)
        runtime.write_bytes(b"synthetic runtime")
        return build, app, ndk, python

    def test_android_library_collection_carries_the_unified_baker(self):
        build, app, ndk, python = self.native_tree()
        with mock.patch.object(build_android, "PORT_BUILD", build), \
                mock.patch.object(build_android, "APP", app):
            names = build_android.collect_libraries(["rac1-ntsc"], ndk, python, True, baker=True)
        for name in ("libopenrac-enhanced-baker.so",):
            self.assertIn(name, names)
            self.assertEqual((app / "src/main/jniLibs/arm64-v8a" / name).read_bytes(), name.encode())

    def test_android_packaging_rejects_a_missing_baker(self):
        build, app, ndk, python = self.native_tree()
        (build/"libopenrac-enhanced-baker.so").unlink()
        with mock.patch.object(build_android, "PORT_BUILD", build), \
                mock.patch.object(build_android, "APP", app), self.assertRaises(SystemExit) as stop:
            build_android.collect_libraries(["rac1-ntsc"], ndk, python, True, baker=True)
        self.assertIn("libopenrac-enhanced-baker.so", str(stop.exception))

    def test_android_no_baker_collection_needs_no_baker(self):
        build, app, ndk, python = self.native_tree()
        (build / "libopenrac-enhanced-baker.so").unlink()
        with mock.patch.object(build_android, "PORT_BUILD", build), \
                mock.patch.object(build_android, "APP", app):
            names = build_android.collect_libraries(["rac1-ntsc"], ndk, python, True, baker=False)
        self.assertNotIn("libopenrac-enhanced-baker.so", names)

    def test_android_library_collection_carries_clang_and_lld_as_libraries(self):
        build, app, ndk, python = self.native_tree()
        llvm = self.dir / "llvm"
        (llvm / "bin").mkdir(parents=True)
        for tool in build_android.PART_TOOLS:
            (llvm / "bin" / tool).write_bytes(tool.encode())
        with mock.patch.object(build_android, "PORT_BUILD", build), \
                mock.patch.object(build_android, "APP", app):
            names = build_android.collect_libraries(["rac1-ntsc"], ndk, python, True, llvm=llvm)
        for tool, name in build_android.PART_TOOLS.items():
            self.assertIn(name, names)
            self.assertEqual((app / "src/main/jniLibs/arm64-v8a" / name).read_bytes(), tool.encode())

    def test_embree_for_android_is_the_ports_target_with_arms_choice(self):
        ndk = self.make_ndk(self.dir / "ndk")
        options = build_android.embree_android_options(ndk)
        self.assertEqual(options["CMAKE_TOOLCHAIN_FILE"], (ndk / "build/cmake/android.toolchain.cmake").as_posix())
        self.assertEqual((options["ANDROID_ABI"], options["ANDROID_PLATFORM"], options["ANDROID_STL"]),
                         ("arm64-v8a", "android-28", "c++_shared"))
        self.assertEqual(options["EMBREE_STATIC_LIB"], "ON")
        # No x86 instruction sets: Embree keeps its own NEON on ARM.
        self.assertNotIn("EMBREE_ISA_AVX", options)

    def test_a_baker_that_does_not_build_is_left_out(self):
        ndk = self.make_ndk(self.dir / "ndk")
        failed = build_android.desktop.BuildError("no compiler")
        with mock.patch.object(build_android, "build_embree", side_effect=failed), \
                contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(build_android.baker_defines(ndk), ["-DOPENRAC_PROBE_BAKER=OFF"])
        self.assertIn("probe baker is left out", err.getvalue())
        with mock.patch.object(build_android, "build_embree", return_value=self.dir / "embree"):
            self.assertEqual(build_android.baker_defines(ndk),
                             ["-DOPENRAC_PROBE_BAKER=ON", f"-Dembree_DIR={(self.dir / 'embree').as_posix()}"])

    def test_ndk_home_from_its_variables_in_order(self):
        first, second = self.make_ndk(self.dir / "one"), self.make_ndk(self.dir / "two")
        env = {name: "" for name in NDK_VARIABLES}
        with mock.patch.dict(os.environ, dict(env, NDK_HOME=str(second), ANDROID_NDK_ROOT=str(first))):
            self.assertEqual(build_android.ndk_home(), second)
        # A variable naming no NDK is passed over.
        with mock.patch.dict(os.environ, dict(env, ANDROID_NDK_HOME=str(self.dir), NDK_HOME=str(first))):
            self.assertEqual(build_android.ndk_home(), first)

    def test_ndk_home_from_the_sdk_takes_the_newest(self):
        for version in ("26.1", "27.0"):
            self.make_ndk(self.dir / "sdk/ndk" / version)
        env = dict({name: "" for name in NDK_VARIABLES}, ANDROID_SDK_ROOT=str(self.dir / "sdk"))
        with mock.patch.dict(os.environ, env):
            self.assertEqual(build_android.ndk_home(), self.dir / "sdk/ndk/27.0")

    def test_no_ndk_stops(self):
        with mock.patch.dict(os.environ, {name: "" for name in NDK_VARIABLES}), self.assertRaises(SystemExit) as stop:
            build_android.ndk_home()
        self.assertIn("ANDROID_NDK_HOME", str(stop.exception))

    def download(self, data: bytes, sha256: str) -> tuple[Path, mock.Mock]:
        urlopen = mock.Mock(return_value=Response(data))
        with mock.patch.object(build_android, "BUILD", self.dir), mock.patch("urllib.request.urlopen", urlopen), \
                contextlib.redirect_stdout(io.StringIO()):
            return build_android.download("stdlib", sha256), urlopen

    def test_download_fetches_checks_and_keeps(self):
        data = b"python for android"
        good = hashlib.sha256(data).hexdigest()
        dest, urlopen = self.download(data, good)
        name = f"target-{build_android.PYTHON_PACKAGE}-stdlib.zip"
        self.assertEqual(dest, self.dir / "downloads" / name)
        self.assertEqual(dest.read_bytes(), data)
        urlopen.assert_called_once_with(f"{build_android.PYTHON_MAVEN}/{build_android.PYTHON_PACKAGE}/{name}")
        # Kept: the second time nothing is fetched.
        _, urlopen = self.download(b"", good)
        urlopen.assert_not_called()

    def test_download_refetches_a_damaged_copy_and_refuses_a_wrong_one(self):
        data = b"python for android"
        good = hashlib.sha256(data).hexdigest()
        dest = self.dir / "downloads" / f"target-{build_android.PYTHON_PACKAGE}-stdlib.zip"
        dest.parent.mkdir(parents=True)
        dest.write_bytes(b"damaged")
        self.assertEqual(self.download(data, good)[0].read_bytes(), data)
        dest.write_bytes(b"damaged")
        with self.assertRaises(SystemExit) as stop:
            self.download(b"something else", good)
        self.assertEqual(str(stop.exception),
                         f"{dest.name}: its SHA-256 is {hashlib.sha256(b'something else').hexdigest()}, not {good}")
        # What failed its check is not kept.
        self.assertEqual(dest.read_bytes(), b"damaged")


if __name__ == "__main__":
    unittest.main()

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""CI preparation isolation and a real metadata-only CMake restore: unittest discover -s tools."""

import copy
import contextlib
import glob
import io
import ntpath
import os
from pathlib import Path
import posixpath
import re
import shutil
import subprocess
import tempfile
import tarfile
import unittest
from unittest import mock

import ci_cache


ROOT = Path(__file__).resolve().parents[1]


def cache_settings(step):
    """Compare action settings without a following step's explanatory comment."""
    return step.split("      with:\n")[1].split("    #")[0].rstrip()


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "CMake and Ninja required")
class DependencySourceRestore(unittest.TestCase):
    def test_restored_source_bypasses_fetch_and_missing_source_still_downloads(self):
        """Fresh configure can use a source-only cache with no FetchContent build state."""
        module = ROOT / "port/cmake/CiDependencySources.cmake"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            restored = build / "_deps/restored-src"
            restored.mkdir(parents=True)
            (restored / "CMakeLists.txt").write_text("add_library(restored INTERFACE)\n")
            payload = root / "CMakeLists.payload"
            payload.write_text("add_library(missing INTERFACE)\n")
            archive = root / "dependency.tar"
            with tarfile.open(archive, "w") as stream:
                stream.add(payload, arcname="CMakeLists.txt")
            (root / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.24)
project(SourceRestore NONE)
include("{module.as_posix()}")
include(FetchContent)
FetchContent_Declare(restored URL "{(root / 'unavailable.tar').as_posix()}")
FetchContent_Declare(missing URL "{archive.as_posix()}")
FetchContent_MakeAvailable(restored missing)
if(NOT TARGET restored OR NOT TARGET missing)
  message(FATAL_ERROR "Both cached and uncached sources are required")
endif()
''', encoding="utf-8")
            for _ in range(2):
                result = subprocess.run(
                    ["cmake", "-G", "Ninja", "-S", str(root), "-B", str(build)],
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertFalse((build / "_deps/restored-subbuild").exists())
            self.assertTrue((build / "_deps/missing-src/CMakeLists.txt").is_file())
            cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
            self.assertNotIn("FETCHCONTENT_SOURCE_DIR_RESTORED:PATH=", cache)


class CachePolicy(unittest.TestCase):
    def test_native_compiler_caches_do_not_save_pr_private_snapshots(self):
        """Checks and launcher builds share the success-only compiler cache policy."""
        for name, count in (("checks.yml", 4), ("launcher.yml", 1)):
            text = (ROOT / ".github/workflows" / name).read_text()
            self.assertEqual(count, text.count("uses: ./.github/actions/compiler-cache"), name)
            self.assertNotIn("uses: hendrikmuhs/ccache-action@", text, name)
        text = (ROOT / ".github/workflows/checks.yml").read_text()
        self.assertIn("CCACHE_COMPILERCHECK: content", text)

    def test_compiler_cache_publishing_uses_success_only_post_action(self):
        """Cancelled jobs cannot publish through ccache-action's unconditional post step."""
        text = (ROOT / ".github/actions/compiler-cache/action.yml").read_text()
        steps = re.split(r"(?m)^    - ", text)
        setup = next(step for step in steps if "uses: hendrikmuhs/ccache-action@" in step)
        self.assertIn('save: "false"', setup)
        self.assertIn('restore: "false"', setup)
        # This pinned action declares post-if: success(); restore has no publishing post step.
        pin = "55cc8345863c7cc4c66a329aec7e433d2d1c52a9"
        write = next(step for step in steps if "uses: actions/cache@" + pin in step)
        read = next(step for step in steps if "uses: actions/cache/restore@" + pin in step)
        self.assertIn("github.event_name != 'pull_request'", write)
        self.assertIn("github.event_name == 'pull_request'", read)
        self.assertEqual(cache_settings(write), cache_settings(read))
        self.assertNotIn("save-always:", text)
        self.assertNotIn("always()", text)
        # Archives restore before setup so cached ccache.conf cannot override current job limits.
        self.assertLess(text.index("uses: actions/cache@"), text.index("uses: hendrikmuhs/ccache-action@"))
        self.assertIn("ccache-${{ inputs.key }}-success-${{ github.run_id }}-${{ github.run_attempt }}", write)
        prefixes = write.split("restore-keys: |\n")[1]
        self.assertLess(prefixes.index("inputs.key }}-success-"), prefixes.rindex("inputs.key }}-"))

    def test_compiler_cache_paths_match_existing_archive_versions(self):
        """Windows and POSIX path spelling must match ccache-action's cacheDir implementation."""
        text = (ROOT / ".github/actions/compiler-cache/action.yml").read_text()
        windows, unix = re.search(
            r"format\(runner.os == 'Windows' && '([^']+)' \|\| '([^']+)', github.workspace\)", text
        ).groups()
        for template, workspace, join in ((windows, r"D:\a\OpenRAC\OpenRAC", ntpath.join),
                                          (unix, "/home/runner/work/OpenRAC/OpenRAC", posixpath.join)):
            self.assertEqual(join(workspace, ".ccache"), template.format(workspace))
        self.assertIn("env.CCACHE_DIR ||", text)

    def test_preparation_restore_and_save_use_identical_isolated_keys(self):
        """PRs restore the same preparation as branch builds, through a read-only action."""
        text = (ROOT / ".github/actions/port-build-cache/action.yml").read_text()
        steps = re.split(r"(?m)^    - ", text)
        preparation = [step for step in steps if "key: prep-v3-" in step]
        self.assertEqual(2, len(preparation))
        write = next(step for step in preparation if "uses: actions/cache@" in step)
        read = next(step for step in preparation if "uses: actions/cache/restore@" in step)
        self.assertIn("github.event_name != 'pull_request'", write)
        self.assertIn("github.event_name == 'pull_request'", read)
        self.assertEqual(cache_settings(write), cache_settings(read))
        self.assertIn("outputs.fingerprint", read)
        self.assertIn(".github/workflows/checks.yml", read)
        self.assertNotIn("games/*/gen", read)
        self.assertNotIn("hashFiles('games/**',", read)
        self.assertNotIn("/_deps/", read)

    def test_dependency_source_key_covers_the_pins(self):
        """A workflow edit keeps sources reusable while a dependency declaration changes the key."""
        text = (ROOT / ".github/actions/port-build-cache/action.yml").read_text()
        steps = [step for step in re.split(r"(?m)^    - ", text) if "key: port-sources-v1-" in step]
        self.assertEqual(2, len(steps))
        write = next(step for step in steps if "uses: actions/cache@" in step)
        read = next(step for step in steps if "uses: actions/cache/restore@" in step)
        self.assertIn("github.event_name != 'pull_request'", write)
        self.assertIn("github.event_name == 'pull_request'", read)
        self.assertEqual(cache_settings(write), cache_settings(read))
        self.assertIn("/_deps/*-src", read)
        for unrelated in ("CMakeCache.txt", "-subbuild", "checks.yml", "outputs.fingerprint",
                          "restore-keys:"):
            self.assertNotIn(unrelated, read)
        for cmake in (ROOT / "port").glob("cmake/*.cmake"):
            if re.search(r"(?im)^\s*FetchContent_Declare\(", cmake.read_text(encoding="utf-8")):
                self.assertIn(cmake.relative_to(ROOT).as_posix(), read)

    def test_hostgen_restore_is_independent_and_always_validated(self):
        """Only generated text is restored, without a Ninja stamp that could bypass validation."""
        text = (ROOT / ".github/actions/port-build-cache/action.yml").read_text()
        steps = [step for step in re.split(r"(?m)^    - ", text) if "key: hostgen-v1-" in step]
        self.assertEqual(2, len(steps))
        write = next(step for step in steps if "uses: actions/cache@" in step)
        read = next(step for step in steps if "uses: actions/cache/restore@" in step)
        self.assertIn("github.event_name != 'pull_request'", write)
        self.assertIn("github.event_name == 'pull_request'", read)
        self.assertEqual(cache_settings(write), cache_settings(read))
        self.assertIn("!port/build/${{ inputs.preset }}/games/*/gen/hostgen.stamp", read)
        self.assertIn("!port/build/${{ inputs.preset }}/games/*/gen/prep", read)
        self.assertIn("port/build/${{ inputs.preset }}/games/*/gen/**/*", read)
        self.assertIn("hashFiles('games/**',", read)
        self.assertIn("'tools/openrac_common/**'", read)
        self.assertIn("outputs.hostgen_fingerprint", read)
        self.assertNotIn("outputs.fingerprint", read)
        self.assertNotIn("CMakeCache.txt", read)
        self.assertIn("inputs.hostgen == 'true'", read)

    def test_hostgen_archive_cannot_recurse_into_excluded_output(self):
        """A cached parent directory must not smuggle excluded stamps or prepared sources into tar."""
        text = (ROOT / ".github/actions/port-build-cache/action.yml").read_text()
        step = next(step for step in re.split(r"(?m)^    - ", text) if "key: hostgen-v1-" in step)
        block = step.split("        path: |\n")[1].split("        key:")[0]
        patterns = [line.strip().replace("${{ inputs.preset }}", "release") for line in block.splitlines()]
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            gen = root / "port/build/release/games/game/gen"
            for name in ("inputs.sha256", "src/unit.c", "hostgen.stamp", "prep/src/unit.c"):
                file = gen / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(name)
            included, excluded = set(), set()
            for pattern in patterns:
                negative = pattern.startswith("!")
                matches = {Path(path).resolve() for path in glob.glob(
                    str(root / pattern.lstrip("!")), recursive=True)}
                (excluded if negative else included).update(matches)
            archive = root / "payload.tar"
            # actions/cache passes matched directories to tar, which recursively adds their children.
            with tarfile.open(archive, "w") as stream:
                for path in sorted(included - excluded):
                    stream.add(path, arcname=path.relative_to(root).as_posix())
            with tarfile.open(archive) as stream:
                names = stream.getnames()
            self.assertIn("port/build/release/games/game/gen/inputs.sha256", names)
            self.assertIn("port/build/release/games/game/gen/src/unit.c", names)
            self.assertFalse(any("prep" in Path(name).parts for name in names), names)
            self.assertFalse(any(Path(name).name == "hostgen.stamp" for name in names), names)

    def test_rust_cache_does_not_save_pr_private_snapshots(self):
        """Large Rust snapshots must not compete with shared native build caches on every PR."""
        text = (ROOT / ".github/actions/launcher-toolchain/action.yml").read_text()
        self.assertIn("save-if: ${{ github.event_name != 'pull_request' }}", text)


@unittest.skipUnless(shutil.which("ccache") and shutil.which("clang"), "ccache and Clang required")
@unittest.skipIf(os.name == "nt", "The compiler timestamp fixture uses a POSIX executable")
class CompilerRestore(unittest.TestCase):
    def test_compiler_reinstallation_reuses_objects_but_replacement_does_not(self):
        """Content identity hits across mtime changes and misses when compiler bytes change."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            compiler, source, obj = root / "compiler", root / "source.c", root / "source.o"
            compiler.write_text('#!/bin/sh\nexec "' + shutil.which("clang") + '" "$@"\n')
            compiler.chmod(0o755)
            source.write_text("int result(void) { return 42; }\n")
            env = {key: value for key, value in os.environ.items() if not key.startswith("CCACHE_")}
            env.update(CCACHE_DIR=str(root / "cache"), CCACHE_CONFIGPATH=str(root / "ccache.conf"),
                       CCACHE_COMPILERCHECK="content", CCACHE_LOGFILE=str(root / "ccache.log"))
            command = ["ccache", str(compiler), "-c", str(source), "-o", str(obj)]
            def compile_result():
                """Return this compilation's cache result without cumulative counters."""
                (root / "ccache.log").unlink(missing_ok=True)
                subprocess.run(command, env=env, check=True, capture_output=True)
                return (root / "ccache.log").read_text()
            self.assertIn("Result: cache_miss", compile_result())
            original = obj.read_bytes()
            changed_time = compiler.stat().st_mtime_ns + 10_000_000_000
            os.utime(compiler, ns=(changed_time, changed_time))
            obj.unlink()
            self.assertIn("Result: direct_cache_hit", compile_result())
            self.assertEqual(original, obj.read_bytes())
            compiler.write_text(compiler.read_text() + "# Compiler replacement\n")
            self.assertIn("Result: cache_miss", compile_result())


class Identity(unittest.TestCase):
    def test_cpu_and_runner_changes_leave_hostgen_identity_unchanged(self):
        """A native configure miss must not discard valid fixed-target MIPS translation."""
        with mock.patch("ci_cache.tool_identity", return_value={"sha256": "compiler"}), \
                mock.patch("ci_cache.system_packages", return_value=["compiler-runtime=1"]), \
                mock.patch("ci_cache.Path.read_text", return_value="flags\t: sse2 avx\n"), \
                mock.patch("ci_cache.platform.release", return_value="kernel-one"), \
                mock.patch.dict(os.environ, {"OPENRAC_CLANG": "/tools/clang", "ImageVersion": "one"}):
            generated = ci_cache.fingerprint(ci_cache.hostgen_identity())
            configured = ci_cache.fingerprint(ci_cache.installed_identity())
            with mock.patch("ci_cache.platform.release", return_value="kernel-two"), \
                    mock.patch("ci_cache.Path.read_text", return_value="flags\t: sse2 avx avx2\n"), \
                    mock.patch.dict(os.environ, {"ImageVersion": "two", "PROCESSOR_IDENTIFIER": "other CPU"}):
                self.assertEqual(generated, ci_cache.fingerprint(ci_cache.hostgen_identity()))
                self.assertNotEqual(configured, ci_cache.fingerprint(ci_cache.installed_identity()))

    def test_hostgen_identity_retains_its_compiler_and_platform_boundaries(self):
        """CPU-independent reuse must still isolate changed tools, Python, paths and platforms."""
        with mock.patch("ci_cache.tool_identity", return_value={"sha256": "compiler"}), \
                mock.patch("ci_cache.system_packages", return_value=["compiler-runtime=1"]), \
                mock.patch.dict(os.environ, {"OPENRAC_CLANG": "/tools/clang"}):
            first = ci_cache.fingerprint(ci_cache.hostgen_identity())
            for target, value in (("tool_identity", {"sha256": "replacement"}),
                                  ("Path.cwd", Path("/other-workspace")),
                                  ("platform.system", "other OS"), ("platform.machine", "other arch")):
                with mock.patch("ci_cache." + target, return_value=value):
                    self.assertNotEqual(first, ci_cache.fingerprint(ci_cache.hostgen_identity()), target)
            with mock.patch("ci_cache.sys.version", "other Python"):
                self.assertNotEqual(first, ci_cache.fingerprint(ci_cache.hostgen_identity()))
            if os.name != "nt":
                with mock.patch("ci_cache.system_packages", return_value=["compiler-runtime=2"]):
                    self.assertNotEqual(first, ci_cache.fingerprint(ci_cache.hostgen_identity()))

    def test_configure_identity_failure_does_not_disable_hostgen_reuse(self):
        """Hostgen can restore safely even if native CPU/configure identity is unavailable."""
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary) / "outputs"
            identity = {"clang": "known", "workspace": "known"}
            with mock.patch.dict(os.environ, {"GITHUB_OUTPUT": str(out)}), \
                    mock.patch("ci_cache.installed_identity", side_effect=RuntimeError("no CPU metadata")), \
                    mock.patch("ci_cache.hostgen_identity", return_value=identity), \
                    contextlib.redirect_stdout(io.StringIO()):
                ci_cache.main()
            self.assertEqual("hostgen_fingerprint=" + ci_cache.fingerprint(identity) + "\n", out.read_text())

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
                    mock.patch("ci_cache.hostgen_identity", side_effect=RuntimeError("no compiler identity")), \
                    contextlib.redirect_stdout(io.StringIO()) as log:
                ci_cache.main()
            self.assertIn("Skipping CI preparation cache", log.getvalue())
            self.assertFalse(out.exists())

    def test_runner_changes_affect_collected_identity(self):
        with mock.patch("ci_cache.tool_identity", return_value={"version": "same compiler"}), \
                mock.patch("ci_cache.system_packages", return_value=["sdk=1"]), \
                mock.patch("ci_cache.Path.read_text", return_value="flags\t: sse2 avx\n"), \
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

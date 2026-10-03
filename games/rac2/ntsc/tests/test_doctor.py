import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
SPEC = importlib.util.spec_from_file_location("doctor", ROOT / "scripts" / "doctor.py")
doctor = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(doctor)


def touch(root, relative):
    path = root.joinpath(*relative.split("/"))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"")
    return path


def gather(argv):
    stream = io.StringIO()
    with contextlib.redirect_stdout(stream):
        code = doctor.doctor(argv)
    return code, stream.getvalue()


class ParsingTests(unittest.TestCase):
    def test_pinned_versions_drops_extras_and_comments(self):
        with tempfile.TemporaryDirectory() as name:
            requirements = Path(name) / "requirements.txt"
            requirements.write_text(
                "# a comment\nsplat64[mips]==0.50.0\nspimdisasm==1.42.4\nPyYAML==6.0.3  # inline\n\n",
                encoding="utf-8")
            pins = doctor.pinned_versions(requirements)
        self.assertEqual(pins["splat64"], "0.50.0")
        self.assertEqual(pins["spimdisasm"], "1.42.4")
        self.assertEqual(pins["PyYAML"], "6.0.3")
        self.assertNotIn("[mips]", pins)

    def test_pinned_versions_of_a_missing_file_is_empty(self):
        self.assertEqual(doctor.pinned_versions(Path("no-such-requirements.txt")), {})

    def test_missing_instruments_lists_every_absent_name(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            self.assertEqual(doctor.missing_instruments(root, doctor.C_INSTRUMENTS),
                             list(doctor.C_INSTRUMENTS))
            touch(root, "bin/ee-gcc2953.exe")
            self.assertNotIn("bin/ee-gcc2953.exe",
                             doctor.missing_instruments(root, doctor.C_INSTRUMENTS))

    def test_inside_repository_accepts_the_checkout_and_rejects_a_sibling(self):
        self.assertTrue(doctor.inside_repository(doctor.ROOT / "build"))
        with tempfile.TemporaryDirectory() as name:
            self.assertFalse(doctor.inside_repository(Path(name)))


class ManifestTests(unittest.TestCase):
    def test_no_runtime_has_no_manifest(self):
        self.assertIsNone(doctor.report_manifest([], None))

    def test_a_previous_run_is_found_next_to_latest_json(self):
        with tempfile.TemporaryDirectory() as name:
            runtime = Path(name)
            manifest = runtime / "runs" / "20261001T000000Z-abcdef01" / "manifest.json"
            manifest.parent.mkdir(parents=True)
            manifest.write_text("{}", encoding="utf-8")
            (runtime / "latest.json").write_text(json.dumps({"manifest": str(manifest)}), encoding="utf-8")
            lines = []
            self.assertEqual(doctor.report_manifest(lines, runtime), manifest)
            self.assertIn("disc need not be re-verified", "\n".join(lines))

    def test_a_dangling_manifest_is_reported_not_trusted(self):
        with tempfile.TemporaryDirectory() as name:
            runtime = Path(name)
            (runtime / "latest.json").write_text(
                json.dumps({"manifest": str(runtime / "gone" / "manifest.json")}), encoding="utf-8")
            lines = []
            self.assertIsNone(doctor.report_manifest(lines, runtime))
            self.assertIn("missing manifest", "\n".join(lines))


class DoctorTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.runtime = self.directory / "runtime"
        manifest = self.runtime / "runs" / "20261001T000000Z-abcdef01" / "manifest.json"
        manifest.parent.mkdir(parents=True)
        manifest.write_text("{}", encoding="utf-8")
        (self.runtime / "latest.json").write_text(json.dumps({"manifest": str(manifest)}), encoding="utf-8")
        self.manifest = manifest
        self.assembly = self.directory / "prodg2"
        for name in doctor.ASSEMBLY_INSTRUMENTS:
            touch(self.assembly, name)
        self.compiler = self.directory / "prodg3"
        for name in doctor.C_INSTRUMENTS:
            touch(self.compiler, name)
        self.wrench = self.directory / "wrenchbuild.exe"
        self.wrench.write_bytes(b"")

    def test_an_empty_environment_still_ends_with_a_command(self):
        # No mock here on purpose: on a bare interpreter (what CI is) the honest next command
        # is the pinned install, and on a prepared one it is setup.py. Both are commands.
        code, output = gather([])
        self.assertEqual(code, 0)
        self.assertIn("build (assembly reconstruction)  not yet", output)
        self.assertIn("C candidates (byte proofs)       not yet", output)
        last = output.rstrip().splitlines()[-1].strip()
        self.assertTrue(last.startswith(("python", "pip")), f"verdict must end with a command, got: {last}")

    def test_a_prepared_environment_asks_for_the_disc_next(self):
        with mock.patch.object(doctor, "installed_version",
                               side_effect=lambda package: doctor.pinned_versions(
                                   doctor.ROOT / "requirements.txt").get(package)):
            code, output = gather([])
        self.assertEqual(code, 0)
        self.assertTrue(output.rstrip().splitlines()[-1].strip().startswith("python scripts/setup.py"))

    def test_a_complete_environment_reaches_the_candidates(self):
        with mock.patch.object(doctor, "installed_version",
                               side_effect=lambda package: doctor.pinned_versions(
                                   doctor.ROOT / "requirements.txt").get(package)):
            code, output = gather(["--toolchain", str(self.assembly), "--c-toolchain", str(self.compiler),
                                   "--wrench", str(self.wrench), "--runtime", str(self.runtime)])
        self.assertEqual(code, 0)
        self.assertIn("build (assembly reconstruction)  yes", output)
        self.assertIn("C candidates (byte proofs)       yes", output)
        self.assertIn("check_candidates.py", output.rstrip().splitlines()[-1])
        self.assertIn(str(self.manifest.parent / "reference" / "boot.elf"), output)

    def test_an_incomplete_toolchain_does_not_count_as_present(self):
        touch(self.directory / "half", "ee/bin/Ps2EeAs.exe")
        code, output = gather(["--toolchain", str(self.directory / "half"),
                               "--runtime", str(self.runtime)])
        self.assertEqual(code, 0)
        self.assertIn("is missing ee/bin/ld.exe", output)
        self.assertIn("not yet", output)

    def test_a_runtime_inside_the_repository_is_refused(self):
        code, output = gather(["--runtime", str(doctor.ROOT / "build")])
        self.assertEqual(code, 0)
        self.assertIn("choose a directory outside it", output)

    def test_a_wrong_package_version_asks_for_the_pinned_install(self):
        with mock.patch.object(doctor, "installed_version", return_value="0.0.1"):
            code, output = gather([])
        self.assertEqual(code, 0)
        self.assertIn("pip install -r requirements.txt", output.rstrip().splitlines()[-1])

    def test_a_missing_disc_image_is_reported(self):
        code, output = gather(["--iso", str(self.directory / "absent.iso")])
        self.assertEqual(code, 0)
        self.assertIn("absent.iso not found", output)

    def test_an_incomplete_checkout_exits_two(self):
        with mock.patch.object(doctor, "ROOT", self.directory):
            code, output = gather([])
        self.assertEqual(code, 2)
        self.assertIn("checkout is incomplete", output)


if __name__ == "__main__":
    unittest.main()

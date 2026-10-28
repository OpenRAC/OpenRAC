"""Compiler setup must preserve pinned inputs and whole-object selection."""
import io
import json
from pathlib import Path, PureWindowsPath
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import gcc323
import libgcc_units


class FrontendTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def archive(self, name, kind=tarfile.REGTYPE):
        path = self.root / "input.tar.bz2"
        with tarfile.open(path, "w:bz2") as archive:
            member = tarfile.TarInfo(name)
            member.type = kind
            if kind == tarfile.REGTYPE:
                member.size = 3
                archive.addfile(member, io.BytesIO(b"abc"))
            else:
                member.linkname = "../../outside"
                archive.addfile(member)
        return path

    def test_regular_pinned_source_tree(self):
        gcc323.extract_source(self.archive("gcc-3.2.3/gcc/file.c"), self.root / "source")
        self.assertEqual((self.root / "source/gcc-3.2.3/gcc/file.c").read_bytes(), b"abc")

    def test_archive_rejects_escape_and_links(self):
        for name, kind in [("../outside", tarfile.REGTYPE),
                           ("/gcc-3.2.3/outside", tarfile.REGTYPE),
                           ("gcc-3.2.3/../../outside", tarfile.REGTYPE),
                           ("gcc-3.2.3/link", tarfile.SYMTYPE),
                           ("gcc-3.2.3/link", tarfile.LNKTYPE)]:
            with self.subTest(name=name, kind=kind), self.assertRaises(ValueError):
                gcc323.extract_source(self.archive(name, kind), self.root / "source")
        self.assertFalse((self.root / "outside").exists())

    def test_corrupted_cache_is_not_used_or_redownloaded_silently(self):
        path = self.root / "cached"
        path.write_bytes(b"incorrect")
        with patch.object(gcc323.urllib.request, "urlopen") as fetch:
            with self.assertRaisesRegex(ValueError, "Checksum mismatch"):
                gcc323.fetch("https://example.invalid/source", path, "0" * 64)
            fetch.assert_not_called()

    def test_container_paths_do_not_change_macro_values(self):
        root = PureWindowsPath("D:/project")
        self.assertEqual(gcc323.container_argument(r"D:\project\build-sn\out.s", root),
                         "/work/build-sn/out.s")
        self.assertEqual(gcc323.container_argument(r"-ID:\project\include", root), "-I/work/include")
        value = r'-DLINE="a\nb"'
        self.assertEqual(gcc323.container_argument(value, root), value)
        for value in (r"D:\elsewhere\input.c", r"..\outside.c", r"D:\project\..\outside.c"):
            with self.assertRaises(ValueError):
                gcc323.container_argument(value, root)

    def test_changed_frontend_binary_is_rejected(self):
        prefix = self.root / "compiler"
        binary = prefix / "bin"
        binary.mkdir(parents=True)
        for name in ("ee-gcc", "cc1", "cpp0"):
            (binary / name).write_bytes(b"original")
        manifest = {"source_sha256": gcc323.SOURCE_SHA256, "patch_sha256": gcc323.PATCH_SHA256,
                    "binaries": {name: gcc323.digest(binary / name) for name in ("ee-gcc", "cc1", "cpp0")}}
        (prefix / "manifest.json").write_text(json.dumps(manifest))
        with patch.object(gcc323, "PREFIX", prefix):
            self.assertEqual(gcc323.native_command(["-S", "input.c"])[-2:], ["-S", "input.c"])
            (binary / "cc1").write_bytes(b"modified")
            with self.assertRaisesRegex(RuntimeError, "binary changed: cc1"):
                gcc323.native_command(["-S", "input.c"])

    def test_missing_compiler_does_not_fall_back_to_sn(self):
        with patch.object(gcc323, "PREFIX", self.root / "missing"):
            with self.assertRaisesRegex(RuntimeError, "--setup"):
                gcc323.native_command(["-S", "input.c"])

    def test_selection_is_for_the_whole_source_object(self):
        config = self.root / "config"
        config.mkdir()
        (config / "text.objects").write_text("build-sn/game/first.o 0x1000\n"
                                            "build-sn/game/fastfunc.o 0x2000 gcc323\n")
        with patch.object(libgcc_units, "ROOT", self.root):
            self.assertEqual(libgcc_units.gcc323_sources(), {"src/game/fastfunc.c"})


if __name__ == "__main__":
    unittest.main()

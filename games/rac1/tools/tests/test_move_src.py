import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import move_src  # noqa: E402


class MoveSrcTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        files = {
            "src/core/00116FA0.c": "int a;\n",
            "src/core/00116FA01.c": "int longer_name;\n",
            "src/game/hud.c": "int b;\n",
            "src/overlays/shared/old.c": "int c;\n",
            "src/versions/us/core/00116FA0/func_1.c": "int d;\n",
            "config/core_text.objects": "build-sn/core/00116FA0.o 0x1 ee29\nbuild-sn/core/00116FA01.o 0x2 ee29\n",
            "config/text.objects": "build-sn/game/hud.o 0x3\n",
            "config/file_cflags.txt": "src/core/00116FA0.c -O1\n",
            "config/versions/us/map.tsv": "func_1\tsrc/core/00116FA0.c\tmatch\n",
            "config/overlays/file_names.tsv": "src/overlays/shared/gen_1.c\told.c\twhy\n",
            "Makefile.sn": 'if [ "$*" = 00116FA0 ]; then x; fi\n',
        }
        for rel, text in files.items():
            path = self.root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        for cmd in (["init", "-q"], ["add", "-A"]):
            subprocess.run(["git", *cmd], cwd=self.root, check=True)

    def tearDown(self):
        self.tmp.cleanup()

    def read(self, rel):
        return (self.root / rel).read_text()

    def test_core_move_updates_every_reference(self):
        move_src.move(self.root, "src/core/00116FA0.c", "src/core/libc/stdio.c")
        self.assertTrue((self.root / "src/core/libc/stdio.c").exists())
        self.assertTrue((self.root / "src/versions/us/core/libc/stdio/func_1.c").exists())
        self.assertEqual(self.read("config/core_text.objects"),
                         "build-sn/core/libc/stdio.o 0x1 ee29\nbuild-sn/core/00116FA01.o 0x2 ee29\n")
        self.assertEqual(self.read("config/file_cflags.txt"), "src/core/libc/stdio.c -O1\n")
        self.assertEqual(self.read("config/versions/us/map.tsv"), "func_1\tsrc/core/libc/stdio.c\tmatch\n")
        self.assertIn('"$*" = libc/stdio ]', self.read("Makefile.sn"))
        self.assertEqual(move_src.load_aliases(self.root), {"core/libc/stdio": "core/00116FA0"})

    def test_move_into_a_folder_named_after_the_file(self):
        move_src.move(self.root, "src/core/00116FA0.c", "src/core/00116FA0/stdio.c")
        self.assertTrue((self.root / "src/versions/us/core/00116FA0/stdio/func_1.c").exists())

    def test_a_header_moves_with_its_source(self):
        (self.root / "include/core").mkdir(parents=True)
        (self.root / "include/core/00116FA0.h").write_text("#ifndef CORE_00116FA0_H\n/* src/core/00116FA0.c */\n")
        (self.root / "src/game/hud.c").write_text('#include "core/00116FA0.h"\n')
        subprocess.run(["git", "add", "-A"], cwd=self.root, check=True)
        move_src.move(self.root, "src/core/00116FA0.c", "src/core/libc/stdio.c")
        self.assertEqual(self.read("include/core/libc/stdio.h"), "#ifndef CORE_LIBC_STDIO_H\n/* src/core/libc/stdio.c */\n")
        self.assertEqual(self.read("src/game/hud.c"), '#include "core/libc/stdio.h"\n')

    def test_second_move_keeps_the_first_report_name(self):
        move_src.move(self.root, "src/core/00116FA0.c", "src/core/a.c")
        move_src.move(self.root, "src/core/a.c", "src/core/b.c")
        self.assertEqual(move_src.load_aliases(self.root), {"core/b": "core/00116FA0"})

    def test_moving_back_drops_the_alias(self):
        move_src.move(self.root, "src/core/00116FA0.c", "src/core/a.c")
        move_src.move(self.root, "src/core/a.c", "src/core/00116FA0.c")
        self.assertEqual(move_src.load_aliases(self.root), {})

    def test_overlay_rename_updates_the_names_table(self):
        move_src.move(self.root, "src/overlays/shared/old.c", "src/overlays/shared/new.c")
        self.assertEqual(self.read("config/overlays/file_names.tsv"), "src/overlays/shared/gen_1.c\tnew.c\twhy\n")
        self.assertEqual(move_src.load_aliases(self.root), {})

    def test_refused_moves(self):
        for old, new in [("src/core/00116FA0.c", "src/game/x.c"),
                         ("src/game/hud.c", "src/core/hud.c"),
                         ("src/overlays/shared/old.c", "src/overlays/shared/sub/old.c"),
                         ("src/core/missing.c", "src/core/x.c"),
                         ("src/game/hud.c", "src/core/00116FA0.c")]:
            with self.subTest(old=old, new=new):
                self.assertIsNotNone(move_src.check_move(self.root, old, new))


if __name__ == "__main__":
    unittest.main()

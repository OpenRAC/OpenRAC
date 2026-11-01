"""Keep private experiment directories out without rejecting retained C variants."""
from pathlib import Path, PurePosixPath, PureWindowsPath
from contextlib import redirect_stdout
from io import StringIO
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import MagicMock, patch

import check_repository


class ScratchHygieneTests(unittest.TestCase):
    def test_source_ownership_accepts_native_windows_and_posix_paths(self):
        for relative in (PureWindowsPath("src/game/example.c"),
                         PurePosixPath("src/game/example.c")):
            with self.subTest(relative=relative):
                source = MagicMock()
                source.relative_to.return_value = relative
                source.read_text.return_value = (
                    "int func_00100000(void)\n{\n    return 0;\n}\n"
                    'INCLUDE_ASM("asm/game", func_00100004);\n'
                )
                root = MagicMock()
                (root / "src").rglob.return_value = [source]
                errors = []
                with patch.object(check_repository, "ROOT", root), redirect_stdout(StringIO()):
                    check_repository.check_sources(
                        {"us": {"func_00100000", "func_00100004"}}, errors
                    )
                self.assertEqual(errors, [])

    def test_empty_source_barriers_are_rejected(self):
        for text in ("do { } while (0);", "do { /* schedule */ ; }\nwhile (0U);"):
            errors = []
            check_repository.check_empty_barriers(text, "src/example.c", errors)
            self.assertEqual(len(errors), 1)

    def test_nonempty_macro_blocks_and_literal_mentions_are_allowed(self):
        for text in ("do { high = value.hi; low = value.lo; } while (0);",
                     '/* do { } while (0) */ const char *s = "do {} while (0)";',
                     '// do {} while (0)\nreturn 0;'):
            errors = []
            check_repository.check_empty_barriers(text, "src/example.c", errors)
            self.assertEqual(errors, [])

    def check_paths(self, paths):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for path in paths:
                target = root / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text("/* authored source fixture */\n", encoding="utf-8")
            with patch.object(check_repository, "ROOT", root), patch(
                "subprocess.run", return_value=SimpleNamespace(stdout="\n".join(paths))
            ):
                errors = []
                check_repository.check_hygiene(errors)
                return errors

    def test_tracked_scratch_is_rejected_even_if_gitignore_would_hide_it(self):
        paths = ["scratch/check-many.sh", "nonmatching/levels/_scratch/screen.txt",
                 "nonmatching/levels/_scratch2/var/body.c"]
        errors = self.check_paths(paths)
        self.assertEqual(len(errors), len(paths))
        for path in paths:
            self.assertTrue(any(path in error and "stay local" in error for error in errors))

    def test_catalogue_named_retained_variants_are_allowed(self):
        self.assertEqual(self.check_paths([
            "nonmatching/levels/func_L00_002D3FB8.c",
            "nonmatching/levels/alternatives/func_L00_002D3FB8-form-01.c",
        ]), [])

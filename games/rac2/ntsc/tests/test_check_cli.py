"""A non-matching or partly matching check must fail command-line automation."""
import contextlib
import io
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import check as function_check


class CheckExitStatus(unittest.TestCase):
    def run_check(self, sizes, differences):
        name = "func_L00_00100000"
        root = function_check.ROOT
        source = root / "src" / "example.c"
        obj = {"symbols": [{"name": name, "type": 2, "section": 1}],
               "sections": [{"size": 0}, {"size": 44}]}
        places = [(number, 0x100000 + number * 0x100) for number in range(len(sizes))]
        placements = [SimpleNamespace(functions={name: [(address, size)]},
                                      derive=Mock(return_value=([], why)))
                      for (_, address), size, why in zip(places, sizes, differences)]
        output = io.StringIO()
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(sys, "argv", ["check.py", name]))
            stack.enter_context(patch.object(function_check.levels, "version_argument"))
            stack.enter_context(patch.object(function_check, "source_of", return_value=source))
            stack.enter_context(patch.object(function_check.build, "compile_c", return_value=Path("unused.o")))
            stack.enter_context(patch.object(function_check.elf, "read_object", return_value=obj))
            stack.enter_context(patch.object(function_check.elf, "read", return_value={}))
            stack.enter_context(patch.object(function_check.levels, "read_boot_functions", return_value=[]))
            stack.enter_context(patch.object(function_check.levels, "read_catalogue",
                                             return_value=[(name, "shared", 44, "", places)]))
            stack.enter_context(patch.object(function_check.build, "level_reference", return_value=Path("unused.elf")))
            stack.enter_context(patch.object(function_check.build, "Placement", side_effect=placements))
            stack.enter_context(contextlib.redirect_stdout(output))
            result = function_check.main()
        return result, output.getvalue()

    def test_all_placements_match(self):
        status, output = self.run_check([44, 44], [None, None])
        self.assertEqual(status, 0)
        self.assertIn("2 of 2 places match", output)

    def test_partial_instruction_match_fails(self):
        status, output = self.run_check([44, 44], [None, "instruction differs"])
        self.assertEqual(status, 1)
        self.assertIn("1 of 2 places match", output)

    def test_equal_size_is_not_a_match(self):
        status, output = self.run_check([44], ["relocated opcode differs"])
        self.assertEqual(status, 1)
        self.assertIn("0 of 1 places match", output)

    def test_size_mismatch_fails(self):
        status, output = self.run_check([48], [None])
        self.assertEqual(status, 1)
        self.assertIn("44 bytes, expected 48", output)
        self.assertIn("0 of 1 places match", output)


if __name__ == "__main__":
    unittest.main()

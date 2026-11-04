import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tidy"))
import data_owners as owners  # noqa: E402

FILES = {"func_1": "src/game/a.c", "func_2": "src/game/a.c", "func_3": "src/game/b.c"}


class OwnerTest(unittest.TestCase):
    def test_ghidra_names_are_spelt_like_ours(self):
        self.assertEqual(owners.normal_name("FUN_001138a8"), "func_001138A8")
        self.assertEqual(owners.normal_name("main"), "main")

    def test_one_file_owns_it(self):
        self.assertEqual(owners.owner_of(["func_1", "func_2"], FILES), "src/game/a.c")

    def test_two_files_do_not(self):
        self.assertIsNone(owners.owner_of(["func_1", "func_3"], FILES))

    def test_a_function_with_no_file_is_not_evidence(self):
        self.assertIsNone(owners.owner_of(["func_1", "func_9"], FILES))


if __name__ == "__main__":
    unittest.main()

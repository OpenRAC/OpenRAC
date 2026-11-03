import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_regressions as cr  # noqa: E402
import check_syntax as cs  # noqa: E402
import gate_loop as gl  # noqa: E402

MAP = "# header\nfunc_1\tsrc/core/a.c\tmatch\tx\nfunc_2\tsrc/core/b.c\tdiffers\tx\n"


def report(percent, functions=None):
    return json.dumps({"measures": {"fuzzy_match_percent": percent},
                       "units": [{"name": "u", "functions": functions or []}]})


class RegressionsTest(unittest.TestCase):
    def test_a_lower_number_is_a_drop(self):
        self.assertEqual(cr.report_drops(report(100.0), report(99.0)), (100.0, 99.0))
        self.assertIsNone(cr.report_drops(report(99.0), report(100.0)))

    def test_a_moved_file_is_not_a_regression(self):
        moved = MAP.replace("src/core/a.c", "src/core/libc/a.c")
        self.assertEqual(cr.map_regressions(MAP, moved), [])

    def test_a_lost_match_is_a_regression(self):
        self.assertEqual(cr.map_regressions(MAP, MAP.replace("a.c\tmatch", "a.c\tdiffers")), ["func_1"])

    def test_a_removed_function_is_a_regression(self):
        self.assertEqual(cr.map_regressions(MAP, "func_2\tsrc/core/b.c\tdiffers\tx\n"), ["func_1"])

    def test_a_new_match_is_fine(self):
        self.assertEqual(cr.map_regressions(MAP, MAP.replace("b.c\tdiffers", "b.c\tmatch")), [])


class GateLoopTest(unittest.TestCase):
    def test_a_fragment_blames_its_shared_file(self):
        self.assertEqual(gl.shared_file("src/versions/jp/game/ui/pause/func_1.c"), "src/game/ui/pause.c")
        self.assertEqual(gl.shared_file("src/game/ui/pause.c"), "src/game/ui/pause.c")

    def test_lost_functions(self):
        old = report(100.0, [{"name": "a", "fuzzy_match_percent": 100.0}, {"name": "b", "fuzzy_match_percent": 100.0}])
        new = report(99.0, [{"name": "a", "fuzzy_match_percent": 100.0}, {"name": "b", "fuzzy_match_percent": 0.0}])
        self.assertEqual(gl.lost_functions(old, new), ["b"])

    def test_version_losses_ignore_a_move(self):
        moved = MAP.replace("src/core/a.c", "src/core/libc/a.c")
        self.assertEqual(gl.version_losses(MAP, moved), set())
        self.assertEqual(gl.version_losses(MAP, MAP.replace("a.c\tmatch", "a.c\tdiffers")), {"src/core/a.c"})

    def test_compile_failures(self):
        log = ("src/overlays/shared/x.c:12: structure has no member\n"
               "src/overlays/shared/y.c:3: warning: something\n"
               "build-sn/versions/jp/src/src/game/ui/pause/src.c:4: bad\n"
               "build-sn/overlays/report/l07_umbris/snagglebeast/src.c:9: bad\n")
        self.assertEqual(gl.compile_failures(log), {"src/overlays/shared/x.c", "src/game/ui/pause.c",
                                                    "src/overlays/l07_umbris/snagglebeast.c"})


class SyntaxKeyTest(unittest.TestCase):
    def test_the_key_names_the_symbol_and_drops_the_place(self):
        line = "/tmp/x.c:12:3: error: redeclaration of 'D_1' with a different type: 'char[]' vs 'char'"
        self.assertEqual(cs.error_key(line), "redeclaration of 'D_1'")

    def test_a_message_with_no_name_is_kept_whole(self):
        self.assertEqual(cs.error_key("x.c:1:1: error: cannot apply asm label to variable after its first use"),
                         "cannot apply asm label to variable after its first use")

    def test_a_new_error_shows_even_when_the_count_is_the_same(self):
        baseline = {"a.c": ["conflicting types for 'f'"]}
        found = {"a.c": ["x.c:1:1: error: no member named 'q' in 'struct S'"]}
        self.assertEqual(cs.new_errors(found, baseline), {"a.c": ["no member named 'q'"]})

    def test_a_known_error_is_not_new(self):
        baseline = {"a.c": ["conflicting types for 'f'"]}
        self.assertEqual(cs.new_errors({"a.c": ["x.c:9:9: error: conflicting types for 'f'"]}, baseline), {})

    def test_two_of_one_error_need_two_rows(self):
        baseline = {"a.c": ["cannot apply asm label"]}
        found = {"a.c": ["x:1:1: error: cannot apply asm label", "x:2:1: error: cannot apply asm label"]}
        self.assertEqual(cs.new_errors(found, baseline), {"a.c": ["cannot apply asm label"]})


if __name__ == "__main__":
    unittest.main()

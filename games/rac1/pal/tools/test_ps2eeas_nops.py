"""Regression checks for shared FP labels and short-loop hazard padding."""
import unittest

from ps2eeas_nops import fp_label_nop, implicit_loop_compare_nop


class SharedFpLabelTests(unittest.TestCase):
    # c.lt.s $f0,$f20; nop; bc1tl ...
    text = b"".join(w.to_bytes(4, "little") for w in (0x4614003C, 0, 0x45030002))

    def check(self, source):
        return fp_label_nop(source.splitlines(True), 0,
                            len(source.splitlines()) - 1, self.text, 8)

    def test_implicit_nop_after_shared_label(self):
        self.assertTrue(self.check("c.lt.s $f0,$f20\n$L174:\n.set noreorder\n"
                                   ".set nomacro\nbc1tl $L152\n"))

    def test_explicit_nop_is_already_correct(self):
        self.assertFalse(self.check("c.lt.s $f0,$f20\n$L174:\nnop\nbc1tl $L152\n"))

    def test_comparison_without_shared_label_is_unchanged(self):
        self.assertFalse(self.check("$L174:\nc.lt.s $f0,$f20\nbc1tl $L152\n"))

    def test_label_after_another_instruction_is_unchanged(self):
        self.assertFalse(self.check("c.lt.s $f0,$f20\nmov.s $f1,$f0\n"
                                    "$L174:\nbc1tl $L152\n"))

    def test_missing_object_nop_is_not_replaced(self):
        lines = ["c.lt.s $f0,$f20\n", "$L174:\n", "bc1tl $L152\n"]
        self.assertFalse(fp_label_nop(lines, 0, 2, self.text, 4))


class LoopCompareNopTests(unittest.TestCase):
    text = b"".join(w.to_bytes(4, "little") for w in (0x4614003C, 0, 0x4503FFFD))

    def check(self, source):
        lines = source.splitlines(True)
        return implicit_loop_compare_nop(self.text, 0, 8, lines, len(lines) - 1)

    def test_implicit_loop_hazard_must_be_preserved(self):
        self.assertTrue(self.check("$L1:\nc.lt.s $f0,$f20\n.set noreorder\n"
                                   ".set nomacro\nbc1tl $L1\n"))

    def test_written_nop_is_not_counted_twice(self):
        self.assertFalse(self.check("$L1:\nc.lt.s $f0,$f20\nnop\nbc1tl $L1\n"))

    def test_other_source_instruction_does_not_lose_hazard(self):
        self.assertFalse(self.check("$L1:\nc.lt.s $f0,$f20\nmov.s $f1,$f0\n"
                                    "bc1tl $L1\n"))


if __name__ == "__main__":
    unittest.main()

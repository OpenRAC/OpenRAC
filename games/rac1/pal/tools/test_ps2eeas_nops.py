"""Regression checks for the shared FP hazard label in func_L00_002C0358."""
import unittest

from ps2eeas_nops import fp_label_nop


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


if __name__ == "__main__":
    unittest.main()

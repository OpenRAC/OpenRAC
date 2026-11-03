import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import data_in_c as d  # noqa: E402

WORDS = """
/* 0 165580 00000001 */ .word 0x00000001
/* 4 165584 00000002 */ .word 0x00000002
"""
POINTER = "/* 0 165580 00000001 */ .word func_00112380\n"
OFFSET = "/* 0 165580 00000001 */ .word D_00170000 + 0x3\n"
LABEL = "/* 0 165580 00000001 */ .word .L00112380\n"
MIXED = "/* 0 165580 00000001 */ .word 0x1\n/* 4 165584 00 */ .byte 0x1\n"


class ValuesTest(unittest.TestCase):
    def test_plain_words(self):
        self.assertEqual(d.parse(WORDS), ("u32", ["0x00000001", "0x00000002"]))

    def test_a_mix_or_a_local_label_is_not_plain(self):
        self.assertIsNone(d.parse_plain(MIXED))
        self.assertIsNone(d.parse_plain(LABEL))

    def test_pointers(self):
        self.assertEqual(d.parse(POINTER), ("u32", ["(u32)ref_func_00112380"]))
        self.assertEqual(d.parse(OFFSET), ("u32", ["(u32)ref_D_00170000 + 0x3"]))

    def test_references_are_aliased_and_listed_once(self):
        text = d.references(["(u32)ref_func_1", "(u32)ref_func_1 + 0x4", "0x5"])
        self.assertEqual(text, 'extern char ref_func_1[] __asm__("func_1");\n')

    def test_a_float_keeps_its_exact_bits(self):
        # The disassembly's raw column is in file order: 1.0f is 00 00 80 3F on this little-endian CPU.
        self.assertEqual(d.parse("/* 0 00165580 0000803F */ .float 1\n"), ("u32", ["0x3F800000"]))

    def test_shorts_bytes_and_a_string(self):
        self.assertEqual(d.parse("/* 0 00165580 */ .short 0x0003\n"), ("u16", ["0x0003"]))
        self.assertEqual(d.parse("/* 0 00165580 */ .byte 0x01\n"), ("u8", ["0x01"]))
        self.assertEqual(d.parse('/* 0 00165580 */ .asciz "Su"\n.align 2\n'), ("char", ['"Su"']))

    def test_two_strings_are_not_one_block(self):
        self.assertIsNone(d.parse_plain('.asciz "a"\n.asciz "b"\n'))

    def test_render(self):
        self.assertEqual(d.render(["0x1"]), "0x1\n")
        self.assertEqual(d.render(["0x1", "0x2"]), "0x1, 0x2,\n")


class FitsRetailTest(unittest.TestCase):
    def test_gcc_aligns_aggregates_to_8(self):
        self.assertTrue(d.fits_retail("D_00165580", 4))
        self.assertTrue(d.fits_retail("D_00165584", 1))
        self.assertFalse(d.fits_retail("D_00165584", 2))
        self.assertFalse(d.fits_retail("D_00165584", 1, "char"))
        self.assertTrue(d.fits_retail("D_00165580", 1, "char"))


class NextPieceTest(unittest.TestCase):
    def test_the_next_block_must_be_on_4(self):
        blocks = {"D_00199810": "", "D_00199812": "", "D_00199818": ""}
        self.assertFalse(d.next_piece_fits("D_00199810", blocks))
        self.assertTrue(d.next_piece_fits("D_00199812", blocks))
        self.assertTrue(d.next_piece_fits("D_00199818", blocks))


class AddTest(unittest.TestCase):
    def test_a_jump_table_is_refused(self):
        with patch.object(d, "all_blocks", return_value={"jtbl_00165580": WORDS}), patch.object(d, "readable_names", return_value={}):
            with patch.object(d, "rows", return_value=[]), self.assertRaises(SystemExit):
                d.add(["jtbl_00165580:shared"])


class SourceTest(unittest.TestCase):
    def write(self, entries, names=None):
        with tempfile.TemporaryDirectory() as folder, patch.object(d, "ROOT", Path(folder)):
            d.write_source("src/data/game/x.c", entries, names or {})
            return (Path(folder) / "src/data/game/x.c").read_text()

    def test_no_values_in_the_source(self):
        text = self.write([("D_00165580", "u32", 2, False), ("D_00165590", "u32", 1, False)])
        self.assertIn('u32 D_00165580[] DATA_SECTION(D_00165580) = {\n#include "data_inc/D_00165580.inc"\n};', text)
        self.assertIn("u32 D_00165590 DATA_SECTION(D_00165590) =", text)
        self.assertNotIn("0x", text)

    def test_a_readable_name_is_used_and_the_section_keeps_the_address(self):
        text = self.write([("D_00165580", "u32", 2, False)], {"D_00165580": "fog_table"})
        self.assertIn("u32 fog_table[] DATA_SECTION(D_00165580) = {", text)

    def test_a_string(self):
        text = self.write([("D_00165580", "char", 1, False)])
        self.assertIn('char D_00165580[] DATA_SECTION(D_00165580) =\n#include "data_inc/D_00165580.inc"\n;', text)

    def test_a_pointer_block_includes_its_references_first(self):
        text = self.write([("D_00165580", "u32", 2, True)])
        self.assertLess(text.index('"data_inc/D_00165580.refs"'), text.index("u32 D_00165580[]"))

    def test_segments(self):
        self.assertEqual(d.segment_of("D_0012F580"), "core")
        self.assertEqual(d.segment_of("D_00165580"), "data")


class TypedTest(unittest.TestCase):
    FIELDS = [(0, "i", 4), (4, "f", 4), (8, "p", 4)]

    def test_each_field_gets_its_form(self):
        out = d.render_typed(["0x7", "0x3F800000", "(u32)ref_D_1"], self.FIELDS, 12)
        self.assertEqual(out[0], "0x7")
        self.assertEqual(float(out[1][:-1]), 1.0)
        self.assertEqual(out[2], "(void *)ref_D_1")

    def test_records_repeat(self):
        self.assertEqual(len(d.render_typed(["0"] * 6, self.FIELDS, 12)), 6)

    def test_narrow_fields_split_a_word(self):
        self.assertEqual(d.render_typed(["0x00020001"], [(0, "i", 2), (2, "i", 2)], 4), ["0x1", "0x2"])

    def test_a_wrong_size_is_refused(self):
        with self.assertRaises(ValueError):
            d.render_typed(["0"] * 5, self.FIELDS, 12)

    def test_zero_alignment_after_the_records_is_allowed(self):
        self.assertEqual(len(d.render_typed(["0"] * 4, self.FIELDS, 12)), 3)
        with self.assertRaises(ValueError):
            d.render_typed(["0", "0", "0", "0x1"], self.FIELDS, 12)

    def test_a_symbol_in_a_float_is_refused(self):
        with self.assertRaises(ValueError):
            d.render_typed(["0", "(u32)ref_D_1", "0"], self.FIELDS, 12)

    def test_non_zero_padding_is_refused(self):
        with self.assertRaises(ValueError):
            d.render_typed(["0x7", "0x9"], [(0, "i", 4)], 8)

    def test_is_zero(self):
        self.assertTrue(d.is_zero(["0", "0x0"]))
        self.assertFalse(d.is_zero(["0", "0x1"]))
        self.assertFalse(d.is_zero(["(u32)ref_D_1"]))


class WholeRecordTest(unittest.TestCase):
    def test_a_label_not_on_8_is_inside_the_block_before_it(self):
        labels = ["D_00000010", "D_00000014", "D_00000018", "jtbl_00000020", "D_00000024"]
        self.assertEqual(d.group_labels(labels, {"D_00000010", "D_00000018"}),
                         {"D_00000010": ["D_00000014"], "D_00000018": []})

    def test_mixed_data_becomes_words(self):
        body = ('.at 0x10\n.word 0x1\n.byte 2, 3\n.align 2\n.at 0x18\n.asciz "ab"\n.align 2\n.word D_00000001\n')
        self.assertEqual(d.parse(body)[1], ["0x00000001", "0x00000302", "0x00006261", "(u32)ref_D_00000001"])

    def test_a_block_that_ends_inside_a_word_is_bytes(self):
        self.assertEqual(d.parse_mixed(".at 0x10\n.word 0x1\n.byte 2\n"), ("u8", ["0x01", "0x00", "0x00", "0x00", "0x02"]))

    def test_a_symbol_off_a_word_is_refused(self):
        self.assertIsNone(d.parse_mixed(".at 0x10\n.byte 1\n.word D_00000001\n"))

    def test_a_line_it_does_not_know_is_refused(self):
        self.assertIsNone(d.parse_mixed(".at 0x10\n.quad 1\n"))


class ScalarTypeTest(unittest.TestCase):
    def test_a_block_not_on_8_is_one_record(self):
        self.assertTrue(d.alignment_allows("D_00000008", 2, 2))
        self.assertTrue(d.alignment_allows("D_00000004", 1, 4))
        self.assertFalse(d.alignment_allows("D_00000004", 1, 2))

    def test_a_scalar_type_splits_words(self):
        self.assertEqual(d.render_typed(["0x00020001"], [(0, "i", 2)], 2), ["0x1", "0x2"])

    def test_a_float_type_prints_floats(self):
        self.assertEqual(float(d.render_typed(["0x3F800000"], [(0, "f", 4)], 4)[0][:-1]), 1.0)


class SegmentTest(unittest.TestCase):
    def test_a_label_finds_its_segment(self):
        self.assertEqual(d.segment_of("D_0012F580"), "core")
        self.assertEqual(d.segment_of("D_00152300"), "core_rdata")
        self.assertEqual(d.segment_of("D_0015ED80"), "core_lit")
        self.assertEqual(d.segment_of("D_0015F000"), "lit")
        self.assertEqual(d.segment_of("D_00165580"), "data")
        self.assertEqual(d.segment_of("D_001E8F00"), "lvl_vtbl")

    def test_blocks_do_not_cross_segments(self):
        self.assertEqual(d.group_labels(["D_0015EFF0", "D_0015F004"], {"D_0015EFF0"}), {"D_0015EFF0": []})

    def test_an_owned_label_ends_a_block(self):
        labels = ["D_00152300", "D_00152304", "D_00152308"]
        self.assertEqual(d.group_labels(labels, {"D_00152300"}, {"D_00152304"}), {"D_00152300": []})

    def test_a_single_number_can_start_a_block(self):
        self.assertTrue(d.starts_block("D_00000004", ".word 0x1\n", True))
        self.assertFalse(d.starts_block("D_00000004", ".word 0x1\n", False))
        self.assertFalse(d.starts_block("D_00000004", ".word 0x1, 0x2\n", True))
        self.assertTrue(d.starts_block("D_00000008", ".word 0x1, 0x2\n", False))


class WriteOutTest(unittest.TestCase):
    NAMES = {"func_1": "handler", "D_2": "next_table"}

    def test_a_table_is_only_symbols_and_zeros(self):
        self.assertTrue(d.is_table(["(u32)ref_func_1", "0", "0x0"]))
        self.assertFalse(d.is_table(["(u32)ref_func_1", "7"]))
        self.assertFalse(d.is_table(["0", "0"]))

    def test_table_text(self):
        text = d.table_text("D_9", ["(u32)ref_func_1", "0"], self.NAMES, {})
        self.assertEqual(text, "void *D_9[] DATA_SECTION(D_9) = {\n    handler,\n    0,\n};\n\n")

    def test_one_value_is_a_scalar(self):
        self.assertEqual(d.table_text("D_9", ["(u32)ref_D_2"], self.NAMES, {}), "void *D_9 DATA_SECTION(D_9) = next_table;\n\n")

    def test_an_offset_is_kept(self):
        self.assertEqual(d.table_values(["(u32)ref_D_2 + 0x10"], self.NAMES, {}), ["(char *)next_table + 0x10"])

    def test_declarations(self):
        self.assertEqual(d.table_declarations(["(u32)ref_func_1", "(u32)ref_D_2"], self.NAMES, {}),
                         ["extern char next_table[];\n", "extern void handler();\n"])

    def test_a_block_in_the_same_file_keeps_its_type(self):
        here = {"next_table": ("u32", 4)}
        self.assertEqual(d.table_declarations(["(u32)ref_D_2"], self.NAMES, here), ["extern u32 next_table[];\n"])
        self.assertEqual(d.table_values(["(u32)ref_D_2"], self.NAMES, {"next_table": ("u32", 1)}), ["&next_table"])

    def test_a_float_block_with_a_place_marker(self):
        body = ".at 0x10\n/* 0 0 0000803F */ .float 1.0\n"
        self.assertTrue(d.is_float_block(body, ["0x3F800000"]))

    def test_float_literals_read_back(self):
        self.assertEqual(float(d.float_literal("0x3F800000")[:-1]), 1.0)
        self.assertIsNone(d.float_literal("0x7F800000"))
        self.assertIsNone(d.float_literal("0x00000001"))
        self.assertIsNone(d.float_literal("0x80000000"))


if __name__ == "__main__":
    unittest.main()

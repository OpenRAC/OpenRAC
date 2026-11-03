"""Compiler table placement controls using synthetic objects and retail data."""
from copy import deepcopy
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import build


def words(*values):
    return struct.pack("<" + "I" * len(values), *values)


class JumpTableValidation(unittest.TestCase):
    def setUp(self):
        self.path = Path("synthetic.o")
        self.obj = {
            "sections": [
                {"name": ".rodata", "type": 1, "flags": 2, "align": 4,
                 "size": 12, "data": words(0, 8, 8)},
                {"name": ".text.example", "type": 1, "flags": 6, "align": 4,
                 "size": 16, "data": bytes(16)},
            ],
            "symbols": [{"name": "", "type": 3, "section": 1}],
            "relocations": {0: [(offset, build.elf.R_MIPS_32, 0) for offset in (0, 4, 8)]},
        }
        self.image = {
            "data": words(0x2000, 0x2008, 0x2008),
            "segments": [{"type": 1, "address": 0x1000, "offset": 0, "filesz": 12}],
            "sections": [{"name": ".data", "type": 1, "flags": 3,
                          "address": 0x1000, "size": 12}],
        }
        self.where = {(self.path, ".text.example"): 0x2000}

    def accepts(self, obj=None, address=0x1000):
        with patch.object(build, "program_functions", return_value={}):
            placement = build.Placement("boot", self.image)
        return placement.jump_table(obj or self.obj, 0, address, self.path, self.where)

    def test_complete_table_and_repeated_case_are_accepted(self):
        self.assertTrue(self.accepts())

    def test_wrong_destination_or_section_base_is_rejected(self):
        self.image["data"] = words(0x2000, 0x200C, 0x2008)
        self.assertFalse(self.accepts())
        self.image["data"] = words(0x3000, 0x3008, 0x3008)
        self.assertFalse(self.accepts())

    def test_case_offset_must_be_aligned_and_inside_text(self):
        for offset in (2, 16, 0xFFFFFFFC):
            with self.subTest(offset=offset):
                self.obj["sections"][0]["data"] = words(offset, 8, 8)
                self.image["data"] = words((0x2000 + offset) & 0xFFFFFFFF, 0x2008, 0x2008)
                self.assertFalse(self.accepts())

    def test_every_word_needs_exactly_one_r32_section_relocation(self):
        good = deepcopy(self.obj["relocations"][0])
        for relocs in (good[:-1], good + [good[0]], [(0, 2, 0), (2, 2, 0), (8, 2, 0)],
                       [(0, build.elf.R_MIPS_26, 0)] + good[1:]):
            with self.subTest(relocs=relocs):
                self.obj["relocations"][0] = relocs
                self.assertFalse(self.accepts())

    def two_tables(self, padding=0, second=16):
        """Two tables of one function in one section: three entries, then the compiler's
        `.align 4` zero words, then a second table at offset SECOND."""
        entries = [0, 8, 8]
        data = words(*entries) + words(*([padding] * ((second - 12) // 4))) + words(4, 12)
        self.obj["sections"][0].update(size=len(data), data=data, align=16)
        self.obj["relocations"][0] = [(o, build.elf.R_MIPS_32, 0) for o in (0, 4, 8, second, second + 4)]
        retail = words(0x2000, 0x2008, 0x2008) + words(*([padding] * ((second - 12) // 4))) + words(0x2004, 0x200C)
        self.image["data"] = retail
        self.image["segments"][0]["filesz"] = len(retail)
        self.image["sections"][0]["size"] = len(retail)

    def test_alignment_padding_between_two_tables_is_accepted(self):
        self.two_tables()
        self.assertTrue(self.accepts())

    def test_padding_must_be_zero_and_end_at_an_aligned_table(self):
        for padding, second in ((1, 16), (0, 20), (0, 32)):
            with self.subTest(padding=padding, second=second):
                self.setUp()
                self.two_tables(padding, second)
                self.assertFalse(self.accepts())

    def test_named_symbol_cannot_substitute_for_case_section(self):
        self.obj["symbols"][0].update(name="D_00002000", type=1)
        self.assertFalse(self.accepts())

    def test_unplaced_or_other_object_text_is_rejected(self):
        self.where = {(Path("other.o"), ".text.example"): 0x2000}
        self.assertFalse(self.accepts())
        self.where = {}
        self.assertFalse(self.accepts())

    def test_only_allocated_readonly_whole_word_progbits_are_accepted(self):
        for field, value in (("type", 8), ("flags", 0), ("flags", 3), ("flags", 6),
                             ("size", 0), ("size", 10), ("align", 1), ("align", 6),
                             ("align", 0x2000)):
            with self.subTest(field=field, value=value):
                obj = deepcopy(self.obj)
                obj["sections"][0][field] = value
                self.assertFalse(self.accepts(obj))
        self.assertFalse(self.accepts(address=0x1002))

    def test_table_must_fit_one_retail_data_section(self):
        for size in (0, 8):
            with self.subTest(size=size):
                self.image["sections"][0]["size"] = size
                self.assertFalse(self.accepts())
        self.image["sections"][0]["size"] = 12
        self.image["sections"].append(deepcopy(self.image["sections"][0]))
        self.assertFalse(self.accepts())

    def test_table_must_fit_actual_segment_even_if_section_is_larger(self):
        self.image["segments"][0]["filesz"] = 8
        self.assertFalse(self.accepts())

    def test_section_size_must_equal_its_data_length(self):
        for size in (8, 16):
            with self.subTest(size=size):
                self.obj["sections"][0]["size"] = size
                self.assertFalse(self.accepts())

    def test_retail_text_is_not_a_data_owner(self):
        for name in (".text", "core.text"):
            with self.subTest(name=name):
                self.image["sections"][0]["name"] = name
                self.assertFalse(self.accepts())

    def test_scalar_pool_without_case_relocations_is_rejected(self):
        self.obj["relocations"][0] = []
        self.image["data"] = self.obj["sections"][0]["data"]
        self.assertFalse(self.accepts())

    def test_one_section_can_hold_tables_for_two_placed_functions(self):
        self.obj["sections"].append({"name": ".text.second", "type": 1, "flags": 6,
                                     "size": 16, "data": bytes(16)})
        self.obj["symbols"].append({"name": "", "type": 3, "section": 2})
        self.obj["relocations"][0][-1] = (8, build.elf.R_MIPS_32, 1)
        self.image["data"] = words(0x2000, 0x2008, 0x3008)
        self.where[self.path, ".text.second"] = 0x3000
        self.assertTrue(self.accepts())
        del self.where[self.path, ".text.second"]
        self.assertFalse(self.accepts(), "a partial table cannot survive its target's withdrawal")

    def test_placed_nontext_target_is_rejected(self):
        self.obj["sections"][1]["name"] = ".data"
        self.where = {(self.path, ".data"): 0x2000}
        self.assertFalse(self.accepts())


def data_word(address, expression="0"):
    return f"/* 000000 {address:08X} 00000000 */ .word {expression}\n"


class SplitTableData(unittest.TestCase):
    def test_preserves_surrounding_words_and_internal_aliases(self):
        before = ".section .data\n" + data_word(0xFFC, "7")
        after = "after:\n" + data_word(0x100C, "9")
        text = before + ".globl table\ntable:\n" + data_word(0x1000)
        text += "case_alias:\n" + data_word(0x1004) + data_word(0x1008) + after
        pieces, aliases = build.split_table_data(text, {0x1000: (12, "input(.rodata)")})
        self.assertEqual(pieces, [("asm", before), ("c", "input(.rodata)"), ("asm", after)])
        self.assertEqual(aliases, {"table": "0x1000", "case_alias": "0x1004"})

    def test_adjacent_tables_keep_their_own_inputs(self):
        text = ".section .data\n" + data_word(0x1000) + data_word(0x1004)
        pieces, _ = build.split_table_data(text, {0x1000: (4, "first"), 0x1004: (4, "second")})
        self.assertEqual([piece for piece in pieces if piece[0] == "c"],
                         [("c", "first"), ("c", "second")])

    def test_incomplete_missing_and_gapped_spans_fail(self):
        cases = (data_word(0x1000), data_word(0x1004),
                 data_word(0x1000) + data_word(0x1008))
        for text in cases:
            with self.subTest(text=text), self.assertRaises(SystemExit):
                build.split_table_data(text, {0x1000: (8, "input")})

    def test_nonword_and_intervening_directives_fail(self):
        for text in (data_word(0x1000).replace(".word", ".byte"),
                     data_word(0x1000) + ".align 4\n" + data_word(0x1004)):
            with self.subTest(text=text), self.assertRaises(SystemExit):
                build.split_table_data(text, {0x1000: (8 if ".align" in text else 4, "input")})

    def test_unmarked_data_before_table_remains_assembly(self):
        before = '.section .data\nprevious: .word 0\n'
        pieces, aliases = build.split_table_data(before + 'table:\n' + data_word(0x1000),
                                                 {0x1000: (4, "input")})
        self.assertEqual(pieces, [("asm", before), ("c", "input")])
        self.assertEqual(aliases, {"table": "0x1000"})

    def test_strings_alignment_and_two_field_comments_survive_boundaries(self):
        for padding in ("", ".align 2\n"):
            with self.subTest(padding=padding):
                before = '.section .data\nmessage:\n/* 000000 00000FF8 */ .asciz "format"\n' + padding
                after = '.align 2\nnext_message:\n/* 000020 00001008 */ .asciz "next"\n'
                text = before + '/* table begins */\n.globl table\ntable:\n'
                text += data_word(0x1000) + data_word(0x1004) + after
                pieces, aliases = build.split_table_data(text, {0x1000: (8, "input")})
                self.assertEqual(pieces, [("asm", before), ("c", "input"), ("asm", after)])
                self.assertEqual(aliases, {"table": "0x1000"})

    def test_two_field_string_comment_inside_table_is_rejected(self):
        text = data_word(0x1000) + '/* 000004 00001004 */ .asciz "bad"\n' + data_word(0x1004)
        with self.assertRaisesRegex(SystemExit, "unsupported directive"):
            build.split_table_data(text, {0x1000: (8, "input")})

    def test_invalid_span_extents_fail_before_scanning(self):
        for address, size in ((0x1002, 4), (0x1000, 0), (0x1000, -4), (0x1000, 6)):
            with self.subTest(address=address, size=size), self.assertRaisesRegex(SystemExit, "invalid"):
                build.split_table_data(data_word(0x1000), {address: (size, "input")})

    def test_multiple_words_or_statements_on_one_marked_line_fail(self):
        for expression in ("0, 0", "0; .word 0"):
            with self.subTest(expression=expression), self.assertRaisesRegex(SystemExit, "whole-word"):
                build.split_table_data(data_word(0x1000, expression), {0x1000: (4, "input")})

    def test_overlapping_table_spans_fail(self):
        text = data_word(0x1000) + data_word(0x1004) + data_word(0x1008)
        with self.assertRaisesRegex(SystemExit, "overlapping"):
            build.split_table_data(text, {0x1000: (8, "first"), 0x1004: (8, "second")})

    def test_no_tables_preserves_assembly_verbatim(self):
        text = ".section .data\nlabel:\n" + data_word(0x1000) + ".align 4\n"
        self.assertEqual(build.split_table_data(text, {}), ([("asm", text)], {}))


class ObjectScopedDependencies(unittest.TestCase):
    def exercise(self, table_conflict=False):
        paths = [Path("first.o"), Path("second.o")]
        objects = []
        for name in ("first", "second"):
            objects.append({
                "sections": [{"name": ".text", "size": 4, "data": bytes(4)},
                             {"name": ".rodata", "size": 12}],
                "symbols": [{"name": name, "type": 2, "section": 0,
                             "size": 4, "value": 0}],
            })
        with patch.object(build, "program_functions", return_value={
                "first": [(0x2000, 4)], "second": [(0x3000, 4)]}), \
             patch.object(build, "sources_for", return_value=paths), \
             patch.object(build, "compile_c", side_effect=lambda source: source), \
             patch.object(build.elf, "read_object", side_effect=objects):
            placement = build.Placement("boot", {"segments": []})

            def derive(obj, index, address):
                if table_conflict:
                    return {"§.rodata": 0x1000}, None
                return ({"§.text": 0x3000} if address == 0x2000 else {}), None

            with patch.object(placement, "derive", side_effect=derive), \
                 patch.object(placement, "jump_table", return_value=True):
                placement.place()
        return placement

    def test_other_objects_same_section_name_cannot_satisfy_dependency(self):
        placement = self.exercise()
        self.assertEqual(set(placement.placed), {"second"})
        self.assertEqual(placement.differ, [("first", "needs .text at its place, which is not C")])

    def test_overlapping_compiler_sections_are_both_withdrawn(self):
        placement = self.exercise(table_conflict=True)
        self.assertEqual(placement.placed, {})
        self.assertEqual(placement.tables, {})
        self.assertEqual({name for name, _ in placement.differ}, {"first", "second"})


if __name__ == "__main__":
    unittest.main()

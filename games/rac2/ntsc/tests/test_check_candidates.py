import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

from test_elf_tools import synthetic_elf


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
SPEC = importlib.util.spec_from_file_location("check_candidates", ROOT / "scripts" / "check_candidates.py")
checker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(checker)


def candidate_elf(payload=b"FUNCTION", symbol_size=8, symbol_address=0x1000,
                  symbol_type=18, section_index=1):
    entry = struct.pack("<IIIBBH", 1, symbol_address, symbol_size, symbol_type, 0, section_index)
    data = synthetic_elf(segments=({"data": payload},), sections=(
        {"name": ".text", "data": payload, "address": 0x1000},
        {"name": ".strtab", "type": 3, "flags": 0, "data": b"\0candidate\0"},
        {"name": ".symtab", "type": 2, "flags": 0, "data": bytes(16) + entry},
    ))
    section_offset = struct.unpack_from("<I", data, 32)[0]
    struct.pack_into("<I", data, section_offset + 3 * 40 + 24, 2)
    struct.pack_into("<I", data, section_offset + 3 * 40 + 36, 16)
    return data


class CandidateGateTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.reference = self.directory / "reference.elf"
        self.reference.write_bytes(synthetic_elf(segments=({"data": b"FUNCTION"},)))
        self.candidate = self.directory / "candidate.elf"

    def compare(self, data, address=0x1000, size=8):
        self.candidate.write_bytes(data)
        return checker.compare_function(self.reference, self.candidate, "candidate", address, size)

    def test_complete_defined_function_matches(self):
        result = self.compare(candidate_elf())
        self.assertTrue(result["matched"])
        self.assertEqual(result["size"], 8)
        self.assertEqual(result["state"], "matched_unintegrated")

    def test_identical_prefix_with_extra_instructions_is_rejected(self):
        result = self.compare(candidate_elf(payload=b"FUNCTIONMORE", symbol_size=12))
        self.assertFalse(result["matched"])
        self.assertEqual(result["reason"], "complete symbol size mismatch")

    def test_zero_size_alias_is_rejected(self):
        self.assertFalse(self.compare(candidate_elf(symbol_size=0))["matched"])

    def test_data_symbol_cannot_count_as_function(self):
        self.assertFalse(self.compare(candidate_elf(symbol_type=17))["matched"])

    def test_absolute_or_undefined_symbol_cannot_count(self):
        for section_index in (0, 0xFFF1):
            with self.subTest(section_index=section_index):
                self.assertFalse(self.compare(candidate_elf(section_index=section_index))["matched"])

    def test_wrong_address_is_rejected(self):
        self.assertFalse(self.compare(candidate_elf(payload=b"FUNCTIONMORE", symbol_address=0x1004))["matched"])

    def test_symbol_outside_its_section_is_rejected(self):
        with self.assertRaises(ValueError):
            self.compare(candidate_elf(symbol_address=0x1004))

    def test_unlinked_object_is_rejected(self):
        data = candidate_elf()
        struct.pack_into("<H", data, 16, 1)
        with self.assertRaises(ValueError):
            self.compare(data)

    def test_one_changed_byte_is_a_mismatch(self):
        result = self.compare(candidate_elf(payload=b"FUNCTI0N"))
        self.assertFalse(result["matched"])
        self.assertEqual(result["different_bytes"], 1)

    def test_symbol_table_wrong_stride_is_rejected(self):
        data = candidate_elf()
        section_offset = struct.unpack_from("<I", data, 32)[0]
        struct.pack_into("<I", data, section_offset + 3 * 40 + 36, 8)
        with self.assertRaises(ValueError):
            self.compare(data)

    def test_symbol_table_wrong_string_table_is_rejected(self):
        data = candidate_elf()
        section_offset = struct.unpack_from("<I", data, 32)[0]
        struct.pack_into("<I", data, section_offset + 3 * 40 + 24, 1)
        with self.assertRaises(ValueError):
            self.compare(data)

    def test_zero_expected_boundary_is_rejected(self):
        self.assertFalse(self.compare(candidate_elf(), size=0)["matched"])


if __name__ == "__main__":
    unittest.main()

"""Synthetic ELF fixtures for the strict inspection and acceptance boundary."""

import hashlib
import importlib.util
import os
from pathlib import Path
import struct
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "elf_tools.py"
MODULE_SPEC = importlib.util.spec_from_file_location("rac2_elf_tools", MODULE_PATH)
elf_tools = importlib.util.module_from_spec(MODULE_SPEC)
MODULE_SPEC.loader.exec_module(elf_tools)


def synthetic_elf(*, elf_type=2, entry=0x1000, segments=(), sections=(), marker=b""):
    program_offset = 52 if segments else 0
    image = bytearray(52 + 32 * len(segments))
    program_headers = []
    for segment in segments:
        payload = segment.get("data", b"ABCD")
        offset = len(image)
        image.extend(payload)
        program_headers.append((
            segment.get("type", 1), offset, segment.get("address", 0x1000),
            segment.get("address", 0x1000), len(payload),
            segment.get("memsz", len(payload)), segment.get("flags", 5),
            segment.get("alignment", 1),
        ))
    section_headers = []
    if sections:
        names = bytearray(b"\0")
        section_headers.append((0,) * 10)
        for section in sections:
            name_offset = len(names)
            names.extend(section.get("name", ".text").encode("utf-8") + b"\0")
            payload = section.get("data", b"CODE")
            section_type = section.get("type", 1)
            offset = section.get("offset", len(image))
            size = section.get("size", len(payload))
            if section_type != 8:
                image.extend(payload)
            section_headers.append((
                name_offset, section_type, section.get("flags", 6),
                section.get("address", 0), offset, size, 0, 0,
                section.get("alignment", 1), 0,
            ))
        names_index = len(section_headers)
        names_offset = len(names)
        names.extend(b".shstrtab\0")
        names_file_offset = len(image)
        image.extend(names)
        section_headers.append((
            names_offset, 3, 0, 0, names_file_offset, len(names), 0, 0, 1, 0,
        ))
        section_offset = len(image)
        for section_header in section_headers:
            image.extend(struct.pack("<10I", *section_header))
    else:
        section_offset = 0
        names_index = 0
    identification = b"\x7fELF\x01\x01\x01" + bytes(9)
    struct.pack_into(
        "<16sHHIIIIIHHHHHH", image, 0,
        identification, elf_type, 8, 1, entry, program_offset, section_offset,
        0, 52, 32 if segments else 0, len(segments),
        40 if section_headers else 0, len(section_headers), names_index,
    )
    for index, program_header in enumerate(program_headers):
        struct.pack_into("<8I", image, program_offset + index * 32, *program_header)
    image.extend(marker)
    return image


def changed(image, offset, value, encoding="I"):
    modified = bytearray(image)
    struct.pack_into("<" + encoding, modified, offset, value)
    return modified


def section_table_offset(image):
    return struct.unpack_from("<I", image, 32)[0]


class ElfToolsTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)

    def write(self, name, image):
        path = self.directory / name
        path.write_bytes(image)
        return path

    def assert_invalid(self, image, reason):
        path = self.write("invalid.elf", image)
        with self.assertRaisesRegex(ValueError, reason):
            elf_tools.read_elf(path)

    def pair(self, reference_image, candidate_image):
        return (
            self.write("reference.elf", reference_image),
            self.write("candidate.elf", candidate_image),
        )

    def compare(self, reference_image, candidate_image):
        return elf_tools.compare_loads(*self.pair(reference_image, candidate_image))

    def test_metadata_contract_and_sha256(self):
        image = synthetic_elf(
            segments=[{"data": b"ABCD", "memsz": 8}],
            sections=[{"name": ".text", "address": 0x1000, "data": b"CODE"}],
        )
        metadata = elf_tools.read_elf(self.write("exec.elf", image))
        self.assertEqual(set(metadata), {
            "sha256", "entry", "type", "machine", "sections", "segments",
        })
        self.assertEqual(metadata["sha256"], hashlib.sha256(image).hexdigest())
        self.assertEqual((metadata["entry"], metadata["type"], metadata["machine"]),
                         (0x1000, 2, 8))
        self.assertEqual(metadata["segments"], [{
            "type": 1, "offset": 84, "address": 0x1000, "filesz": 4,
            "memsz": 8, "flags": 5, "alignment": 1,
        }])
        self.assertEqual(metadata["sections"][1], {
            "name": ".text", "type": 1, "flags": 6, "address": 0x1000,
            "offset": 88, "size": 4, "alignment": 1,
        })

    def test_empty_tables_are_valid(self):
        metadata = elf_tools.read_elf(self.write("empty.elf", synthetic_elf()))
        self.assertEqual(metadata["sections"], [])
        self.assertEqual(metadata["segments"], [])

    def test_bad_identification_machine_type_and_version(self):
        image = synthetic_elf()
        cases = (
            (0, 0, "Invalid ELF magic", "B"),
            (4, 2, "ELF32 little-endian", "B"),
            (5, 2, "ELF32 little-endian", "B"),
            (6, 0, "version", "B"),
            (18, 3, "MIPS", "H"),
            (16, 3, "ET_REL or ET_EXEC", "H"),
            (20, 0, "version", "I"),
            (40, 51, "header size", "H"),
            (40, 100, "header outside", "H"),
        )
        for offset, value, reason, encoding in cases:
            with self.subTest(offset=offset, value=value):
                self.assert_invalid(changed(image, offset, value, encoding), reason)

    def test_truncated_headers(self):
        image = synthetic_elf()
        for length in (0, 4, 16, 51):
            with self.subTest(length=length):
                self.assert_invalid(image[:length], "Truncated ELF header")

    def test_segment_table_bounds_and_stride(self):
        image = synthetic_elf(segments=[{}])
        for offset, value, encoding in (
            (28, len(image), "I"), (28, 0xFFFFFFF0, "I"),
            (28, 0, "I"), (28, 20, "I"), (42, 31, "H"), (44, 2, "H"),
        ):
            with self.subTest(offset=offset, value=value):
                self.assert_invalid(changed(image, offset, value, encoding), "segments")
        self.assert_invalid(image[:83], "segments")

    def test_section_table_bounds_and_stride(self):
        image = synthetic_elf(elf_type=1, sections=[{}])
        for offset, value, encoding in (
            (32, 0, "I"), (32, len(image) - 1, "I"),
            (32, 0xFFFFFFF0, "I"), (46, 39, "H"), (48, 4, "H"),
        ):
            with self.subTest(offset=offset, value=value):
                self.assert_invalid(changed(image, offset, value, encoding), "section")
        self.assert_invalid(image[:-1], "sections")
        initial_type = section_table_offset(image) + 4
        self.assert_invalid(changed(image, initial_type, 1), "Section zero")

    def test_orphan_table_offsets_and_name_index(self):
        image = synthetic_elf()
        self.assert_invalid(changed(image, 28, 52), "offset without entries")
        self.assert_invalid(changed(image, 50, 1, "H"), "Missing section table")

    def test_section_payload_bounds(self):
        image = synthetic_elf(elf_type=1, sections=[{}])
        section_offset = section_table_offset(image) + 40
        for offset, value in (
            (section_offset + 16, len(image) - 2),
            (section_offset + 16, 0xFFFFFFFF),
            (section_offset + 20, 0xFFFFFFFF),
        ):
            with self.subTest(offset=offset, value=value):
                self.assert_invalid(changed(image, offset, value), "Section 1.*bounds")

    def test_nobits_needs_no_payload_but_cannot_supply_bytes(self):
        image = synthetic_elf(elf_type=1, sections=[{
            "name": ".bss", "type": 8, "offset": 0xFFFFFFF0,
            "size": 16, "address": 0x2000,
        }])
        path = self.write("nobits.elf", image)
        self.assertEqual(elf_tools.read_elf(path)["sections"][1]["size"], 16)
        with self.assertRaisesRegex(ValueError, "not file-backed"):
            elf_tools.address_bytes(path, 0x2000, 4)

    def test_segment_payload_bounds_including_nonload_segments(self):
        for segment_type in (0, 1, 4):
            image = synthetic_elf(segments=[{"type": segment_type}])
            for offset, value in ((56, len(image) - 1), (56, 0xFFFFFFFF), (68, 100)):
                with self.subTest(segment_type=segment_type, offset=offset):
                    self.assert_invalid(changed(image, offset, value), "Segment 0.*bounds")

    def test_load_filesz_must_not_exceed_memsz(self):
        self.assert_invalid(synthetic_elf(segments=[{"memsz": 3}]), "filesz exceeds memsz")

    def test_address_space_overflow(self):
        self.assert_invalid(
            synthetic_elf(segments=[{"address": 0xFFFFFFFE}]), "address space"
        )
        self.assert_invalid(synthetic_elf(elf_type=1, sections=[{
            "address": 0xFFFFFFFF, "data": b"AB",
        }]), "address space")

    def test_alignment_checks(self):
        self.assert_invalid(synthetic_elf(segments=[{"alignment": 3}]), "power of two")
        self.assert_invalid(synthetic_elf(sections=[{"alignment": 3}]), "power of two")
        for alignment in (0, 1, 4, 8, 4096):
            with self.subTest(alignment=alignment):
                elf_tools.read_elf(self.write(
                    "aligned.elf", synthetic_elf(segments=[{"alignment": alignment}])
                ))

    def test_sdk_load_alignment_incongruence_with_synthetic_payload(self):
        load_headers = (
            (1, 4096, 1048704, 1048704, 2432832, 2432832, 7, 4096),
            (1, 2437120, 25165824, 25165824, 88931, 88931, 6, 4096),
        )
        image = bytearray(2437120 + 88931)
        image[:52] = synthetic_elf()[:52]
        image = changed(image, 28, 52)
        image = changed(image, 42, 32, "H")
        image = changed(image, 44, 2, "H")
        for index, load_header in enumerate(load_headers):
            struct.pack_into("<8I", image, 52 + index * 32, *load_header)
        path = self.write("sdk-synthetic.elf", image)
        metadata = elf_tools.read_elf(path)
        self.assertEqual(metadata["segments"], [{
            "type": header[0], "offset": header[1], "address": header[2],
            "filesz": header[4], "memsz": header[5], "flags": header[6],
            "alignment": header[7],
        } for header in load_headers])
        self.assertNotEqual(1048704 % 4096, 4096 % 4096)
        self.assertEqual(elf_tools.address_bytes(path, 1048704, 16), bytes(16))
        self.assertEqual(elf_tools.address_bytes(path, 25165824 + 88930, 1), bytes(1))
        self.assert_invalid(changed(image, 52 + 32 + 4, len(image)), "Segment 1.*bounds")
        self.assert_invalid(changed(image, 52 + 32 + 20, 88930), "filesz exceeds memsz")

    def test_duplicate_section_names_preserve_individual_offsets(self):
        image = synthetic_elf(elf_type=1, sections=[
            {"name": "mc1.data", "flags": 3, "data": b"AB"},
            {"name": "mc1.data", "flags": 3, "data": b"CD"},
        ])
        sections = elf_tools.read_elf(self.write("duplicate-names.elf", image))["sections"]
        self.assertEqual([section["name"] for section in sections[1:3]],
                         ["mc1.data", "mc1.data"])
        self.assertNotEqual(sections[1]["offset"], sections[2]["offset"])

    def test_invalid_section_names_and_table_type(self):
        image = synthetic_elf(sections=[{}])
        table_offset = section_table_offset(image)
        self.assert_invalid(changed(image, 50, 3, "H"), "index outside")
        self.assert_invalid(changed(image, table_offset + 80 + 4, 8), "SHT_STRTAB")
        self.assert_invalid(changed(image, table_offset + 40, 100), "name offset outside")
        self.assert_invalid(changed(image, 50, 0, "H"), "without a string table")
        names_offset, names_size = struct.unpack_from("<II", image, table_offset + 96)
        for offset in (names_offset, names_offset + names_size - 1):
            with self.subTest(offset=offset):
                self.assert_invalid(changed(image, offset, 65, "B"), "string table")

    def test_extended_counts_and_name_index(self):
        image = synthetic_elf(segments=[{}], sections=[{}])
        table_offset = section_table_offset(image)
        extended = changed(image, 48, 0, "H")
        extended = changed(extended, table_offset + 20, 3)
        extended = changed(extended, 50, 0xFFFF, "H")
        extended = changed(extended, table_offset + 24, 2)
        extended = changed(extended, 44, 0xFFFF, "H")
        extended = changed(extended, table_offset + 28, 1)
        metadata = elf_tools.read_elf(self.write("extended.elf", extended))
        self.assertEqual(len(metadata["sections"]), 3)
        self.assertEqual(metadata["sections"][1]["name"], ".text")
        self.assertEqual(len(metadata["segments"]), 1)
        self.assert_invalid(changed(extended, table_offset + 20, 0), "extended section count")
        self.assert_invalid(changed(extended, table_offset + 20, 0xFFFFFFFF), "sections")
        self.assert_invalid(changed(extended, table_offset + 28, 0), "extended program count")
        self.assert_invalid(changed(extended, table_offset + 28, 0xFFFFFFFF), "segments")
        self.assert_invalid(changed(synthetic_elf(), 44, 0xFFFF, "H"), "section table")

    def test_larger_table_strides_are_bounded(self):
        image = synthetic_elf(segments=[{}])
        image[84:84] = bytes(8)
        image = changed(image, 42, 40, "H")
        image = changed(image, 56, 92)
        path = self.write("padded-table.elf", image)
        self.assertEqual(elf_tools.address_bytes(path, 0x1000, 4), b"ABCD")
        self.assert_invalid(changed(image, 42, 100, "H"), "segments")

    def test_read_address_inside_load_and_exact_endpoint(self):
        path = self.write("exec.elf", synthetic_elf(segments=[{"data": b"ABCDEFGH"}]))
        self.assertEqual(elf_tools.address_bytes(path, 0x1002, 6), b"CDEFGH")

    def test_invalid_address_and_size(self):
        path = self.write("exec.elf", synthetic_elf(segments=[{}]))
        for address, size in (
            (0x1000, 0), (0x1000, -1), (-1, 1), (1 << 32, 1),
            (0xFFFFFFFF, 2), (0x1000, 1.5), (0x1000, True), (True, 1),
        ):
            with self.subTest(address=address, size=size), self.assertRaises(ValueError):
                elf_tools.address_bytes(path, address, size)

    def test_outside_and_bss_address_ranges(self):
        path = self.write("exec.elf", synthetic_elf(segments=[{"memsz": 8}]))
        for address, size, reason in (
            (0xFFF, 1, "outside"), (0x1008, 1, "outside"),
            (0x1004, 1, "not file-backed"), (0x1003, 2, "not file-backed"),
            (0x1000, 9, "not file-backed"),
        ):
            with self.subTest(address=address, size=size):
                with self.assertRaisesRegex(ValueError, reason):
                    elf_tools.address_bytes(path, address, size)

    def test_adjacent_loads_can_supply_one_range(self):
        path = self.write("adjacent.elf", synthetic_elf(segments=[
            {"address": 0x1004, "data": b"EFGH"},
            {"address": 0x1000, "data": b"ABCD"},
        ]))
        self.assertEqual(elf_tools.address_bytes(path, 0x1002, 4), b"CDEF")

    def test_gaps_and_overlapping_segments_are_rejected(self):
        for second_address, second_data, reason in (
            (0x1006, b"EFGH", "outside"),
            (0x1002, b"CDEF", "Ambiguous"),
            (0x1001, b"BC", "Ambiguous"),
        ):
            with self.subTest(second_address=second_address):
                path = self.write("mapping.elf", synthetic_elf(segments=[
                    {"data": b"ABCD"}, {"address": second_address, "data": second_data},
                ]))
                with self.assertRaisesRegex(ValueError, reason):
                    elf_tools.address_bytes(path, 0x1000, 8 if reason == "outside" else 4)

    def test_bss_overlap_is_ambiguous(self):
        path = self.write("overlap.elf", synthetic_elf(segments=[
            {"data": b"AB", "memsz": 8}, {"address": 0x1004},
        ]))
        with self.assertRaisesRegex(ValueError, "Ambiguous"):
            elf_tools.address_bytes(path, 0x1004, 2)

    def test_exec_never_falls_back_to_sections(self):
        path = self.write("section-only.elf", synthetic_elf(sections=[{"address": 0x1000}]))
        with self.assertRaisesRegex(ValueError, "outside"):
            elf_tools.address_bytes(path, 0x1000, 1)

    def test_exec_uses_load_bytes_even_when_section_contents_differ(self):
        path = self.write("exec.elf", synthetic_elf(
            segments=[{"data": b"LOAD"}],
            sections=[{"address": 0x1000, "data": b"CODE"}],
        ))
        self.assertEqual(elf_tools.address_bytes(path, 0x1000, 4), b"LOAD")

    def test_rel_never_reads_load_segments_instead_of_executable_sections(self):
        path = self.write("object.elf", synthetic_elf(elf_type=1, segments=[{}]))
        with self.assertRaisesRegex(ValueError, "outside"):
            elf_tools.address_bytes(path, 0x1000, 1)

    def test_rel_reads_only_executable_sections(self):
        path = self.write("object.elf", synthetic_elf(elf_type=1, sections=[
            {"data": b"CODE"}, {"name": ".data", "flags": 3, "data": b"DATA"},
        ]))
        self.assertEqual(elf_tools.read_elf(path)["type"], 1)
        self.assertEqual(elf_tools.address_bytes(path, 1, 3), b"ODE")
        nonexec = self.write("nonexec.elf", synthetic_elf(elf_type=1, sections=[{"flags": 3}]))
        with self.assertRaisesRegex(ValueError, "outside"):
            elf_tools.address_bytes(nonexec, 0, 1)

    def test_rel_overlapping_exec_sections_are_ambiguous(self):
        path = self.write("object.elf", synthetic_elf(elf_type=1, sections=[{}, {}]))
        with self.assertRaisesRegex(ValueError, "Ambiguous"):
            elf_tools.address_bytes(path, 0, 1)

    def test_noninitial_null_section_payload_still_has_bounds(self):
        image = synthetic_elf(sections=[{"type": 0}])
        self.assert_invalid(
            changed(image, section_table_offset(image) + 56, 0xFFFFFFFF),
            "Section 1.*bounds",
        )

    def test_address_at_last_elf32_byte(self):
        path = self.write("last-byte.elf", synthetic_elf(segments=[{
            "address": 0xFFFFFFFF, "data": b"Z",
        }]))
        self.assertEqual(elf_tools.address_bytes(path, 0xFFFFFFFF, 1), b"Z")

    def test_compare_all_segments_with_different_nonloaded_metadata(self):
        segments = [{"data": b"ABCD"}, {"address": 0x2000, "data": b"EFGHI", "memsz": 9}]
        result = self.compare(
            synthetic_elf(segments=segments, marker=b"reference"),
            synthetic_elf(segments=segments, marker=b"candidate"),
        )
        self.assertEqual(set(result), {"matched", "bytes_compared", "reason"})
        self.assertTrue(result["matched"])
        self.assertEqual(result["bytes_compared"], 9)
        self.assertIn("match", result["reason"])

    def test_compare_accepts_different_file_offsets_and_segment_order(self):
        reference = synthetic_elf(segments=[{}, {"address": 0x2000, "data": b"EFGH"}])
        candidate = synthetic_elf(segments=[
            {"type": 4, "data": b"note"},
            {"address": 0x2000, "data": b"EFGH"}, {},
        ])
        result = self.compare(reference, candidate)
        self.assertTrue(result["matched"])
        self.assertEqual(result["bytes_compared"], 8)

    def test_compare_accepts_different_declared_load_alignments(self):
        result = self.compare(
            synthetic_elf(segments=[{"alignment": 4096}]),
            synthetic_elf(segments=[{"alignment": 1}]),
        )
        self.assertTrue(result["matched"])
        self.assertEqual(result["bytes_compared"], 4)

    def test_difference_in_second_load_cannot_match(self):
        result = self.compare(
            synthetic_elf(segments=[{}, {"address": 0x2000, "data": b"EFGH"}]),
            synthetic_elf(segments=[{}, {"address": 0x2000, "data": b"EFGX"}]),
        )
        self.assertFalse(result["matched"])
        self.assertEqual(result["bytes_compared"], 8)
        self.assertIn("PT_LOAD 1 content mismatch", result["reason"])
        self.assertIn("0x00002003", result["reason"])

    def test_first_byte_difference_reports_one_compared_byte(self):
        result = self.compare(
            synthetic_elf(segments=[{}]), synthetic_elf(segments=[{"data": b"XBCD"}]),
        )
        self.assertFalse(result["matched"])
        self.assertEqual(result["bytes_compared"], 1)

    def test_compare_rejects_load_metadata_differences(self):
        reference = synthetic_elf(segments=[{}, {"address": 0x2000}])
        for changes, field in (
            ({"address": 0x2001}, "address"), ({"data": b"ABC"}, "filesz"),
            ({"memsz": 8}, "memsz"), ({"flags": 7}, "flags"),
        ):
            with self.subTest(field=field):
                candidate = synthetic_elf(segments=[{}, {"address": 0x2000, **changes}])
                result = self.compare(reference, candidate)
                self.assertFalse(result["matched"])
                self.assertEqual(result["bytes_compared"], 0)
                self.assertIn(field, result["reason"])

    def test_compare_rejects_entry_count_and_type_differences(self):
        reference = synthetic_elf(segments=[{}])
        for candidate, reason in (
            (synthetic_elf(segments=[{}], entry=0x1001), "Entry point"),
            (synthetic_elf(segments=[{}, {"address": 0x2000}]), "count"),
            (synthetic_elf(elf_type=1, segments=[{}]), "ET_EXEC"),
        ):
            with self.subTest(reason=reason):
                result = self.compare(reference, candidate)
                self.assertFalse(result["matched"])
                self.assertEqual(result["bytes_compared"], 0)
                self.assertIn(reason, result["reason"])

    def test_compare_rejects_missing_loads_on_either_side(self):
        loaded = synthetic_elf(segments=[{}])
        for empty in (synthetic_elf(), synthetic_elf(segments=[{"type": 4}])):
            for reference, candidate in ((loaded, empty), (empty, loaded)):
                with self.subTest(reference_loaded=reference is loaded):
                    result = self.compare(reference, candidate)
                    self.assertFalse(result["matched"])
                    self.assertEqual(result["bytes_compared"], 0)
                    self.assertIn("PT_LOAD", result["reason"])

    def test_compare_rejects_overlapping_and_empty_loads(self):
        for segments, reason in (
            ([{}, {"address": 0x1002}], "overlap"),
            ([{"data": b"", "memsz": 8}], "no file-backed bytes"),
        ):
            with self.subTest(reason=reason):
                result = self.compare(
                    synthetic_elf(segments=segments, marker=b"reference"),
                    synthetic_elf(segments=segments, marker=b"candidate"),
                )
                self.assertFalse(result["matched"])
                self.assertEqual(result["bytes_compared"], 0)
                self.assertIn(reason, result["reason"])

    def test_compare_counts_file_bytes_and_checks_zero_fill_extent(self):
        reference = synthetic_elf(segments=[
            {"data": b"AB", "memsz": 4}, {"address": 0x2000, "data": b"", "memsz": 8},
        ], marker=b"reference")
        candidate = synthetic_elf(segments=[
            {"data": b"AB", "memsz": 4}, {"address": 0x2000, "data": b"", "memsz": 8},
        ], marker=b"candidate")
        result = self.compare(reference, candidate)
        self.assertTrue(result["matched"])
        self.assertEqual(result["bytes_compared"], 2)
        result = self.compare(reference, changed(candidate, 52 + 32 + 20, 9))
        self.assertFalse(result["matched"])
        self.assertIn("memsz", result["reason"])

    def test_same_path_alias_and_identical_copy_are_inconclusive(self):
        image = synthetic_elf(segments=[{}])
        reference, candidate = self.pair(image, image)
        alias = reference.parent / "." / reference.name
        for target in (reference, alias, candidate):
            with self.subTest(target=target.name):
                with self.assertRaisesRegex(ValueError, "Inconclusive"):
                    elf_tools.compare_loads(reference, target)

    def test_hardlink_to_reference_is_inconclusive(self):
        reference = self.write("reference.elf", synthetic_elf(segments=[{}]))
        alias = self.directory / "hardlink.elf"
        os.link(reference, alias)
        with self.assertRaisesRegex(ValueError, "same file"):
            elf_tools.compare_loads(reference, alias)

    def test_compare_does_not_hide_malformed_candidate(self):
        with self.assertRaisesRegex(ValueError, "Truncated"):
            self.compare(synthetic_elf(segments=[{}]), b"\x7fELF")

    def test_fresh_candidate_and_equal_timestamps(self):
        candidate = self.write("candidate.elf", b"candidate")
        first = self.write("first.c", b"first")
        second = self.write("second.c", b"second")
        baseline = 1_700_000_000_000_000_000
        os.utime(first, ns=(baseline, baseline))
        os.utime(second, ns=(baseline + 100, baseline + 100))
        os.utime(candidate, ns=(baseline + 100, baseline + 100))
        self.assertIsNone(elf_tools.assert_fresh(candidate, [first, second]))
        os.utime(candidate, ns=(baseline + 200, baseline + 200))
        self.assertIsNone(elf_tools.assert_fresh(candidate, iter([first, second])))

    def test_stale_candidate_detected_against_second_input(self):
        candidate = self.write("candidate.elf", b"candidate")
        first = self.write("first.c", b"first")
        second = self.write("second.c", b"second")
        baseline = 1_700_000_000_000_000_000
        os.utime(first, ns=(baseline, baseline))
        os.utime(candidate, ns=(baseline + 100, baseline + 100))
        os.utime(second, ns=(baseline + 200, baseline + 200))
        with self.assertRaisesRegex(ValueError, "older"):
            elf_tools.assert_fresh(candidate, [first, second])

    def test_missing_candidate_and_missing_input(self):
        candidate = self.write("candidate.elf", b"candidate")
        missing = self.directory / "missing.c"
        with self.assertRaisesRegex(ValueError, "Input missing"):
            elf_tools.assert_fresh(candidate, [missing])
        with self.assertRaisesRegex(ValueError, "Candidate missing"):
            elf_tools.assert_fresh(missing, [])

    def test_freshness_rejects_directories(self):
        candidate = self.write("candidate.elf", b"candidate")
        with self.assertRaisesRegex(ValueError, "Input must be a regular file"):
            elf_tools.assert_fresh(candidate, [self.directory])
        with self.assertRaisesRegex(ValueError, "Candidate must be a regular file"):
            elf_tools.assert_fresh(self.directory, [])


if __name__ == "__main__":
    unittest.main()

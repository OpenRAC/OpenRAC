"""Offline pipeline regressions using synthetic ISO data and mocked ELF metadata."""

import copy
import hashlib
import importlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))
pipeline_setup = importlib.import_module("setup")
pipeline_build = importlib.import_module("build")


def iso_directory_record(name, sector, size, directory=False):
    identifier = name.encode("ascii") if isinstance(name, str) else name
    length = 33 + len(identifier)
    length += length % 2
    record = bytearray(length)
    record[0] = length
    struct.pack_into("<I", record, 2, sector)
    struct.pack_into(">I", record, 6, sector)
    struct.pack_into("<I", record, 10, size)
    struct.pack_into(">I", record, 14, size)
    record[25] = 2 if directory else 0
    struct.pack_into("<H", record, 28, 1)
    struct.pack_into(">H", record, 30, 1)
    record[32] = len(identifier)
    record[33:33 + len(identifier)] = identifier
    return record


def synthetic_structure():
    return {
        "entry": 0x400080,
        "segments": [
            {"type": 4, "offset": 0x80, "address": 0, "filesz": 16,
             "memsz": 16, "flags": 4, "alignment": 1},
            {"type": 1, "offset": 0x100, "address": 0x400080, "filesz": 0x80,
             "memsz": 0x80, "flags": 7, "alignment": 4096},
            {"type": 1, "offset": 0x400, "address": 0x900000, "filesz": 0x30,
             "memsz": 0x50, "flags": 6, "alignment": 4096},
        ],
        "sections": [
            {"name": ".text", "type": 1, "flags": 6, "address": 0x400080,
             "offset": 0x100, "size": 0x20, "alignment": 16},
            {"name": "core.text", "type": 1, "flags": 6, "address": 0x4000A0,
             "offset": 0x120, "size": 0x20, "alignment": 16},
            {"name": ".jump-table", "type": 1, "flags": 2, "address": 0x4000C0,
             "offset": 0x140, "size": 0x20, "alignment": 16},
            {"name": "mc1.data", "type": 1, "flags": 3, "address": 0x4000E0,
             "offset": 0x160, "size": 0x20, "alignment": 16},
            {"name": "mc1.data", "type": 1, "flags": 3, "address": 0x900000,
             "offset": 0x400, "size": 0x30, "alignment": 16},
            {"name": ".bss", "type": 8, "flags": 3, "address": 0x900030,
             "offset": 0xFFFFFFF0, "size": 0x20, "alignment": 16},
            {"name": ".debug", "type": 1, "flags": 0, "address": 0,
             "offset": 0x430, "size": 16, "alignment": 1},
        ],
    }


class PipelineTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)

    def write(self, name, content):
        path = self.directory / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return path

    def iso(self, records, directory_size=None, image_size=8 * 2048):
        image = bytearray(image_size)
        contents = b"".join(records)
        image[2048:2048 + len(contents)] = contents
        path = self.write("synthetic.iso", image)
        return path, {"sector": 1, "size": len(contents) if directory_size is None else directory_size}

    def configured(self, structure=None):
        metadata = synthetic_structure() if structure is None else structure
        reference_size = max(0x440, max(
            section["offset"] + section["size"]
            for section in metadata["sections"] if section["type"] != 8
        ))
        reference = self.write("synthetic.elf", bytes(reference_size))
        build_directory = self.directory / "generated"
        build_directory.mkdir()
        with mock.patch.object(pipeline_build, "read_elf", return_value=metadata) as reader:
            twins = pipeline_build.generate_config(reference, build_directory, "synthetic")
        reader.assert_called_once_with(reference)
        document = json.loads((build_directory / "config" / "splat.yaml").read_text(encoding="utf-8"))
        return build_directory, twins, document

    def linker_fixture(self, content, sources=(), name="linker"):
        build_directory = self.directory / name
        (build_directory / "config").mkdir(parents=True)
        (build_directory / "assets").mkdir()
        (build_directory / "assets" / "synthetic.elf").write_bytes(b"synthetic")
        script = build_directory / "config" / "rac2.ld"
        script.write_text(content, encoding="ascii")
        for source in sources:
            path = build_directory / "asm_pp" / source
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(".word 0\n", encoding="ascii")
        return build_directory, script

    def test_modules_import_from_scripts_directory(self):
        self.assertEqual(Path(pipeline_setup.__file__).resolve(), SCRIPTS / "setup.py")
        self.assertEqual(Path(pipeline_build.__file__).resolve(), SCRIPTS / "build.py")

    def test_subprocess_failures_raise_and_preserve_merged_log(self):
        arguments = ["synthetic-tool", "argument with spaces"]
        for module, function_name in ((pipeline_setup, "command"), (pipeline_build, "checked")):
            with self.subTest(function=function_name):
                log = self.directory / f"{function_name}.log"

                def failed_run(actual_arguments, **options):
                    self.assertEqual(actual_arguments, arguments)
                    self.assertEqual(options["stderr"], subprocess.STDOUT)
                    options["stdout"].write(b"stdout evidence\nstderr evidence\n")
                    return subprocess.CompletedProcess(actual_arguments, 23)

                with mock.patch.object(module.subprocess, "run", side_effect=failed_run) as runner:
                    with self.assertRaisesRegex(ValueError, r"23.*log"):
                        if function_name == "command":
                            module.command(arguments, log)
                        else:
                            module.checked(arguments, self.directory, log)
                runner.assert_called_once()
                self.assertEqual(log.read_bytes(), b"stdout evidence\nstderr evidence\n")
                if function_name == "checked":
                    self.assertEqual(runner.call_args.kwargs["cwd"], self.directory)

    def test_subprocess_zero_is_accepted_and_logs_are_closed(self):
        for module, function_name in ((pipeline_setup, "command"), (pipeline_build, "checked")):
            with self.subTest(function=function_name):
                log = self.directory / f"{function_name}.log"
                with mock.patch.object(module.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as runner:
                    if function_name == "command":
                        module.command(["synthetic-tool"], log)
                    else:
                        module.checked(["synthetic-tool"], self.directory, log)
                self.assertTrue(runner.call_args.kwargs["stdout"].closed)
                self.assertEqual(log.read_bytes(), b"")

    def test_archive_warning_requires_explicit_allowance(self):
        for returncode, allow_warning, accepted in (
            (1, False, False), (1, True, True), (2, True, False), (-1, True, False),
        ):
            with self.subTest(returncode=returncode, allow_warning=allow_warning):
                with mock.patch.object(pipeline_setup.subprocess, "run", return_value=subprocess.CompletedProcess([], returncode)):
                    if accepted:
                        pipeline_setup.command(["synthetic-tool"], self.directory / "warning.log", allow_warning=True)
                    else:
                        with self.assertRaises(ValueError):
                            pipeline_setup.command(["synthetic-tool"], self.directory / "warning.log", allow_warning=allow_warning)

    def test_require_identity_accepts_pinned_fields_and_extra_measurements(self):
        expected = {"size": 17, "sha256": "synthetic digest"}
        actual = {**expected, "sha1": "additional digest"}
        self.assertIsNone(pipeline_setup.require_identity(actual, expected, "Synthetic ISO"))

    def test_require_identity_rejects_mismatch_and_missing_pin(self):
        expected = {"size": 17, "sha256": "synthetic digest"}
        for actual, failed_fields in (
            ({"size": 18, "sha256": expected["sha256"]}, ("size",)),
            ({"size": 17}, ("sha256",)),
            ({"size": 18, "sha256": "different"}, ("size", "sha256")),
        ):
            with self.subTest(actual=actual), self.assertRaises(ValueError) as caught:
                pipeline_setup.require_identity(actual, expected, "Synthetic ISO")
            self.assertIn("Synthetic ISO", str(caught.exception))
            for field in failed_fields:
                self.assertIn(field, str(caught.exception))

    def test_iso_entries_skip_dot_records_and_sector_padding(self):
        first_sector = b"".join([
            iso_directory_record(b"\0", 1, 4096, True),
            iso_directory_record(b"\1", 1, 4096, True),
            iso_directory_record("BOOT.ELF;1", 4, 12),
        ])
        second_sector = iso_directory_record("LEVEL.WAD;1", 6, 15)
        path, directory = self.iso([first_sector.ljust(2048, b"\0"), second_sector], 4096)
        self.assertEqual(pipeline_setup.iso_entries(path, directory), [
            {"sector": 4, "size": 12, "directory": False, "name": "BOOT.ELF"},
            {"sector": 6, "size": 15, "directory": False, "name": "LEVEL.WAD"},
        ])

    def test_iso_entries_preserve_directory_flag(self):
        path, directory = self.iso([iso_directory_record("G", 5, 2048, True)])
        self.assertEqual(pipeline_setup.iso_entries(path, directory), [
            {"sector": 5, "size": 2048, "directory": True, "name": "G"},
        ])

    def test_iso_directory_extent_bounds(self):
        path = self.write("bounds.iso", bytes(4096))
        for directory in (
            {"sector": -1, "size": 34}, {"sector": 2, "size": 1},
            {"sector": 1, "size": 2049}, {"sector": 0xFFFFFFFF, "size": 34},
            {"sector": 0, "size": 16 * 1024 * 1024 + 1},
        ):
            with self.subTest(directory=directory), self.assertRaisesRegex(ValueError, "out of bounds"):
                pipeline_setup.iso_entries(path, directory)

    def test_iso_directory_negative_length_is_rejected(self):
        path = self.write("negative-size.iso", bytes(4096))
        with self.assertRaises(ValueError):
            pipeline_setup.iso_entries(path, {"sector": 1, "size": -1})

    def test_iso_child_extent_outside_file_is_rejected(self):
        for sector, size in ((8, 1), (7, 2049), (0xFFFFFFFF, 1)):
            with self.subTest(sector=sector, size=size):
                path, directory = self.iso([iso_directory_record("BAD;1", sector, size)])
                with self.assertRaisesRegex(ValueError, "ISO file out of bounds"):
                    pipeline_setup.iso_entries(path, directory)

    def test_iso_truncated_directory_record_header_and_name(self):
        complete = iso_directory_record("FILE;1", 4, 12)
        for truncated in (complete[:20], complete[:35]):
            with self.subTest(length=len(truncated)):
                path, directory = self.iso([truncated])
                with self.assertRaises(ValueError):
                    pipeline_setup.iso_entries(path, directory)

    def test_iso_declared_record_length_cannot_exceed_directory(self):
        truncated = iso_directory_record("A", 4, 12)
        truncated[0] += 20
        path, directory = self.iso([truncated])
        with self.assertRaises(ValueError):
            pipeline_setup.iso_entries(path, directory)

    def test_iso_record_cannot_cross_sector_boundary(self):
        first = iso_directory_record("FIRST;1", 4, 12)
        crossing = iso_directory_record("SECOND;1", 5, 12)
        path, directory = self.iso([
            first.ljust(2040, b"\0"), crossing,
        ], 4096)
        image = bytearray(path.read_bytes())
        position = 2048 + len(first)
        while position < 2048 + 2040:
            remaining = 2048 + 2040 - position
            filler_length = min(remaining, 200)
            filler = iso_directory_record(b"\0", 1, 4096, True)
            filler.extend(bytes(filler_length - len(filler)))
            filler[0] = filler_length
            image[position:position + filler_length] = filler
            position += filler_length
        path.write_bytes(image)
        with self.assertRaises(ValueError):
            pipeline_setup.iso_entries(path, directory)

    def test_extract_record_reads_exact_extent_without_adjacent_bytes(self):
        image = bytes(2048) + b"PAYLOAD!TRAILER"
        path = self.write("payload.iso", image)
        destination = self.directory / "payload.bin"
        record = {"sector": 1, "size": 8, "directory": False}
        pipeline_setup.extract_record(path, record, destination)
        self.assertEqual(destination.read_bytes(), b"PAYLOAD!")

    def test_extract_record_accepts_exact_eof_and_empty_file(self):
        path = self.write("payload.iso", bytes(2048) + b"END")
        for size, expected in ((3, b"END"), (0, b"")):
            with self.subTest(size=size):
                destination = self.directory / "payload.bin"
                pipeline_setup.extract_record(path, {"sector": 1, "size": size, "directory": False}, destination)
                self.assertEqual(destination.read_bytes(), expected)

    def test_extract_record_rejects_directory_before_opening_output(self):
        path = self.write("payload.iso", bytes(4096))
        destination = self.directory / "payload.bin"
        with self.assertRaisesRegex(ValueError, "Expected ISO file"):
            pipeline_setup.extract_record(path, {"sector": 1, "size": 10, "directory": True}, destination)
        self.assertFalse(destination.exists())

    def test_extract_record_detects_truncation_and_out_of_bounds_sector(self):
        path = self.write("truncated.iso", bytes(2048) + b"SHORT")
        for sector, size in ((1, 6), (2, 1), (0xFFFFFFFF, 1)):
            with self.subTest(sector=sector, size=size):
                with self.assertRaisesRegex(ValueError, "Truncated ISO file"):
                    pipeline_setup.extract_record(path, {"sector": sector, "size": size, "directory": False}, self.directory / "payload.bin")

    def test_generate_config_two_loads_homonyms_and_both_twin_types(self):
        build_directory, twins, document = self.configured()
        segments = [segment for segment in document["segments"]
                    if isinstance(segment, dict) and segment["type"] != "bin"]
        self.assertEqual([segment["start"] for segment in segments], [0x100, 0x120, 0x140, 0x160, 0x400])
        self.assertEqual([segment["vram"] for segment in segments], [0x400080, 0x4000A0, 0x4000C0, 0x4000E0, 0x900000])
        self.assertEqual([segment["type"] for segment in segments], ["code", "code", "data", "data", "data"])
        self.assertEqual([segment["name"] for segment in segments], ["text_100", "core_text_120", "jump_table_140", "mc1_data_160", "mc1_data_400"])
        self.assertEqual(len({segment["name"] for segment in segments}), len(segments))
        self.assertEqual(twins, {
            "build/asm/100.s.o": "build/asm/text_100.s.o",
            "build/asm/120.s.o": "build/asm/core_text_120.s.o",
            "build/asm/data/140.data.s.o": "build/asm/data/jump_table_140.data.s.o",
            "build/asm/data/160.data.s.o": "build/asm/data/mc1_data_160.data.s.o",
            "build/asm/data/400.data.s.o": "build/asm/data/mc1_data_400.data.s.o",
        })
        self.assertEqual(segments[-1]["bss_size"], 0x20)
        self.assertEqual(document["segments"][-1], [0x430])
        self.assertEqual(document["sha1"], hashlib.sha1(bytes(0x440)).hexdigest())
        self.assertEqual((build_directory / "assets" / "synthetic.elf").read_bytes(), bytes(0x440))
        self.assertEqual((build_directory / "config" / "symbol_addrs.txt").read_bytes(), b"")

    def test_generate_config_offsets_are_derived_from_metadata(self):
        structure = synthetic_structure()
        for segment in structure["segments"]:
            if segment["type"] == 1:
                segment["offset"] += 0x500
        for section in structure["sections"]:
            if section["type"] != 8:
                section["offset"] += 0x500
        build_directory, twins, document = self.configured(structure)
        self.assertIn("build/asm/600.s.o", twins)
        self.assertIn("build/asm/data/900.data.s.o", twins)
        self.assertNotIn("build/asm/100.s.o", twins)
        self.assertEqual(document["segments"][-1], [0x930])

    def test_generate_config_never_maps_gap_between_loads(self):
        structure = synthetic_structure()
        build_directory, twins, document = self.configured(structure)
        entries = document["segments"]
        for index, segment in enumerate(entries):
            if not isinstance(segment, dict):
                continue
            next_entry = entries[index + 1]
            next_start = next_entry["start"] if isinstance(next_entry, dict) else next_entry[0]
            end = segment.get("end", next_start)
            if segment["type"] == "bin":
                self.assertTrue(all(
                    end <= load["offset"] or segment["start"] >= load["offset"] + load["filesz"]
                    for load in structure["segments"] if load["type"] == 1
                ), "Unloaded binary gap must not intersect PT_LOAD file bytes")
                continue
            owners = [load for load in structure["segments"] if load["type"] == 1
                      and load["offset"] <= segment["start"] < load["offset"] + load["filesz"]]
            self.assertEqual(len(owners), 1)
            self.assertLessEqual(end, owners[0]["offset"] + owners[0]["filesz"],
                                 f"{segment['name']} extends into the gap before the next PT_LOAD")

    def test_generate_config_rejects_no_loads_and_uncovered_second_load(self):
        for fault in ("no loads", "missing section", "wrong address", "section crosses end"):
            with self.subTest(fault=fault):
                structure = synthetic_structure()
                if fault == "no loads":
                    structure["segments"] = []
                elif fault == "missing section":
                    structure["sections"] = [section for section in structure["sections"] if section["offset"] != 0x400]
                elif fault == "wrong address":
                    structure["sections"][4]["address"] += 4
                else:
                    structure["sections"][4]["size"] += 1
                reference = self.write(f"{fault}.elf", bytes(0x440))
                directory = self.directory / fault
                directory.mkdir()
                with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
                    with self.assertRaisesRegex(ValueError, "PT_LOAD"):
                        pipeline_build.generate_config(reference, directory, "synthetic")
                self.assertFalse((directory / "assets").exists())

    def test_generate_config_ignores_nobits_offset_outside_file(self):
        structure = synthetic_structure()
        build_directory, twins, document = self.configured(structure)
        self.assertNotIn(0xFFFFFFF0, [segment["start"] for segment in document["segments"] if isinstance(segment, dict)])
        self.assertFalse(any("bss" in target for target in twins.values()))

    def test_data_tails_return_exact_synthetic_remainders_without_changing_input(self):
        for remainder in (1, 2, 3):
            with self.subTest(remainder=remainder):
                structure = synthetic_structure()
                structure["segments"][2]["filesz"] += remainder
                structure["segments"][2]["memsz"] += remainder
                structure["sections"][4]["size"] += remainder
                structure["sections"][5]["address"] += remainder
                image = bytearray(0x440)
                tail = b"XYZ"[:remainder]
                image[0x430:0x430 + remainder] = tail
                reference = self.write("synthetic-tail.elf", image)
                with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
                    self.assertEqual(pipeline_build.data_tails(reference), {"mc1_data_400": tail})
                self.assertEqual(reference.read_bytes(), image)

    def test_data_tails_handle_both_loads_and_duplicate_data_names(self):
        structure = synthetic_structure()
        structure["segments"][1]["filesz"] += 3
        structure["segments"][1]["memsz"] += 3
        structure["sections"][3]["size"] += 3
        structure["segments"][2]["filesz"] += 3
        structure["segments"][2]["memsz"] += 3
        structure["sections"][4]["size"] += 3
        structure["sections"][5]["address"] += 3
        image = bytearray(0x440)
        image[0x180:0x183] = b"ABC"
        image[0x430:0x433] = b"XYZ"
        reference = self.write("two-tails.elf", image)
        with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
            self.assertEqual(pipeline_build.data_tails(reference), {
                "mc1_data_160": b"ABC", "mc1_data_400": b"XYZ",
            })

    def test_data_tails_skip_aligned_loads(self):
        reference = self.write("aligned.elf", bytes(0x440))
        with mock.patch.object(pipeline_build, "read_elf", return_value=synthetic_structure()):
            self.assertEqual(pipeline_build.data_tails(reference), {})

    def test_data_tails_reject_missing_ambiguous_or_code_tail_section(self):
        for fault in ("missing", "ambiguous", "code"):
            with self.subTest(fault=fault):
                structure = synthetic_structure()
                structure["segments"][2]["filesz"] += 3
                structure["segments"][2]["memsz"] += 3
                structure["sections"][4]["size"] += 3
                structure["sections"][5]["address"] += 3
                if fault == "missing":
                    structure["sections"].pop(4)
                elif fault == "ambiguous":
                    structure["sections"].append(copy.deepcopy(structure["sections"][4]))
                else:
                    structure["sections"][4]["name"] = ".text"
                reference = self.write("invalid-tail.elf", bytes(0x440))
                with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
                    with self.assertRaisesRegex(ValueError, "uniquely identified data section"):
                        pipeline_build.data_tails(reference)

    def test_adapt_linker_inserts_three_tail_bytes_inside_own_data_section(self):
        content = """SECTIONS
{
  .text_100 0x400080 :
  {
    *(.text)
    text_100_DATA_END = .;
  }
  .mc1_data_400 0x900000 :
  {
    *(.data)
    mc1_data_400_DATA_END = .;
  }
}
"""
        directory, script = self.linker_fixture(content)
        reference = directory / "assets" / "synthetic.elf"
        image = bytearray(0x440)
        image[0x430:0x433] = b"XYZ"
        reference.write_bytes(image)
        structure = synthetic_structure()
        structure["segments"][2]["filesz"] += 3
        structure["segments"][2]["memsz"] += 3
        structure["sections"][4]["size"] += 3
        structure["sections"][5]["address"] += 3
        with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
            pipeline_build.adapt_linker(directory, {})
        rewritten = script.read_text(encoding="ascii")
        data_body = re.search(r"(?s)\.mc1_data_400[^{}]*\{([^}]*)\}\s*:load1", rewritten).group(1)
        code_body = re.search(r"(?s)\.text_100[^{}]*\{([^}]*)\}\s*:load0", rewritten).group(1)
        self.assertEqual(re.findall(r"BYTE\(0x([0-9A-F]+)\);", data_body), ["58", "59", "5A"])
        self.assertLess(data_body.index("BYTE(0x5A);"), data_body.index("mc1_data_400_DATA_END = .;"))
        self.assertNotIn("BYTE(", code_body)
        self.assertEqual(reference.read_bytes(), image)

    def test_adapt_linker_rejects_missing_or_duplicate_tail_marker(self):
        for count in (0, 2):
            with self.subTest(marker_count=count):
                markers = "    mc1_data_400_DATA_END = .;\n" * count
                content = "SECTIONS\n{\n  .mc1_data_400 0x900000 :\n  {\n" + markers + "  }\n}\n"
                directory, script = self.linker_fixture(content, name=f"markers-{count}")
                with mock.patch.object(pipeline_build, "data_tails", return_value={"mc1_data_400": b"XYZ"}):
                    with self.assertRaisesRegex(ValueError, "unique linker data-tail insertion point"):
                        pipeline_build.adapt_linker(directory, {})
                self.assertEqual(script.read_text(encoding="ascii"), content)

    def test_adapt_linker_removes_unloaded_gap_blocks_before_assigning_loads(self):
        content = """SECTIONS
{
  .text_100 0x400080 :
  {
    *(.text)
  }
  unloaded_gap_180_ROM_START = .;
  .unloaded_gap_180 0x0 :
  {
    build/assets/unloaded_gap_180.bin.o(.data)
  }
  unloaded_gap_180_VRAM_END = .;
  .mc1_data_400 0x900000 :
  {
    *(.data)
  }
}
"""
        directory, script = self.linker_fixture(content)
        with mock.patch.object(pipeline_build, "read_elf", return_value=synthetic_structure()):
            pipeline_build.adapt_linker(directory, {})
        rewritten = script.read_text(encoding="ascii")
        self.assertNotIn("unloaded_gap_180", rewritten)
        self.assertEqual(re.findall(r"}\s*:(load\d+)", rewritten), ["load0", "load1"])

    def test_adapt_linker_assigns_each_load_with_its_flags_and_dynamic_addresses(self):
        content = """SECTIONS
{
  .text_100 0x400080 :
  {
    build/asm/100.s.o(.text)
    . = ALIGN(16);
    text_100_END = .;
  }
  .mc1_data_400 0x900000 : SUBALIGN(16)
  {
    build/asm/data/400.data.s.o(.data)
    HIDDEN(data_END = .);
  }
  .bss_430 0x900030 :
  {
    *(.bss)
  }
}
"""
        directory, script = self.linker_fixture(content, ("text_100.s", "data/mc1_data_400.data.s"))
        twins = {
            "build/asm/100.s.o": "build/asm/text_100.s.o",
            "build/asm/data/400.data.s.o": "build/asm/data/mc1_data_400.data.s.o",
        }
        with mock.patch.object(pipeline_build, "read_elf", return_value=synthetic_structure()):
            pipeline_build.adapt_linker(directory, twins)
        rewritten = script.read_text(encoding="ascii")
        self.assertTrue(rewritten.startswith("INCLUDE config/undefined_symbols.ld\nPHDRS\n"))
        self.assertEqual(re.findall(r"load(\d+) PT_LOAD FLAGS\((\d+)\);", rewritten), [("0", "7"), ("1", "6")])
        for section_name, segment_name in (("text_100", "load0"), ("mc1_data_400", "load1"), ("bss_430", "load1")):
            self.assertRegex(rewritten, rf"(?s)\.{section_name}\s+0x[0-9A-Fa-f]+\s*:[^{{]*\{{[^}}]*\}}\s*:{segment_name}")
        self.assertIn("build/asm/text_100.s.o(.text)", rewritten)
        self.assertIn("build/asm/data/mc1_data_400.data.s.o(.data)", rewritten)
        self.assertNotIn("build/asm/100.s.o", rewritten)
        self.assertNotIn("build/asm/data/400.data.s.o", rewritten)
        self.assertNotIn("SUBALIGN", rewritten)
        self.assertNotIn("HIDDEN", rewritten)
        self.assertNotIn(". = ALIGN(16);", rewritten)

    def test_adapt_linker_keeps_offset_object_if_named_twin_is_missing(self):
        content = """SECTIONS
{
  .text_100 0x400080 :
  {
    build/asm/100.s.o(.text)
  }
}
"""
        directory, script = self.linker_fixture(content)
        with mock.patch.object(pipeline_build, "read_elf", return_value=synthetic_structure()):
            pipeline_build.adapt_linker(directory, {"build/asm/100.s.o": "build/asm/text_100.s.o"})
        self.assertIn("build/asm/100.s.o(.text)", script.read_text(encoding="ascii"))

    def test_adapt_linker_rejects_unmapped_or_ambiguous_output_section(self):
        for ambiguous in (False, True):
            with self.subTest(ambiguous=ambiguous):
                structure = synthetic_structure()
                address = 0x400080 if ambiguous else 0x800000
                if ambiguous:
                    structure["segments"].append(copy.deepcopy(structure["segments"][1]))
                directory = self.directory / str(ambiguous)
                (directory / "config").mkdir(parents=True)
                (directory / "assets").mkdir()
                (directory / "assets" / "synthetic.elf").write_bytes(b"synthetic")
                script = directory / "config" / "rac2.ld"
                original = f"SECTIONS\n{{\n  .outside 0x{address:X} :\n  {{\n    *(.text)\n  }}\n}}\n"
                script.write_text(original, encoding="ascii")
                with mock.patch.object(pipeline_build, "read_elf", return_value=structure):
                    with self.assertRaisesRegex(ValueError, "outside unique PT_LOAD"):
                        pipeline_build.adapt_linker(directory, {})
                self.assertEqual(script.read_text(encoding="ascii"), original)


if __name__ == "__main__":
    unittest.main()

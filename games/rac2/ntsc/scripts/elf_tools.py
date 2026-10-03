"""Strict ELF32 little-endian MIPS inspection and byte comparison."""

from __future__ import annotations

import hashlib
import os
import stat
import struct
from collections.abc import Iterable
from pathlib import Path


ET_REL = 1
ET_EXEC = 2
EM_MIPS = 8
PT_LOAD = 1
SHT_NULL = 0
SHT_STRTAB = 3
SHT_NOBITS = 8
SHF_EXECINSTR = 4

_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
_SECTION = struct.Struct("<IIIIIIIIII")
_SEGMENT = struct.Struct("<IIIIIIII")
_ADDRESS_LIMIT = 1 << 32
_EXTENDED_INDEX = 0xFFFF


def _file_span(offset: int, size: int, length: int, label: str) -> None:
    if offset > length or size > length - offset:
        raise ValueError(f"{label} outside file bounds")


def _address_span(address: int, size: int, label: str) -> None:
    if address < 0 or address >= _ADDRESS_LIMIT or size > _ADDRESS_LIMIT - address:
        raise ValueError(f"{label} outside ELF32 address space")


def _alignment(alignment: int, label: str) -> None:
    if alignment > 1 and alignment & (alignment - 1):
        raise ValueError(f"{label} alignment is not a power of two")


def _table(
    offset: int, count: int, stride: int, minimum: int, length: int, label: str
) -> None:
    if not count:
        if offset:
            raise ValueError(f"{label} offset without entries")
        return
    if offset < _HEADER.size or stride < minimum:
        raise ValueError(f"Invalid {label} offset or entry size")
    _file_span(offset, count * stride, length, label)


def _parse(data: bytes) -> dict:
    if len(data) < _HEADER.size:
        raise ValueError("Truncated ELF header")
    (
        identification, elf_type, machine, version, entry, program_offset,
        section_offset, header_flags, header_size, program_stride, program_count,
        section_stride, section_count, names_index,
    ) = _HEADER.unpack_from(data)
    if identification[:4] != b"\x7fELF":
        raise ValueError("Invalid ELF magic")
    if identification[4] != 1 or identification[5] != 1:
        raise ValueError("Expected ELF32 little-endian encoding")
    if identification[6] != 1 or version != 1:
        raise ValueError("Unsupported ELF version")
    if elf_type not in (ET_REL, ET_EXEC):
        raise ValueError("Expected ET_REL or ET_EXEC")
    if machine != EM_MIPS:
        raise ValueError("Expected MIPS machine")
    if header_size < _HEADER.size:
        raise ValueError("Invalid ELF header size")
    _file_span(0, header_size, len(data), "ELF header")

    if section_offset:
        _table(section_offset, 1, section_stride, _SECTION.size, len(data), "sections")
        initial_section = _SECTION.unpack_from(data, section_offset)
        if initial_section[1] != SHT_NULL:
            raise ValueError("Section zero must be SHT_NULL")
        if not section_count:
            section_count = initial_section[5]
            if not section_count:
                raise ValueError("Missing extended section count")
        if names_index == _EXTENDED_INDEX:
            names_index = initial_section[6]
        if program_count == _EXTENDED_INDEX:
            program_count = initial_section[7]
            if not program_count:
                raise ValueError("Missing extended program count")
    elif section_count or names_index or program_count == _EXTENDED_INDEX:
        raise ValueError("Missing section table")

    _table(
        section_offset, section_count, section_stride, _SECTION.size,
        len(data), "sections",
    )
    _table(
        program_offset, program_count, program_stride, _SEGMENT.size,
        len(data), "segments",
    )
    raw_sections = [
        _SECTION.unpack_from(data, section_offset + index * section_stride)
        for index in range(section_count)
    ]
    sections = []
    for index, section in enumerate(raw_sections):
        (
            name_offset, section_type, flags, address, offset, size,
            link, info, alignment, entry_size,
        ) = section
        if index:
            _alignment(alignment, f"Section {index}")
            _address_span(address, size, f"Section {index}")
            if section_type != SHT_NOBITS:
                _file_span(offset, size, len(data), f"Section {index}")
        sections.append({
            "name": "", "type": section_type, "flags": flags, "address": address,
            "offset": offset, "size": size, "alignment": alignment,
        })

    if names_index:
        if names_index >= section_count:
            raise ValueError("Section name table index outside sections")
        names_section = sections[names_index]
        if names_section["type"] != SHT_STRTAB:
            raise ValueError("Section name table must be SHT_STRTAB")
        names_start = names_section["offset"]
        names = data[names_start:names_start + names_section["size"]]
        if not names or names[0] != 0 or names[-1] != 0:
            raise ValueError("Invalid section name string table")
        for section, raw_section in zip(sections, raw_sections):
            name_offset = raw_section[0]
            if name_offset >= len(names):
                raise ValueError("Section name offset outside string table")
            name_end = names.find(b"\0", name_offset)
            section["name"] = names[name_offset:name_end].decode(
                "utf-8", errors="surrogateescape"
            )
    elif any(section[0] for section in raw_sections):
        raise ValueError("Section names without a string table")

    segments = []
    for index in range(program_count):
        (
            segment_type, offset, address, physical_address, filesz, memsz,
            flags, alignment,
        ) = _SEGMENT.unpack_from(data, program_offset + index * program_stride)
        _file_span(offset, filesz, len(data), f"Segment {index}")
        _address_span(address, memsz, f"Segment {index}")
        _alignment(alignment, f"Segment {index}")
        if segment_type == PT_LOAD:
            if filesz > memsz:
                raise ValueError(f"PT_LOAD {index} filesz exceeds memsz")
        segments.append({
            "type": segment_type, "offset": offset, "address": address,
            "filesz": filesz, "memsz": memsz, "flags": flags,
            "alignment": alignment,
        })
    return {
        "sha256": hashlib.sha256(data).hexdigest(), "entry": entry,
        "type": elf_type, "machine": machine, "sections": sections,
        "segments": segments,
    }


def read_elf(path: str | os.PathLike[str]) -> dict:
    """Read a validated ELF snapshot; malformed ELF raises ValueError.

    SDK ELF files may have incongruent PT_LOAD addresses and file offsets.
    Declared alignment is retained, while file and memory bounds remain strict.
    """
    return _parse(Path(path).read_bytes())


def _mappings(elf: dict) -> list[dict]:
    if elf["type"] == ET_EXEC:
        return [segment for segment in elf["segments"] if segment["type"] == PT_LOAD]
    return [
        {
            "address": section["address"], "offset": section["offset"],
            "memsz": section["size"],
            "filesz": 0 if section["type"] == SHT_NOBITS else section["size"],
        }
        for section in elf["sections"]
        if section["flags"] & SHF_EXECINSTR and section["type"] != SHT_NULL
    ]


def _mapped_bytes(data: bytes, mappings: list[dict], address: int, size: int) -> bytes:
    end = address + size
    boundaries = {address, end}
    relevant = []
    for mapping in mappings:
        start = mapping["address"]
        memory_end = start + mapping["memsz"]
        if start < end and address < memory_end:
            relevant.append(mapping)
            for boundary in (start, start + mapping["filesz"], memory_end):
                if address < boundary < end:
                    boundaries.add(boundary)
    ordered = sorted(boundaries)
    chunks = []
    for start, stop in zip(ordered, ordered[1:]):
        owners = [
            mapping for mapping in relevant
            if mapping["address"] <= start < mapping["address"] + mapping["memsz"]
        ]
        if not owners:
            raise ValueError("Address range outside mapped memory")
        if len(owners) != 1:
            raise ValueError("Ambiguous address range")
        mapping = owners[0]
        relative = start - mapping["address"]
        if stop - mapping["address"] > mapping["filesz"]:
            raise ValueError("Address range is not file-backed")
        offset = mapping["offset"] + relative
        chunks.append(data[offset:offset + stop - start])
    return b"".join(chunks)


def address_bytes(path: str | os.PathLike[str], address: int, size: int) -> bytes:
    """Read uniquely backed bytes from PT_LOAD, or executable ET_REL sections.

    ET_REL section addresses are unrelocated; overlapping executable sections
    are ambiguous. Adjacent uniquely backed mappings may span one request.
    """
    if type(address) is not int or type(size) is not int or size <= 0:
        raise ValueError("Address and positive size must be integers")
    _address_span(address, size, "Requested range")
    data = Path(path).read_bytes()
    elf = _parse(data)
    return _mapped_bytes(data, _mappings(elf), address, size)


def compare_loads(
    reference: str | os.PathLike[str], candidate: str | os.PathLike[str]
) -> dict:
    """Compare all load segments of distinct ET_EXEC snapshots.

    The same file and whole-file identical copies are inconclusive (ValueError).
    Nonloaded metadata and file offsets may differ. bytes_compared counts bytes
    examined through the first differing byte, or all file-backed load bytes.
    """
    reference_path = Path(reference)
    candidate_path = Path(candidate)
    if reference_path.samefile(candidate_path):
        raise ValueError("Inconclusive comparison: reference and candidate are the same file")
    reference_data = reference_path.read_bytes()
    candidate_data = candidate_path.read_bytes()
    reference_elf = _parse(reference_data)
    candidate_elf = _parse(candidate_data)
    if reference_data == candidate_data:
        raise ValueError("Inconclusive comparison: identical ELF files")

    def result(matched: bool, bytes_compared: int, reason: str) -> dict:
        return {"matched": matched, "bytes_compared": bytes_compared, "reason": reason}

    if reference_elf["type"] != ET_EXEC or candidate_elf["type"] != ET_EXEC:
        return result(False, 0, "Both files must be ET_EXEC")
    reference_loads = _mappings(reference_elf)
    candidate_loads = _mappings(candidate_elf)
    if not reference_loads or not candidate_loads:
        return result(False, 0, "Both files must contain PT_LOAD segments")
    if reference_elf["entry"] != candidate_elf["entry"]:
        return result(False, 0, "Entry point mismatch")
    if len(reference_loads) != len(candidate_loads):
        return result(False, 0, "PT_LOAD segment count mismatch")
    fields = ("address", "filesz", "memsz", "flags")
    reference_loads = sorted(reference_loads, key=lambda segment: segment["address"])
    candidate_loads = sorted(candidate_loads, key=lambda segment: segment["address"])
    for index, (reference_load, candidate_load) in enumerate(
        zip(reference_loads, candidate_loads)
    ):
        for field in fields:
            if reference_load[field] != candidate_load[field]:
                return result(False, 0, f"PT_LOAD {index} {field} mismatch")
    for label, loads in (("Reference", reference_loads), ("Candidate", candidate_loads)):
        previous_end = 0
        for segment in loads:
            if not segment["memsz"]:
                continue
            if segment["address"] < previous_end:
                return result(False, 0, f"{label} PT_LOAD memory ranges overlap")
            previous_end = segment["address"] + segment["memsz"]
    if not any(segment["filesz"] for segment in reference_loads):
        return result(False, 0, "PT_LOAD segments contain no file-backed bytes")

    bytes_compared = 0
    for index, (reference_load, candidate_load) in enumerate(
        zip(reference_loads, candidate_loads)
    ):
        size = reference_load["filesz"]
        reference_offset = reference_load["offset"]
        candidate_offset = candidate_load["offset"]
        reference_bytes = reference_data[reference_offset:reference_offset + size]
        candidate_bytes = candidate_data[candidate_offset:candidate_offset + size]
        if reference_bytes != candidate_bytes:
            mismatch = next(
                offset for offset, (reference_byte, candidate_byte)
                in enumerate(zip(reference_bytes, candidate_bytes))
                if reference_byte != candidate_byte
            )
            return result(
                False, bytes_compared + mismatch + 1,
                f"PT_LOAD {index} content mismatch at address "
                f"0x{reference_load['address'] + mismatch:08x}",
            )
        bytes_compared += size
    return result(True, bytes_compared, "All PT_LOAD bytes and metadata match")


def assert_fresh(
    candidate: str | os.PathLike[str], inputs: Iterable[str | os.PathLike[str]]
) -> None:
    """Require existing regular files and candidate mtime >= every input mtime."""
    def modified_time(path: str | os.PathLike[str], label: str) -> int:
        try:
            file_path = Path(path)
            status = file_path.stat()
            if not stat.S_ISREG(status.st_mode):
                raise ValueError(f"{label} must be a regular file")
            return status.st_mtime_ns
        except OSError as error:
            raise ValueError(f"{label} missing or inaccessible") from error

    candidate_time = modified_time(candidate, "Candidate")
    for input_path in inputs:
        if candidate_time < modified_time(input_path, "Input"):
            raise ValueError("Candidate is older than an input")

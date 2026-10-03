from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import uuid
import zlib
from datetime import datetime, timezone
from pathlib import Path

from elf_tools import read_elf

ROOT = Path(__file__).resolve().parents[1]
TARGET = json.loads((ROOT / "config" / "target.json").read_text(encoding="utf-8"))


def hashes(path: Path) -> dict:
    digests = {name: hashlib.new(name) for name in ("sha1", "md5", "sha256")}
    checksum = 0
    with path.open("rb") as stream:
        while block := stream.read(16 * 1024 * 1024):
            for digest in digests.values():
                digest.update(block)
            checksum = zlib.crc32(block, checksum)
    return {"size": path.stat().st_size, "crc32": f"{checksum:08x}",
            **{name: digest.hexdigest() for name, digest in digests.items()}}


def require_identity(actual: dict, expected: dict, label: str) -> None:
    failures = [name for name, value in expected.items() if actual.get(name) != value]
    if failures:
        raise ValueError(f"{label}: wrong {', '.join(failures)}")


def command(arguments: list[str], log: Path, allow_warning: bool = False) -> None:
    with log.open("wb") as stream:
        try:
            result = subprocess.run(arguments, stdout=stream, stderr=subprocess.STDOUT, timeout=900)
        except subprocess.TimeoutExpired as error:
            raise ValueError(f"Tool timed out; see {log}") from error
    accepted = (0, 1) if allow_warning else (0,)
    if result.returncode not in accepted:
        raise ValueError(f"Tool failed with code {result.returncode}; see {log}")


def iso_record(record: bytes) -> dict:
    if len(record) < 34 or len(record) < 33 + record[32]:
        raise ValueError("Invalid ISO9660 directory record")
    return {"sector": struct.unpack_from("<I", record, 2)[0],
            "size": struct.unpack_from("<I", record, 10)[0],
            "directory": bool(record[25] & 2),
            "name": record[33:33 + record[32]].decode("ascii").split(";")[0]}


def iso_entries(iso: Path, directory: dict) -> list[dict]:
    offset = directory["sector"] * 2048
    length = directory["size"]
    if offset < 0 or length < 0 or offset + length > iso.stat().st_size or length > 16 * 1024 * 1024:
        raise ValueError("ISO directory out of bounds")
    with iso.open("rb") as stream:
        stream.seek(offset)
        content = stream.read(length)
    entries = []
    position = 0
    while position < len(content):
        record_size = content[position]
        if not record_size:
            position = (position // 2048 + 1) * 2048
            continue
        if position + record_size > len(content) or position % 2048 + record_size > 2048:
            raise ValueError("Invalid ISO9660 directory record out of bounds")
        record = iso_record(content[position:position + record_size])
        if record["name"] not in ("\x00", "\x01"):
            if record["sector"] * 2048 + record["size"] > iso.stat().st_size:
                raise ValueError("ISO file out of bounds")
            entries.append(record)
        position += record_size
    return entries


def extract_record(iso: Path, record: dict, destination: Path) -> None:
    if record["directory"]:
        raise ValueError("Expected ISO file")
    with iso.open("rb") as stream, destination.open("wb") as output:
        stream.seek(record["sector"] * 2048)
        remaining = record["size"]
        while remaining:
            block = stream.read(min(remaining, 16 * 1024 * 1024))
            if not block:
                raise ValueError("Truncated ISO file")
            output.write(block)
            remaining -= len(block)


def find_entry(entries: list[dict], name: str) -> dict:
    matches = [entry for entry in entries if entry["name"] == name]
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one ISO entry {name}")
    return matches[0]


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify and prepare RAC2 v1.01 locally")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--iso", type=Path)
    source.add_argument("--archive", type=Path)
    parser.add_argument("--runtime", required=True, type=Path)
    parser.add_argument("--sevenzip", type=Path)
    parser.add_argument("--wrench", type=Path)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    if runtime == ROOT or runtime in ROOT.parents or ROOT in runtime.parents:
        raise ValueError("Runtime must be outside the source repository")
    run = runtime / "runs" / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8])
    run.mkdir(parents=True)
    iso = args.iso.resolve() if args.iso else None
    if args.archive:
        if not args.sevenzip or not args.sevenzip.is_file():
            raise ValueError("--sevenzip is required for an archive")
        inputs = run / "inputs"
        inputs.mkdir()
        print("Extracting source archive", flush=True)
        command([str(args.sevenzip), "e", str(args.archive.resolve()), "*.iso", f"-o{inputs}", "-y"],
                run / "extract.log", allow_warning=True)
        images = list(inputs.glob("*.iso"))
        if len(images) != 1:
            raise ValueError("Archive must contain exactly one ISO")
        iso = images[0]
    if iso is None or not iso.is_file():
        raise ValueError("ISO missing")
    print("Verifying entire ISO: size, SHA-1, MD5, CRC32, SHA-256", flush=True)
    iso_hashes = hashes(iso)
    require_identity(iso_hashes, TARGET["iso"], "ISO")
    with iso.open("rb") as stream:
        stream.seek(16 * 2048)
        descriptor = stream.read(2048)
    if descriptor[:7] != b"\x01CD001\x01":
        raise ValueError("Expected ISO9660 primary volume descriptor")
    label = descriptor[40:72].decode("ascii").strip()
    if label != TARGET["volume_label"]:
        raise ValueError("Wrong volume label")
    root = iso_entries(iso, iso_record(descriptor[156:156 + descriptor[156]]))
    reference = run / "reference"
    reference.mkdir()
    cnf = reference / "SYSTEM.CNF"
    extract_record(iso, find_entry(root, "SYSTEM.CNF"), cnf)
    cnf_text = cnf.read_text(encoding="ascii")
    if not re.search(r"BOOT2\s*=\s*cdrom0:\\" + re.escape(TARGET["serial"]) + r";1", cnf_text):
        raise ValueError("Wrong boot serial")
    if not re.search(r"VER\s*=\s*1\.01\b", cnf_text):
        raise ValueError("Wrong SYSTEM.CNF version")
    boot = reference / "boot.elf"
    extract_record(iso, find_entry(root, TARGET["serial"]), boot)
    boot_hashes = hashes(boot)
    require_identity(boot_hashes, TARGET["boot"], "boot ELF")
    structure = read_elf(boot)
    (reference / "boot-structure.json").write_text(json.dumps(structure, indent=2) + "\n", encoding="utf-8")
    game_files = iso_entries(iso, find_entry(root, "G"))
    level_files = sorted(entry["name"] for entry in game_files if re.fullmatch(r"LEVEL\d+\.WAD", entry["name"]))
    if len(level_files) != TARGET["expected_levels"]:
        raise ValueError("Unexpected LEVEL WAD count")
    gp_sections = [section for section in structure["sections"] if section["name"] == ".reginfo"]
    if len(gp_sections) != 1 or gp_sections[0]["size"] != 24:
        raise ValueError("Missing or malformed MIPS .reginfo")
    gp = struct.unpack_from("<I", boot.read_bytes(), gp_sections[0]["offset"] + 20)[0]
    manifest = {"schema": 1, "verified_at": datetime.now(timezone.utc).isoformat(),
                "target": TARGET["serial"], "iso": {"path": str(iso), **iso_hashes},
                "boot": {"path": str(boot), **boot_hashes, "entry": structure["entry"], "gp": gp},
                "level_wads": level_files, "overlays": [], "g1": "not_run", "g3": "not_run",
                "decompiled_functions": 0, "compiler_flags": None}
    if args.wrench:
        if not args.wrench.is_file():
            raise ValueError("Wrench executable missing")
        print("Extracting RAC2 with Wrench", flush=True)
        destination = run / "unpacked"
        command([str(args.wrench.resolve()), "unpack", str(iso), "-o", str(destination), "-s"],
                run / "wrench.log")
        overlays = sorted(destination.glob("*/levels/*/overlay.elf"))
        if len(overlays) != TARGET["expected_levels"]:
            raise ValueError("Unexpected overlay count")
        extracted_boots = list(destination.glob("*/boot_elf.elf"))
        if len(extracted_boots) != 1 or extracted_boots[0].read_bytes() != boot.read_bytes():
            raise ValueError("Wrench boot disagrees with ISO boot")
        for overlay in overlays:
            overlay_info = read_elf(overlay)
            if not any(segment["type"] == 1 for segment in overlay_info["segments"]):
                raise ValueError(f"Overlay has no PT_LOAD: {overlay.parent.name}")
            private_copy = reference / "levels" / overlay.parent.name / "overlay.elf"
            private_copy.parent.mkdir(parents=True)
            shutil.copy2(overlay, private_copy)
            manifest["overlays"].append({"level": overlay.parent.name, "path": str(private_copy),
                                          "sha256": overlay_info["sha256"]})
        manifest["wrench"] = {"sha256": hashes(args.wrench)["sha256"], "exit_code": 0}
        baseline_path = ROOT / "config" / "overlays.json"
        if baseline_path.is_file():
            baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
            expected = {entry["level"]: entry["sha256"] for entry in baseline["levels"]}
            actual = {entry["level"]: entry["sha256"] for entry in manifest["overlays"]}
            if actual != expected:
                raise ValueError("Extracted overlays differ from the pinned RAC2 baseline")
    manifest_path = run / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    (runtime / "latest.json").write_text(json.dumps({"manifest": str(manifest_path)}, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"manifest": str(manifest_path), "boot_sha256": boot_hashes["sha256"],
                      "gp": f"0x{gp:08X}", "level_wads": len(level_files),
                      "overlays": len(manifest["overlays"])}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error) as error:
        print(f"Preparation failed: {error}")
        raise SystemExit(2)

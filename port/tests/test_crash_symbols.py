# SPDX-License-Identifier: GPL-3.0-or-later
"""Run a tiny Windows executable with local, missing and mismatched PDBs.

Usage: python test_crash_symbols.py SYMBOL_TEST_EXE OTHER_TEST_EXE
Only temporary copies are changed; original build artifacts remain untouched.
"""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile


def relocate_debug_path(data):
    """Remove the build's absolute CodeView path, requiring adjacent-PDB discovery."""
    data = bytearray(data)
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[:2] == b'MZ' and data[pe:pe + 4] == b'PE\0\0'
    sections = struct.unpack_from('<H', data, pe + 6)[0]
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    magic = struct.unpack_from('<H', data, optional)[0]
    assert magic in (0x10b, 0x20b)
    directory = optional + (112 if magic == 0x20b else 96) + 6 * 8
    debug_rva, debug_size = struct.unpack_from('<II', data, directory)
    for section in range(sections):
        at = optional + optional_size + section * 40
        virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', data, at + 8)
        if rva <= debug_rva < rva + max(virtual_size, raw_size):
            debug = raw + debug_rva - rva
            break
    else:
        raise AssertionError('no debug directory')
    for at in range(debug, debug + debug_size, 28):
        kind, size, _, pointer = struct.unpack_from('<IIII', data, at + 12)
        if kind != 2 or data[pointer:pointer + 4] != b'RSDS':
            continue
        start = pointer + 24
        end = data.index(0, start, pointer + size)
        name = bytes(data[start:end]).replace(b'\\', b'/').split(b'/')[-1]
        # A basename fits even builds using /PDBALTPATH or very short build paths.
        # The unrelated working directory contains no PDB, so only the moved pair works.
        data[start:end] = name.ljust(end - start, b'\0')
        return data
    raise AssertionError('no RSDS PDB record')


def main(binary, other):
    binary, other = Path(binary).resolve(), Path(other).resolve()
    assert binary.with_suffix('.pdb').is_file(), 'test build must produce a PDB'
    assert other.with_suffix('.pdb').is_file(), 'mismatch fixture must produce a PDB'
    image = relocate_debug_path(binary.read_bytes())
    with tempfile.TemporaryDirectory(prefix='openrac symbols ') as temporary:
        root = Path(temporary)
        cwd = root / 'unrelated working directory'
        cwd.mkdir()
        for mode in ('local', 'missing', 'mismatched'):
            folder = root / (mode + ' moved build \u00e9')
            folder.mkdir()
            executable = folder / binary.name
            executable.write_bytes(image)
            if mode != 'missing':
                source = binary if mode == 'local' else other
                shutil.copyfile(source.with_suffix('.pdb'), executable.with_suffix('.pdb'))
            env = {key: value for key, value in os.environ.items()
                   if key not in ('_NT_SYMBOL_PATH', '_NT_ALT_SYMBOL_PATH')}
            args = [str(executable)] + ([] if mode == 'local' else ['--expect-missing'])
            result = subprocess.run(args, cwd=cwd, env=env, capture_output=True,
                                    text=True, errors='replace', timeout=30)
            assert result.returncode == 0, f'{mode}: {result.stdout}\n{result.stderr}'
            print(f'{mode}: passed')


if __name__ == '__main__':
    main(*sys.argv[1:])

# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare Novalis water from exported disc inputs, without starting the game."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile

import bake_water

ROOT = next(p for p in Path(__file__).resolve().parents if (p/'editor/formats.py').is_file())
sys.path.insert(1, str(ROOT/'editor'))
from formats import overlay_sections, wad  # noqa: E402


def overlay_memory(raw):
    """Load authored tables and apply only the first-use changes affecting rest geometry."""
    data = wad(raw) if raw[:3] == b'WAD' else raw
    memory = bytearray(32 << 20)
    sections = overlay_sections(data)['sections']
    for section in sections:
        start, size, offset = section['address'], section['bytes'], section['offset']
        memory[start:start+size] = data[offset:offset+size]
    # update_pool_tiles_and_effects sets the pond's 16x16 grid spacing on first use.
    struct.pack_into('<6f', memory, 0x1cad00, 16, 16, -8, -8, 1, 1)
    # update_scroll_layers_and_queue_draw replaces both endpoint heights from Z - 0.45.
    # Its opposite ripple offsets cancel in the rest-height midpoint used by the bake.
    for layer in range(4):
        record = 0x1fbc80 + layer*0x60
        pointer, = struct.unpack_from('<I', memory, record)
        count, = struct.unpack_from('<I', memory, record+12)
        if not 3 <= count <= 4096 or pointer+count*16 > len(memory):
            raise ValueError('invalid Novalis water layer')
        for vertex in range(count):
            address = pointer+vertex*16+8
            height, = struct.unpack_from('<f', memory, address)
            struct.pack_into('<2f', memory, address, height-.45, height-.45)
    return memory


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def source_overlay(level, disc):
    """Older exports can recover this same immutable input from their matching disc."""
    path = level/'bake-inputs/overlay.bin'
    if path.is_file():
        return path
    if disc is None:
        raise ValueError('missing Novalis water input; re-extract the level or pass --disc')
    from disc import Disc
    with Disc(disc) as source:
        if source.version.key != 'ntsc':
            raise ValueError('Novalis water requires an NTSC disc')
        info = source.survey()['levels'][1]['ranges']['data']
        data = source.sectors(info['lba'], info['sectors'])
        offset, size = struct.unpack_from('<II', data)
        if not size or offset+size > len(data):
            raise ValueError('invalid Novalis overlay range')
        path.parent.mkdir(exist_ok=True)
        path.write_bytes(data[offset:offset+size])
    return path


def prepare(level, manifest, disc=None):
    """Build and cache water beside its level, before GI or reflection preparation."""
    if (manifest.get('game'), manifest.get('version'), manifest.get('level')) != ('rac1', 'ntsc', 1):
        return
    tag = manifest.get('bake_inputs', {}).get('water')
    if tag not in (None, 'rac1-ntsc-novalis-v1'):
        raise ValueError('unsupported Novalis water input')
    overlay = source_overlay(level, disc)
    output = level/'water.bin'
    mesh = level/'water.mesh'
    stamp = level/'water-bake.json'
    inputs = {name: digest(path) for name, path in (
        ('overlay', overlay), ('collision', level/'collision.glb'),
        ('boundaries', level/'water-boundaries.json'),
        ('baker', Path(bake_water.__file__)), ('preparation', Path(__file__)))}
    try:
        previous = json.loads(stamp.read_text()) if stamp.is_file() else {}
    except ValueError:
        previous = {}
    if not isinstance(previous, dict):
        previous = {}
    if (previous.get('version') == 1 and previous.get('inputs') == inputs
            and output.is_file() and mesh.is_file()
            and previous.get('outputs') == [digest(output), digest(mesh)]):
        return
    memory = overlay_memory(overlay.read_bytes())
    # Publish only after both files have been produced; their shared fingerprint rejects a torn pair.
    with tempfile.TemporaryDirectory(prefix='.water-', dir=level) as temporary:
        staged = Path(temporary)/'water.bin'
        bake_water.bake(memory, level/'collision.glb', staged,
                        boundaries=level/'water-boundaries.json' if (level/'water-boundaries.json').is_file() else None)
        staged.replace(output)
        staged.with_suffix('.mesh').replace(mesh)
    stamp.write_text(json.dumps({'version': 1, 'inputs': inputs,
                                 'outputs': [digest(output), digest(mesh)]}, indent=2)+'\n')

# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare Novalis water from exported disc inputs, without starting the game."""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import subprocess

import bake_water

from water_sources import overlay_memory


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


def prepare(level, manifest, disc=None, *, baker=None):
    """Build and cache water beside its level, before GI or reflection preparation."""
    if (manifest.get('game'), manifest.get('version'), manifest.get('level')) != ('rac1', 'ntsc', 1):
        return
    tag = manifest.get('bake_inputs', {}).get('water')
    if tag not in (None, 'profile-v1'):
        raise ValueError('unsupported Novalis water input')
    overlay = source_overlay(level, disc)
    output = level/'water.bin'
    mesh = level/'water.mesh'
    stamp = level/'water-bake.json'
    inputs = {name: digest(path) for name, path in (
        ('overlay', overlay), ('collision', level/'collision.glb'),
        ('boundaries', level/'water-boundaries.json'),
        ('baker', Path(bake_water.__file__)), ('preparation', Path(__file__)))}
    if baker is not None:
        inputs['native'] = json.loads(subprocess.check_output(
            [str(baker), '--print-water-profile'], text=True))
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
        boundaries = level/'water-boundaries.json' if (level/'water-boundaries.json').is_file() else None
        if baker is None:
            bake_water.bake(memory, level/'collision.glb', staged, boundaries=boundaries)
        else:
            bake_water.bake_native(memory, level/'collision.glb', staged, baker, boundaries=boundaries)
        staged.replace(output)
        staged.with_suffix('.mesh').replace(mesh)
    stamp.write_text(json.dumps({'version': 1, 'inputs': inputs,
                                 'outputs': [digest(output), digest(mesh)]}, indent=2)+'\n')

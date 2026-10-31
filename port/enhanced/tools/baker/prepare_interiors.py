#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read collision and authored lighting transitions from exports or an owned disc.

No game execution or bake changes. Output contains derived game data and must
remain in an ignored local directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))
from openrac_common import paths  # noqa: E402

ROOT = paths.repo_root(__file__)
sys.path.insert(0, str(ROOT / 'editor'))
from disc import Disc, SECTOR
from level import decoded
from formats import unpack
from collision import decode


def prepare_export(level_dir, manifest, gameplay):
    """Read the exported collision mesh and retained gameplay without reopening a disc."""
    import gltf_read
    from collision import colour
    from seed_regions import transitions

    collision_path = level_dir/'collision.glb'
    document = gltf_read.read(collision_path)
    water = [channel ** 2.2 for channel in colour(0)]
    triangles = []
    for index, node in enumerate(document.list('nodes')):
        if 'mesh' not in node:
            continue
        world = document.node_world(index)
        for primitive in document.list('meshes')[node['mesh']]['primitives']:
            if primitive.get('mode', 4) != 4:
                raise ValueError('interior inference requires collision triangles')
            attributes = primitive['attributes']
            vertices = [gltf_read.point(world, p) for p in document.accessor(attributes['POSITION'])]
            colours = document.accessor(attributes['COLOR_0']) if 'COLOR_0' in attributes else None
            indices = ([row[0] for row in document.accessor(primitive['indices'])]
                       if 'indices' in primitive else list(range(len(vertices))))
            if len(indices) % 3:
                raise ValueError('truncated interior collision triangle')
            for start in range(0, len(indices), 3):
                face = indices[start:start+3]
                # The collision exporter encodes surface zero (water) in linear vertex colour.
                # Water blocks rays but must not seed walkable floor samples.
                support = colours is None or not all(
                    abs(colours[i][c]-water[c]) < 1e-5 for i in face for c in range(3))
                triangles.append([*[v for i in face for v in vertices[i]], int(support)])
    records = []
    for transition in transitions(gameplay):
        if transition['flags'] & 1:
            records.append(dict(id=transition['index'], inverse=transition['inverse'],
                                light=transition['light_sets'], ambient=[
                                    [value & 255, (value >> 8) & 255, (value >> 16) & 255]
                                    for value in transition['colours']]))
    return dict(format=1, game=manifest['game']+'-'+manifest['version'], level=manifest['level'],
                source_sha256=hashlib.sha256(gameplay+collision_path.read_bytes()).hexdigest(),
                transitions=records, triangles=triangles)


def prepare(disc_path, level):
    with Disc(disc_path) as disc:
        toc = disc.read(disc.version.toc_lba * SECTOR, disc.version.toc_size)
        info = disc.level_header(toc, level)
        section = info['ranges'][disc.version.gameplay]
        gameplay = decoded(disc.sectors(section['lba'], section['sectors']))
        section = info['ranges']['data']
        data = disc.sectors(section['lba'], section['sectors'])
        parts = []
        for index in (2, 10):  # core index and core data, editor/level.py SECTION_NAMES
            offset, size = unpack('<ii', data, index * 8)
            parts.append(decoded(data[offset:offset+size]))
        index, core = parts
        collision_offset, = unpack('<I', index, 0x14)
        # The decoder bounds every tree/leaf reference within the remaining core.
        collision = decode(core[collision_offset:])
        start, = unpack('<I', gameplay, 0x80)
        count = unpack('<i', gameplay, start)[0] if start else 0
        transitions = []
        for i in range(count):
            at = start + 16 + 16 * count + i * 128
            flags, = unpack('<I', gameplay, at+0x50)
            if not flags & 1:
                continue
            colours = unpack('<II', gameplay, at+0x40)
            lights = unpack('<ii', gameplay, at+0x48)
            transitions.append({'id':i,'inverse':unpack('<16f',gameplay,at),
                                'light':[v if v>=0 else 11 for v in lights],
                                'ambient':[[v&255,(v>>8)&255,(v>>16)&255] for v in colours]})
        return {'format':1,'game':'rac1-'+disc.version.key,'level':level,
                'source_sha256':hashlib.sha256(gameplay+core[collision_offset:]).hexdigest(),
                'transitions':transitions,
                'triangles':[[*[v/64 for p in triangle[:3] for v in p],int((triangle[3]&31)!=0)]
                             for triangle in collision.triangles]}


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc',type=Path,required=True)
    parser.add_argument('--level',type=int,required=True,choices=range(19))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=prepare(args.disc,args.level)
    with args.output.open('x') as out:
        json.dump(result,out,separators=(',',':'),allow_nan=False)
    print(f"{len(result['triangles'])} collision triangles, {len(result['transitions'])} lighting transitions")

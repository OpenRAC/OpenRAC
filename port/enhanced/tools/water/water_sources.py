# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Decode Novalis water source tables for native baking, without bake orchestration or numerics."""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

ROOT = next(p for p in Path(__file__).resolve().parents if (p/'editor/formats.py').is_file())
sys.path.insert(1, str(ROOT/'editor'))
from formats import overlay_sections, wad

VERSION = 3
CELL_SIZE = 0.5
MAX_BODIES = 64
# The reviewed Novalis records in each disc's level program: pool_light (the pond grid),
# pool_tiles, pool_colors (the authored masks), the flowing, bounded and four scroll layers and
# the three cave pools. PAL's are games/rac1's names (include/names.h) of the NTSC decomp's.
ADDRESSES = {
    'ntsc': dict(grid=0x1cad00, tiles=0x1e34c0, masks=0x1fa590,
                 layers=(0x1e2fc0, 0x1e3380, 0x1fbc80, 0x202d80)),
    'pal': dict(grid=0x1caf80, tiles=0x1e3780, masks=0x1fa850,
                layers=(0x1e32a0, 0x1e3660, 0x1fbf40, 0x203040)),
}


def overlay_memory(raw, version="ntsc"):
    """Load authored tables and apply only the first-use changes affecting rest geometry."""
    at = ADDRESSES[version]
    data = wad(raw) if raw[:3] == b'WAD' else raw
    memory = bytearray(32 << 20)
    sections = overlay_sections(data)['sections']
    for section in sections:
        start, size, offset = section['address'], section['bytes'], section['offset']
        memory[start:start+size] = data[offset:offset+size]
    # update_pool_tiles_and_effects sets the pond's 16x16 grid spacing on first use.
    struct.pack_into('<6f', memory, at['grid'], 16, 16, -8, -8, 1, 1)
    # update_scroll_layers_and_queue_draw replaces both endpoint heights from Z - 0.45.
    # Its opposite ripple offsets cancel in the rest-height midpoint used by the bake.
    for layer in range(4):
        record = at['layers'][2] + layer*0x60
        pointer, = struct.unpack_from('<I', memory, record)
        count, = struct.unpack_from('<I', memory, record+12)
        if not 3 <= count <= 4096 or pointer+count*16 > len(memory):
            raise ValueError('invalid Novalis water layer')
        for vertex in range(count):
            address = pointer+vertex*16+8
            height, = struct.unpack_from('<f', memory, address)
            struct.pack_into('<2f', memory, address, height-.45, height-.45)
    return memory


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def read_surfaces(ram, version='ntsc'):
    at = ADDRESSES[version]
    def read(fmt, address):
        if address < 0 or address + struct.calcsize(fmt) > len(ram):
            raise ValueError('water record outside snapshot')
        return struct.unpack_from('<'+fmt, ram, address)
    # Reviewed Novalis draw callbacks: pond, flowing strips and the three cave pools.
    # Each pool patch has sixteen 4x4 sub-blocks, selected by its authored mask table (not the live +0x1e draw mask).
    grid = read('6f', at['grid'])
    if not (-64 < grid[2] < 0 and -64 < grid[3] < 0 and 0 < grid[4] < 8 and 0 < grid[5] < 8):
        raise ValueError('Novalis water is not initialized (or this is a different version)')
    squares = []
    for patch in range(21):
        address = at['tiles'] + patch*0x1190
        x,y,z = read('3f', address)
        # The +0x1e draw mask is cleared for off-camera volume groups.
        # Use the authored table copied into it by the pool update callback.
        mask, = read('I', at['masks']+patch*4)
        mask &= 0xffff
        for bit in range(16):
            if mask & (1 << bit):
                x0=x+grid[2]+(bit%4)*4*grid[4]
                y0=y+grid[3]+(bit//4)*4*grid[5]
                squares.append([(x0,y0,z),(x0+4*grid[4],y0,z),
                                (x0,y0+4*grid[5],z),(x0+4*grid[4],y0+4*grid[5],z)])
    if not squares: raise ValueError('Novalis pond patches have not been initialized')
    # Connect adjacent squares at the same elevation; never merge overlapping storeys.
    groups=[]
    while squares:
        group=[squares.pop()]
        changed=True
        while changed:
            changed=False
            for square in squares[:]:
                if any(abs(square[0][2]-other[0][2]) < .05 and
                       len(set(square) & set(other)) >= 2 for other in group):
                    group.append(square); squares.remove(square); changed=True
        triangles=[t for q in group for t in ((q[0],q[1],q[2]),(q[2],q[1],q[3]))]
        groups.append((1, triangles, (0.,0.,0.)))
    flowing,bounded,four,cave=at['layers']
    for source,address,count in ((2,flowing,8),(3,bounded,2),(4,four,4),
                                 (5,cave,1),(6,cave+0x60,1),(7,cave+0xc0,1)):
        for i in range(count):
            record=address+i*0x60
            pointer,=read('I',record)
            n,=read('I',record+0xc)
            if not 3 <= n <= 4096:
                raise ValueError('invalid scrolling-water vertex count')
            points=[]
            for j in range(n):
                x,y,z0,z1=read('4f',pointer+j*16)
                points.append((x,y,(z0+z1)*.5))
            triangles=[]
            for a,b,c in zip(points,points[1:],points[2:]):
                normal=cross(sub(b,a),sub(c,a))
                if dot(normal,normal)>1e-8:
                    triangles.append((a,b,c))
            if triangles:
                groups.append((source,triangles,(0.,0.,-1.) if source<5 else (0.,0.,0.)))
    if not groups or len(groups)>MAX_BODIES:
        raise ValueError('unsupported Novalis water body count')
    # Multiple painted layers and adjoining strip segments share one physical surface.
    # Taking the highest rest height avoids simulating a separate wake for every art layer.
    merged=[g for g in groups if g[0]==1]
    for source in (2,3,4,5,6,7):
        triangles=[t for kind,ts,_ in groups if kind==source for t in ts]
        if triangles: merged.append((source,triangles,(0.,0.,-1.) if source<5 else (0.,0.,0.)))
    return merged


def load_boundaries(output, override=None):
    """Use the exported level profile unless the caller explicitly overrides it."""
    path = override if override is not None else output.parent / 'water-boundaries.json'
    config = json.loads(path.read_text(encoding='utf-8')) if override is not None or path.is_file() else {}
    if not isinstance(config,dict) or set(config)-{'open_edges'}:
        raise ValueError('water boundaries only support open_edges')
    for edge in config.get('open_edges',[]):
        if not isinstance(edge,dict) or set(edge)-{'a','b','width'}:
            raise ValueError('invalid water outlet')
        for key in ('a','b'):
            if key not in edge or len(edge[key])!=3 or not all(isinstance(v,(int,float)) and math.isfinite(v) for v in edge[key]):
                raise ValueError('outlet endpoints require three finite coordinates')
        width=edge.get('width',2.)
        if not isinstance(width,(int,float)) or not math.isfinite(width) or width<=0:
            raise ValueError('outlet width must be positive and finite')
    return config


def disc_version(output):
    """The disc of the export the output sits in (its manifest.json), NTSC-U without one."""
    manifest = output.parent / 'manifest.json'
    version = json.loads(manifest.read_text(encoding='utf-8')).get('version') if manifest.is_file() else None
    if version is not None and version not in ADDRESSES:
        raise ValueError('Novalis water baking supports the NTSC-U and PAL discs only')
    return version or 'ntsc'


def native_request(ram, collision, output, boundaries=None, version=None):
    """Decode source tables into the shared native water request without baking or publishing."""
    version = version or disc_version(output)
    surfaces = read_surfaces(ram, version)
    config = load_boundaries(output, boundaries)
    fingerprint = hashlib.sha256(
        json.dumps([VERSION, CELL_SIZE, surfaces, config], sort_keys=True).encode()
        + collision.read_bytes()).hexdigest()
    return dict(
        version=1, collision=str(collision.resolve()), fingerprint=fingerprint,
        pool_spacing=struct.unpack_from('<2f', ram, ADDRESSES[version]['grid']+0x10), boundaries=config,
        surfaces=[dict(source=source, pond=source == 1, flowing=2 <= source <= 4,
                       triangles=triangles, flow=flow) for source, triangles, flow in surfaces])

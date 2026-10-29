#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bake Novalis water boundaries from a local initialized game-memory snapshot (NTSC-U or PAL).

No captured assets belong in source control. The snapshot is obtained with
OPENRAC_DUMP_RAM=600 while running Novalis. Output goes beside manifest.json.
"""
import argparse
import hashlib
import heapq
import json
import math
from pathlib import Path
import struct

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


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def contains(p, a, b, c):
    def edge(a, b, p):
        return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
    e = [edge(a,b,p),edge(b,c,p),edge(c,a,p)]
    return min(e) >= -1e-7 or max(e) <= 1e-7


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


def distance_field(wet, width, height, cell):
    """Eight-neighbour distance to shore; internal patch seams are absent from wet."""
    distances=[math.inf]*len(wet); queue=[]
    for y in range(height):
        for x in range(width):
            at=y*width+x
            if not wet[at]:
                distances[at]=0.; heapq.heappush(queue,(0.,at))
    while queue:
        distance,at=heapq.heappop(queue)
        if distance != distances[at]: continue
        x,y=at%width,at//width
        for dy in (-1,0,1):
            for dx in (-1,0,1):
                nx,ny=x+dx,y+dy
                if not (dx or dy) or not (0<=nx<width and 0<=ny<height): continue
                target=ny*width+nx
                value=distance+cell*math.hypot(dx,dy)
                if value<distances[target]:
                    distances[target]=value; heapq.heappush(queue,(value,target))
    return distances


def collision_triangles(path):
    """The exported collision mesh uses world-space triangles (no animated objects)."""
    data=Path(path).read_bytes()
    if data[:4]!=b'glTF' or struct.unpack_from('<I',data,4)[0]!=2:
        raise ValueError('expected collision GLB version 2')
    at=12; document=None; binary=None
    while at+8<=len(data):
        size,kind=struct.unpack_from('<II',data,at); at+=8
        chunk=data[at:at+size]; at+=size
        if kind==0x4e4f534a: document=json.loads(chunk)
        elif kind==0x004e4942: binary=chunk
    if document is None or binary is None: raise ValueError('incomplete collision GLB')
    def accessor(index):
        a=document['accessors'][index]; view=document['bufferViews'][a['bufferView']]
        types={5126:'f',5125:'I',5123:'H',5121:'B'}
        components={'SCALAR':1,'VEC3':3,'VEC4':4}[a['type']]
        fmt='<'+types[a['componentType']]*components
        stride=view.get('byteStride',struct.calcsize(fmt))
        start=view.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
    triangles=[]; water_triangles=[]
    for node in document.get('nodes',[]):
        if 'mesh' not in node: continue
        if any(k in node for k in ('matrix','translation','rotation','scale')):
            raise ValueError('water baker requires world-space collision export')
        for primitive in document['meshes'][node['mesh']]['primitives']:
            if primitive.get('mode',4)!=4: continue
            points=accessor(primitive['attributes']['POSITION'])
            indices=[i[0] for i in accessor(primitive['indices'])] if 'indices' in primitive else range(len(points))
            colours=accessor(primitive['attributes']['COLOR_0']) if 'COLOR_0' in primitive['attributes'] else None
            # editor/collision.py's documented surface-0 (water) palette, in linear GLB colours.
            water_colour=tuple(c**2.2 for c in (.2,.45,1.))
            for i in range(0,len(indices),3):
                target=water_triangles if colours and all(abs(colours[indices[i]][a]-water_colour[a])<1e-5 for a in range(3)) else triangles
                target.append(tuple(points[indices[i+j]] for j in range(3)))
    return triangles,water_triangles


def clip_banks(wet, heights, width, height, origin, cell, solids):
    """Cut terrain at the waterline; overhead bridges and the bottom stay out of the mask."""
    blocked=[False]*len(wet)
    water_z=[h for h in heights if math.isfinite(h)]
    zmin,zmax=min(water_z),max(water_z)
    for tri in solids:
        if max(p[2] for p in tri)<zmin-.1 or min(p[2] for p in tri)>zmax+.5: continue
        lo=[max(0,int(math.floor((min(p[a] for p in tri)-origin[a])/cell-.5))) for a in range(2)]
        hi=[min((width,height)[a]-1,int(math.ceil((max(p[a] for p in tri)-origin[a])/cell+.5))) for a in range(2)]
        if lo[0]>hi[0] or lo[1]>hi[1]: continue
        a,b,c=tri
        determinant=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        for y in range(lo[1],hi[1]+1):
            for x in range(lo[0],hi[0]+1):
                index=y*width+x
                if not wet[index]: continue
                p=(origin[0]+(x+.5)*cell,origin[1]+(y+.5)*cell)
                if abs(determinant)>1e-8:
                    if not contains(p,a,b,c): continue
                    wa=((b[1]-c[1])*(p[0]-c[0])+(c[0]-b[0])*(p[1]-c[1]))/determinant
                    wb=((c[1]-a[1])*(p[0]-c[0])+(a[0]-c[0])*(p[1]-c[1]))/determinant
                    z=wa*a[2]+wb*b[2]+(1.-wa-wb)*c[2]
                    if heights[index]-.1<=z<=heights[index]+.5: blocked[index]=True
                elif min(q[2] for q in tri)-.1<=heights[index]<=max(q[2] for q in tri)+.1:
                    # Vertical walls have zero projected area; conservatively cover their segment.
                    for j in range(3):
                        q,r=tri[j],tri[(j+1)%3]; d=sub(r[:2],q[:2])
                        t=max(0.,min(1.,dot(sub(p,q[:2]),d)/max(dot(d,d),1e-8)))
                        if math.hypot(p[0]-q[0]-t*d[0],p[1]-q[1]-t*d[1])<cell*.55:
                            blocked[index]=True
    for i,solid in enumerate(blocked):
        if solid: wet[i]=0.
    return blocked


def bake_body(source, triangles, flow, cell=CELL_SIZE, open_edges=(), solids=(), water_collision=()):
    points=[p for triangle in triangles for p in triangle]
    if not all(math.isfinite(v) and abs(v)<100000 for p in points for v in p):
        raise ValueError('invalid water coordinates')
    # All Novalis fields use world XY with height along Z, including sloped strips.
    u=(1.,0.,0.)
    v=(0.,1.,0.)
    normal=(0.,0.,1.)
    centre=(*points[0][:2],0.)
    projected=[(dot(sub(p,centre),u),dot(sub(p,centre),v)) for p in points]
    low=[math.floor(min(p[a] for p in projected)/cell)*cell-cell for a in range(2)]
    high=[math.ceil(max(p[a] for p in projected)/cell)*cell+cell for a in range(2)]
    width,height=[int(round((high[a]-low[a])/cell)) for a in range(2)]
    if width*height>1024*1024 or min(width,height)<1:
        raise ValueError('water grid exceeds cell budget')
    origin=tuple(centre[a]+u[a]*low[0]+v[a]*low[1] for a in range(3))
    wet=[0.]*(width*height)
    heights=[-math.inf]*(width*height)
    for first in range(0,len(projected),3):
        triangle=projected[first:first+3]
        lo=[max(0,int((min(p[a] for p in triangle)-low[a])/cell)) for a in range(2)]
        hi=[min((width,height)[a]-1,int((max(p[a] for p in triangle)-low[a])/cell)) for a in range(2)]
        for y in range(lo[1],hi[1]+1):
            for x in range(lo[0],hi[0]+1):
                point=(low[0]+(x+.5)*cell,low[1]+(y+.5)*cell)
                if contains(point,*triangle):
                    a,b,c=triangle
                    determinant=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
                    if abs(determinant)<1e-8: continue
                    wa=((b[1]-c[1])*(point[0]-c[0])+(c[0]-b[0])*(point[1]-c[1]))/determinant
                    wb=((c[1]-a[1])*(point[0]-c[0])+(a[0]-c[0])*(point[1]-c[1]))/determinant
                    z=wa*points[first][2]+wb*points[first+1][2]+(1.-wa-wb)*points[first+2][2]
                    wet[y*width+x]=1.
                    heights[y*width+x]=max(heights[y*width+x],z)
    if source==1 and water_collision:
        supported=[False]*len(wet)
        for triangle in water_collision:
            a,b,c=triangle
            determinant=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
            if abs(determinant)<1e-8: continue
            lo=[max(0,int(math.floor((min(p[d] for p in triangle)-origin[d])/cell))) for d in range(2)]
            hi=[min((width,height)[d]-1,int(math.ceil((max(p[d] for p in triangle)-origin[d])/cell))) for d in range(2)]
            for y in range(lo[1],hi[1]+1):
                for x in range(lo[0],hi[0]+1):
                    index=y*width+x
                    if not wet[index]: continue
                    p=(origin[0]+(x+.5)*cell,origin[1]+(y+.5)*cell)
                    if not contains(p,a,b,c): continue
                    wa=((b[1]-c[1])*(p[0]-c[0])+(c[0]-b[0])*(p[1]-c[1]))/determinant
                    wb=((c[1]-a[1])*(p[0]-c[0])+(a[0]-c[0])*(p[1]-c[1]))/determinant
                    z=wa*a[2]+wb*b[2]+(1.-wa-wb)*c[2]
                    if abs(z-heights[index])<1.: supported[index]=True
        if not any(supported): raise ValueError('pond waterline does not match collision export')
        wet=[float(bool(w and support)) for w,support in zip(wet,supported)]
    blocked=clip_banks(wet,heights,width,height,origin,cell,solids) if solids else [False]*len(wet)
    distances=distance_field(wet,width,height,cell)
    bank_distance=distance_field([not b for b in blocked],width,height,cell) if any(blocked) else [math.inf]*len(wet)
    # Open edges are explicit world-space line segments, with a radius in game units.
    # Mark an absorbing strip near each outlet; solid boundaries use Neumann reflection.
    cells=[]
    for y in range(height):
        for x in range(width):
            at=y*width+x
            p=tuple(origin[a]+u[a]*(x+.5)*cell+v[a]*(y+.5)*cell for a in range(3))
            p=(*p[:2],heights[at] if wet[at] else 0.)
            # Unobstructed ends of flowing strips are absorbing; solid banks reflect.
            absorb=0.
            if 2<=source<=4 and wet[at] and distances[at]<2. and bank_distance[at]>2.:
                absorb=max(0.,1.-distances[at]/2.)
            for edge in open_edges:
                a,b=edge['a'],edge['b']; ab=sub(b,a)
                t=max(0.,min(1.,dot(sub(p,a),ab)/max(dot(ab,ab),1e-8)))
                d=math.sqrt(sum((p[k]-a[k]-t*ab[k])**2 for k in range(3)))
                absorb=max(absorb,max(0.,1.-d/edge.get('width',2.)))
            cells.append((512.+heights[at] if wet[at] else -(512.+heights[at]) if math.isfinite(heights[at]) else 0.,distances[at],absorb,0.))
    if 2<=source<=4:
        highest=max(points,key=lambda p:p[2]); lowest=min(points,key=lambda p:p[2])
        if highest[2]-lowest[2]>.5: flow=sub(lowest,highest)
        else: flow=sub(points[-1],points[0])
    tangent=(dot(flow,u),dot(flow,v))
    length=math.hypot(*tangent)
    tangent=tuple(t/length for t in tangent) if length>.01 else (0.,0.)
    metadata=[(*origin,float(source)),(*u,float(width)),(*v,float(height)),(*normal,0.),(*tangent,cell,0.)]
    return metadata,cells


def hydraulic_field(body, cells, solids=()):
    """Effective shallow-water depth and reference current, independently of visual rest height."""
    width,height=int(body[1][3]),int(body[2][3]);dx=body[4][2]
    depths=[.9 if body[0][3]==1 or body[0][3]>=5 else .45 for _ in cells]
    floors=[-math.inf]*len(cells)
    for a,b,c in solids:
        det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(det)<1e-8: continue
        lo=[max(0,int(math.floor((min(p[d] for p in (a,b,c))-body[0][d])/dx-.5))) for d in range(2)]
        hi=[min((width,height)[d]-1,int(math.ceil((max(p[d] for p in (a,b,c))-body[0][d])/dx-.5))) for d in range(2)]
        for y in range(lo[1],hi[1]+1):
            for x in range(lo[0],hi[0]+1):
                at=y*width+x
                if cells[at][0]<.5: continue
                point=(body[0][0]+(x+.5)*dx,body[0][1]+(y+.5)*dx)
                if not contains(point,a,b,c): continue
                wa=((b[1]-c[1])*(point[0]-c[0])+(c[0]-b[0])*(point[1]-c[1]))/det
                wb=((c[1]-a[1])*(point[0]-c[0])+(a[0]-c[0])*(point[1]-c[1]))/det
                z=wa*a[2]+wb*b[2]+(1-wa-wb)*c[2]
                rest=cells[at][0]-512.
                if z<rest-.1 and z>floors[at]: floors[at]=z
    current=(0.,0.) if body[0][3]==1 else tuple(v*.8 for v in body[4][:2])
    result=[]
    for i,c in enumerate(cells):
        optical_depth=max(0.,c[0]-512.-floors[i]) if math.isfinite(floors[i]) else depths[i]
        depths[i]=max(.25,min(1.5,optical_depth))
        result.append((depths[i],*current,optical_depth))
    return result


def write_bake(path, bodies, fingerprint, solids=()):
    metadata=[];cells=[];hydraulics=[]
    for body_index, (body, field) in enumerate(bodies):
        hydraulics.extend(hydraulic_field(body,field,solids))
        body=[tuple(row) for row in body]
        body[3]=(*body[3][:3],float(len(cells)))
        metadata.extend(body);cells.extend((*c[:3],float(body_index)) for c in field)
    if len(cells)>4*1024*1024:
        raise ValueError('water atlas exceeds budget')
    with Path(path).open('wb') as out:
        out.write(struct.pack('<8sIII32s',b'ORWATER1',VERSION,len(bodies),len(cells),fingerprint))
        for value in metadata+cells+hydraulics: out.write(struct.pack('<4f',*value))
    return len(cells)


def dense_triangle_weights(triangle):
    """Bake dyadic longest-edge subdivision. Output is a triangle list of barycentric UV maps."""
    result=[]
    def position(w):
        return tuple(sum(triangle[i][axis]*w[i] for i in range(3)) for axis in range(2))
    def split(weights):
        points=[position(w) for w in weights]
        lengths=[sum((points[i][axis]-points[(i+1)%3][axis])**2 for axis in range(2)) for i in range(3)]
        edge=max(range(3),key=lambda i:lengths[i])
        if lengths[edge]<=CELL_SIZE*CELL_SIZE+1e-6:
            result.extend((w[1],w[2]) for w in weights); return
        a,b,c=weights[edge],weights[(edge+1)%3],weights[(edge+2)%3]
        middle=tuple((x+y)*.5 for x,y in zip(a,b))
        split((a,middle,c));split((middle,b,c))
    split(((1.,0.,0.),(0.,1.,0.),(0.,0.,1.)))
    return result


def mesh_shape(triangle):
    a,b,c=triangle
    return tuple(math.floor((p[axis]-a[axis])*256+.5) for p in (b,c) for axis in range(2))


def write_rest_mesh(path, surfaces, pool_spacing=(1.,1.), fingerprint=bytes(32)):
    triangles=[(source,t) for source,ts,_ in surfaces for t in ts]
    templates={}
    # The pond draw uses the grid inside each authored four-cell sub-block.
    dx,dy=pool_spacing
    cell=((0.,0.,0.),(dx,0.,0.),(0.,dy,0.),(dx,dy,0.))
    candidates=[t for _,t in triangles]+[(cell[0],cell[1],cell[2]),(cell[1],cell[2],cell[3])]
    for triangle in candidates:
        key=mesh_shape(triangle)
        if key not in templates: templates[key]=dense_triangle_weights(triangle)
    with Path(path).open('wb') as out:
        out.write(struct.pack('<8sII32s',b'ORWMESH2',len(triangles),len(templates),fingerprint))
        for source,triangle in triangles:
            out.write(struct.pack('<I9f',source,*(v for point in triangle for v in point)))
        for shape,vertices in templates.items():
            out.write(struct.pack('<4iI',*shape,len(vertices)))
            for vertex in vertices: out.write(struct.pack('<2f',*vertex))



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


def bake(ram, collision, output, boundaries=None, version=None):
    """Bake the same rest geometry from a snapshot or offline initialized overlay tables."""
    version=version or disc_version(output)
    surfaces=read_surfaces(ram,version)
    config=load_boundaries(output,boundaries)
    solids,water_collision=collision_triangles(collision)
    bodies=[bake_body(*surface,open_edges=config.get('open_edges',[]),solids=solids,water_collision=water_collision) for surface in surfaces]
    fingerprint=hashlib.sha256(json.dumps([VERSION,CELL_SIZE,surfaces,config],sort_keys=True).encode()+collision.read_bytes()).digest()
    output.parent.mkdir(parents=True,exist_ok=True)
    count=write_bake(output,bodies,fingerprint,solids)
    write_rest_mesh(output.with_suffix('.mesh'), surfaces, struct.unpack_from('<2f',ram,ADDRESSES[version]['grid']+0x10),fingerprint)
    print(f'Baked {len(bodies)} water bodies, {count} cells, {output.stat().st_size} bytes',flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ram',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--collision',type=Path,required=True)
    parser.add_argument('--boundaries',type=Path,help='Override the exported water-boundaries.json: {"open_edges":[{"a":[x,y,z],"b":[x,y,z],"width":2}]}')
    parser.add_argument('--version',choices=sorted(ADDRESSES),help="The snapshot's disc; default: the export's manifest.json, else ntsc")
    args=parser.parse_args()
    bake(args.ram.read_bytes(),args.collision,args.output,args.boundaries,args.version)


if __name__=='__main__': main()

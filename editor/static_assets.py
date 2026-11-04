# SPDX-License-Identifier: MIT
# Copyright (c) 2026 the OpenRAC contributors
"""Import GLB/glTF scenery and bake static copies or export spinning props at Pack.

python editor/static_assets.py --source DONOR_LEVEL/ties/tie_123.glb --project PROJECT \
    --level 2 --name MyDecor --at X Y Z [--collision mesh]
Place, duplicate or transform the imported nodes below Game/StaticAssets in Godot.
Spinning props go below Game/AnimatedAssets. Inputs and generated game assets stay private.
Skeletal animation and behaviors are refused.
"""

import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import uuid

from overrides import file_ops
import gamedata
import gltf_read
from gltf import Gltf
from scenery import matrix_of

GROUP = 'Game/StaticAssets'
ANIMATED_GROUP = 'Game/AnimatedAssets'
MAX_RENDERED_INSTANCES = (1 << 31) - 1  # LevelScene::Group::count is a signed int.
STAMP = 'static-assets.json'
PNG = b'\x89PNG\r\n\x1a\n'
JPEG = b'\xff\xd8\xff'
IMAGE_TYPES = ((PNG, '.png'), (JPEG, '.jpg'))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_model(path: Path, root: Path):
    """Read a GLB or glTF; flatten its buffers and keep dependencies inside the source root."""
    document = gltf_read.read(path)
    root = root.resolve()
    for buffer in document.list('buffers'):
        uri = buffer.get('uri')
        if uri and not uri.startswith('data:'):
            referenced = gltf_read.resolve(path.parent, uri, path.name)
            if not referenced.is_relative_to(root):
                raise ValueError('A buffer refers outside the source level or authoring project')
    for image in document.list('images'):
        referenced = document.image_path(image)
        if referenced is not None and not referenced.is_relative_to(root):
            raise ValueError('A texture refers outside the source level or authoring project')

    # GLB output has one embedded buffer. Merge .gltf's external/data buffers and rewrite each
    # buffer view so the copied model no longer depends on files outside its content-addressed
    # bundle.
    offsets = []
    total = 0
    for data in document.buffers:
        offsets.append(total)
        total += len(data)
    for view in document.list('bufferViews'):
        index = view.get('buffer', 0)
        if not 0 <= index < len(offsets):
            raise ValueError(f'{path.name}: buffer view names a missing buffer')
        view['byteOffset'] = offsets[index] + view.get('byteOffset', 0)
        view['buffer'] = 0
    document.doc['buffers'] = [{'byteLength': total}]
    document.buffers = [b''.join(document.buffers)]
    return document


def validate(document):
    """Only static triangles and material extensions whose references we preserve."""
    if document.list('skins') or document.list('animations'):
        raise ValueError('Import static ties/shrubs first; skins, animations and behaviors are not supported')
    unsupported = set(document.doc.get('extensionsUsed', [])) - {'KHR_materials_unlit', 'KHR_texture_transform'}
    if unsupported:
        raise ValueError('Unsupported static material extension: ' + ', '.join(sorted(unsupported)))
    for node in document.list('nodes'):
        if 'skin' in node or 'weights' in node:
            raise ValueError('A skinned or morphing node cannot be imported as static scenery')
    for mesh in document.list('meshes'):
        for primitive in mesh['primitives']:
            if primitive.get('mode', 4) != 4 or primitive.get('targets'):
                raise ValueError('Static imports require triangles without morph targets')
            if any(k.startswith(('JOINTS_', 'WEIGHTS_')) for k in primitive['attributes']):
                raise ValueError('An object with bone weights is not static scenery')


def images(document):
    """Copy referenced PNG/JPEG textures, naming by content to avoid donor-name collisions."""
    result = {}
    for image in document.list('images'):
        data = document.image_bytes(image)
        suffix = next((ext for signature, ext in IMAGE_TYPES if data.startswith(signature)), None)
        if suffix is None:
            raise ValueError('Static imports currently require PNG or JPEG textures')
        name = f'static_{digest(data)[:24]}{suffix}'
        if name in result and result[name] != data:
            raise ValueError('Imported texture identifiers collide; no image was overwritten')
        result[name] = data
        image.clear()
        image['uri'] = 'textures/' + name
    return result


def placements(project: Path, text: str):
    """Resolve instance transforms below Game, including user-created grouping nodes."""
    if GROUP not in text:
        return []
    import collision_scene
    ext, _, _ = collision_scene.read_scene(text)
    nodes, _ = gamedata.parse_scene(text)
    result = []
    for path, node in nodes.items():
        if not path.startswith(GROUP + '/') or not node['instance']:
            continue
        instance = gamedata.parse_value(node['instance'])
        uri = ext.get(instance.args[0]) if isinstance(instance, gamedata.Call) and instance.args else None
        if not uri or not uri.startswith('res://'):
            raise ValueError(f'{path}: use an imported GLB/glTF model, not an external scene')
        source = (project / uri[6:]).resolve()
        if not source.is_relative_to(project.resolve()) or source.suffix.lower() not in ('.glb', '.gltf'):
            raise ValueError(f'{path}: static assets must be GLB/glTF files inside the project')
        matrix = matrix_of(gamedata.placed(nodes, node))
        if not all(math.isfinite(v) for v in matrix):
            raise ValueError(f'{path}: its transform is not finite')
        gltf_read.invert(matrix)  # refuse zero scale before generating any partial output
        collision = gamedata.meta_of(node).get('static_collision', False)
        if type(collision) is not bool:
            raise ValueError(f'{path}: rc1_static_collision must be true or false')
        result.append((path, source, matrix, collision))
    return result


def animated_placements(project: Path, text: str):
    """Resolve spinning GLB/glTF props in the Godot scene, including grouping transforms."""
    if ANIMATED_GROUP not in text:
        return []
    import collision_scene
    ext, _, _ = collision_scene.read_scene(text)
    nodes, _ = gamedata.parse_scene(text)
    result = []
    for path, node in nodes.items():
        if not path.startswith(ANIMATED_GROUP + '/') or not node['instance']:
            continue
        instance = gamedata.parse_value(node['instance'])
        uri = ext.get(instance.args[0]) if isinstance(instance, gamedata.Call) and instance.args else None
        if not uri or not uri.startswith('res://'):
            raise ValueError(f'{path}: use a project-local GLB/glTF model')
        source = (project / uri[6:]).resolve()
        if not source.is_relative_to(project.resolve()) or source.suffix.lower() not in ('.glb', '.gltf'):
            raise ValueError(f'{path}: animated props must use project-local GLB/glTF files')
        matrix = matrix_of(gamedata.placed(nodes, node))
        if len(matrix) != 16 or not all(math.isfinite(v) for v in matrix):
            raise ValueError(f'{path}: its transform is not finite')
        gltf_read.invert(matrix)  # refuse a zero scale before generating partial output
        meta = gamedata.meta_of(node)
        try:
            axis = [float(meta.get(f'spin_{axis}', default)) for axis, default in
                    (('x', 0), ('y', 0), ('z', 1))]
            speed = float(meta.get('spin_speed', 1.0))
            phase = float(meta.get('spin_phase', 0.0))
            move_axis = [float(meta.get(f'move_{axis}', 0.0)) for axis in ('x', 'y', 'z')]
            move_distance = float(meta.get('move_distance', 0.0))
            move_speed = float(meta.get('move_speed', 1.0))
            move_phase = float(meta.get('move_phase', 0.0))
        except (TypeError, ValueError):
            raise ValueError(f'{path}: animation axes, distances, speeds and phases must be numbers') from None
        length = math.sqrt(sum(component * component for component in axis))
        move_length = math.sqrt(sum(component * component for component in move_axis))
        if not all(math.isfinite(v) for v in (*axis, speed, phase, *move_axis,
                                               move_distance, move_speed, move_phase)) \
                or not math.isfinite(length) or length <= 1e-8 \
                or not math.isfinite(move_length):
            raise ValueError(f'{path}: spin axis must be non-zero and spin values finite')
        if abs(speed) > 100 or abs(phase) > 1_000_000 or abs(move_speed) > 100 \
                or abs(move_phase) > 1_000_000 or abs(move_distance) > 1_000_000:
            raise ValueError(f'{path}: animation speeds must be within ±100 and distances/phases within ±1,000,000')
        if move_length <= 1e-8:
            if move_distance != 0:
                raise ValueError(f'{path}: a non-zero movement distance needs a non-zero movement axis')
            move_axis = [0.0, 0.0, 0.0]
        else:
            move_axis = [v / move_length for v in move_axis]
        result.append((path, source, matrix, [v / length for v in axis], speed, phase,
                       move_axis, move_distance, move_speed, move_phase))
        if len(result) > MAX_RENDERED_INSTANCES:
            raise ValueError(f'{ANIMATED_GROUP} exceeds the renderer\'s signed instance-count limit '
                             f'of {MAX_RENDERED_INSTANCES}')
    return result


def import_asset(source: Path, project: Path, level: int, name: str, at, collision=False):
    """Copy a static model and dependencies, then add one editable scene instance with a backup."""
    source, project = Path(source).resolve(), Path(project).resolve()
    if not source.is_file() or source.suffix.lower() not in ('.glb', '.gltf'):
        raise ValueError('Choose an existing .glb or .gltf file; behavior imports come later')
    if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', name):
        raise ValueError('Use a node name containing letters, digits and underscores')
    if len(at) != 3 or not all(math.isfinite(v) for v in at):
        raise ValueError('The placement needs three finite coordinates')
    directory = project / 'levels' / f'level_{level:02}'
    if any(not path.resolve().is_relative_to(project) for path in
           (directory, directory / 'imported_static', project / '.openrac-backups')):
        raise ValueError('The scene, imported assets and backups must stay inside the project')
    scene = directory / f'level_{level:02}.tscn'
    before = scene.read_text()
    nodes, _ = gamedata.parse_scene(before)
    if 'Game' not in nodes or f'{GROUP}/{name}' in nodes:
        raise ValueError('The project must contain Game and the new node name must be unused')
    if collision and ('Game/Collision' not in nodes or not (directory / 'collision/collision.json').is_file()):
        raise ValueError('Re-extract this project with collision metadata before importing solid geometry')
    # Extracted level assets share textures at the level root. Creator GLBs may keep
    # dependencies in their own folder or anywhere inside their Godot project.
    source_root = next((p for p in (source.parent, *source.parents)
                        if (p / 'project.godot').is_file()), None)
    if source_root is None:
        source_root = source.parent.parent if source.parent.name in ('ties', 'shrubs', 'mobys') else source.parent
    document = read_model(source, source_root)
    validate(document)
    if not any(order for _, order, _ in triangles(document, gltf_read.trs())):
        raise ValueError('The selected static model contains no triangles')
    textures = images(document)
    model = Gltf()
    model.doc = document.doc
    model.buffer = bytearray(document.buffers[0])
    data = model.glb()
    asset_id = digest(data)[:24]
    bundle = directory / 'imported_static' / asset_id
    if bundle.exists() and ((bundle / 'model.glb').read_bytes() != data or any(
            not (bundle / 'textures' / n).is_file() or (bundle / 'textures' / n).read_bytes() != b
            for n, b in textures.items())):
        bundle = directory / 'imported_static' / (asset_id + '_' + uuid.uuid4().hex[:8])
    if not bundle.exists():
        bundle.mkdir(parents=True)
        (bundle / 'model.glb').write_bytes(data)
        (bundle / 'textures').mkdir()
        for filename, image in textures.items():
            (bundle / 'textures' / filename).write_bytes(image)
        extracted = source.parent.name in ('ties', 'shrubs', 'mobys')
        provenance = {'format': 1, 'source_level': source.parent.parent.name if extracted else None,
                      'kind': source.parent.name if extracted else 'creator-glb', 'source_model': source.name,
                      'source_sha256': digest(source.read_bytes()), 'textures': sorted(textures)}
        (bundle / 'source.json').write_text(json.dumps(provenance, indent=2) + '\n')
    resource = 'static_' + uuid.uuid4().hex[:12]
    uri = 'res://' + (bundle / 'model.glb').relative_to(project).as_posix()
    # Godot requires external resources before any sub-resource or node declaration.
    first_section = re.search(r'^\[(?:sub_resource|node) ', before, re.M)
    if first_section is None:
        raise ValueError('The level scene has no node declarations')
    insertion = first_section.start()
    after = before[:insertion] + f'[ext_resource type="PackedScene" path="{uri}" id="{resource}"]\n\n' + before[insertion:]
    if GROUP not in nodes:
        after += '\n[node name="StaticAssets" type="Node3D" parent="Game"]\n'
    after += (f'\n[node name="{name}" parent="{GROUP}" instance=ExtResource("{resource}")]\n'
              'transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, ' + ', '.join(map(str, at)) + ')\n'
              f'metadata/rc1_static_collision = {str(collision).lower()}\n')
    backups = project / '.openrac-backups'
    backups.mkdir(exist_ok=True)
    backup = backups / f'{scene.stem}-{uuid.uuid4().hex}.tscn'
    shutil.copyfile(scene, backup)
    temporary = scene.with_suffix('.tscn.tmp')
    temporary.write_text(after, newline='\n')
    file_ops.replace(temporary, scene)
    return {'node': f'{GROUP}/{name}', 'model': uri, 'textures': len(textures), 'backup': str(backup)}


def determinant(m):
    return (m[0] * (m[5]*m[10]-m[9]*m[6]) - m[4] * (m[1]*m[10]-m[9]*m[2])
            + m[8] * (m[1]*m[6]-m[5]*m[2]))


def triangles(document, matrix):
    """World-space primitives; mirrored instances keep outward triangle winding."""
    validate(document)
    for index, node in enumerate(document.list('nodes')):
        if 'mesh' not in node:
            continue
        world = gltf_read.multiply(matrix, document.node_world(index))
        inverse = gltf_read.invert(world)
        for primitive in document.list('meshes')[node['mesh']]['primitives']:
            attrs = {key: document.accessor(value) for key, value in primitive['attributes'].items() if key != '_LIGHT_SLOT'}
            attrs['POSITION'] = [gltf_read.point(world, p) for p in attrs['POSITION']]
            if not all(math.isfinite(x) and abs(x) < 1e6 for p in attrs['POSITION'] for x in p):
                raise ValueError('A static mesh has non-finite or out-of-range transformed vertices')
            if 'NORMAL' in attrs:
                def normal(n):
                    v = [sum(inverse[r*4+c] * n[c] for c in range(3)) for r in range(3)]
                    length = math.sqrt(sum(x*x for x in v))
                    return tuple(x/length for x in v) if length else (0.0, 0.0, 1.0)
                attrs['NORMAL'] = [normal(n) for n in attrs['NORMAL']]
            if 'TANGENT' in attrs:
                tangents = []
                for tangent in attrs['TANGENT']:
                    v = gltf_read.vector(world, tangent[:3])
                    length = math.sqrt(sum(x*x for x in v))
                    tangents.append(tuple(x/length for x in v) + (tangent[3] * (-1 if determinant(world) < 0 else 1),)
                                    if length else (1.0, 0.0, 0.0, 1.0))
                attrs['TANGENT'] = tangents
            attrs.setdefault('COLOR_0', [(1.0, 1.0, 1.0, 1.0)] * len(attrs['POSITION']))
            order = [v[0] for v in document.accessor(primitive['indices'])] if 'indices' in primitive else list(range(len(attrs['POSITION'])))
            if len(order) % 3 or any(i < 0 or i >= len(attrs['POSITION']) for i in order):
                raise ValueError('A static mesh has invalid triangle indices')
            if determinant(world) < 0:
                order = [i for at in range(0, len(order), 3) for i in (order[at], order[at+2], order[at+1])]
            yield attrs, order, primitive.get('material')


def collision_triangles(project: Path, text: str):
    """The same instance transforms as the visual bake, opted into per object."""
    import collision as col
    result = []
    for _, source, matrix, solid in placements(project, text):
        if solid:
            for attrs, order, _ in triangles(read_model(source, project), matrix):
                for i in range(0, len(order), 3):
                    p = tuple(col.quantise(attrs['POSITION'][j]) for j in order[i:i+3])
                    if not col.degenerate(p):
                        result.append((col.canonical(p), col.DEFAULT_SURFACE))
    return result


def remap_material(value, texture_offset, key=''):
    if isinstance(value, dict):
        if key.endswith('Texture') and 'index' in value:
            value['index'] += texture_offset
        for k, v in value.items():
            remap_material(v, texture_offset, k)
    elif isinstance(value, list):
        for v in value:
            remap_material(v, texture_offset)


def bake(project: Path, level_dir: Path, text: str):
    """Combine authored terrain and static instances without needing new runtime object classes."""
    base = read_model(level_dir / 'terrain.glb', project)
    textures = images(base)
    target = Gltf()
    target.doc = copy.deepcopy(base.doc)
    target.buffer = bytearray(base.buffers[0])
    for key in ('images', 'textures', 'samplers', 'materials', 'meshes', 'nodes', 'accessors', 'bufferViews'):
        target.doc.setdefault(key, [])
    placed = placements(project, text)
    for name, source, matrix, _ in placed:
        document = read_model(source, project)
        validate(document)
        for filename, data in images(document).items():
            if filename in textures and textures[filename] != data:
                raise ValueError('Static textures collide with the terrain; no image was overwritten')
            textures[filename] = data
        image_offset, sampler_offset = len(target.doc['images']), len(target.doc['samplers'])
        texture_offset, material_offset = len(target.doc['textures']), len(target.doc['materials'])
        target.doc['images'].extend(document.list('images'))
        target.doc['samplers'].extend(document.list('samplers'))
        for texture in copy.deepcopy(document.list('textures')):
            if 'source' in texture:
                texture['source'] += image_offset
            if 'sampler' in texture:
                texture['sampler'] += sampler_offset
            target.doc['textures'].append(texture)
        for material in copy.deepcopy(document.list('materials')):
            remap_material(material, texture_offset)
            target.doc['materials'].append(material)
        target.doc['extensionsUsed'] = sorted(set(target.doc.get('extensionsUsed', [])) | set(document.doc.get('extensionsUsed', [])))
        for attrs, order, material in triangles(document, matrix):
            if not order:
                continue
            primitive = {'attributes': {key: target.floats(value, bounds=key == 'POSITION') for key, value in attrs.items()},
                         'indices': target.indices(order), 'mode': 4}
            if material is not None:
                primitive['material'] = material + material_offset
            target.doc['meshes'].append({'name': name, 'primitives': [primitive]})
            target.node(name, len(target.doc['meshes']) - 1)
    payload = target.glb()
    inputs = {'textures': [(name, digest(data)) for name, data in sorted(textures.items())],
              'collision': [(name, solid) for name, _, _, solid in placed]}
    signature = digest(payload + json.dumps(inputs, sort_keys=True).encode())
    return payload, textures, {'format': 1, 'signature': signature, 'instances': len(placed)}


def write_packed(project: Path, level_dir: Path, text: str, out: Path):
    if not placements(project, text) and not (out / STAMP).exists():
        return None
    payload, textures, stamp = bake(project, level_dir, text)
    previous = json.loads((out / STAMP).read_text()) if (out / STAMP).exists() else {}
    stamp['managed_textures'] = sorted(set(previous.get('managed_textures', [])) | set(textures))
    out.mkdir(parents=True, exist_ok=True)
    (out / 'terrain.glb').write_bytes(payload)
    (out / 'textures').mkdir(exist_ok=True)
    for name, data in textures.items():
        (out / 'textures' / name).write_bytes(data)
    (out / STAMP).write_text(json.dumps(stamp, indent=2) + '\n')
    return stamp


def write_animated(project: Path, text: str, out: Path):
    """Bundle spinning visual props and their transforms into the mod's animated.json."""
    placed = animated_placements(project, text)
    if not placed:
        return 0
    objects = []
    for node, source, matrix, axis, speed, phase, move_axis, move_distance, move_speed, move_phase in placed:
        document = read_model(source, project)
        validate(document)
        if not any(order for _, order, _ in triangles(document, gltf_read.trs())):
            raise ValueError(f'{node}: the selected animated model contains no triangles')
        textures = images(document)
        model = Gltf()
        model.doc = document.doc
        model.buffer = bytearray(document.buffers[0])
        payload = model.glb()
        asset_id = digest(payload)[:24]
        bundle = out / 'animated' / asset_id
        bundle.mkdir(parents=True, exist_ok=True)
        model_path = bundle / 'model.glb'
        if model_path.exists() and model_path.read_bytes() != payload:
            raise ValueError(f'{node}: animated asset identifier collision; no file was overwritten')
        model_path.write_bytes(payload)
        (bundle / 'textures').mkdir(exist_ok=True)
        for filename, data in textures.items():
            texture_path = bundle / 'textures' / filename
            if texture_path.exists() and texture_path.read_bytes() != data:
                raise ValueError(f'{node}: animated texture identifier collision; no file was overwritten')
            texture_path.write_bytes(data)
        objects.append({'model': f'animated/{asset_id}/model.glb', 'matrix': matrix,
                        'spin_axis': axis, 'spin_speed': speed, 'phase': phase,
                        'move_axis': move_axis, 'move_distance': move_distance,
                        'move_speed': move_speed, 'move_phase': move_phase})
    out.mkdir(parents=True, exist_ok=True)
    (out / 'animated.json').write_text(json.dumps({'format': 1, 'objects': objects}, indent=2) + '\n')
    return len(objects)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True,
                        help='static GLB/glTF from a donor level or your own project')
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--level', type=int, required=True)
    parser.add_argument('--name', required=True)
    parser.add_argument('--at', type=float, nargs=3, required=True)
    parser.add_argument('--collision', choices=('none', 'mesh'), default='none')
    args = parser.parse_args()
    try:
        result = import_asset(args.source, args.project, args.level, args.name, args.at, args.collision == 'mesh')
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f'static import: {error}\n')
    print(f"Imported {result['node']} with {result['textures']} texture(s). It is static scenery, not game behavior.")
    print("Reopen the scene, place it and Pack again.")
    print(f"Scene backup: {result['backup']}")


if __name__ == '__main__':
    main()

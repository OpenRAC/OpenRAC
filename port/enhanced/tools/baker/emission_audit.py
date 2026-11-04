# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Report full-bright static surfaces for explicit emission review; never classify them as emitters."""
import json
import sys

from seed_regions import ROOT

sys.path.insert(0, str(ROOT/'editor'))
import gltf_read


def audit(level_dir, output_dir):
    """Inventory saturated authored static colours, including each instance's tie/shrub light slots."""
    manifest = json.loads((level_dir/'manifest.json').read_text())
    placements = json.loads((level_dir/manifest['placements']).read_text())
    profile_path = level_dir/'probe-emission.json'
    profile = json.loads(profile_path.read_text()) if profile_path.exists() else {}
    meshes = {}
    candidates = {}

    def inspect(path, layer, instance):
        if path not in meshes:
            document = gltf_read.read(level_dir/path)
            parts = []
            for mesh in document.list('meshes'):
                for primitive in mesh['primitives']:
                    material = document.list('materials')[primitive.get('material', 0)]
                    texture = material.get('pbrMetallicRoughness', {}).get('baseColorTexture', {}).get('index')
                    if texture is None or primitive.get('mode', 4) != 4:
                        continue
                    image = document.list('textures')[texture]['source']
                    uri = (level_dir/path).parent/document.list('images')[image]['uri']
                    uri = uri.resolve().relative_to(level_dir.resolve()).as_posix()
                    attributes = primitive['attributes']
                    count = len(document.accessor(attributes['POSITION']))
                    colours = document.accessor(attributes['COLOR_0']) if 'COLOR_0' in attributes else [[1]*4]*count
                    slots = document.accessor(attributes['_LIGHT_SLOT']) if '_LIGHT_SLOT' in attributes else None
                    indices = ([i[0] for i in document.accessor(primitive['indices'])]
                               if 'indices' in primitive else list(range(count)))
                    emitted = any(c > 0 for c in material.get('emissiveFactor', [0, 0, 0]))
                    parts.append((uri, colours, slots, indices, emitted))
            meshes[path] = parts
        for uri, colours, slots, indices, emitted in meshes[path]:
            packed = instance.get('colours', [])
            if packed and slots:
                colours = [([((packed[int(slot[0])] >> (8*c)) & 255)/128 for c in range(3)]
                            if 0 <= slot[0] < len(packed) else colour)
                           for slot, colour in zip(slots, colours)]
            # These are the original terrain and tie/shrub output clamps, not emission thresholds.
            maximum = (255 if layer == 'terrain' else 243)/128
            bright = [all(c >= maximum-1e-5 for c in colour[:3]) for colour in colours]
            count = sum(all(bright[i] for i in indices[j:j+3]) for j in range(0, len(indices), 3))
            if count:
                entry = candidates.setdefault(uri, dict(texture=uri, fullbright_triangles=0,
                    layers=set(), defined=emitted or isinstance(profile.get(uri), dict)))
                entry['fullbright_triangles'] += count
                entry['layers'].add(layer)

    if manifest.get('terrain'):
        inspect(manifest['terrain'], 'terrain', {})
    for plural, kind in (('ties', 'tie'), ('shrubs', 'shrub')):
        for instance in placements.get(plural, []):
            model = manifest.get('classes', {}).get(kind, {}).get(str(instance['class']), {})
            if model.get('mesh'):
                inspect(model['mesh'], kind, instance)
    report = dict(version=1, note='Review candidates only; brightness does not prove emission.',
                  candidates=[{**entry, 'layers': sorted(entry['layers'])}
                              for _, entry in sorted(candidates.items())])
    (output_dir/'emission-audit.json').write_text(json.dumps(report, indent=2)+'\n')
    undefined = sum(not entry['defined'] for entry in candidates.values())
    print(f'Emission audit: {undefined} undefined full-bright material candidates (not automatically emitted)', flush=True)
    return report

"""Resolve reviewed emission definitions by exported texture content, never by brightness."""
import hashlib
import json
import math
from pathlib import Path
import tempfile


def resolve_materials(directory, catalog_path):
    """Match explicit texture identities across levels and return ordinary emission overrides."""
    unified = catalog_path.with_name('material-overrides.json')
    if catalog_path.name in ('emission-materials.json', 'material-overrides.json') and unified.is_file():
        catalog = projected_catalog(unified)
    elif catalog_path.is_file():
        catalog = json.loads(catalog_path.read_text())
    else:
        return {}
    if not isinstance(catalog, dict) or catalog.get('version') != 1 or not isinstance(catalog.get('materials'), list):
        raise ValueError('emission material catalog requires version 1 and a material list')
    definitions = {}
    for material in catalog['materials']:
        if not isinstance(material, dict):
            raise ValueError('emission material must be an object')
        identity = material.get('texture_sha256', '')
        if not isinstance(identity, str) or len(identity) != 64 or any(c not in '0123456789abcdef' for c in identity):
            raise ValueError('emission texture identity requires a SHA-256 digest')
        if identity in definitions:
            raise ValueError('duplicate emission texture identity')
        emission = material.get('emission')
        if not isinstance(emission, dict) or not isinstance(material.get('name'), str):
            raise ValueError('emission material requires a name and emission object')
        for key, value in emission.items():
            if key in ('vertex_colour', 'average_for_bake'):
                if not isinstance(value, bool):
                    raise ValueError(f'emission {key} requires a boolean')
            elif key == 'contrast':
                if (isinstance(value, bool) or not isinstance(value, (int, float))
                        or not math.isfinite(value) or not 1 <= value <= 4):
                    raise ValueError('emission contrast must be between one and four')
            elif key in ('runtime_intensity', 'bake_intensity', 'diffuse_scale'):
                if (isinstance(value, bool) or not isinstance(value, (int, float))
                        or not math.isfinite(value) or value < 0 or (key == 'diffuse_scale' and value > 1)):
                    raise ValueError('invalid emission material intensity or diffuse scale')
            else:
                raise ValueError('unknown emission material setting: '+key)
        definitions[identity] = emission
    resolved = {}
    for texture in sorted((directory/'textures').glob('*.png')):
        identity = hashlib.sha256(texture.read_bytes()).hexdigest()
        if identity in definitions:
            resolved[texture.relative_to(directory).as_posix()] = definitions[identity]
    return resolved


def apply_materials(directory, catalog_path, level_defaults=None):
    """Refresh catalog and tracked level defaults while preserving independent local overrides."""
    resolved = resolve_materials(directory, catalog_path)
    for key, value in (level_defaults or {}).items():
        resolved[key] = ({**resolved[key], **value}
                         if key in resolved and isinstance(value, dict) else value)
    path = directory/'probe-emission.json'
    existing = json.loads(path.read_text()) if path.exists() else {}
    if not isinstance(existing, dict):
        raise ValueError('emission profile must be an object')
    previous = existing.pop('_catalog_defaults', {})
    if not isinstance(previous, dict):
        raise ValueError('generated emission defaults must be an object')
    for texture, value in previous.items():
        current = existing.get(texture)
        if isinstance(current, dict) and isinstance(value, dict):
            for field, generated in value.items():
                if current.get(field) == generated:
                    current.pop(field)
            if not current:
                existing.pop(texture)
        elif current == value:
            existing.pop(texture, None)
    combined = dict(resolved)
    for texture, value in existing.items():
        combined[texture] = ({**combined[texture], **value}
                             if texture in combined and isinstance(value, dict) else value)
    if resolved:
        combined['_catalog_defaults'] = resolved
    if not combined and not previous:
        return
    text = json.dumps(combined, indent=2)+'\n'
    if not path.exists() or json.loads(path.read_text()) != combined:
        # Unique owned staging leaves interrupted writes harmless and preserves the previous profile on failure.
        with tempfile.TemporaryDirectory(prefix='.probe-emission-', dir=directory) as staged:
            temporary = Path(staged)/path.name
            temporary.write_text(text)
            temporary.replace(path)


def projected_settings(rule, catalog):
    """Project tracked unscoped emission rules for legacy generated sidecars only."""
    setting = dict(catalog.get('presets', {}).get(rule.get('preset'), {}))
    for group, values in rule.get('set', {}).items():
        setting[group] = ({**setting.get(group, {}), **values}
                          if isinstance(values, dict) else values)
    emission = dict(setting.get('emission', {}))
    for key in ('bake_mode', 'bake_mesh_rgb', 'bake_use_cutout'):
        emission.pop(key, None)
    if 'diffuse_scale' in setting.get('surface', {}):
        emission['diffuse_scale'] = setting['surface']['diffuse_scale']
    return emission


def projected_catalog(path):
    """Adapt version catalog defaults; class/slot matching remains in the native material resolver."""
    catalog = json.loads(path.read_text())
    if catalog.get('version') != 1 or not isinstance(catalog.get('overrides'), list):
        raise ValueError('material catalog requires version 1 and overrides')
    entries = []
    for rule in catalog['overrides']:
        match = rule.get('match', {})
        if not rule.get('enabled', True) or set(match) != {'texture_file_sha256'}:
            continue
        emission = projected_settings(rule, catalog)
        entries.append({'name':rule.get('name',rule['id']),
                        'texture_sha256':match['texture_file_sha256'], 'emission':emission})
    return {'version':1, 'materials':entries}


def level_material_defaults(path, defaults):
    """Retain non-material bake inputs while projecting tracked texture-path emission defaults."""
    if not path.is_file():
        return defaults
    catalog = json.loads(path.read_text())
    result = dict(defaults)
    for rule in catalog.get('overrides', []):
        match = rule.get('match', {})
        if rule.get('enabled', True) and set(match) == {'texture_path'}:
            result[match['texture_path']] = projected_settings(rule, catalog)
    return result

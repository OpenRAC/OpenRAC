"""Resolve reviewed emission definitions by exported texture content, never by brightness."""
import hashlib
import json
import math
from pathlib import Path
import tempfile


def resolve_materials(directory, catalog_path):
    """Match explicit texture identities across levels and return ordinary emission overrides."""
    if not catalog_path.is_file():
        return {}
    catalog = json.loads(catalog_path.read_text())
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
            if key == 'vertex_colour':
                if not isinstance(value, bool):
                    raise ValueError('emission vertex_colour requires a boolean')
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

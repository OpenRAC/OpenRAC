"""Copy authored lighting settings into a native level export, shared by bake and runtime."""

import json
from pathlib import Path
import shutil

from .emission import apply_materials


PROFILE_ROOT = Path(__file__).resolve().parents[2] / "port/enhanced/lighting/profiles"


def write_bake_profile(level, directory: Path) -> None:
    """Copy only this game's, disc version's and level's profile into the new export."""
    source = PROFILE_ROOT / f"{level.game}-{level.version}" / f"level_{level.id:02}"
    profile = source / "probe-bake.json"
    if profile.is_file():
        shutil.copyfile(profile, directory / profile.name)
    write_emission_profile(level, directory)


def write_emission_profile(level, directory: Path) -> None:
    """Share reviewed definitions with existing exports without replacing local authoring."""
    source = PROFILE_ROOT/f'{level.game}-{level.version}'
    explicit = source/f'level_{level.id:02}'/'probe-emission.json'
    defaults = json.loads(explicit.read_text()) if explicit.is_file() else {}
    if not isinstance(defaults, dict):
        raise ValueError('tracked emission profile must be an object')
    apply_materials(directory, source/'emission-materials.json', defaults)

"""Copy authored enhanced graphics settings into a native level export, shared by bake and runtime."""

import hashlib
import json
from pathlib import Path
import shutil

from .emission import apply_materials, level_material_defaults


WATER_PROFILE_ROOT = Path(__file__).resolve().parents[2] / "port/enhanced/water/profiles"
PROFILE_ROOT = Path(__file__).resolve().parents[2] / "port/enhanced/lighting/profiles"
# Releases that share another release's reviewed profiles: Going Commando's PAL and v2.00 discs.
# Their levels are v1.01's with few changes (of the 23,224 files of an export, PAL changes 89 and
# v2.00 1,040, most in level 3's renumbered moby textures), every level's gameplay light bank is
# v1.01's, and the profiles name only a light set, a strategy and textures by their SHA-256.
SHARED_PROFILES = {("rac2", "pal"): "ntsc", ("rac2", "ntsc2"): "ntsc"}


def profile_name(game: str, version: str) -> str:
    """The folder of PROFILE_ROOT (and the water profiles) that holds GAME and VERSION's profiles."""
    return f"{game}-{SHARED_PROFILES.get((game, version), version)}"


def write_bake_profile(level, directory: Path) -> None:
    """Copy only this game's, disc version's and level's profile into the new export."""
    for root, filename in ((PROFILE_ROOT, "probe-bake.json"),
                           (PROFILE_ROOT, "lighting-environments.json"),
                           (WATER_PROFILE_ROOT, "water-boundaries.json"),
                           (WATER_PROFILE_ROOT, "ocean-profile.json")):
        source = root / profile_name(level.game, level.version) / f"level_{level.id:02}" / filename
        # Unconfigured levels use the baker and runtime's built-in defaults.
        if source.is_file():
            shutil.copyfile(source, directory / source.name)
            if filename == "lighting-environments.json":
                digest = hashlib.sha256((directory / filename).read_bytes()).hexdigest()
                (directory / ".lighting-environments.sha256").write_text(digest, encoding="ascii")
    for filename in ("object-lights.json", "foliage-materials.json"):
        shared = PROFILE_ROOT / level.game / filename
        if shared.is_file():
            shutil.copyfile(shared, directory / shared.name)
    write_material_profiles(level, directory)
    write_emission_profile(level, directory)


def write_emission_profile(level, directory: Path) -> None:
    """Share reviewed definitions with existing exports without replacing local authoring."""
    source = PROFILE_ROOT/profile_name(level.game, level.version)
    explicit = source/f'level_{level.id:02}'/'probe-emission.json'
    defaults = json.loads(explicit.read_text()) if explicit.is_file() else {}
    if not isinstance(defaults, dict):
        raise ValueError('tracked emission profile must be an object')
    defaults = level_material_defaults(explicit.with_name('material-overrides.json'), defaults)
    apply_materials(directory, source/'emission-materials.json', defaults)


def write_material_profiles(level, directory: Path) -> None:
    """Publish tracked material defaults separately from local and mod authoring."""
    sources = (("shared", PROFILE_ROOT/level.game),
               ("version", PROFILE_ROOT/profile_name(level.game, level.version)),
               ("level", PROFILE_ROOT/profile_name(level.game, level.version)/f"level_{level.id:02}"))
    for layer, source in sources:
        catalog = source/"material-overrides.json"
        target = directory/"materials"/layer/catalog.name
        # The native bake publishes this layer together with the generated textures.
        if (source/"emission-textures.json").is_file():
            continue
        if catalog.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            assets = target.parent/"textures"
            for texture in (source/"material-textures").glob("*.png"):
                if texture.is_file():
                    assets.mkdir(exist_ok=True)
                    shutil.copyfile(texture, assets/texture.name)
            shutil.copyfile(catalog, target)
        else:
            target.unlink(missing_ok=True)

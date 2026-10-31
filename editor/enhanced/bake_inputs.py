"""Retain unmodified authored inputs needed by offline bake analysis.

This is extraction only: no liquid geometry, colours or emission are derived.
All written files are disc data and must stay in the local extracted level.
"""


import json

from . import profiles


def write_bake_inputs(level, directory):
    if level.game != "rac1" or level.version not in ("pal", "ntsc"):
        return None
    directory = directory / "bake-inputs"
    directory.mkdir(exist_ok=True)
    # Keep the selected gameplay copy, rather than making the bake reopen a disc.
    # Empty synthetic exports are permitted; the planner bounds-checks its input.
    (directory / "gameplay.bin").write_bytes(level.gameplay)
    metadata = {"gameplay": "rac1-v1"}
    profile = profiles.PROFILE_ROOT / f"{level.game}-{level.version}" / f"level_{level.id:02}" / "probe-bake.json"
    settings = json.loads(profile.read_text()) if profile.is_file() else {}
    if 'water_sources' in settings:
        if not isinstance(settings['water_sources'], dict):
            raise ValueError('water_sources requires an object')
        overlay = level.stored.get("overlay")
        if not overlay:
            raise ValueError("configured water bake input missing: overlay")
        (directory / "overlay.bin").write_bytes(overlay)
        metadata["water"] = "profile-v1"
    if level.id != 9:
        return metadata
    for name in ("overlay", "core_index", "core_data"):
        data = level.stored.get(name)
        if not data:
            raise ValueError("Gaspar bake input missing: " + name)
        (directory / (name + ".bin")).write_bytes(data)
    # Each disc's overlay keeps the liquid's record at its own address
    # (port/enhanced/tools/probes/liquids.h).
    return {**metadata, "liquid": f"rac1-{level.version}-gaspar-v1"}

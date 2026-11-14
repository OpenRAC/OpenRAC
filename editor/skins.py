"""Skins: a moby of the level drawn as another model, from another level or game, animated.

The Assets dock (addons/openrac_library) adds them: a moby of a class the level has, which the game
runs as its own (its logic), with rc1_skin metadata naming the model to draw instead (a skinned
.glb under res://library, as the extractor wrote it). Pack writes them beside the level's
placements, as the port reads them (port/game/common/frontend/skins.cpp):

    <out>/level_NN/skins.json      {"format": 1, "skins": [{"spawn_id", "model", "scale",
                                     "animations"}]}
    <out>/level_NN/skins/<id>/model.glb, textures/   each model once, by its content

"spawn_id" is the moby's spawn id (+0xB2 in the game's moby), "scale" what the moby's own scale is
multiplied by, "animations" the skin's animation for each of the class's sequences (-1: the same
number when the skin has it, else its first).

The node's metadata: rc1_skin (res:// path of the model), rc1_skin_scale, rc1_skin_animations (a
list of animation numbers) and rc1_skin_asset (game/disc/level_NN/mobys/class, where it came from).
"""

import json
import re
from pathlib import Path

import static_assets
from pack import scene_mobys


def res_path(project: Path, value) -> Path:
    """The file of a res:// path in PROJECT."""
    text = str(value).strip().strip('"')
    if not text.startswith("res://"):
        raise ValueError(f"rc1_skin must be a res:// path, not {text!r}")
    path = (project / text[len("res://"):]).resolve()
    if not path.is_relative_to(project.resolve()):
        raise ValueError(f"rc1_skin {text} is outside the project")
    return path


def numbers(value) -> list[int]:
    """rc1_skin_animations as a list of ints (Godot writes [1, 2] or PackedInt32Array(1, 2))."""
    if value is None:
        return []
    if isinstance(value, list):
        return [int(v) for v in value]
    if isinstance(value, (int, float)):
        return [int(value)]
    inner = re.fullmatch(r"\s*(?:Packed(?:Int32|Int64)Array\(|\[)(.*?)[\])]\s*", str(value))
    if inner is None:
        raise ValueError(f"rc1_skin_animations must be a list of numbers, not {value!r}")
    return [int(float(v)) for v in inner.group(1).split(",") if v.strip()]


def skinned(text: str) -> list[dict]:
    """The scene's mobys with a skin."""
    return [node for node in scene_mobys(text) if "skin" in node["meta"]]


def write_skins(project: Path, text: str, out: Path, spawn_ids: dict[str, int]) -> tuple[list[str], list[str]]:
    """Writes the scene's skins into OUT (the packed level's folder): what was done and what not."""
    nodes = skinned(text)
    changes, problems, entries, written = [], [], [], {}
    for node in nodes:
        name = node["name"]
        spawn_id = spawn_ids.get(name, -1)
        if not 0 <= spawn_id < 0xFFFF:
            problems.append(f"{name}: no spawn id, so its skin is not used")
            continue
        try:
            source = res_path(project, node["meta"]["skin"])
            if source not in written:
                document = static_assets.read_model(source, project)
                if not document.list("skins") or not document.list("animations"):
                    raise ValueError(f"{source.name} has no skeleton or animations to play")
                textures = static_assets.images(document)
                model = static_assets.Gltf()
                model.doc = document.doc
                model.buffer = bytearray(document.buffers[0])
                payload = model.glb()
                asset_id = static_assets.digest(payload)[:24]
                bundle = out / "skins" / asset_id
                (bundle / "textures").mkdir(parents=True, exist_ok=True)
                (bundle / "model.glb").write_bytes(payload)
                for filename, data in textures.items():
                    (bundle / "textures" / filename).write_bytes(data)
                written[source] = f"skins/{asset_id}/model.glb"
            scale = float(node["meta"].get("skin_scale", 1.0))
            if not 0.0 < scale < 1000.0:
                raise ValueError(f"rc1_skin_scale {scale} must be above 0 and below 1000")
            entries.append({"spawn_id": spawn_id, "model": written[source], "scale": scale,
                            "animations": numbers(node["meta"].get("skin_animations"))})
            origin = str(node["meta"].get("skin_asset", source.name)).strip('"')
            changes.append(f"{name}: drawn as {origin}")
        except (OSError, ValueError, KeyError, IndexError) as error:
            problems.append(f"{name}: skin not used: {error}")
    if entries:
        out.mkdir(parents=True, exist_ok=True)
        (out / "skins.json").write_text(json.dumps({"format": 1, "skins": entries}, indent=1) + "\n")
    elif (out / "skins.json").exists():
        (out / "skins.json").unlink()
    return changes, problems

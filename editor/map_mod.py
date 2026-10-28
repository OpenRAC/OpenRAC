"""Export a creator's Godot world as an added ship destination or a standalone map.

Pack the edited project with extract.py first, then:
  python editor/map_mod.py --destination --packed PACKED --project PROJECT --template-level 2 \
      --name "My world" --map-id "author.my-world" --out build/my-world
Arrival defaults to Ratchet's placement. Omit --destination for a standalone map.
Add the resulting folder (or a zip of its contents) in the launcher's Mods tab.
"""

import argparse
import json
from pathlib import Path
import shutil
import tempfile

import overrides


def export_map(packed: Path, project: Path, out: Path, name: str, level: int,
               spawn: list[float] | None = None, heading: float | None = None, skip_movies: bool = True,
               skip_cutscenes: bool = True, destination: bool = False, map_id: str | None = None) -> dict:
    """Only copies inputs; never changes the editor project, packed files or an existing export."""
    packed, project, out = Path(packed), Path(project), Path(out)
    if out.exists():
        raise overrides.PackError(f"{out} already exists; choose a new export folder")
    note = overrides.read_json(packed / overrides.PACKED)
    if note.get("game", "rac1") != "rac1" or note.get("version") not in ("pal", "ntsc"):
        raise overrides.PackError("Pack this project's Ratchet & Clank level first (extract.py pack)")
    manifest = {"format": overrides.DESTINATION_PACK_FORMAT if destination else overrides.MAP_PACK_FORMAT,
                "game": "rac1", "version": note["version"], "name": name,
                "map": {"level": level, "spawn": spawn, "heading": heading, "skip_movies": skip_movies,
                        "skip_cutscenes": skip_cutscenes}}
    prefix = f"level_{level:02}"
    level_dir = next((p for p in (project / "levels" / prefix, project / prefix, project)
                      if (p / "terrain.glb").is_file()), None)
    if level_dir is None:
        raise overrides.PackError(f"{project} has no {prefix}/terrain.glb")
    scene = level_dir / f"{prefix}.tscn"
    if spawn is None:
        import pack
        if not scene.is_file():
            raise overrides.PackError("Keep the editable level scene to read Ratchet's arrival position")
        heroes = [n for n in pack.scene_mobys(scene.read_text()) if n['class_id'] == 0]
        if len(heroes) != 1:
            raise overrides.PackError("Place exactly one Ratchet start in the scene, or supply --spawn X Y Z")
        position, rotation, _ = pack.placement(heroes[0]['transform'])
        spawn = list(position)
        if heading is None:
            heading = rotation[2]
    manifest['map']['spawn'] = spawn
    manifest['map']['heading'] = heading if heading is not None else 0.0
    if map_id is not None:
        if not map_id.strip() or len(map_id) > 120 or any(ord(c) < 32 for c in map_id):
            raise overrides.PackError("--map-id must be a non-empty creator-qualified identifier up to 120 characters")
        manifest['export'] = 'destination:' + map_id
    overrides.read_manifest(json.dumps(manifest).encode(), str(out))
    if destination:
        import collision_scene
        if not scene.is_file():
            raise overrides.PackError("A destination needs its editable level scene with a return ship")
        _, _, nodes = collision_scene.read_scene(scene.read_text())
        ship = next((n for n in nodes if n["name"] == "Ship" and n["parent"] == "Game/LevelSettings"), None)
        if ship is None or collision_scene.transform_of(ship)[9] <= 0:
            raise overrides.PackError("Keep a working Ship under Game/LevelSettings for the return journey")
    files = {rel: path for rel, path in overrides.layer_files(packed).items()
             if rel.startswith(prefix + "/")}
    terrain = level_dir / "terrain.glb"
    import static_assets
    packed_level = packed / prefix
    scene_text = scene.read_text() if scene.is_file() else ''
    authoring = next((p for p in (level_dir, *level_dir.parents) if (p / 'project.godot').is_file()), project)
    if static_assets.placements(authoring, scene_text) or (packed_level / static_assets.STAMP).exists():
        _, _, current = static_assets.bake(authoring, level_dir, scene_text)
        stamp = overrides.read_json(packed_level / static_assets.STAMP) if (packed_level / static_assets.STAMP).exists() else {}
        if stamp.get('signature') != current['signature']:
            raise overrides.PackError('Static imports changed since packing; Pack the project again before export')
        terrain = packed_level / 'terrain.glb'
        for managed in stamp.get('managed_textures', []):
            files.pop(f'{prefix}/textures/{managed}', None)
    document, _ = overrides.glb_parts(terrain.read_bytes())
    files[prefix + "/terrain.glb"] = terrain
    for texture in overrides.model_textures(document):
        path = terrain.parent / "textures" / (texture + ".png")
        if path.is_file() and not path.is_symlink():
            files[f"{prefix}/textures/{texture}.png"] = path
    patches = []
    patch_file = packed / "patches.tsv"
    if patch_file.is_file():
        for line in patch_file.read_text().splitlines():
            if not line.strip() or line.startswith("#"):
                continue
            fields = line.split("\t")
            if len(fields) != 3:
                raise overrides.PackError("Packed patches.tsv has a malformed row")
            first, count, rel = fields
            try:
                valid = int(first) >= 0 and int(count) > 0
            except ValueError:
                valid = False
            if not valid or not overrides.FILES.fullmatch(rel):
                raise overrides.PackError("Packed patches.tsv has an invalid sector range or file")
            if rel.startswith(prefix + "/"):
                if rel not in files:
                    raise overrides.PackError(f"Packed level file {rel} is missing")
                patches.append(line)
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".map-export-", dir=out.parent) as temporary:
        stage = Path(temporary) / "map"
        stage.mkdir()
        for rel, path in files.items():
            target = stage / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
        (stage / "patches.tsv").write_text("# first sector\tsectors\tfile\n" + "\n".join(patches) + "\n")
        (stage / overrides.MANIFEST).write_text(json.dumps(manifest, indent=2, allow_nan=False) + "\n")
        _, _, _, problems = overrides.check(stage)
        if problems:
            raise overrides.PackError("\n".join(problems))
        # copytree refuses an existing destination, including one created while we validated.
        shutil.copytree(stage, out)
    return manifest


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--packed", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--map-id", help="stable creator-qualified identity, e.g. author.my-world; reuse it for updates")
    parser.add_argument("--level", "--template-level", dest="level", type=int, required=True,
                        help="existing program/resources to reuse; NOT the new destination's number")
    parser.add_argument("--destination", action="store_true", help="add a ship destination, preserving the original planet")
    parser.add_argument("--spawn", type=float, nargs=3, metavar=("X", "Y", "Z"), help="defaults to Ratchet's scene position")
    parser.add_argument("--heading", type=float, help="heading in radians; defaults to Ratchet's scene heading")
    parser.add_argument("--keep-movies", action="store_true", help="keep the original disc's movie sequences")
    parser.add_argument("--keep-cutscenes", action="store_true", help="keep the template's in-game scenes")
    args = parser.parse_args(argv)
    try:
        export_map(args.packed, args.project, args.out, args.name, args.level, args.spawn,
                   args.heading, not args.keep_movies, not args.keep_cutscenes, args.destination, args.map_id)
    except (OSError, ValueError) as error:
        parser.exit(1, f"map: {error}\n")
    print(f"Exported map {args.name}: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

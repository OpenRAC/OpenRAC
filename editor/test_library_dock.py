"""The Assets dock's library (addons/openrac_library/library.gd) on hand-built level projects of
two games and two discs: it finds them, tells what a placed asset does in the edited level, and
brings another level's skinned moby in as a still model that editor/static_assets.py packs. Runs
the library headless in Godot, so only when told where Godot is (no disc needed):

    OPENRAC_GODOT=/path/to/godot python3 -m unittest test_library_dock
"""

import json
import os
import shutil
import struct
import subprocess
import tempfile
import unittest
import zlib
from pathlib import Path

import gltf_read
import static_assets
from godot import install_addon

GODOT = os.environ.get("OPENRAC_GODOT")
HERE = Path(__file__).parent


def png() -> bytes:
    """A 1x1 white PNG."""
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"\x00\xff\xff\xff")) + chunk(b"IEND", b""))


def glb(skinned: bool) -> bytes:
    """A triangle with a texture by its relative path, bound to one joint when SKINNED, with an
    animation, as the extractor's moby models are."""
    positions = struct.pack("<9f", 0, 0, 0, 1, 0, 0, 0, 1, 0)
    uvs = struct.pack("<6f", 0, 0, 1, 0, 0, 1)
    joints = struct.pack("<12B", *([0] * 12))
    weights = struct.pack("<12f", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0)
    binds = struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
    times, rotations = struct.pack("<2f", 0, 1), struct.pack("<8f", 0, 0, 0, 1, 0, 0, 0, 1)
    parts = [positions, uvs] + ([joints, weights, binds, times, rotations] if skinned else [])
    views, offset = [], 0
    for part in parts:
        views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(part)})
        offset += len(part)
    accessors = [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
                 {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC2"}]
    attributes = {"POSITION": 0, "TEXCOORD_0": 1}
    doc = {"asset": {"version": "2.0"}, "scene": 0,
           "images": [{"uri": "../textures/t.png"}], "textures": [{"source": 0}],
           "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}],
           "meshes": [{"primitives": [{"attributes": attributes, "material": 0}]}]}
    if skinned:
        accessors += [{"bufferView": 2, "componentType": 5121, "count": 3, "type": "VEC4"},
                      {"bufferView": 3, "componentType": 5126, "count": 3, "type": "VEC4"},
                      {"bufferView": 4, "componentType": 5126, "count": 1, "type": "MAT4"},
                      {"bufferView": 5, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0], "max": [1]},
                      {"bufferView": 6, "componentType": 5126, "count": 2, "type": "VEC4"}]
        attributes.update(JOINTS_0=2, WEIGHTS_0=3)
        doc["nodes"] = [{"name": "joint_000"}, {"name": "mesh", "mesh": 0, "skin": 0}]
        doc["scenes"] = [{"nodes": [0, 1]}]
        doc["skins"] = [{"joints": [0], "inverseBindMatrices": 4}]
        doc["animations"] = [{"name": "seq_00", "samplers": [{"input": 5, "output": 6}],
                              "channels": [{"sampler": 0, "target": {"node": 0, "path": "rotation"}}]}]
    else:
        doc["nodes"] = [{"name": "mesh", "mesh": 0}]
        doc["scenes"] = [{"nodes": [0]}]
    doc["accessors"], doc["bufferViews"] = accessors, views
    binary = b"".join(parts)
    doc["buffers"] = [{"byteLength": len(binary)}]
    text = json.dumps(doc).encode()
    text += b" " * (-len(text) % 4)
    binary += b"\0" * (-len(binary) % 4)
    body = struct.pack("<I4s", len(text), b"JSON") + text + struct.pack("<I4s", len(binary), b"BIN\0") + binary
    return struct.pack("<4sII", b"glTF", 2, 12 + len(body)) + body


def level_project(root: Path, game: str, disc: str, number: int, title: str, mobys: dict, ties: list) -> Path:
    """A one-level project as the extractor writes it, with the class scenes and models given."""
    project = root / game / disc / f"level_{number:02}"
    level = project / "levels" / f"level_{number:02}"
    for folder in ("mobys", "ties", "shrubs", "textures"):
        (level / folder).mkdir(parents=True, exist_ok=True)
    (project / "project.godot").write_text(f'config_version=5\n\n[application]\n\nconfig/name="{title}"\n')
    (level / "level.json").write_text(json.dumps({"level": number, "version": disc}))
    (level / f"level_{number:02}.tscn").write_text(
        '[gd_scene format=3]\n\n[node name="Level" type="Node3D"]\n\n[node name="Game" type="Node3D" parent="."]\n')
    (level / "textures" / "t.png").write_bytes(png())
    for number_, (name, scale) in mobys.items():
        (level / "mobys" / f"moby_{number_}.glb").write_bytes(glb(skinned=True))
        (level / "mobys" / f"moby_{number_}.tscn").write_text(
            '[gd_scene format=3]\n\n[node name="Moby" type="Node3D"]\n'
            f'metadata/rc1_class = {number_}\nmetadata/rc1_class_name = "{name}"\n\n'
            '[node name="Model" parent="."]\n'
            f'transform = Transform3D({scale}, 0, 0, 0, {scale}, 0, 0, 0, {scale}, 0, 0, 0)\n')
    for number_ in ties:
        (level / "ties" / f"tie_{number_}.glb").write_bytes(glb(skinned=False))
    install_addon(project)
    return project


@unittest.skipUnless(GODOT, "set OPENRAC_GODOT")
class LibraryOnProjects(unittest.TestCase):
    def test_finds_tells_and_brings_in(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "levels"
            here = level_project(root, "rac1", "ntsc", 5, "Ratchet & Clank: Rilgar (NTSC-U)",
                                 {10: ("vendor", 0.25)}, [1000])
            level_project(root, "rac1", "pal", 3, "Ratchet & Clank: Kerwan (PAL)",
                          {10: ("vendor", 0.25), 75: ("red_ship", 0.5)}, [1000, 682])
            level_project(root, "rac3", "ntsc", 1, "Ratchet & Clank: Up Your Arsenal: Veldin (NTSC-U)",
                          {10: ("something", 1.0)}, [])
            (root / "rac1" / "ntsc-backups" / "old").mkdir(parents=True)
            out = Path(temp) / "result.json"
            run = subprocess.run([GODOT, "--headless", "--path", str(here), "--script",
                                  str(HERE / "test_library_dock.gd"), "--", str(out)],
                                 capture_output=True, text=True, timeout=300)
            self.assertTrue(out.is_file(), run.stdout + run.stderr)
            result = json.loads(out.read_text())

            self.assertEqual(result["place"], {"game": "rac1", "disc": "ntsc"})
            self.assertEqual(sorted(result["levels"]), ["rac1 ntsc 5 Rilgar", "rac1 pal 3 Kerwan",
                                                        "rac3 ntsc 1 Veldin"])
            # The same class in this game is the game's own; another class, or another game's, is a look.
            self.assertEqual(result["status"], {"rac1/pal/mobys/10": "full", "rac1/pal/mobys/75": "look",
                                                "rac1/pal/ties/1000": "full", "rac1/pal/ties/682": "look",
                                                "rac3/ntsc/mobys/10": "look", "rac1/ntsc/mobys/10": "full",
                                                "rac1/ntsc/ties/1000": "full"})
            # names/rac1.tsv's readable name wins over the class scene's code name.
            self.assertEqual(result["names"]["rac1/pal/mobys/75"], ["Red ship 1", 0.5])
            self.assertEqual(result["full_path"], "res://levels/level_05/mobys/moby_10.tscn")
            self.assertEqual(result["look_path"], "res://library/rac1/pal/level_03/mobys/moby_75.glb")

            # The copy: still, its texture beside it by the same relative path, and static_assets takes it.
            copy = here / "library" / "rac1" / "pal" / "level_03" / "mobys" / "moby_75.glb"
            self.assertTrue((copy.parent.parent / "textures" / "t.png").is_file())
            document = static_assets.read_model(copy, here)
            static_assets.validate(document)
            self.assertEqual(sum(len(order) // 3 for _, order, _ in static_assets.triangles(document, gltf_read.trs())), 1)
            self.assertEqual(json.loads(copy.with_suffix(".source.json").read_text())["class"], 75)
            # Its indices stay whole numbers (Godot reads every JSON number as a float).
            self.assertIsInstance(document.doc["accessors"][0]["count"], int)

            # On a class's logic: the model as it is, skeleton and animations kept.
            # Under a folder Godot does not import, as the skinned importer is kept out of it.
            self.assertEqual(result["skin_path"], "res://library/skins/rac1/pal/level_03/mobys/moby_75.glb")
            self.assertEqual(result["skin_animations"], ["seq_00"])
            self.assertEqual(result["skin_look"], "Skeleton3D")
            self.assertEqual(result["class_label"], "vendor")
            skins_dir = here / "library" / "skins"
            self.assertTrue((skins_dir / ".gdignore").is_file())
            skin = gltf_read.read(skins_dir / "rac1" / "pal" / "level_03" / "mobys" / "moby_75.glb")
            self.assertTrue(skin.list("skins") and skin.list("animations"))
            self.assertTrue((skins_dir / "rac1" / "pal" / "level_03" / "textures" / "t.png").is_file())
            self.assertFalse((skins_dir / "rac1" / "pal" / "level_03" / "textures" / "t.png.import").exists())


if __name__ == "__main__":
    unittest.main()

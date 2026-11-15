"""What the extractor's two level writers share: textures, glTF models, the scene text and
level.json. The port's writer (port.py) writes the game's data from it; the level editor's (OpenRAC
Editor's tools/godot.py) a Godot project.
"""

from dataclasses import replace
import json
from pathlib import Path
import math
import struct

from collision import Collision, collision_mesh, decode
from formats import png
import games
from gltf import Gltf
from level import Level
from mesh import Mesh
from moby_class import GREY, METAL_CHROME, METAL_GLASS, UNTEXTURED, MobyClass
from sky import panorama
from terrain import terrain

GAME_TO_GODOT = [1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1]  # (x, y, z) -> (x, z, -y)
# Placements store W = 0.01 (0.0 on some); only other values become metadata.
STORED_W = struct.unpack("<f", struct.pack("<f", 0.01))[0]
COLLISION_ALPHA = 0.55
MIN_SCALE = 1e-4  # the smallest joint scale the editor's models get (LevelWriter.scale)
PANORAMA = (2048, 1024)  # About one texel per pixel around the horizon.


class Raw(str):
    """A value already in Godot's text syntax, such as ExtResource("id")."""


def number(x: float) -> str:
    """The shortest text that reads back as the same float32, as Godot writes it."""
    target = struct.pack("<f", x)
    for digits in range(1, 10):
        shortest = float(f"{x:.{digits}g}")
        if struct.pack("<f", shortest) == target:
            break
    text = repr(shortest).removesuffix(".0")
    return "0" if text == "-0" else text


def transform(m: list[float]) -> Raw:
    """Transform3D lists basis rows, then the origin; m is column-major."""
    values = [m[col * 4 + row] for row in range(3) for col in range(3)] + m[12:15]
    return Raw(f"Transform3D({', '.join(map(number, values))})")


def apply(m: list[float], p) -> tuple[float, float, float]:
    return tuple(sum(m[k * 4 + i] * p[k] for k in range(3)) + m[12 + i] for i in range(3))


def value(v) -> str:
    if isinstance(v, Raw):
        return v
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, float):
        return number(v)
    if isinstance(v, list):
        return f"[{', '.join(map(value, v))}]"
    return json.dumps(v) if isinstance(v, str) else str(v)


class Scene:
    """A text scene (.tscn) built node by node."""

    def __init__(self):
        self.resources, self.subresources, self.nodes, self.editable = [], [], [], []

    @staticmethod
    def block(head: str, properties: dict) -> str:
        return "\n".join([head] + [f"{k.replace('__', '/')} = {value(v)}" for k, v in properties.items()])

    def resource(self, kind: str, path: str, rid: str) -> Raw:
        self.resources.append(f'[ext_resource type="{kind}" path="{path}" id="{rid}"]')
        return Raw(f'ExtResource("{rid}")')

    def subresource(self, kind: str, rid: str, **properties) -> Raw:
        self.subresources.append(self.block(f'[sub_resource type="{kind}" id="{rid}"]', properties))
        return Raw(f'SubResource("{rid}")')

    def node(self, name: str, parent: str | None = None, kind: str | None = None,
             instance: str | None = None, index: int | None = None, **properties) -> None:
        head = f'[node name="{name}"'
        head += f' type="{kind}"' if kind else ""
        head += f' parent="{parent}"' if parent is not None else ""
        head += f' index="{index}"' if index is not None else ""
        head += f" instance={instance}" if instance else ""
        self.nodes.append(self.block(head + "]", properties))

    def text(self) -> str:
        parts = ["[gd_scene format=3]", "\n".join(self.resources), *self.subresources, *self.nodes]
        parts += ["\n".join(f'[editable path="{path}"]' for path in self.editable)] if self.editable else []
        return "\n\n".join(p for p in parts if p) + "\n"


def collision_glb(data: Collision) -> bytes:
    """The renderer export of a packed static collision block."""
    gltf = Gltf()
    gltf.node("Collision", gltf.coloured(collision_mesh(data), gltf.overlay("collision", COLLISION_ALPHA)))
    return gltf.glb()


class LevelWriter:
    """Textures, meshes and the scene for one level: what the port's writer (port.py) and the level
    editor's (OpenRAC Editor) share. The scene text it builds is only written by the editor's."""

    # level.json's "schema" (stats): the editor's writer sets its project format's; the port's
    # manifest keeps 3, the value it has always carried.
    schema = 3

    # The collision as tiles to edit where the level packs back (the editor's writer); the port's
    # writer keeps the one collision.glb its viewer draws.
    editable_collision = False
    # Zero joint scales become MIN_SCALE in the models (scale(): the editor's writer); the port's
    # writer keeps them.
    godot_scales = False

    def __init__(self, project: Path, level: Level, normals=None):
        self.level = level
        # The executable's (cos, sin) table: with it the terrain is lit as the game lights it at
        # level load (lighting.py), so the editor shows the level's own light and shade.
        self.normals = normals
        self.name = f"level_{level.id:02}"
        self.dir = project / "levels" / self.name
        self.res = f"res://levels/{self.name}"
        self.textures = {}
        self.scene = Scene()
        self.corners = []
        self.fmt = games.of(level)
        self.moby_extra = {}  # moby index -> its pvar metadata (gamedata.py)
        self.stats = {"schema": self.schema, "level": level.id,
                      "overlay": {"entry_point": level.overlay["entry_point"],
                                  "sections": [{k: s[k] for k in ("address", "bytes", "type")}
                                               for s in level.overlay["sections"]]},
                      "mesh_instances": 0, "meshes": 0, "triangles": 0}
        self.editable_collision = self.editable_collision and self.fmt.packable
        if self.fmt.game != "rac1":  # Ratchet & Clank's level.json stays as it was
            self.stats["game"] = self.fmt.game

    def material(self, gltf: Gltf, key, depth: int) -> int:
        # The metal packets' maps are named metal_chrome / metal_glass: the port's renderer draws
        # those faces as the game's shine pass (sphere-mapped, blended over the moby).
        name = ("moby_untextured" if key == UNTEXTURED else "metal_chrome" if key == METAL_CHROME
                else "metal_glass" if key == METAL_GLASS else f"{key[0]}_{key[1]:04}")
        # A third element is the axes the game clamps the texture on (mesh.texture_key).
        image, clamp = (key[:2], key[2]) if isinstance(key, tuple) and len(key) == 3 else (key, 0)
        if key == UNTEXTURED:
            texture = GREY
        elif not self.fmt.lenient or image in self.level.textures:
            texture = self.level.texture(*image)
        else:
            # A later game's face whose texture the level does not hold (a chrome map Going
            # Commando keeps elsewhere, for one): grey, as the game's untextured faces, and noted.
            texture = GREY
            if f"texture {name}: not in the level, drawn grey" not in self.level.problems:
                self.level.problems.append(f"texture {name}: not in the level, drawn grey")
        self.textures[name] = texture
        return gltf.material(name, f"{'../' * depth}textures/{name}.png", cutout=texture.cutout, clamp=clamp)

    def glb(self, path: str, meshes: list[Mesh], depth: int, light_uv2: bool = False) -> str:
        """Write meshes, one node each, to path; return its res:// path. light_uv2: a tie's,
        shrub's or moby's light slots go out as UV2.x too, for the editor's lit shaders."""
        gltf = Gltf()
        gltf.light_uv2 = light_uv2 and self.godot_scales
        for mesh in meshes:
            materials = {key: self.material(gltf, key, depth) for key in mesh.faces}
            gltf.node(mesh.name, gltf.mesh(mesh, materials))
        target = self.dir / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(gltf.glb())
        return f"{self.res}/{path}"

    def moby_glb(self, path: str, moby: MobyClass) -> str:
        """A moby class's mesh and, for an animated class, its skeleton (joint
        nodes in their bind pose, with a child node for joints scaled after
        the chain) and one animation per sequence. A looping sequence's name
        ends in _loop, which Godot's importer turns into a looping animation.
        """
        bones = moby.skeleton
        if bones is None:
            return self.glb(path, [moby.mesh] + ([moby.metal] if moby.metal is not None else []), 1, True)
        count = len(bones.joints)
        scaled = [j for j, joint in enumerate(bones.joints) if joint.post_scale]
        # Every node is a skin joint, so that Godot makes each one a bone;
        # vertices of a post-scaled joint use its child node.
        slot = {j: count + i for i, j in enumerate(scaled)}
        skins = [tuple((slot.get(j, j), w) for j, w in pairs) for pairs in moby.mesh.skins]
        gltf = Gltf()
        gltf.light_uv2 = self.godot_scales  # a glow packet's vertices: UV2.x -2 (rc1/lit_moby_glow.gdshader)
        mesh = gltf.mesh(replace(moby.mesh, skins=skins), {key: self.material(gltf, key, 1) for key in moby.mesh.faces})
        first = len(gltf.doc["nodes"])
        post = {j: first + slot[j] for j in scaled}
        for j, joint in enumerate(bones.joints):
            children = [first + c for c, other in enumerate(bones.joints) if other.parent == j]
            children += [post[j]] if j in post else []
            gltf.node(f"joint_{j:03}", root=joint.parent is None, translation=list(joint.translation),
                      rotation=list(joint.rotation), scale=self.scale(joint.scale),
                      **({"children": children} if children else {}))
        for j in post:
            gltf.node(f"joint_{j:03}_post", root=False)
        skin = gltf.skin(list(range(first, first + count + len(scaled))),
                         [joint.inverse_bind for joint in bones.joints] + [bones.joints[j].inverse_bind for j in scaled])
        gltf.node(moby.mesh.name, mesh, skin=skin)
        if moby.metal is not None:
            # The metal packets' mesh, skinned to the same joints (its own node: the main mesh's
            # primitives stay first).
            metal_skins = [tuple((slot.get(j, j), w) for j, w in pairs) for pairs in moby.metal.skins]
            metal = gltf.mesh(replace(moby.metal, skins=metal_skins),
                              {key: self.material(gltf, key, 1) for key in moby.metal.faces})
            gltf.node(moby.metal.name, metal, skin=skin)
        for animation in bones.animations:
            gltf.animation(animation.name + ("_loop" if animation.loop else ""), animation.times,
                           [(post[j], "scale", [self.scale(v) for v in values]) if kind == "post_scale"
                            else (first + j, kind, [self.scale(v) for v in values] if kind == "scale" else values)
                            for (j, kind), values in animation.tracks.items()])
        target = self.dir / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(gltf.glb())
        return f"{self.res}/{path}"

    def scale(self, value) -> list[float]:
        """A joint's scale as Godot can import it. The game hides a part by scaling its joint to
        zero; Godot's importer cannot take a rotation out of a zero basis and reports an error for
        each, so the editor's models get MIN_SCALE instead, too small to see (the same picture).
        The port's models keep the game's zeros (PortLevelWriter)."""
        if not self.godot_scales:
            return list(value)
        return [v if abs(v) >= MIN_SCALE else math.copysign(MIN_SCALE, v) for v in value]

    def place(self, mesh: Mesh, matrix: list[float] | None = None) -> None:
        """Count a placed mesh, and keep its corners in Godot space for check.gd."""
        low, high = mesh.bounds()
        for p in ((x, y, z) for x in (low[0], high[0]) for y in (low[1], high[1]) for z in (low[2], high[2])):
            x, y, z = apply(matrix, p) if matrix else p
            self.corners.append((x, z, -y))
        self.stats["mesh_instances"] += 1
        self.stats["triangles"] += mesh.triangles

    def write_collision(self, block: bytes, more: list[bytes] = ()) -> None:
        """collision.glb and a hidden Game/Collision; it is not counted with the textured meshes.
        MORE: a later game's chunks' collision blocks, put in the same mesh."""
        data = decode(block)
        for extra in more:
            chunk = decode(extra)
            data.cells += chunk.cells
            data.faces += chunk.faces
            data.triangles += chunk.triangles
            data.hero_groups += chunk.hero_groups
        self.stats["collision"] = {"cells": data.cells, "faces": data.faces, "triangles": len(data.triangles),
                                   "surfaces": {str(k): v for k, v in data.surfaces().items()},
                                   "hero_groups": data.hero_groups}
        if not data.triangles:
            return
        if self.editable_collision and not more:
            self.write_editable_collision(block)
            return
        (self.dir / "collision.glb").write_bytes(collision_glb(data))
        self.scene.node("Collision", "Game", instance=self.scene.resource("PackedScene", f"{self.res}/collision.glb", "collision"),
                        visible=False)
        # Godot's glTF import ignores vertex colours, so the mesh gets the material here.
        material = self.scene.subresource("StandardMaterial3D", "collision_material", transparency=1, shading_mode=0,
                                          cull_mode=2, vertex_color_use_as_albedo=True,
                                          albedo_color=Raw(f"Color(1, 1, 1, {number(COLLISION_ALPHA)})"))
        self.scene.node("Collision", "Game/Collision", index=0, surface_material_override__0=material)
        self.scene.editable.append("Game/Collision")

    def write_editable_collision(self, block: bytes) -> None:
        """The collision as tiles to edit: the level editor's writer only (editable_collision)."""
        raise NotImplementedError

    def write_light_bank(self, bank) -> None:
        """light_bank.json: the level's 16 directional light sets as floats (colour A, direction A,
        colour B, direction B; directions in Godot's axes), which rc1/import_lit.gd gives the
        mobys' lit_moby shader."""
        def floats(words):
            return [struct.unpack("<f", struct.pack("<I", w))[0] for w in words]

        sets = [[floats(colour_a), self.godot_axes(floats(dir_a)), floats(colour_b), self.godot_axes(floats(dir_b))]
                for colour_a, dir_a, colour_b, dir_b in bank]
        (self.dir / "light_bank.json").write_text(json.dumps({"sets": sets}) + "\n", encoding="utf-8", newline="\n")

    @staticmethod
    def godot_axes(v: list[float]) -> list[float]:
        """A game (x, y, z, w) vector in Godot's axes (GAME_TO_GODOT)."""
        return [v[0], v[2], -v[1], v[3]]

    def write_terrain(self, fragments: list[Mesh], lod: int) -> None:
        if self.godot_scales:
            # The game multiplies a texel by the vertex colour as stored (0x80 = 1.0, up to 2.0);
            # Godot multiplies in linear light, so the editor's terrain carries the colour in
            # linear (c ** 2.2) to look as the game's does. The port's terrain keeps the game's.
            fragments = [replace(m, colours=[tuple(c ** 2.2 for c in rgba[:3]) + tuple(rgba[3:]) for rgba in m.colours])
                         if m.colours else m for m in fragments]
        path = self.glb("terrain.glb", fragments, 0)
        self.scene.node("Terrain", "Game", instance=self.scene.resource("PackedScene", path, "terrain"))
        self.scene.editable.append("Game/Terrain")
        for mesh in fragments:
            self.place(mesh)
        self.stats["meshes"] += len(fragments)
        self.stats["terrain"] = {"lod": lod, "fragments": len(fragments),
                                 "triangles": sum(m.triangles for m in fragments)}

    def write_objects(self, family: str, classes: dict[int, Mesh], placements: list[dict]) -> None:
        """One GLB per class, and one instance of it per placement."""
        group, title = f"{family.capitalize()}s", family.capitalize()
        scenes = {class_id: self.scene.resource("PackedScene",
                                                self.glb(f"{family}s/{family}_{class_id}.glb", [mesh], 1, True),
                                                f"{family}_{class_id}")
                  for class_id, mesh in classes.items()}
        self.scene.node(group, "Game", "Node3D")
        # The instances' lit colours, one row each, a column per light slot (the glb's UV2.x):
        # rc1/import_lit.gd gives the class a shader that reads its instance's row.
        lit = [p for p in placements if p.get("colours")]
        rows = {p["index"]: row for row, p in enumerate(lit)}
        if lit:
            width = max(len(p["colours"]) for p in lit)
            # RGBA lists here, RGBA words packed little-endian for the port (port.py): the same bytes.
            pixels = b"".join(b"".join(struct.pack("<I", c) if isinstance(c, int) else bytes(c) for c in p["colours"])
                              .ljust(width * 4, b"\x80") for p in lit)
            (self.dir / f"{family}s_light.png").write_bytes(png(width, len(lit), 6, pixels))
        for p in placements:
            fields = {f"metadata__rc1_{k}": v for k, v in p.items()
                      if k not in ("class_id", "matrix", "stored_w", "ambient", "colours")}
            if p["stored_w"] != STORED_W:
                fields["metadata__rc1_matrix_w"] = p["stored_w"]
            self.scene.node(f"{title}_{p['index']:04}", f"Game/{group}", instance=scenes[p["class_id"]],
                            transform=transform(p["matrix"]), **fields)
            if p["index"] in rows:
                self.scene.node(classes[p["class_id"]].name, f"Game/{group}/{title}_{p['index']:04}",
                                **{"instance_shader_parameters/light_row": rows[p["index"]]})
            self.place(classes[p["class_id"]], p["matrix"])
        self.stats["meshes"] += len({p["class_id"] for p in placements})
        self.stats[f"{family}s"] = {"classes": len(classes), "instances": len(placements),
                                    "class_triangles": sum(m.triangles for m in classes.values())}

    def write_environment(self, data) -> None:
        """The sky as a panorama (sky.png), flat ambient light and a sun.

        The game centres its sky on the camera and never moves it, so a
        panorama shows exactly what the shells do, at infinity.
        """
        environment = {"ambient_light_source": 2, "ambient_light_color": Raw("Color(0.85, 0.9, 1, 1)"),
                       "ambient_light_energy": 0.7}
        if data is None:
            environment.update(background_mode=1, background_color=Raw("Color(0, 0, 0, 1)"))
        else:
            width, height = PANORAMA
            (self.dir / "sky.png").write_bytes(png(width, height, 2, panorama(data, width, height)))
            texture = self.scene.resource("Texture2D", f"{self.res}/sky.png", "sky_png")
            material = self.scene.subresource("PanoramaSkyMaterial", "sky_material", panorama=texture)
            environment.update(background_mode=2, sky=self.scene.subresource("Sky", "sky", sky_material=material))
            self.stats["sky"] = {"shells": len(data.shells), "triangles": sum(m.triangles for m in data.shells),
                                 "textures": len(data.textures), "panorama": [width, height]}
        self.scene.node("WorldEnvironment", ".", "WorldEnvironment",
                        environment=self.scene.subresource("Environment", "environment", **environment))
        self.scene.node("Sun", ".", "DirectionalLight3D", rotation=Raw("Vector3(-0.959931, -0.610865, 0)"),
                        light_energy=0.8)

    def finish(self) -> dict:
        """Textures, the scene and level.json."""
        (self.dir / "textures").mkdir(parents=True, exist_ok=True)
        for name, texture in sorted(self.textures.items()):
            (self.dir / "textures" / f"{name}.png").write_bytes(texture.png())
        self.stats["textures"] = len(self.textures)
        self.stats["bounds"] = [[min(p[i] for p in self.corners) for i in range(3)],
                                [max(p[i] for p in self.corners) for i in range(3)]] if self.corners else [[0, 0, 0], [0, 0, 0]]
        if self.fmt.lenient:
            self.stats["problems"] = problem_summary(self.level.problems)
        (self.dir / f"{self.name}.tscn").write_text(self.scene.text(), encoding="utf-8", newline="\n")
        (self.dir / "level.json").write_text(json.dumps(self.stats, indent=2) + "\n", encoding="utf-8", newline="\n")
        return self.stats


def chunk_terrain(level, lod: int, lights, problems) -> list[Mesh]:
    """The terrain of a later game's streamed chunks (level.chunks), its fragments named
    Terrain_cN_NNN; nothing for Ratchet & Clank's levels, which have none."""
    meshes = []
    for chunk, tfrags, _ in getattr(level, "chunks", ()):
        if tfrags is None:
            continue
        for mesh in terrain(tfrags, lod, lights, problems):
            mesh.name = f"Terrain_c{chunk}_{mesh.name.removeprefix('Terrain_')}"
            meshes.append(mesh)
    return meshes


def problem_summary(problems: list[str]) -> list[str]:
    """What a later game's level left out, one line per class or fragment: the instances of a
    class that was not extracted are counted on its line, not listed one by one."""
    lines, counts = [], {}
    for line in problems:
        if " instance " in line and "left out with its class" in line:
            key = (line.split(" instance ")[0], line.rsplit(" ", 1)[1])
            counts[key] = counts.get(key, 0) + 1
        else:
            lines.append(line)
    lines += [f"{family} class {class_id}: {count} instance(s) left out with it" for (family, class_id), count in sorted(counts.items())]
    return lines

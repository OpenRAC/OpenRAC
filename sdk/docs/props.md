# Props: a mod's own models in the level

A **prop** is a model of the mod's (a `.glb` in its folder) that the port draws in the level,
among the game's mobys and lit as they are. The game never sees it: it has no collision, no
update and no place in the game's memory. A board under the hero's feet, a weapon in his hand, the
spark where a shot hits: native mods, interface 1.12 (1.18 for a prop on a joint).

## Making one and placing it

| Call | Since | What |
|---|---|---|
| `prop_create(mesh)` | 1.12 | A prop of `mesh`, a `.glb` path in the mod's folder: its id, or -1 (no level, no such file, no room). Hidden until posed |
| `prop_pose(id, position, rotation, scale)` | 1.12 | At a place, turned by a unit quaternion (x, y, z, w), scaled |
| `prop_pose(id, matrix)` | 1.12 | By a 4 by 4 matrix, column-major (`matrix[12..14]` the place) |
| `prop_pose(id, matrix, tint, blend)` | 1.12 | Also its colour times `tint` (RGBA) and drawn opaque, blended at its alpha, or added to what is behind it (`OPENRAC_PROP_OPAQUE`, `_ALPHA`, `_ADD`): a glow, a spark, a fading flash. A blended prop casts no shadow |
| `prop_hide(id, hidden)` | 1.12 | Hides it or shows it again |
| `prop_remove(id)` | 1.12 | Takes it out, its place free for another |
| `prop_attach(id, slot, joint, flags, offset)` | 1.18 | Fixes it to a moby's joint (below) |
| `prop_frame(id, out)` | 1.18 | Where it was drawn in the last frame (below) |

In C++ these are `mod.prop_create()` and so on; `mod.has_props()` and `mod.has_prop_attach()` say
whether the port has them. The [C interface](reference/c-api.md) says every case each refuses.

```cpp
int board = -1;
std::uint32_t loads = ~0u;

void tick() {
    const openrac_play_status status = mod.play_status();
    if (status.loads != loads) {          // a new level: its props went with the last one
        loads = status.loads;
        board = mod.prop_create("board.glb");
    }
    openrac_pose pose{};
    pose.size = sizeof pose;
    if (board >= 0 && mod.hero_pose(pose)) {
        const float up[4] = {0, 0, 0, 1};
        mod.prop_pose(board, pose.position, up);
        mod.prop_hide(board, false);
    }
}
```

## Things to know

- **Props go with their level.** When a level loads, every prop is gone and its id answers no
  more. Make them again when `play_status().loads` changes.
- **256 props, for all mods together** (`OPENRAC_PROPS_MAX`). `prop_create` answers -1 once they
  are taken. A mod that makes many (particles, a crowd) should make what it needs once per level,
  reuse them (hide one, pose it elsewhere), log when it is refused, and leave room for other mods.
- **The first time a mesh is drawn costs time.**
  - A mesh the level was read without has the front end read the level's scenery again with it:
    a hitch, once a run, when the first prop of that mesh is made. The meshes asked for are read
    with every level after.
  - The renderer's first draw of a mesh, a picture or a blend can stall a frame too. A mod that
    shows a prop only at a key moment (a weapon's first shot) can pose it once, tiny and faint,
    just after the level loads.
- **Units.** The `.glb` is in the game's units (those of `hero_pose` and `moby_list`), read as an
  assets `classes.json` mesh is. A model taken from the game keeps its class's
  scale outside the mesh: put it in the pose (or the offset below).
- **Pictures.** A `.glb`'s textures are found by their `uri`, beside it in the mod's folder.

## A prop on a joint (1.18)

A prop posed from the mod's tick lags a frame behind the animation, and the mod does not see a
moby's joints. `prop_attach(id, slot, joint, flags, offset)` fixes the prop to joint `joint` of
the moby in `slot`. From the next frame the renderer draws it at that joint's frame as it draws
the moby, times `offset`: a 4 by 4 matrix in the joint's frame, column-major, the moby's own scale
left out (null for none). It never lags.

- **The hero's hand.** With `OPENRAC_ATTACH_HELD`, `joint` is one of the hero's attach points as
  `hero_held` names them (`openrac_held_piece.joint`: where the game puts the piece in his hand,
  on his back, on his head). With no offset, the prop's origin is where the game puts its own
  piece, so a weapon modelled with its grip at the origin sits in his hand.
- **Hiding the game's piece.** To take the place of what he holds, hide its moby with
  `moby_hide(slot, 1)`. Since 1.18 the port keeps a held piece hidden while it stays in his hand,
  though the game shows his pieces again each tick.
- **Drawn with its moby.** The prop is not drawn while the moby is not (hidden, culled, gone), nor
  for a joint its class lacks. Its tint and blend stay as `prop_pose` last gave them.
- **Animated in parts.** A model in parts, each its own prop on the same joint, plays its own
  animation if the mod gives each part a new offset each tick.
- **Off the joint.** `prop_pose` takes the prop off the joint again.

**Where it is.** `prop_frame(id, out)` gives the matrix the prop was drawn with in the last frame:
for one on a joint, the joint's frame then, times the offset. A point of the mesh carried by that
matrix is where it was drawn: a weapon's muzzle, the way its barrel points, through a somersault
as through a walk. It answers no for a prop not posed, or not drawn on its joint yet (the frame
after `prop_attach`).

```cpp
// The gun in his hand; its shots leave from its muzzle (0, -0.6, 0 in the gun's mesh).
int hero_slot = -1;
for (const openrac_moby& m : mod.moby_list()) {
    if (m.oclass == 0) {                  // Ratchet & Clank's hero is class 0
        hero_slot = m.slot;
    }
}
openrac_held held{};
held.size = sizeof held;
if (mod.hero_held(held) && held.count > 0 && held.pieces[0].slot == 0) {
    mod.prop_attach(gun, hero_slot, held.pieces[0].joint, OPENRAC_ATTACH_HELD);
}
float m[16];
if (mod.prop_frame(gun, m)) {
    const float muzzle[3] = {m[12] - 0.6f * m[4], m[13] - 0.6f * m[5], m[14] - 0.6f * m[6]};
    // ... fire from muzzle ...
}
```

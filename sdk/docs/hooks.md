# Hooks

A mod changes what the game does at **hook points**: functions of the game that
OpenRAC lets mods take over. With no mod on a hook point, the game's own code
runs, exactly as on the console.

## Four ways to use a hook point

| | What your function does | Example |
|---|---|---|
| **wrap** | does something, calls `original`, maybe does more after | change an argument, then let the game go on |
| **replace** | never calls `original` | write a behaviour anew |
| **before** | runs before the rest; the game's code always runs | count jumps |
| **after** | runs after the rest | read the result, nudge a value |

In Luau, `openrac.hook(id, fn)` wraps or replaces (`fn(original, ...)`), and
`openrac.before` and `openrac.after` take a plain function. In C and C++, a
hook is a wrap or a replace; before and after are a wrap that calls
`original` first or last.

```lua
openrac.hook("rac1.player.set_planar_velocity", function(original, angle)
    original(angle)   -- the game's own, or the mod before yours
    openrac.log(`moving along {angle}`)
end)

openrac.after("rac1.player.advance_animation", function()
    -- runs after the game has moved the animation on
end)
```

## Several mods on one hook point

Mods on the same hook point form a chain, in the order of the Mods tab. The
mod **lower** in the list (loaded later) runs **first**, and its `original`
is the mod above it, down to the game's code. So:

- a mod that calls `original` lets the others run too;
- a mod that does not call `original` stops the chain there; the launcher
  shows when mods share a hook point, so players can order them;
- script and native mods share the same chain.

Take hooks only while your mod loads (in `openrac_mod_load`, in your C++
`load`, or at the top level of a script). Hook points are the same on every
disc a mod supports.

## Calling a hook point yourself (interface 1.21)

A mod sometimes does what the game does at a hook point itself: a weapon of the
mod's takes its rounds as the game's weapons do. Calling the hook point, rather
than redoing the game's work, lets the other mods on it apply: another mod's
infinite ammo then covers your weapon too.

`hook_entry(name, signature)` gives the hook point's entry, the function the
game calls there: calling it runs the chain from the mod loaded last down to the
game's own code, as when the game calls it. It takes an id or an alias and the
same signature as `hook`, so it is the same on every disc, where `function()`
needs the disc's own name of the function. It is null for a hook point this
OpenRAC lacks or another signature.

```cpp
// C++: typed by the hook point, asked by its id.
if (const auto take_ammo = mod.hook_entry<openrac::rac1::Weapon_TakeAmmo>()) {
    if (take_ammo(item, 1) == 0) {
        // too little ammo: the game's own answer, or another mod's
    }
}
```

In C, `api->hook_entry("rac1.weapon.take_ammo", OPENRAC_RAC1_SIG_Weapon_TakeAmmo)`.

If your mod took the same hook point, its own replacement runs too: tell your
own call apart (a flag set around it) when the replacement must not act on it.

## Stable ids

Every hook point has a permanent **id** of the form
`<game>.<area>.<name>`. Ids never change, even when OpenRAC renames the game
function behind them (the decompilation renames functions as it learns what
they do, and the port's code is moving to C++). Write ids in new mods.

The names hook points had first (`Hero_TurnToHeading`, from the
decompilation) stay as **aliases**: a mod written with one keeps loading.

| Id | Alias | What the game does there |
|---|---|---|
| `rac1.pad.process_report` | `Pad_ProcessReport` | turns one pad report into the pad's state, each poll |
| `rac1.player.update_motion_input` | `Hero_UpdateMotionInput` | reads the stick into the hero, early in his update |
| `rac1.player.turn_to_heading` | `Hero_TurnToHeading` | eases the hero's facing toward where he moves |
| `rac1.player.set_planar_velocity` | `Hero_SetPlanarVelocity` | sets his ground velocity along an angle |
| `rac1.camera.follow.apply_overrides` | `FollowCamera_ApplyOverrides` | the follow camera's per-tick requests from the hero's state |
| `rac1.player.advance_animation` | `Hero_AdvanceAnimation` | moves his animation on, once a tick |
| `rac1.player.update` | `Hero_Update` | the hero's whole update, once a tick (interface 1.12) |

The [reference](reference/hooks-rac1.md) lists every hook point with its
signature and parameters, generated from OpenRAC's table.

Ratchet & Clank: Going Commando has hook points of its own, `rac2.<area>.<name>`
on its three discs (NTSC-U v1.01 `ntsc`, PAL `pal` and NTSC-U v2.00 `ntsc2`),
in `openrac/games/rac2.h` (`OPENRAC_RAC2_HOOK_<name>`) and
[its reference](reference/hooks-rac2.md). Where one does a rac1 hook point's
job it has the same `<area>.<name>` (`rac2.player.take_damage`), so a mod for
both games ([several games](manifest.md#a-mod-for-several-games)) reads alike in
each. Its signatures are rac2's own.

Ratchet & Clank: Up Your Arsenal's are `rac3.<area>.<name>` on its PAL (`pal`)
and NTSC-U (`ntsc`) discs, in `openrac/games/rac3.h` (`OPENRAC_RAC3_HOOK_<name>`)
and [its reference](reference/hooks-rac3.md): the hero's damage and death, item
gifts and upgrades, the vendor's price and list, ammo, bolt drops and pickups,
enemies' deaths, hits and the level's load. Where one does a rac2 hook point's
job it has the same `<area>.<name>` (`rac3.item.give`).

## Moby updates (experimental)

The behaviour catalogue lists the game's moby classes (crates, enemies, ...),
their update functions and their per-class variables (pvars). A class's update
is a hook point with the id `rac1.moby.<class>.update` (alias
`Moby_<Class>_Update`), once OpenRAC has made it one. Several classes may share
one update function: check the moby's class in your hook. This part names the
game's internals and is **experimental**: names, fields and signatures may
change as the decompilation goes on.

## Moving the hero yourself (interface 1.12)

A mod that replaces how the hero moves (a hoverboard) takes `rac1.player.update`,
his whole update each tick (one hook point over the level loop's three copies of
it), and does not call the original: it runs its own physics on
`collision_triangles(lo, hi, ...)` (the level's and the mobys' collision around
him) and puts him where it worked out with `hero_set_pose`, which also starts
his animation's sequences through the game's own and does what the update does
to show him (his moby, what he holds, his drawing); `hero_set_joints` turns his
joints on top of that animation (a riding stance; any mod may turn joints, and
every mod's turns apply together). Only the mod holding the hook point may set
his pose. Props (`prop_create` with a `.glb` of the mod's)
and the mod's own sounds (`sound_load` with a `.wav` or `.ogg`) go with it:

```cpp
mod.hook<openrac::rac1::Hero_Update>([&](auto /*original*/) {
    const float lo[3] = {x - 4, y - 4, z - 4}, hi[3] = {x + 4, y + 4, z + 4};
    const auto tris = mod.collision_triangles(lo, hi);
    // ... the board's physics on tris ...
    mod.hero_set_pose(pose);
    mod.prop_pose(board, pose.position, rotation);
});
```

Keep calling the original when the mod is off (the board put away): the game
moves him again from where the pose left him.

A prop of a skinned `.glb` can bend: `mod.prop_set_skin(id, matrices, count)` (1.29)
gives it one 4 by 4 matrix per joint, column-major, each taking the mesh's bind-pose
vertex to where the joint puts it (the identity is the bind pose), and `prop_pose`
still places the whole mesh. `openrac_api.prop_set_skin` in `openrac_mod.h` numbers the
joints and says the rest.
A moby of the game's can look like something else: `mod.model_create("x.glb")` makes a model
of a mesh in the mod's folder (1.29), and `mod.moby_model_slot(slot, model)` draws the moby in
that array slot with it (`moby_list` and `moby_slot_of` name the slot). The game's update,
collision and animation are untouched; only what is drawn changes. `openrac_api.moby_model_slot`
in `openrac_mod.h` says what the matrix, the pose flag and the slot's lifetime are.
`mod.moby_model_class(class, scope, model)` (1.29) does the same for every moby of a class, in the world
(`OPENRAC_MODEL_WORLD`) or in the game's menus (`OPENRAC_MODEL_MENU`, the bolt price icon and the vendor's
preview); it changes all those mobys for every player running the mod, so a slot is the narrower choice.

## What a hook cannot do

- Hook a function OpenRAC has not made a hook point. Ask for one in an
  [issue](https://github.com/OpenRAC/OpenRAC/issues): adding a hook point
  changes nothing for players without mods.
- Take a hook point after load, or let go of one.
- Keep pointers into the game's memory across levels: a level moves its data.

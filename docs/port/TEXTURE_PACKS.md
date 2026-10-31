# Texture packs

A texture pack replaces the game's textures with your own pictures, at any
size: an upscaled or repainted set. It works for the levels' textures (the
world, the models, the sky) and for the 2D layer (the HUD, the menus, the
font). Code: [`port/renderer/texture_pack.cpp`](../../port/renderer/texture_pack.cpp).

## Where it goes

`<data>/texture_packs/<game id>/`, where `<data>` is the folder the launcher
gives the game (`--data`) and the game id is `rac1-pal` or `rac1-ntsc`. Every
`.png` in it or in its sub-folders whose name is a texture's hash is used; other
files are left alone. The setting `texture_pack` (on by default,
[SETTINGS.md](SETTINGS.md)) turns it off. The log says how many textures the
pack has at the start.

## Making one

1. Dump the textures: start the game with
   `OPENRAC_DUMP_TEXTURES=<folder>`, and play the places you want to cover.
   Every texture the game shows is written once to the folder as
   `<hash>.png`: 16 hex digits, the hash of its size and texels as the port
   converted them.
2. Edit or upscale the pictures you want. Keep the file name. Any size works:
   the game samples a replacement over the same area as the original.
3. Put the edited files in `<data>/texture_packs/<game id>/` and start the game.

Leave out the pictures you did not change: an unchanged copy only costs memory.
Textures the game makes while it runs (a fade, an animated screen) change from
frame to frame, so each frame is a texture of its own; replacing those is not
useful.

## PCSX2 packs

PCSX2 names its replacements by a hash of the texture as it lies in the
console's video memory. The port converts textures before it sees them, so
those names do not match. A PCSX2 pack can be carried over by dumping with
`OPENRAC_DUMP_TEXTURES`, matching the pictures to the pack's by eye or by image
comparison, and copying the pack's picture under the dumped name.

The dumped pictures are taken from your disc. Do not share them; share only the
pictures you made.

# Texture packs

A texture pack replaces the game's textures with your own pictures, at any
size: an upscaled or repainted set. It works for the levels' textures (the
world, the models, the sky) and for the 2D layer (the HUD, the menus, the
font). Code: [`port/renderer/texture_pack.cpp`](../../port/renderer/texture_pack.cpp);
the launcher's side: [`launcher/core/src/texture_packs.rs`](../../launcher/core/src/texture_packs.rs).

## Where it goes

`<data>/texture_packs/<game id>/`, where `<data>` is the folder the launcher
gives the game (`--data`) and the game id is the port's:

| Game | Game id |
|---|---|
| Ratchet & Clank (PAL) | `rac1-pal` |
| Ratchet & Clank (NTSC-U) | `rac1-ntsc` |
| Going Commando (NTSC-U) | `rac2-ntsc` |

Every `.png` in it or in its sub-folders whose name is a texture's hash is
used; other files are left alone, and the folders are only for you. The
setting `texture_pack` (on by default, [SETTINGS.md](SETTINGS.md)) turns it
off. The log says how many textures the pack has at the start.

## In the launcher

A game's page has a **Texture packs** section, in three steps:

1. **Save the game's textures while you play.** Switch it on and play: every
   picture the game shows is saved once into the textures folder (below).
   The counts per folder open that folder. It stays on until you switch it
   off; the launcher starts the game with `OPENRAC_DUMP_TEXTURES` for you.
2. **Make your own pictures.** Open a saved picture in any image editor,
   upscale or repaint it, keep its file name, and put it in your pack
   (**Open your pack**), in any folder.
3. **Your texture pack.** Its switch is the `texture_pack` setting. **Add a
   pack…** reads a shared `.zip`: it says how many textures it has and in
   which folders, refuses another game's pack, and adds it to yours (a
   picture for the same texture replaces yours) or replaces yours, which is
   kept beside it as `<game id> (before import <date>)`. **Share my pack…**
   saves your pack as a `.zip` with its folders and a manifest, and leaves
   out any picture that is still byte for byte the one the game saved: those
   are your disc's, not yours to share.

The textures folder is `texture_dumps/<game id>/` in the launcher's data folder
(`%LOCALAPPDATA%\openrac`, `~/Library/Application Support/OpenRAC`,
`~/.local/share/openrac`), not the pack's, so the game's own pictures never
end up in a pack by themselves.

## Making one by hand

1. Dump the textures: start the game with
   `OPENRAC_DUMP_TEXTURES=<folder>`, and play the places you want to cover.
   Every texture the game shows is written once as `<hash>.png`: 16 hex
   digits, the hash of its size and texels as the port converted them. They
   are sorted into folders:
   - `Interface/`: the 2D layer, which the port converts as the game uploads
     it: the HUD (bolt counter, health, the weapon icons), the menus, the
     pause screens and the font. It is one folder: the port cannot tell a HUD
     icon from a menu's picture.
   - `Planets/<planet>/`: the textures a level's scenery and models use, under
     the game's own name of the level (`Oozla`, `Aranos (tutorial)`).
   - `Shared/`: a level's texture that a second planet uses too is moved
     there from the first planet's folder.

   A second run into the same folder writes only what is new, and still moves
   textures to `Shared/`.
2. Edit or upscale the pictures you want. Keep the file name. Any size works:
   the game samples a replacement over the same area as the original.
3. Put the edited files in `<data>/texture_packs/<game id>/`, in any folders,
   and start the game.

Leave out the pictures you did not change: an unchanged copy only costs memory.
Textures the game makes while it runs (a fade, an animated screen) change from
frame to frame, so each frame is a texture of its own; replacing those is not
useful.

## A pack's .zip

A shared pack is a `.zip` of the pack folder's contents, folders kept, with
`openrac-texture-pack.json` at its top:

```json
{ "format": 1, "game": "rac2-ntsc", "name": "Crisp HUD", "textures": 120, "madeWith": "OpenRAC launcher 0.10.1" }
```

The launcher accepts a `.zip` without the manifest, and one whose contents
sit in a single folder of their own. It refuses one whose manifest names
another game, a file outside the `.zip`'s folder, a link, a `.png` not named
by a texture (except an animation's frames, in a folder named by a texture),
two pictures for one texture, and a file that is not the picture its name
says. Other files (a readme) are left out.

## PCSX2 packs

PCSX2 names its replacements by a hash of the texture as it lies in the
console's video memory. The port converts textures before it sees them, so
those names do not match. A PCSX2 pack can be carried over by dumping with
`OPENRAC_DUMP_TEXTURES`, matching the pictures to the pack's by eye or by image
comparison, and copying the pack's picture under the dumped name.

The dumped pictures are taken from your disc. Do not share them; share only the
pictures you made.

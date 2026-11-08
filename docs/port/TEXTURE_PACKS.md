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
| Going Commando (USA v1.01) | `rac2-ntsc` |
| Going Commando (USA v2.00) | `rac2-ntsc2` |
| Going Commando (Europe) | `rac2-pal` |

Every `.png` in it or in its sub-folders whose name is a texture's hash is
used; other files are left alone, and the folders are only for you. The
setting `texture_pack` (on by default, [SETTINGS.md](SETTINGS.md)) turns it
off. It is a mod like the others: `--no-mods` (the launcher's Mods switch
under Play off) leaves it out too. The log says how many textures the pack
has at the start.

## In the launcher

Your pack is a mod. It is the first row of **Graphics and sound packs** on
**Mods → My mods**, with a switch (the `texture_pack` setting) and a menu:

- **Share my pack…** saves it as a `.zip`. Pictures that are still exactly the
  game's own are left out (they come from your disc).
- **Replace with a pack…** puts in another pack and keeps the old one as
  `<game id> (before import <date>)`.

To add a shared pack, use **Add a mod** (a file or a drop). It is checked,
another game's pack is refused, and its pictures are added to yours.

**Make your own** opens the steps (Models and textures; on Android a fold):

1. **Save the game's textures while you play.** Switch it on and play. Each
   picture the game shows is saved once.
2. **Make your own pictures.** Edit or upscale a saved picture, keep its file
   name, and put it in your pack (**Open your pack**), in any folder.

Saved pictures go to `texture_dumps/<game id>/` in the launcher's data folder,
not into your pack.

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

## Animated textures

A replacement can move: a few pictures shown one after another, like a flip
book. The game's own moving textures can be found in a dump, and any texture,
moving or not, can be given frames of your own.

### How the games move their textures

The games make a texture move in four ways. Only the first two can be replaced,
frame by frame.

- **Frames swapped in turn.** The game has a few separate pictures and shows
  them one after another in the same place. Each frame is a texture of its own,
  with its own name in a dump.
- **Colours cycled.** The picture stays the same and only its colours change
  (its palette). The port sees a new texture for each set of colours, so each
  step again has its own name.
- **Scrolling.** The same picture slides across a surface (water, conveyor
  belts). It is one texture, so replacing it replaces it everywhere it slides;
  there are no frames to replace.
- **Made while the game runs.** Fades, the picture drawn into a screen (the
  vendor's screen, the ship's cockpit display) and other textures the game
  draws itself change every frame. They cannot be replaced.

Examples from Going Commando (NTSC-U):

- Frames swapped: the ship's 18 paint jobs are separate textures the game
  loads over the ship's (one at a time, picked in the Ship Shack, not
  animated). Among the effects, a dump of Maktar (level 2) found two effect
  textures that each alternate with a second picture of the same size, held
  for several ticks each (`Animated/6c69a5b16d5290e5/` and
  `Animated/73172b9529f8c4b4/`).
- Colours cycled: the effects' and particles' pictures are 256-colour images
  with their palette in the game's memory, so a palette the game changes
  gives a new texture each time. A dump of levels 0 to 13 found none doing it
  while standing at the start.
- Scrolling: the sea (levels 6, 8, 11, 13, 16 and 18), the water sheets,
  the waterfalls and the scrolling light bands are drawn with moving texture
  coordinates over one texture.
- Made while the game runs: the Fizzwidget cockpit display (a picture drawn
  into a texture each tick), the vendor's item previews and the fades.

### Finding the frames in a dump

When you dump textures (see [Making one by hand](#making-one-by-hand)), the
port watches each place a texture is drawn (a spot of the HUD, an effect). When
one place shows the same few pictures in turn, twice over, each for at least
two ticks, it writes all of them together into the dump's `Animated` folder:

```
dump/
  Animated/
    3f2a91c07be45d10/      named after one of its frames
      3f2a91c07be45d10.png
      8c01d2e6a9f3b744.png
      e19b07c3d5a26f80.png
      order.txt            the frames, in the order the game shows them
```

The pictures keep their texture names, so each one is still a texture of its
own: a picture named by a hash is always replaced on its own, wherever it lies
in a pack. The folder only tells you they belong together. Effects' and
particles' textures, which the port makes from the game's memory, go into
`Shared/`.
The log says what changed between them: `texels of their own` (frames swapped
in turn) or `the same texels with new palettes` (colours cycled).

### Making your own animation

Any texture can be animated, whether the game moves it or not. Make a folder
in your pack named after the texture (its 16 hex digits, without `.png`), and
put the frames in it, numbered from `0001.png`. A folder counts as an
animation only when it holds such numbered pictures; pictures named by a hash
in it are still textures of their own:

```
texture_packs/<game id>/
  3f2a91c07be45d10/
    0001.png
    0002.png
    0003.png
    0004.png
    animation.json         optional
```

`animation.json` says how fast and how the frames play. Both settings are
optional:

```json
{ "fps": 12, "mode": "pingpong" }
```

- `fps`: frames a second. Without it, 10.
- `mode`: `loop` (start again from the first frame, the default), `pingpong`
  (play backwards to the first, then forwards again) or `once` (stop on the
  last frame).

Instead of a folder, an animated GIF named after the texture works too
(`3f2a91c07be45d10.gif`). Its own frame delays are used.

Some rules:

- All frames of one animation must be the same size. When they are not, the
  log says which frame differs and the texture is not animated (a still
  `<hash>.png` beside it is used if there is one).
- The frames can be any size, as still replacements can.
- The frames follow the game's clock, not the wall clock: they stop when the
  game is paused, and run at the same speed at 30, 60 or 240 frames a second.
- An animation starts when its texture is first drawn.
- Frames are shown one at a time, never blended into each other.
- The frames of all animations together may hold up to 256 MB. An animation
  past that shows only its first frame; the log says so, and gives the total
  each time an animation is read.

### Worked example: a blinking bolt icon

1. Start the game with `OPENRAC_DUMP_TEXTURES=<folder>` and play until the
   bolt counter shows on the HUD (pick up a few bolts).
2. Quit and open the dump folder. Look in `Interface/` for the bolt icon. Say it is `5d0c7a1b2e3f4a96.png`.
3. In your pack folder, make a folder `5d0c7a1b2e3f4a96`.
4. Make four pictures from the icon: copy it into an image editor and save
   four versions, for example the icon brighter and brighter, as `0001.png`,
   `0002.png`, `0003.png` and `0004.png` in that folder.
5. Optionally add `animation.json` with `{ "fps": 8, "mode": "pingpong" }`.
6. Start the game. The log says
   `texture pack: 5d0c7a1b2e3f4a96.png animated, 4 frame(s) of 32x32` when the
   icon is first shown, and it pulses. Pause the game and it holds still.

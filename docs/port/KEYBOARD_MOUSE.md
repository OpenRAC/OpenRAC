# Keyboard and mouse

Ratchet & Clank (PAL and NTSC-U) and Going Commando (NTSC-U, PAL and NTSC-U
v2) play with a keyboard and mouse out of the box: WASD moves, the mouse turns
the camera, left click fires. It is built into the port
(`port/game/rac1/keyboard_mouse/`, `port/game/rac2/keyboard_mouse/`) and
always on. It used to be the keyboard-mouse mod of openrac-mods.

## Gamepad or keyboard

The game starts on the gamepad. It switches to the keyboard and mouse as soon
as you use them, and back as soon as you press the pad. Nothing to set.

- **Switches to the keyboard and mouse:** a key that plays, a mouse button,
  the wheel, or a real push of the mouse (6 pixels in a quarter second).
- **Switches back:** a pad button, a stick or trigger past half way, or a
  finger on a touch screen.
- **Never switches:** F1 to F12, system keys, a repeating key, and the motion
  SDL sends when it catches or frees the cursor.

On the gamepad the game reads the pad exactly as before, so its logic is the
original's. On the keyboard and mouse the cursor is caught in play and free in
menus, scenes, the overlay (F3) and when the window loses focus.

## The mouse

- **Raw input, no acceleration.** Windows raw input, XInput2 raw motion on X11,
  unaccelerated motion on Wayland, GameController raw deltas on macOS.
- **Ratchet & Clank:** the mouse turns the camera itself, by an angle in
  proportion to its motion, behind Ratchet and in first person. Between ticks
  the picture already shows the next tick's turn, so the view follows the
  mouse at the display's rate. Mouse up always looks up (use *Invert* for the
  other way). A pad's right stick takes the camera back.
- **Going Commando:** the mouse is the right stick, so the camera turns as the
  stick turns it.

## Default controls

| Action | Keys | PS2 button |
|---|---|---|
| Move, walk (hold) | W A S D, Left Alt | left stick |
| Jump | Space | Cross |
| Fire | Left click | Circle (Square in Going Commando's ship) |
| Quick Select (hold, point with the mouse) | Right click | Triangle |
| Melee (wrench) | F, middle click | Square (Circle, missiles, in the ship) |
| Crouch | Left Ctrl, C | R1 |
| Strafe (hold) | Left Shift | L2 (Ratchet & Clank: only with the lock-strafe mod) |
| First-person view | V, back side button | L1 |
| Crouch and strafe (R2) | R, front side button | R2 |
| Map, pause | Tab or M, Escape or P | Select, Start |
| Ratchet & Clank only: camera behind Ratchet | Q | none |
| Ratchet & Clank only: weapon 1 to 8, next, previous | 1 to 8, wheel | none |

In menus: arrows (or WASD) and the wheel move, Enter or left click confirm,
Escape, Backspace or right click go back, E is Circle, Ctrl and V are R1 and
L1. Space, Enter or Escape skip scenes. A key still held when the game moves
between play, a menu or a scene is ignored until you let it go, so the Escape
that paused does not also close the menu.

In Ratchet & Clank menus the cursor also points: the item under it gets the
focus, a click confirms (*Mouse in menus* turns this off). At a vendor, click
the Buy or Exit monitor to press it, or an icon on the strip to select it; a
click anywhere else does nothing. A click on a button icon the game shows with
its text (the map's "View Missions", "Exit") presses that button. On the map,
drag with the left button to move it and turn the wheel to zoom.

In Going Commando menus a click on a list item moves the selection to it and
confirms, and the cursor over one moves the selection there (not in lists that
scroll round their middle, such as the skill points'). A click on a button
icon in a text, or the rest of its line (Help's "Exit", a dialog's "Yes" and
"No"), presses that button; in a dialog a click anywhere else does nothing. In a grid (weapons,
gadgets, items, Quick Select) a click on a cell moves the selection frame there
and confirms; once the frame has moved, the cursor over a cell moves it there.

The strafe key presses L2 in Ratchet & Clank only while the lock-strafe mod is
loaded, so the game behaves as on a pad (L2 is first-person view there).

## Key prompts

While you use the keyboard and mouse, every button icon of the game (the four
face buttons, L1, L2, R1, R2 and both sticks) shows your keys instead: a key
cap with its name ("SPACE", "F", "F12", two lines for "NUM 4" or "PG DN"),
an arrow for the arrow keys, a mouse with the button lit for a click, the
side buttons or the wheel, "WASD" for the left stick and a mouse for the
right one. Every key and mouse button you can bind has its icon, so the icons
always show what Controls has. They are drawn as the game draws its own L1
button (its tab, its white letters), so they match. On the gamepad the PS2
icons come back. Only the picture of the game's font changes; the *Key
prompts* setting turns it off.

The game's icons are 24 x 16 texels. The renderer draws the painted font from
a picture six times as big, with smooth strokes, so the keys stay sharp on a
4K screen (`renderer/textures/sharp_cells.h`). The game's memory and the
texture's hash stay as painted.

## Changing the controls

- **In the game:** Options (F1), Controls, *Keyboard and mouse*. Pick a key's
  row to replace it (press the key, button or wheel; Esc cancels), add a second
  one or remove one. A mark shows a key another action uses too.
- **In the launcher:** Mods, Built in, *Keyboard and mouse*.

Both show the same page, from the manifest: Presets (Default, Left-handed,
Arrow keys; *Custom* when the keys match none), Reset to defaults, then the
sections Mouse, Movement, Combat, Weapons, Camera, Interface and Menus, with
the same names and order. The game's help pane shows each setting's line.

Changes apply at once, from either place, while the game runs. They are kept
as `kbm_<key> = <value>` lines in `<data>/settings/<game>-<version>.ini`
(SETTINGS.md).

| Section | Setting | Default | What it does |
|---|---|---|---|
| Mouse | Horizontal sensitivity | 1.0 | How far the camera turns left and right for the mouse's motion |
| Mouse | Vertical sensitivity | 1.0 | How far the camera turns up and down for the mouse's motion |
| Mouse | Invert vertical | off | Mouse up looks down |
| Mouse | Camera tilt | Stays | *Levels out*: the tilt eases back when the mouse rests |
| Mouse | Lock the cursor in play | on | Off leaves the cursor free (no mouse camera) |
| Movement | Walk speed | 0.5 | How fast the walk key moves |
| Movement | Ramp-up time (ms) | 60 | How long the movement keys take to reach full speed |
| Weapons | Quick Select dead zone | 30 | Pixels the mouse moves before the ring picks a slot |
| Interface | Key icons on screen | on | The button icons show your keys |
| Interface | Mouse in menus | on | Ratchet & Clank: the cursor points at menu items |

## Good to know

- An installed copy of the old mod is not loaded; set its settings again on
  the built-in entry.
- The log says `keyboard and mouse on the <disc> disc`, and warns about two
  actions on one key.
- A keyboard used as player 2 keeps the port's own keyboard table.

## Tests

`ctest -R "keyboard_mouse|key_prompts|active_device|mod_"`: binds and clashes,
stick and camera turn, every game state's keys, menus under the mouse, the
Quick Select, Going Commando's contexts, key prompts painted and put back
byte for byte, live settings, and both built-ins loaded as the port loads them.

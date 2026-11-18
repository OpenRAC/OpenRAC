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
- **Sensitivity in Ratchet & Clank:** at 1 one count turns 0.086 degrees, as
  CS sensitivity 3.9. Both menus show your setting as CS sensitivity and cm a
  full turn at *Your mouse's DPI*.

## Default controls

| Action | Keys | PS2 button |
|---|---|---|
| Move, walk (hold) | W A S D, Left Alt | left stick |
| Jump | Space | Cross |
| Fire | Left click | Circle (Square in Going Commando's ship) |
| Quick Select (hold, point with the mouse) | Right click | Triangle |
| Wrench | F, middle click | Square (Circle, missiles, in the ship) |
| Crouch | Left Ctrl, C | R1 |
| Strafe | Left Shift | L2 (Ratchet & Clank: only with the lock-strafe mod) |
| First-person view | V, back side button | L1 |
| Map, pause | Tab or M, Escape or P | Select, Start |
| Ratchet & Clank only: camera behind Ratchet | Q | none |
| Ratchet & Clank only: weapon 1 to 8, next, previous | 1 to 8, wheel | none |

In menus: arrows (or WASD) and the wheel move, Enter or left click confirm,
Escape, Backspace or right click go back, E is Circle, Ctrl and V are R1 and
L1. Space, Enter or Escape skip scenes. A key still held when the game moves
between play, a menu or a scene is ignored until you let it go, so the Escape
that paused does not also close the menu.

In Ratchet & Clank menus the cursor also points: the item under it gets the
focus, a click confirms (*Mouse in menus* turns this off).

The strafe key presses L2 in Ratchet & Clank only while the lock-strafe mod is
loaded, so the game behaves as on a pad (L2 is first-person view there).

## Key prompts

While you use the keyboard and mouse, the game's own button icons show your
keys ("SPACE", "LMB") instead of Cross or Circle, and the PS2 icons come back
on the gamepad. Only the picture of the game's font changes; the *Key prompts*
setting turns it off.

## Changing the controls

- **In the game:** Options (F1), Controls, *Keyboard and mouse*. Pick a key's
  row to replace it (press the key, button or wheel; Esc cancels), add a second
  one or remove one. A mark shows a key another action uses too.
- **In the launcher:** Mods, Built in, *Keyboard and mouse*, Settings. Presets:
  Default, Left-handed, Arrow keys.

Changes apply at once, from either place, while the game runs. They are kept
as `kbm_<key> = <value>` lines in `<data>/settings/<game>-<version>.ini`
(SETTINGS.md).

| Setting | Default | What it does |
|---|---|---|
| Mouse sensitivity | 1.0 | How far the camera turns for the mouse's motion |
| Up and down sensitivity | 1.0 | The up and down turn against the side to side one |
| Your mouse's DPI | 800 | Only for the CS and cm hint (Ratchet & Clank) |
| Mouse acceleration | 0 | Above 0 a fast flick turns further |
| Invert the mouse's up and down | off | Mouse up looks down |
| Camera tilt | Stays | *Levels out*: the tilt eases back when the mouse rests |
| Capture the cursor in play | on | Off leaves the cursor free (no mouse camera) |
| Mouse in menus | on | Ratchet & Clank: the cursor points at menu items |
| Key prompts | on | The button icons show your keys |
| Walk speed | 0.5 | How fast the walk key moves |
| Stick ramp (ms) | 60 | How long the movement keys take to reach full speed |
| Quick Select dead zone | 30 | Pixels the mouse moves before the ring picks a slot |

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

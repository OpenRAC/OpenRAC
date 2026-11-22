# Keyboard, mouse and gyro

Ratchet & Clank (PAL v2, PAL v1, NTSC-U, NTSC-J) and Going Commando (NTSC-U,
NTSC-U v2, PAL) work with a keyboard and mouse out of the box. It is built into
the port and always on. (R&C PAL v1 and NTSC-J are not tested on a disc yet.)

## Gamepad or keyboard

The game starts on the gamepad. It switches to the keyboard and mouse when you
use them, and back when you touch the pad.

- **To keyboard and mouse:** a game key, a mouse button, the wheel, or a real
  mouse move.
- **Back to the pad:** a button, a stick or trigger past half way, or a touch.
- **Never:** F1 to F12, Alt, system keys, a repeating key.

The switch only picks the icons and the menu cursor. In play both always work
at once: the cursor is locked in play on the pad too, so a mouse (or a gyro
that Steam Input or DS4Windows turns into a mouse) turns the camera while you
hold the pad. *Mouse with a gamepad* off gives the old way: the mouse camera
only on the keyboard and mouse. To stop the icons flipping when you mix them,
set *Button icons* to *Always keys* or *Always gamepad*.

## The mouse

- Raw input, no acceleration, on every system.
- The mouse turns the camera behind Ratchet directly, by an angle for each
  pixel, in both games. Up and down tilt stays where you leave it.
- R&C: first person works the same way. Frames between ticks already show the
  turn, so the view follows the mouse at your screen's rate.
- Other cameras (scenes, GC's ship and vehicles) get the mouse as the right
  stick. A pad's right stick always wins.

## Gyro

*Gyro aiming* reads the controller's gyro itself through SDL, not as a mouse.
So there is no screen edge, no mouse acceleration, and the sticks keep working.
It works with any controller SDL reads a gyro from: DualShock 4, DualSense,
Switch Pro, Joy-Con, Steam Controller and Steam Deck (when Steam does not take
it). On a phone or a handheld with no such controller it reads the device's own
gyro. It is off by default.

- **Always** or **Only while aiming** (R&C: first person, L1 or L2 held; GC:
  strafing, L2 or R2 held). Never in menus or the Quick Select.
- **Sensitivity** 1 turns the camera as far as the controller turned.
- **Turn by:** *Player space* works however you hold the controller. *Yaw*,
  *Roll*, or both, are the classic ways.
- **Gyro button:** hold to aim, hold to pause (to re-centre), or press to
  switch. It still presses its own button in the game.
- **Smoothing** hides hand shake on slow turns only. **Dead zone** cuts very
  slow turns. **Calibrate when still**: put the controller down for two
  seconds and it learns the drift.

The gyro only feeds the camera the same way the mouse does. Game logic does not
change. In GC the pad's right stick wins over the mouse and the gyro on the
follow camera.

## Default controls

| Action | Keys | PS2 button |
|---|---|---|
| Move, walk (hold) | W A S D, Left Alt | left stick |
| Jump | Space | Cross |
| Fire | Left click | Circle (Square in GC's ship) |
| Quick Select (hold, point with mouse) | Right click | Triangle |
| Wrench | F, middle click | Square |
| Crouch | Left Ctrl, C | R1 |
| Strafe (hold) | Left Shift | L2 (R&C: only with the lock-strafe mod) |
| First person | V, back side button | L1 |
| R2 | R, front side button | R2 |
| Map, pause | Tab or M, Escape or P | Select, Start |
| R&C: camera behind Ratchet | Q | none |
| R&C: weapon 1 to 8, next, previous | 1 to 8, wheel | none |

In menus: arrows, WASD or the wheel move; Enter or left click confirm;
Escape, Backspace or right click go back; E is Circle. Space, Enter or Escape
skip scenes. A key held while the game changes screen is ignored until you let
it go.

**Clicking in menus:** the item under the cursor gets selected, a click
confirms it, and a click next to the items does nothing. This works for lists,
grids, vendors, the Quick Select ring, button icons with text ("Exit", "Yes")
and the map (drag to move, wheel to zoom). *Mouse in menus* turns it off.

## Key icons

While you use the keyboard and mouse, the game's button icons show your keys
instead, drawn in the game's own style and sharp at 4K. On the pad the PS2
icons come back. Only the font picture changes. *Key icons on screen* turns it
off.

## Changing the controls

In the game (F1, Controls, *Keyboard, mouse and gyro*) or in the launcher (Mods,
Built in). Both show the same page and changes apply at once. They are saved as
`kbm_<key> = <value>` in `<data>/settings/<game>-<version>.ini`.

| Setting | Default | What it does |
|---|---|---|
| Horizontal / vertical sensitivity | 1.0 | How far the camera turns |
| Invert vertical | off | Mouse up looks down |
| Camera tilt | Stays | *Levels out*: the tilt goes back when the mouse stops |
| Lock the cursor in play | on | Off: free cursor, no mouse camera |
| Mouse with a gamepad | on | The mouse turns the camera on the pad too |
| Gyro aiming | Off | *Always* or *Only while aiming* |
| Gyro sensitivity | 2.0 | Camera turn for a turn of the controller |
| Gyro smoothing | 3 | Degrees a second below which turns are smoothed |
| Walk speed | 0.5 | Speed of the walk key |
| Ramp-up time (ms) | 60 | Time for the move keys to reach full speed |
| Quick Select dead zone | 30 | Pixels before the ring picks a slot |
| Key icons on screen | on | Icons show your keys |
| Button icons | Last used | Or always keys, or always the gamepad |
| Mouse in menus | on | Point and click in menus |

## Code

| Where | What |
|---|---|
| `port/game/common/keyboard_mouse/kbm.hpp` | Shared logic with no game in it: binds, keys to stick, mouse camera (`turn_orbit`), Quick Select, pad report |
| `port/game/common/keyboard_mouse/core.hpp` | Shared glue: settings, binds per tick, menu keys, cursor, pad report, key icons |
| `port/game/common/keyboard_mouse/openrac-mod.json` | Settings and binds for all games (the launcher reads it too) |
| `port/platform/gyro.h` | The gyro filter: calibration, spaces, dead zone, smoothing (no SDL) |
| `port/platform/input.cpp` | Reads the gyro: player 1's gamepad, or the device's own |
| `port/game/<game>/keyboard_mouse/` | Only the game's own parts: where the game is, its menus, its camera, its addresses per version |

### Adding a game

1. Write `port/game/<game>/keyboard_mouse/keyboard_mouse.cpp` on top of
   `core.hpp`.
2. Add addresses for **every** version of the game. CI checks this
   (`port/tools/versionmap/test_kbm_discs.py`).
3. Turn the follow camera with `kbm::turn_orbit`.
4. List it in `port/game/<game>/built_in_mods.cpp` and add a test.

The pad must not change: RAM must match with and without `--no-mods` on a pad
route.

## Tests

`ctest -R "keyboard_mouse|key_prompts|active_device|gyro|mod_"`

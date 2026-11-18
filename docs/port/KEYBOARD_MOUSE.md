# Keyboard and mouse

Ratchet & Clank (PAL v2.00 and NTSC-U) plays with a keyboard and mouse out of
the box: WASD to move, the mouse to aim the camera, left click to fire. It is
built into the port (`port/game/rac1/keyboard_mouse/`) and always on. It was
the keyboard-mouse mod of openrac-mods, and its settings, binds and presets
are the same.

## The gamepad and the keyboard

The game starts on the gamepad. It switches to the keyboard and mouse the
moment you use them, and back to the gamepad the moment you press it, with
nothing to set. Like most PC ports (Dusklight, for one), both always work;
the switch only decides what the game listens to for the camera and the
cursor:

- **To the keyboard and mouse:** a key that plays (letters, digits, arrows,
  Space, Enter, Escape, Shift, Ctrl, Alt), a mouse button, the wheel, or a
  real push of the mouse (6 pixels within a quarter of a second, so a nudge
  of the desk does not count).
- **Back to the gamepad:** a button, or a stick or trigger pushed past half
  way. A finger on a touch screen switches back too.
- **Never:** the port's own keys (F1 to F12), system keys (Windows, Cmd,
  volume), a key held down repeating, and the motion SDL sends when the
  cursor is let go or caught.

On the gamepad the cursor is free and hidden, and the game reads the pad
exactly as before: with no key or mouse motion the mod sends the game
nothing, so the game logic is the original's. On the keyboard and mouse the
cursor is caught in play and comes back in menus, scenes, the overlay (F3)
and when the window loses focus; in menus it shows while you move it and
hides after 2 seconds of rest. Android and handhelds start and stay on the
gamepad unless a keyboard or mouse is plugged in and used.

## The mouse

- **Raw input, no acceleration.** The camera turns by the mouse's own counts:
  Windows raw input, XInput2 raw motion on X11, unaccelerated relative motion
  on Wayland, and the GameController framework's raw deltas on macOS (SDL's
  macOS motion carries the system's acceleration). The port tells SDL not to
  scale motion (`SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE` off, speed 1). The
  *Mouse acceleration* setting stays at 0 unless you want it.
- **Every frame.** The game turns the camera at its tick (60 or 50 a second);
  between ticks, the picture already shows the turn the next tick will make
  (frontend/camera_latch.cpp), so the view follows the mouse at the
  display's rate, 144 Hz or more. Only the picture: the tick turns the camera
  by exactly the same angle.
- No motion is lost or doubled: motion between ticks adds up and the tick
  reads it once.

## Default controls

| Action | Keys | PS2 button |
|---|---|---|
| Move | W A S D | left stick |
| Walk (hold) | Left Alt | left stick, half way |
| Camera | the mouse | none (the mod turns the camera itself) |
| Jump | Space | Cross |
| Fire | Left click | Circle |
| Quick Select (hold) | Right click | Triangle |
| Wrench | F, middle click | Square |
| Crouch | Left Ctrl, C | R1 |
| Strafe | Left Shift | L2 |
| First-person view (hold) | V, the mouse's back side button | L1 |
| Map | Tab, M | Select |
| Camera behind Ratchet | Q | none (the mod does it) |
| Pause | Escape, P | Start |
| Quick Select slots 1 to 8 | 1 to 8 | none (equips the item at once) |
| Next / previous weapon | wheel down / wheel up | none |

In menus, scenes and movies the same keys do what you'd expect there:

| Where | Keys | What happens |
|---|---|---|
| Title screen | Space, Enter, Escape or left click | Start |
| Menus (main menu, pause menu and its pages, the ship's map, vendor, save and load) | arrows or WASD; wheel up and down | move (a held arrow repeats) |
| | Enter, Space or left click | confirm (Cross) |
| | Escape, Backspace or right click | back (Triangle); at the top of the pause menu this closes it |
| | Tab or M | close the pause menu (Select) |
| | Left Ctrl or C, V | R1 and L1: the next or previous slot or planet where a page uses them |
| | E | Circle, where a page reads it: the Quit page's quit, a setting stepped back |
| The map (ship's or pause menu's) | Enter or left click | travel to the planet shown (Circle) |
| | Space | the map's next page (Cross) |
| "Quit?" questions | Enter, Space or left click | Cross (what the game shows as Cross) |
| | Escape | go on playing |
| | Backspace or right click | Triangle (what the game shows as Triangle) |
| Memory card questions (create save data, format) | E | yes (Circle) |
| | Backspace or right click | no (Triangle) |
| Cutscenes, movies, the slideshow | Space, Enter, Escape, Backspace or a click | skip (Start) |
| The ship's landing, take-off and flight | the same | skip (Start and Cross) |

A key you are still holding when the game switches between play, a menu or a
scene is ignored until you let it go. So the Escape that opened the pause menu
doesn't also close it, and the Space that skipped a scene doesn't also jump.
Skipping a cutscene needs OpenRAC's *Skip cutscenes* option (on by default),
exactly as Start on a pad does.

- **Quick Select.** Hold right click to open the ring. Move the mouse
  towards the slot you want, then let go to take that item. Slot 1 is at the
  top, and the slots go clockwise from there. Small movements don't count, so
  the ring stays where it is until the mouse has moved a little way (the
  dead zone setting below). Moving the other way changes slot at once.
- **Number keys.** Keys 1 to 8 equip whatever is in that slot of the Quick
  Select, without opening the ring. The wheel steps through the slots that
  hold something. In a vehicle, these keys do nothing.
- **Strafe.** Left Shift holds L2. With the [lock-strafe](https://github.com/OpenRAC/openrac-mods/tree/master/lock-strafe)
  mod in its *Hold L2 or R2* mode, holding Left Shift strafes.
- **The mouse camera.** The mouse turns the camera itself, as in a PC
  game: the view turns as far as the mouse moves, in the same frame, with
  none of the slow easing and speed-up the console's right stick has.
  - Mouse right turns the view right and mouse up looks up, always. The
    game's own camera options (*Normal* or *Inverted*) are for the
    controller and don't change the mouse. If you want mouse up to look
    down, turn on *Invert the mouse's up and down* in the mod's settings.
  - This works for the camera behind Ratchet and for the first-person view
    (hold V). Behind Ratchet, the tilt stays where you leave it, unless you
    set *Camera tilt* to *Levels out*.
  - The game still moves the camera where it always does: it follows
    Ratchet, and a few places lock it or swing it round. In those places
    it stops the mouse just as it stops the right stick.
  - Moving a controller's right stick hands the camera back to the
    controller, which then works exactly as on the console.
  - A few scenes have a camera of their own. There the mouse still acts as
    the right stick.
- **Camera behind Ratchet.** Q swings the camera round behind him and
  levels it. Moving the mouse stops the swing.
- **The cursor.** It is hidden and kept inside the window while you play.
  You get it back in menus, on the title screen, during scenes, when you
  switch to another window, and when OpenRAC's own overlay (F3) is open.
- **The mouse in menus.** With the cursor free, it points at the game's own
  menus:
  - **Hover selects.** This works on the main menu and on the pause menu and
    its pages (Options, Help, Goodies…): the item under the cursor gets the
    focus, and in a list the selection moves to the row under it. In the
    item grids (Weapons, Gadgets, the Quick Select editor) it moves to the
    cell under it.
    Only the items the arrows could move to take the focus: a label or the
    Quick Select's item preview never does.
  - **Elsewhere, moving the mouse steps the selection** the way it moves, as
    the d-pad would. This covers the vendor, the "Quit?" questions and the
    ship's map.
  - **Clicks and the wheel.** Left click confirms; clicking an item that isn't
    selected yet selects it first, then confirms. Right click goes back, and
    the wheel scrolls lists.
  - The *Mouse in menus* setting turns hover and stepping off, leaving only
    the clicks and the wheel.
- **There is only one Quick Select** in the first game. It holds eight slots.
  In a vehicle, right click opens the vehicle's own ring.

## Changing the controls

In the launcher, go to **Mods** and choose **Settings** on the Keyboard and
mouse row (under **Built in**).

- **Rebind an action.** Every action shows its keys as buttons. Click one,
  then press the key, click the mouse button or turn the wheel you want for
  it.
  - Each action can have two binds. Click **Add** to give it a second one.
  - The **×** next to a bind removes it.
  - To cancel while the launcher is waiting for a key, click anywhere else.
- **Clashes.** If two actions share a key, both are marked *Also bound to …*.
  The game would do both at once, so give one of them another key. A key
  used both by a menu action and by an in-play action is not a clash, because
  the menu actions only count in menus.
- **Presets.** The **Controls** row sets every binding at once:
  - **Default**: the layout above.
  - **Left-handed**: move with I J K L; Right Alt walks; H is the wrench;
    Right Ctrl or N crouches; Right Shift strafes; U is first-person view;
    O swings the camera behind Ratchet.
  - **Arrow keys**: move with the arrow keys; Right Alt walks; Right Ctrl
    crouches; Right Shift strafes.

  You can change single bindings after choosing a preset.

These settings also adjust how the mouse and keys feel:

| Setting | Default | What it does |
|---|---|---|
| Mouse sensitivity | 1.0 | How far the camera turns for a given mouse movement. At 1, moving the mouse 100 pixels turns it about 9 degrees |
| Mouse acceleration | 0 | Above 0, a fast flick turns further than a slow drag. At 0 the turn is always in proportion to the movement |
| Invert the mouse's up and down | off | Mouse up tilts the camera down. The game's own invert options never change the mouse |
| Camera tilt | Stays | *Stays*: the camera behind Ratchet keeps the tilt you give it. *Levels out*: it comes back level when the mouse rests, as it does with a controller |
| Capture the cursor in play | on | Off leaves the cursor visible; the mouse then doesn't turn the camera |
| Mouse in menus | on | The cursor points at menu items (hover or steps); off keeps only clicks and the wheel |
| Walk speed | 0.5 | How fast the walk key moves Ratchet, from 0.2 to 0.9 |
| Stick ramp (ms) | 60 | How long the movement keys take to reach full speed: a tap nudges, holding runs |
| Quick Select dead zone | 30 | How far, in pixels, the mouse has to move before the ring picks a slot |

Changes take effect the next time the game starts. They are kept in the
version's settings file (`<data>/settings/<game>-<version>.ini`) as
`kbm_<key> = <value>` lines, beside the other PC settings (SETTINGS.md), so
updating OpenRAC keeps them.

**Matching another game's sensitivity.** At *Mouse sensitivity* 1 one count
of the mouse turns the camera about 0.09 degrees. Counter-Strike turns
0.022 degrees per count for each point of its sensitivity, so CS sensitivity
`s` feels the same here at `s / 4.09` (CS 2 is about 0.49 here).

## Good to know

- An installed copy of the old keyboard-mouse mod is not loaded any more;
  the game log says `keyboard-mouse: it is built into OpenRAC now`, and the
  launcher no longer lists it. Its settings were the launcher's: set them
  again on the built-in entry.
- If OpenRAC's own keys (F2, F3, F9 to F11) are bound to an action, they
  still do OpenRAC's thing as well.
- The game's log shows the line `[mod keyboard-mouse] keyboard and mouse on
  the pal disc`, plus a warning for any clash between bindings.
- A keyboard used as player 2 (Settings, Controls) keeps the port's own
  keyboard layout; the mouse and these binds are player 1's.

## Tests

`ctest -R "keyboard_mouse|active_device"`: the binds and their clashes, the
stick and the camera turn, the cameras through the game's memory, the Quick
Select ring, every game state's keys, the menus under the mouse, the switch
between devices, and the built-in entry loaded the way the port loads it.

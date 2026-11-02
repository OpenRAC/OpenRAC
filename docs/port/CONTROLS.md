# The controls

What presses the game's pad: the files that say it, how they layer, the
names in them, which controller each player is, and the window in the game
that changes them. Keyboard and mouse remapping are not done yet: the
keyboard keeps the button bindings of `bind_*` (keys named in them) and the
port's own stick keys, and the mouse is a mod's (the `keyboard-mouse` mod).

Code: [platform/bindings.h](../../port/platform/bindings.h) (the names, the
table the pad is pressed from, the tuning, what is held back when it
changes), [platform/controls_file.h](../../port/platform/controls_file.h)
(one file, line by line), [platform/players.h](../../port/platform/players.h)
(the controllers' ids, which is which player),
[game/common/controls_store.h](../../port/game/common/controls_store.h) (the
files on disk, read, written, watched),
[game/common/debug/controls_window.cpp](../../port/game/common/debug/controls_window.cpp)
(the window), [launcher/core/src/controls.rs](../../launcher/core/src/controls.rs)
(the launcher's side).

## 1. The files

| File | What it holds |
|---|---|
| `<data home>/openrac/controls/global.ini` | Every game's controls, the players |
| `<data home>/openrac/controls/<game>.ini` | One game's own controls, over every game's: `rac1.ini` serves both `rac1-pal` and `rac1-ntsc` |
| `<data home>/openrac/controls/devices.ini` | The controllers the port has seen, by id and name, for the launcher |
| `<data home>/openrac/controls/gamecontrollerdb.txt` | Optional: mappings, in SDL's own format, for controllers SDL does not know as gamepads; added to SDL's at start |

`<data home>` is the one the settings file is in ([SETTINGS.md](SETTINGS.md),
section 1); `--controls DIR` names another folder. A file is
`key = value` a line, `#` for a comment, as the settings file is; the last
line of a key counts. `format = 1` says which format wrote it: a file of a
newer format is read for what this build knows and never written, so going
back to an older build loses nothing.

The port writes a file when something changes in it (through a file beside
it, then a rename), and keeps every line it does not change, a comment or a
key it does not know included. It reads both files again within a second
when either changes on disk, so the launcher's changes apply while the game
runs.

## 2. Layers

Each player's controls are read from these layers, the later over the
earlier:

1. the port's defaults (the table below);
2. `global.ini`, its keys for every player;
3. `global.ini`, the player's own keys;
4. `<game>.ini`, its keys for every player;
5. `<game>.ini`, the player's own keys.

A player's own key is any `bind_*` or `pad_*` key with `p1.` or `p2.` in
front: `p2.bind_cross = PadNorth` makes North Cross for player 2 only, and
`p2.pad_right_invert_y = 1` turns over player 2's right stick. A player with
none of their own reads what every player reads. A game's change for every
player wins over the global file's change for one player, as the more
specific file always does.

A `bind_*` value replaces only the kinds it names: keys, or pad inputs. A
value with only pad inputs leaves the keys to the layer below, and the other
way about. So a game can change the controller's Cross and keep the
keyboard's, and an empty or misspelt value never leaves a control with
nothing. A `pad_*` setting replaces its own value. The players are the
global file's only.

Resetting:

- one control: its line in that file goes, and the layer below shows;
- a game: its file goes;
- every game: `global.ini` loses its `bind_*` and `pad_*` lines, the
  players' own included, and keeps the rest (which controller each player
  is);
- one player: that player's own lines (`p1.*` or `p2.*`) leave the file, and
  the player reads every player's controls again.

The bindings that [#123](https://github.com/OpenRAC/OpenRAC/pull/123) kept in
`settings/<id>.ini` (`bind_*` keys) move once into `<game>.ini` at the next
start, those the file does not already name, and leave the settings file.

## 3. Bindings

`bind_<control> = name, name, ...`, at most four keys and four pad inputs.

| Control | Key | Default keys | Default pad input |
|---|---|---|---|
| Cross | `bind_cross` | `Space` | `PadSouth` |
| Circle | `bind_circle` | `KeyE` | `PadEast` |
| Square | `bind_square` | `KeyF` | `PadWest` |
| Triangle | `bind_triangle` | `KeyR` | `PadNorth` |
| L1, R1 | `bind_l1`, `bind_r1` | `ShiftLeft`, `KeyQ` | `PadLeftShoulder`, `PadRightShoulder` |
| L2, R2 | `bind_l2`, `bind_r2` | `Digit1`, `Digit3` | `PadLeftTrigger`, `PadRightTrigger` |
| L3, R3 | `bind_l3`, `bind_r3` | `KeyC`, `KeyV` | `PadLeftStick`, `PadRightStick` |
| Start | `bind_start` | `Enter` (and Escape, always) | `PadStart` |
| Select | `bind_select` | `Backspace` | `PadBack` |
| D-pad | `bind_up`, `bind_down`, `bind_left`, `bind_right` | the arrow keys | `PadDpadUp` ... |
| Left stick | `bind_lstick_up`, `_down`, `_left`, `_right` | (W A S D, the port's own) | `PadLeftStickUp` ... |
| Right stick | `bind_rstick_up`, `_down`, `_left`, `_right` | (I J K L, the port's own) | `PadRightStickUp` ... |

Names:

- **Keys**: as the SDK and the launcher name them
  ([keys.h](../../sdk/include/openrac/keys.h)): `KeyW` is the key left of E
  on any layout, `Space`, `ShiftLeft`. Keys press buttons only; the sticks'
  keys are the port's own until keyboard remapping comes.
- **Pad inputs**, by position as SDL names them, so the bottom face button
  is `PadSouth` on any controller: `PadSouth`, `PadEast`, `PadWest`,
  `PadNorth`, `PadBack`, `PadGuide`, `PadStart`, `PadLeftStick`,
  `PadRightStick` (the clicks), `PadLeftShoulder`, `PadRightShoulder`,
  `PadDpadUp/Down/Left/Right`, `PadMisc1` to `PadMisc6` (a DualSense's mic,
  an Xbox pad's Share), `PadLeftPaddle1/2`, `PadRightPaddle1/2`,
  `PadTouchpad`, `PadLeftTrigger`, `PadRightTrigger`, and each stick's four
  directions, `PadLeftStickUp` ... `PadRightStickRight`.
- **Chords**: two pad inputs joined by `+` press only together
  (`PadLeftShoulder+PadSouth`). While a chord is held its second input does
  not also press what it presses alone; the first one does.
- `none` binds nothing, of either kind; `PadNone` binds no pad input and
  leaves the keys named beside it (`Space,PadNone`).

A trigger or a stick's direction on a button presses it past the trigger's
press point (below) or half way, with its pressure. A button on a stick's
direction pushes the stick fully that way; two opposite directions cancel.
Two controls may share a pad input: both press. The window marks them.

## 4. Controller settings

| Key | Values (default first) | What it does |
|---|---|---|
| `pad_left_deadzone`, `pad_right_deadzone` | `0`, up to `50` (percent) | A push shorter than this is none; the rest of the travel is stretched over the whole. The game has its own dead zone (`d >= 0x30`), so the port adds none by default |
| `pad_left_outer`, `pad_right_outer` | `100`, down to `50` | A push this long is a full one |
| `pad_left_sensitivity`, `pad_right_sensitivity` | `100`, `25` to `300` | A push is this much as long, held within the stick's square |
| `pad_left_invert_x`, `_invert_y`, `pad_right_invert_x`, `_invert_y` | `0`, `1` | The axis turned over |
| `pad_trigger_threshold` | about `13`, `1` to `100` (percent) | How far a trigger goes before it holds its button |
| `pad_face_buttons` | `position`, `labels` | `labels`: a pad marked Nintendo's way (A on the right) is read by its marks, so the button marked A is `PadSouth` |
| `pad_background` | `1`, `0` | The controllers count while another window has the keyboard |

The settings apply to the controller's own stick before anything reads it,
so a stick bound to another stick or to a button is tuned the same way. The
squaring that makes a round stick reach the DualShock 2's corners
([pad.h](../../port/platform/pad.h)) comes after them.

The game's dead zone is per axis (a cross on the stick), so on a diagonal
each axis loses 0x30 of only 0.71 of the push, and Ratchet crawls where a
straight push of the same length runs. He has two speeds: he runs once the
stick's length as the game reads it reaches 0.819 (`hero_speed_steps[0]`,
an axis pushed about 86% of the way), and below that walks at about a sixth
of the speed (0.9 against 5.7 units a second). Last of all, the port pushes each axis
out past that dead zone (`stick_pair_bytes`), so the game reads a diagonal in
its own direction and as long as a straight push. Straight pushes and the
corners give the same bytes as before.

## 5. Players

```text
player1 = auto, keyboard-fallback
player2 = auto
```

Each player names what it reads from: `auto` (the first controller no one
else has), a controller's id, `keyboard` (the keyboard, always),
`keyboard-fallback` (the keyboard while the player has no controller), or
`none`. The on-screen pad is always player 1's. A player's named controllers
come first, whichever order they connected in, and come back to that player
when they reconnect; `auto` after them is a fallback.

The defaults: the first controller is player 1 and the second player 2, and
the keyboard is player 1's only while no controller is. Plugging a controller
in takes the keyboard off the game and pulling it out gives it back; a key
held as that happens waits to be let go. Before this, the keyboard and the
first controller were player 1 together: `player1 = auto, keyboard` keeps
that. The port's own keys (F1 to F13, Alt) and the mods' keyboard are not
players' and always work.

A controller's id is SDL's GUID for its model, with its serial number where
SDL has one (`<guid>-<serial>`); two of one model without serials are told
apart by the order they connected in (`<guid>#2`). SDL's instance numbers,
new each time, are never kept.

## 6. The window

F13, or Controls in the menu bar (Windows and Linux), the macOS menu, or the
touch drawer, opens it over the game. It changes the running game's file or
the global one, for every player or for player 1 or 2 alone ("For:"; a
player chosen, "Player N as every player" takes their own controls away):

- **Bindings**: each control, what presses it on a controller, where that
  came from (default, every game, this game), and a mark on a pad input
  another control has too. Change, Add and Chord wait for a press on any
  controller; the game reads nothing from the controllers meanwhile, and Esc
  cancels. A press another control has alone is offered to be taken from it.
- **Sticks and triggers**: section 4's settings, with the stick drawn as the
  controller gives it and as the game will read it.
- **Players**: the controllers connected, with a light that shows a press,
  each player's controller and keyboard, and what the game reads from each
  player now.

A change is written and applied at once. A button held as its binding
changes (the capture's own press, a thumb resting on it) does not press its
new control until it is let go and pressed again; the pad is read afresh
each frame, so a button let go, a controller pulled out or a binding changed
never leaves one held.

## 7. What it cannot do

- The game never sees pressure: the pad library's replacement
  ([lib/pad.c](../../port/game/common/lib/pad.c)) rebuilds it from the
  buttons. A trigger's travel presses a button past the press point; its
  depth goes no further ([CONTROLLERS.md](CONTROLLERS.md), section 8).
- No game asks for rumble, so there is no vibration setting.
- Keyboard keys on the sticks, chords of keys, the mouse: later, with
  keyboard remapping. The `keyboard-mouse` mod does them now.
- A controller SDL does not know as a gamepad, and that no mapping in
  `gamecontrollerdb.txt` (section 1) names, is not read. SDL's own list
  covers most.
- The port's keys (F1 to F13, Alt) do what they do whatever is bound.
- The defaults are the same DualShock 2 layout for every game.

The launcher has the commands a controls page will use (`controls_view`, for
every player or one, `controls_set`, `controls_reset`, `controls_reset_player`,
`controls_reset_players`, `controls_export`, `controls_import`,
`controls_copy`) and no page yet.

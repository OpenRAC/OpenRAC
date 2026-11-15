# Settings

Settings are options a player changes in the launcher's Mods tab, under your
mod's row. You declare them in the manifest; the launcher draws the controls,
keeps the values, and the game hands them to your mod at start. A change counts
from the game's next start.

## Declaring them

```json
"settings": [
  { "key": "mode", "name": "Mode", "type": "choice", "default": "lock",
    "choices": [ { "value": "lock", "name": "Lock with weapons" },
                 { "value": "hold", "name": "Hold L2 or R2" } ],
    "description": "How the strafe starts." },
  { "key": "trace", "name": "Trace", "type": "bool", "default": false },
  { "key": "every", "name": "Log every", "type": "int", "default": 10, "min": 1, "max": 600 },
  { "key": "scale", "name": "Speed", "type": "float", "default": 1.0, "min": 0.5, "max": 3.0, "step": 0.1 },
  { "key": "greeting", "name": "Greeting", "type": "string", "default": "hi", "max_length": 40 }
]
```

| Field | |
|---|---|
| `key` | lower case, digits and `_`: what your code asks for |
| `name` | the label players see |
| `type` | `bool`, `int`, `float`, `choice`, `string` or `keybind` (interface 1.2) |
| `default` | the value until the player changes it |
| `description` | one line under the label (optional) |
| `min`, `max` | `int` and `float` |
| `step` | `float` |
| `choices` | `choice`: two or more `{"value", "name"}` |
| `max_length` | `string` |
| `binds` | `keybind`: how many inputs it takes, 1 or 2 (2 when absent) |
| `group` | `keybind`: keybinds of the same group may not share an input (the launcher warns); of different groups they may |

The launcher shows a switch for `bool`, a segmented control or a list for
`choice`, "press a key" buttons for `keybind`, and a field for the others.

A mod for several games ([manifest](manifest.md#a-mod-for-several-games)) may
give a setting `"games": ["rac1"]`, the games it is for: the launcher shows it
there only, and the mod gets its default in the others.

## Key bindings

A `keybind` (interface 1.2: `"api": "1.2"`) holds up to two inputs as
names, comma-separated: `"KeyF,MouseMiddle"`, or `""` for none. Keys are
named by where they are on the keyboard, as the web's `KeyboardEvent.code`
names them (`KeyW`, `Digit1`, `Space`, `ShiftLeft`, `ArrowUp`, `Numpad0`,
`F5`); the mouse's are `MouseLeft`, `MouseRight`, `MouseMiddle`, `MouseBack`
and `MouseForward`, the wheel's `WheelUp`, `WheelDown`, `WheelLeft` and
`WheelRight`. `openrac/keys.h` has the whole table and turns a name into
what `input()` reports (see [native mods in C](native-c.md)).

```json
{ "key": "jump", "name": "Jump", "type": "keybind", "default": "Space", "group": "play" },
{ "key": "menu_back", "name": "Menu: back", "type": "keybind", "default": "Backspace", "group": "menu" }
```

## Presets

Presets (interface 1.2) set several settings at once, of any type: sliders,
choices, switches, text and keybinds. The launcher shows them as buttons above
the settings, beside a Default button of its own (a preset with the id
`default` takes its place); the button whose values the settings have is
lit. Each value must be one its setting takes: in range, one of the choices,
and so on.

```json
"presets": [
  { "id": "easy", "name": "Easy",
    "values": { "damage_taken": 0.5, "enemy_count": 0.75, "mode": "hold", "chaos": false } },
  { "id": "arrows", "name": "Arrow keys",
    "values": { "move_forward": "ArrowUp", "move_back": "ArrowDown" } }
]
```

A preset covers every keybind of the mod and each setting one of its presets
names: the ones of those it does not name go back to their defaults. A
setting no preset names (a Trace switch, a seed) keeps the player's value.

## Reading them

| | |
|---|---|
| Luau | `openrac.setting("mode")`: a boolean, number or string; `nil` for a key the manifest does not declare |
| C++ | `mod.setting<std::string>("mode")`, `setting<bool>`, `setting<int>`, `setting<double>`, ... |
| C | `api->setting_string(mod, "mode")`, `setting_bool`, `setting_int`, `setting_float` |

A keybind reads as a string (`setting_string`, `setting<std::string>`); in C and
C++ `openrac_bind_from_name_n` (`openrac/keys.h`) turns each name into a bind.

A value that is missing, of the wrong type or out of range is the default:
your code always gets a valid value.

Where the values are kept: [where mods live](where-mods-live.md)
(`plugins.json`).

## Changing them from the game (interface 1.15)

A mod with an options page of its own (a page of the pause menu, say) sets its
settings there, and the launcher shows them and the next start has them:

| | |
|---|---|
| Luau | `openrac.set_setting("mode", "hold")`: `true` when it was set |
| C++ | `mod.set_setting("mode", "hold")`, `set_setting("count", 3)`, `set_setting("trace", true)`: `true` when it was set |
| C | `api->setting_set_string(mod, "mode", "hold")`, `setting_set_bool`, `setting_set_int`, `setting_set_float`: `OPENRAC_HOOK_OK` when it was set |

The value is checked as the launcher checks the list: the setting's type (a
choice, a string and a keybind are set as strings), its range, its choices,
its length and its binds. A key your manifest does not declare, another type or
a value out of its limits is refused, and the setting keeps its value. Set, it
is read back at once, and written into `plugins.json`; in purist mode it is
this run's only.

To hear the player change your settings in the launcher while the game runs,
ask during load:

| | |
|---|---|
| Luau | `openrac.on_settings(function() ... end)` |
| C++ | `mod.on_settings([] { ... })` |
| C | `api->on_settings(mod, fn, user)` |

The port looks at the list about once a second; after it changed, your
function runs once the new values are yours. Your own `set_setting` calls are
not told. A mod that does not ask keeps the values it started with.

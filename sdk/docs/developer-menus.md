# Developer menus: a mod's own tab in OpenRAC's menu

A native mod can add a tab of its own to OpenRAC's menu (the overlay the player opens over the
game, on a computer as on Android): rows the player presses, and since interface 1.19 switches
and choices. A cheats menu, a savestate list, a debug view turned on and off. Native mods,
interface 1.10 (1.19 for switches and choices).

## Making the menu

| Call | Since | What |
|---|---|---|
| `developer_menu(mod, title, items, count)` | 1.10 | The tab `title` and its rows. Only from `openrac_mod_load`. Each item has a label, an optional shortcut shown beside it, a nonzero action id of the mod's, and an optional group: rows of a group go on a card under its heading |
| `take_developer_menu_action(mod)` | 1.10 | The next action the player chose, or 0 for none. Only from the mod's tick-boundary callback |
| `developer_menu_set_label(mod, action, label)` | 1.10 | A row's new label. Only from the tick-boundary callback |
| `developer_menu_switch(mod, action, on)` | 1.19 | Makes the row a switch, on or off, or sets it again |
| `developer_menu_choice(mod, action, values, count, index)` | 1.19 | Makes the row a choice among `values`, shown between two arrows, or sets it again |
| `developer_menu_value(mod, action)` | 1.19 | A switch's 0 or 1, a choice's index; -1 for a plain row |

A switch or a choice is made, or set again, from `openrac_mod_load` (after `developer_menu`) or
the tick-boundary callback. The C++ layer has the 1.19 calls as `mod.menu_switch()`,
`mod.menu_choice()`, `mod.menu_value()` and `mod.has_menu_switches()`; the 1.10 ones are in
`mod.api()`. The [C interface](reference/c-api.md) says every case each refuses.

## The menu pauses the game

Opening OpenRAC's menu pauses the game, and while it is paused no tick runs, so the mod's
tick-boundary callback does not run either. A press on a plain row is queued and reaches the mod
only once the menu is closed; a label the mod changes in answer shows the next time the menu
opens. A row whose label says a state ("Invincible: Off") therefore looks stuck while the menu is
open.

A switch or a choice does not wait: the menu turns it over or moves it as soon as the player
presses it, and queues its action as a press. At its tick boundary the mod takes the action and
reads the state the row was left in, rather than turning its own state over: two presses before
the menu closes queue the action twice and leave the switch as it was.

```c
enum { ACTION_INVINCIBLE = 1, ACTION_SPEED = 2 };
static const char* const speeds[] = {"x1", "x2", "x4"};
static int invincible, speed;

static void on_tick_boundary(void* user, uint32_t tick) {
    uint32_t action;
    while ((action = api->take_developer_menu_action(mod)) != 0) {
        if (action == ACTION_INVINCIBLE) {
            invincible = api->developer_menu_value(mod, ACTION_INVINCIBLE);
        } else if (action == ACTION_SPEED) {
            speed = api->developer_menu_value(mod, ACTION_SPEED);
        }
    }
    /* A state the mod changed itself (a hotkey, the pause menu's page): the row follows. */
    api->developer_menu_switch(mod, ACTION_INVINCIBLE, invincible);
}

/* In openrac_mod_load: */
const openrac_developer_menu_item items[] = {
    {"Invincible", NULL, ACTION_INVINCIBLE, NULL},
    {"Speed", NULL, ACTION_SPEED, NULL},
};
api->developer_menu(mod, "Cheats", items, 2);
if (api->api_minor >= 19) {
    api->developer_menu_switch(mod, ACTION_INVINCIBLE, 0);
    api->developer_menu_choice(mod, ACTION_SPEED, speeds, 3, 0);
}
api->on_tick_boundary(mod, on_tick_boundary, NULL);
```

On a port older than 1.19 the rows stay plain rows: a mod that wants to load there too keeps a
state in their labels and turns its own state over on each press, as 1.10 mods do.

## Limits

- A title is the mod's own: "Game", "View", "Graphics", "Sound", "Developer" and another mod's
  titles are refused, and so is an action id used twice.
- The menu never calls the mod's code: everything the mod learns, it learns at its tick boundary.
- Script mods have no developer menu.

# Gameplay decision queries (native SDK 1.40, proposed)

`on_play_query` lets a native mod decide a small part of a game operation without
replacing the game function. The host emits a synchronous `openrac_play_query` on
the game thread. Only `value` is an output; `original` is the unchanged game value,
and `alternate` is a supplied alternative such as the first-visit reward or an
unlocked menu entry. With no owner, on unload, or after a failed mod load, the
original game value is used. The decision is never written to a save or to private
activation bytes.

Register during `openrac_mod_load`, with a nonempty bit mask of query kinds and a
callback. Each kind has one owner: another registration for an owned kind returns
`OPENRAC_HOOK_TAKEN`. A game with no query points returns `OPENRAC_HOOK_UNKNOWN`.
Currently all three Going Commando discs have the same points. A callback must not
throw; if it does, its registration is disabled until unload and queries fall back
to the original value. A nested query while a callback is running also falls back
to its own original value. Invalid boolean or negative reward outputs are ignored.

| Kind | Subject / index | Original / alternate |
|---|---|---|
| CHALLENGE_AVAILABLE | controller if available; index 0 arena, 1 spaceship, 2 race | prerequisite boolean / 1 |
| ITEM_RESPAWN | controller; item ID, or -1 for a trigger-based return | automatic item-related spawn/return guard / 0 |
| REVISIT_BOLTS, REVISIT_XP | subject 0; placement-record index | revisit reward / first-visit reward |
| MENU_TEXT, MENU_DATA | extras menu table; entry 0 or 1 | computed text/data / unlocked text/data |
| MENU_ACTION | subject 0; shortcut menu action 0 | permit the original action (1) / cancel (0) |
| HERO_OVERRIDE | moving gate moby; relevant item ID (50) | hero override guard / 0 |

Boolean query values must be 0 or 1. A mod that cancels MENU_ACTION can request a
supported `play_travel` destination through the ordinary API. Text IDs and data
are those of the running disc; use the supplied alternate rather than hardcoded
localized values. `subject` is an offset in game memory, not a native pointer.
These points are synchronous and may occur during level loading or paused menus;
a mod should keep its handler short and avoid network operations there.

The twenty native source locations and verified definition hashes are recorded
in `port/game/rac2/play_queries.json`. Hostgen inserts neutral queries into build
copies only. Every definition hash and expected insertion count must match; source
drift stops generation rather than silently losing part of a feature. No generated
function body or retail data belongs in a source distribution. The matching
decompilation is never edited.

These query points are general-purpose. They contain no Archipelago settings,
seed state, item receipts or hub destination. Those decisions belong to the mod.

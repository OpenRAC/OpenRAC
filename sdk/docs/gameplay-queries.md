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

Challenge queries apply only to boolean prerequisite conditions. Spaceship completion
records also select reward-table entries (0, 1 or 2); those numeric reads remain
unchanged and never pass through the boolean query.

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

## Review validation

Spaceship result-panel tests exercise reward index 2 with no overriding callback
and with prerequisites overridden, and verify that each disc keeps its numeric
reward-index reads. Only the six prerequisite reads in that panel are queried.

The unrelated `tail_callee` workaround and empty headless menu-page provider were
removed from this proposal. Upstream's RAC3 return-value correction is included
by merging `develop`; `program.py` is identical to upstream.

Native hostgen output was compared with upstream `2fa98d071` using the same
Clang, game configurations, header paths and pinned sources (the same standard
OpenRAC decomp patch series applied to RAC1). Both generators ran
with `OPENRAC_HOSTGEN_NO_DISC=1`. All emitted `.c` and `.h` files were compared
byte for byte, excluding prepared input copies; no path or content normalization
was applied.

| Program | Pinned source | Generated C/header files | Differences |
|---|---|---:|---:|
| rac1-pal | f911d6e4b971 | 398 | 0 |
| rac3-pal | 0ed135192c8b | 5,479 | 0 |

To reproduce, extract that upstream revision's `port/tools/hostgen`,
`port/tools/versionmap`, `port/game` and `tools/openrac_common`. Prepare the pinned
sources with the standard decomp patches, then run its `hostgen.py` and this
checkout's `hostgen.py`
against the same absolute `--game` and `--source` paths, using separate `--out`
directories. The game configurations and header paths must be identical: anonymous
type names include header paths. For RAC1 use `port/game/rac1-pal/hostgen.json`
and `games/rac1/decomp`; for RAC3 use `port/game/rac3-pal/hostgen.json` and
`games/rac3/decomp`. Compare every generated C/header relative path and its bytes.

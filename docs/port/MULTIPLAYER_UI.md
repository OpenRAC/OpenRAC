# Native multiplayer screens

How OpenRAC draws the multiplayer menus' lists, staging room and scoreboards as native screens
that look like the game's own, and how a server can change them. The core is
[port/mpui](../../port/mpui/README.md); the games reach it through
[openrac/mp_ui.h](../../port/game/common/include/openrac/mp_ui.h).

## 1. Shape

```
 server UI description (JSON, versioned)          game's lobby flow (level 39, translated)
            |  UI packet (Server, id 10)                       |  screen ids, pad events, data
            v                                                  v
 port/mpui  (openrac::mpui, C++, knows no game)       port/game/rac3-pal/host/mp_ui.c (C)
   Description  -- parse + validate, vanilla default     binding: hooks, reads game data,
   Model        -- tables of rows (games, players)       maps actions onto the game's own controls
   Screen tree  -- components, layout, focus             skin: draws with level 39's own routines
   Navigator    -- events -> focus, scroll, actions             ^
            ^                                                  |
            +---- C contract openrac/mp_ui.h (game/common/mp_ui.cpp) ----+
```

- **Core** (`port/mpui/`, target `openrac_mpui`, layer 2, depends on `common` only): no drawing API,
  no game address. Unit-tested on its own.
- **Contract** (`port/game/common/include/openrac/mp_ui.h`, implemented in `game/common/mp_ui.cpp`):
  C types only; the game's C feeds events and rows and gets back draw commands and actions.
- **Per game**: a *skin* (fonts, colours, panel and highlight drawing, sounds, through the game's own
  routines) and a *binding* (which game screen each native screen sits on, where its rows come from,
  how each action reaches the game). rac4 writes only these two files.

## 2. Components (core)

| Component | What | Scales by |
|---|---|---|
| `Screen` | title, a column of components, footer hints; replaces or overlays one game screen | |
| `Label` / `Text` | a line or wrapped text (announcements, MOTD) | |
| `Button` | Cross runs its action | |
| `Choice` | Left/Right step values (game options, server options) | |
| `Table` | rows x columns of a model table, a cursor, a window of `rows` visible, scroll arrows, "n / m" | any row count: window + L1/R1 pages |
| `PlayerList` | a `Table` of the room's players (name, skill, team, skin, ready, ping) | 24+ |
| `Scoreboard` | a `Table` sorted by a column, team groups | 24+ |
| `Dialog` | modal message / yes-no over a screen | |

Layout is a vertical stack in the game's 512x448 picture: each component asks the skin for its
height (font line height, row pitch), the screen places them inside its panel rectangle. A table's
visible rows = what fits (or the description's `rows`), never a fixed 8.

## 3. Data model

`Model` holds named tables (`games`, `players`, `scores`, server-defined ones), each a list of rows of
string cells keyed by column field (`name`, `players`, `map`, `mode`, `skill`, `team`, `skin`,
`ready`, `ping`, ...), plus a stable row key (world id, client id) so the cursor stays on the same
game or player when the list refreshes. The binding refills tables each frame (cheap: diffed by key).

## 4. Navigation and input

Events are the game's own (`Up Down Left Right Cross Circle Triangle Square L1 R1 Start Select`); the
binding converts the game's event (rac3: 0x00686138's codes) to them. The navigator moves focus over
focusable components, the cursor inside a table (wraps, keeps it visible), L1/R1 page a table, and
plays the skin's sounds (move, select, back, denied). An event the native screen does not use goes
on to the game unchanged (Triangle back, Square, Start), so the original flow keeps working.

## 5. Server UI description (schema 1)

JSON, at most 32 KB, `"schema": 1`. Unknown keys are refused (a newer server's extras must bump
`schema`), every string is limited (titles 40, labels 32, text 512 characters), counts capped
(screens 16, columns 8, items 24, choices 16). A description that fails validation is dropped whole
and the client keeps the vanilla one; the reason is logged.

```json
{
  "schema": 1, "name": "OpenRAC", "revision": 4,
  "limits": { "max_players": 24 },
  "text": { "motd": "Welcome", "announcements": ["Double XP weekend"] },
  "modes": [ { "id": 2, "name": "SIEGE" }, { "id": 101, "name": "KING OF THE HILL", "base": 2 } ],
  "maps":  [ { "id": 40, "name": "BAKISI ISLES", "modes": [1, 2, 3] } ],
  "options": [ { "key": "no_v2s", "label": "WEAPON V2S", "choices": ["ON", "OFF"], "scope": "game" },
               { "key": "vote_map", "label": "NEXT MAP", "choices": ["BAKISI", "HOVEN"] } ],
  "screens": [
    { "id": "game_list", "title": "FIND GAME", "rows": 10,
      "columns": [ { "field": "name", "label": "GAME", "width": 50 }, { "field": "players", "label": "PLAYERS", "width": 20 } ] },
    { "id": "staging", "rows": 12, "columns": [ ... ] },
    { "id": "rules", "title": "SERVER RULES", "on": "lobby",
      "items": [ { "kind": "text", "label": "No spawn camping" },
                 { "kind": "button", "label": "ACCEPT", "action": "back" } ] }
  ]
}
```

Custom content sits on the game's own: a mode or map with `base` is a server's custom one that
runs on that stock mode or level (a map's `modes` may force its mode). An option's `scope` says
whose it is: `server` (the default) is the player's own choice, sent with `ACTION`; `game` is a rule
of one game, which its host sets in the staging room and every member gets. These follow what the
existing private servers for these games offer: custom modes over stock ones, custom maps loading a
stock level, and a host-set block of game rules shared with the room (the Horizon Private Server
project, https://github.com/Horizon-Private-Server/horizon-server and its UYA plugin, read for
reference only; no code was taken). OpenRAC's descriptions carry them as data, never as code.

Actions are a closed set: `back`, `open:<screen>`, `game:<name>` (the binding's own: `join`,
`ready`, `start`, `leave`, `team`, `create`), `server:<key>` (sends `UI ACTION key value` to the
server). Screens with an id the binding knows (`game_list`, `staging`, `scoreboard`) sit on the
game's screen; others are extra native screens opened by `open:` or shown on a hook (`on`).

**Vanilla fallback**: the client carries a built-in description equal to UYA's own layout (its
columns, eight rows a page as the game shows, no extras). With no server description it is used,
so a player sees the original screens.

**Wire** (Server, `:common` enums): packet `UI` (id 10), feature bit 32. Opcodes: `GET` (client:
family) -> `DESCRIPTION` (server: revision int, JSON bytes), `ACTION` (client: key string, value
int) -> `NOTICE` (server: text string, shown as a dialog) or a new `DESCRIPTION`. The server reads
`ui/<family>.json` from `UI_DIRECTORY` (validated at start, rejected files logged), falling back to
its built-in vanilla description.

## Modal screens and notices

A screen opened by an `open:` action, or a server's `NOTICE`, is drawn modally over whatever is up,
on a panel of its own (a field and a one-pixel edge in the game's colours). It takes every event; Back closes it, and the game's screen under it never sees the press.
A notice is its text, wrapped, over an OK button.

## Trying descriptions and long lists

`OPENRAC_RAC3_MP_UI=FILE` makes the lobby use a description file in place of the vanilla one until a
server sends its own, and `OPENRAC_RAC3_MP_UI_MOCK=N` adds `N` made-up players to the staging and
score lists, to try their paging past eight rows before the game's tables hold more
([DEBUGGING.md](DEBUGGING.md)).

## 6. Binding to rac3's lobby (level 39, PAL; NTSC-U carried)

| Hook (mp/libraries.tsv wrap) | Why |
|---|---|
| 0x00686138 menu event from the pad | read the event first: a native screen takes what it uses, the rest goes on; injects `Cross` for mapped actions |
| 0x006862D0 screen manager update | after it, note the current screen id (mgr+0x1C0[mgr+0x270]) and hide the game widgets the native screen replaces |
| 0x00686768 screen manager draw | hide the game widgets the native screen replaces for this draw alone, so the game's focus still reaches them |
| 0x005D69C0 GUI element render | place the native cursor's bar (the game's own element) before it, print the native text after it |

Bound screens: staging (0x21: the player rows, widgets 0xF+n..0x57+n, become a `PlayerList` of up
to 24, from tNW_GameSettings for slots 0-7 and the port's room for the rest), the two game lists
(0x0F, 0x19: widget 8 becomes a `Table` of the lobby's records), later the end-of-match
scoreboard. Actions reuse the game's own controls: JOIN on native row k sets the game list's
cursor (widget vtable +0x88) and focus (screen+0x290 = 8), then injects Cross, so the game's own
join path runs; ready, team and start do the same on the staging widgets. Nothing calls the lobby's
internals directly.

## 7. Drawing: one draw layer

The core draws through one layer the game supplies (`openrac_mp_ui_skin`): text, filled rectangles,
the game's images (sprites, by the skin's own numbering) and the highlight bar, with the font's
measures, the panel's colours and the menu sounds. Nothing in the core knows how they reach the
screen, so a game's skin can move to the port's native 2D path without touching the screens.

rac3's skin today prints with level 39's FontPrint (font via FontSelect 0x00591BA0, widths
FontTextWidth 0x00591F98) in the menus' gold 0x8066CCFF, focused pulse to 0x66D6EEFA over 0x25
frames, disabled 0x80808080, and places the game's own highlight-bar element (0x00689F00); sounds
are 0x00688340 (3 move, 4 select, 0x17 back). Rectangles and images draw nothing yet: they arrive
with the native UI work, which replaces those draw functions with the port's GS-free 2D items
(`openrac_game_ui_item`). Button tokens are the SDK 1.31 characters (`\x10` Cross ...).

## 8. rac4 reuse

rac4 adds `port/game/rac4-*/host/mp_ui.c` with its skin (its font and box routines) and binding
(its screen ids and widget numbers). The core, the contract, the schema and the Server side are
shared; the server keys descriptions by game family, so `rac4-*.json` sits beside `rac3-*.json`.

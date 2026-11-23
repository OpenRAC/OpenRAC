# mpui

The native multiplayer screens: the lobby's game lists, the staging room's players, scoreboards and
the screens a server adds, as components laid out in the game's picture, with focus, cursors that
scroll through any number of rows, and the data model they read. A server's UI description (JSON,
schema 1) says which screens, columns, options and text a lobby shows. It knows no game: each game's
binding fills the tables from its memory, hands the screens its menu events and draws them through
a skin of its own routines (openrac/mp_ui.h). It does not draw, play sounds or talk to a server
itself.

## Interface

- `description.h`: `Description` and its parts, `parse_description` (validates schema 1 whole),
  `parse_action`.
- `model.h`: `Row`, `Table`, `Model`: the tables a screen reads, rows keyed so a cursor survives a
  refresh.
- `skin.h`: `Skin`, the game's look (font measures, text, highlight bar, panel, scroll marks,
  sounds); `Rect`, `TextStyle`, `Sound`.
- `screen_view.h`: `ScreenView`, one screen laid out in an area: `handle(Event)` gives an `Outcome`
  (consumed, action, row), `draw(Skin)`; `Event`, `Mode`.

The games' C reaches it through `game/common/include/openrac/mp_ui.h`, implemented in
`game/common/lib/mp_ui.cpp`.

## Depends on

`common` (the JSON reader, `Result`). No third-party library.

## State and threads

A `ScreenView` and a `Model` are plain objects their owner keeps (the contract's context, which the
game's binding owns). Only the game's thread uses them.

## Errors

`parse_description` returns `Result<Description>`: the message names the first key or value that
breaks the schema, and nothing of a refused description is kept. The rest cannot fail: an event a
screen has no use for comes back not consumed. The contract logs a refused description as a warning.

## Configuration

None. The description comes from the game's binding (its vanilla layout) or the server.

## Tests

- `mpui_description`: a full description is read as written; other schemas, bad JSON, unknown keys
  and every limit are refused; actions are the closed set.
- `mpui_screen_view`: a table shows the rows that fit or the spec's; Down scrolls past a page,
  L1/R1 page, Up wraps; the cursor follows its row by key across refreshes; disabled rows refuse;
  an overlay passes unused events to the game; choices step; drawing shows the page, the bar, the
  page count and the marks; long cells are cut to their column.

## Further reading

[docs/port/MULTIPLAYER_UI.md](../../docs/port/MULTIPLAYER_UI.md): the design, the schema, the wire
and the games' bindings.

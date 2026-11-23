# The 2D layer every game shares

`openrac/ui.h` (`game/common/include`): what a game's wrapped 2D routines (HUD, text, menus) hand the window in place of
drawing, from the game's own data. The core turns it into native quads (`openrac/ui_item.h`) and
records them through the frame recorder (`openrac/frame.h`); no GS word is made or read. A game
says where its data is through an `openrac_ui_adapter` (its 2D screen, its font pages, its image
tables); the core knows no game. Design: [FRAME_RECORDER.md](../../docs/port/FRAME_RECORDER.md),
section 6.

## Interface

- `openrac_ui_set_adapter`, `openrac_ui_screen`: the game's adapter and 2D screen.
- `openrac_ui_draw_glyph`: a glyph of text from a font page's cell, drawn from the page's distance
  field (sharp at any resolution).
- `openrac_ui_draw_quad`, `openrac_ui_draw_rect`: a quad of a game image (or of colour alone), a
  colour a corner, shaded or flat.
- `openrac_ui_scissor`: the game's own 2D scissor, in its screen pixels.
- `openrac_ui_image`: an image of one of the game's tables, through its adapter.

## Depends on

`openrac/frame.h` (the recorder's C API, `game/common/program/game_host.cpp`). Built into
`openrac_game_common` (`cmake/Games.cmake`), which every game links.

## Games

- Up Your Arsenal: `port/game/rac3-pal/host/ui_adapter.c` (its 2D screen, fonts from the font
  headers), `host/ui2d.c` (the prints' glyphs, flat boxes, the scissor).

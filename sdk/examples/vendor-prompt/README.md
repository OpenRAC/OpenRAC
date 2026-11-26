# Vendor prompt (example)

An example for mod authors, for mod interface 1.31: a panel with a button
prompt, "Square Buy all", drawn over the Gadgetron vendor in Ratchet & Clank
and Going Commando.

- `on_overlay_phases(fn, user, OPENRAC_OVERLAY_MENU)` draws over the frames of a
  level with a menu up, where `on_overlay` draws nothing; `overlay->phase` says
  which phase a frame is.
- `OPENRAC_UI_SQUARE` in the overlay's text is a button token: the port draws
  it as the player's device has Square (the game's icon on a PlayStation pad, X
  on an Xbox pad, the bound key in brackets on the keyboard and mouse). The same
  tokens work in `hud_text_set`'s lines.
- `button_label(OPENRAC_PAD_SQUARE, &kind)` gives that label as text.

```cpp
mod.on_overlay_phases(draw, nullptr, OPENRAC_OVERLAY_MENU);
// in draw: the panel, then the line with its token
overlay->rect(overlay->context, 176, 268, 160, 28, 0x000000B0u);
overlay->text(overlay->context, 200, 274, 16, 0xFFFFFFFFu, OPENRAC_UI_SQUARE " Buy all");
```

Build it as the other example:

    cmake -S sdk/examples/vendor-prompt -B build/vendor-prompt -G Ninja
    cmake --build build/vendor-prompt

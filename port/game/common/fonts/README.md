# The in-game menu's typefaces

The launcher's (`launcher/docs/STYLE.md`), so the menu in the game reads like the launcher:
Inter for its text, Orbitron for its headings. Both are under the SIL Open Font License 1.1
(`OFL-Inter.txt`, `OFL-Orbitron.txt`). `cmake/Games.cmake` (`openrac_menu_fonts`) builds them
into the game as bytes; `debug/menu_ui.cpp` hands them to Dear ImGui.

- `Inter-Regular.ttf`, `Inter-Medium.ttf`, `Inter-SemiBold.ttf`: Inter 4.1 (`extras/ttf/` of
  https://github.com/rsms/inter/releases/tag/v4.1), cut to Latin and Latin Extended-A, a few
  dashes, quotes and arrows (Inter has no reserved font name, so a subset may keep it):

  ```sh
  pyftsubset Inter-Regular.ttf --unicodes="U+0020-007E,U+00A0-017F,U+2013-2014,U+2018-201E,U+2022,U+2026,U+00B7,U+2190-2193,U+2212" \
      --layout-features='kern' --no-hinting --output-file=Inter-Regular.ttf
  ```

- `Orbitron.ttf`: `ofl/orbitron/Orbitron[wght].ttf` of https://github.com/google/fonts, as it
  is (Orbitron is a reserved font name, so the file is not changed).

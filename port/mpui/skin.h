// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What a game gives the multiplayer screens to look like its own: its font's measures, its text,
// highlight bar and panels drawn with its own routines, and its menu sounds. Positions are in the
// game's 512 x 448 picture, as the games' 2D layers place them.

#pragma once

#include <cstdint>
#include <string_view>

namespace openrac::mpui {

// A rectangle in the game's picture: its top left corner and its size.
struct Rect {
    float x = 0.0f;  // Left edge.
    float y = 0.0f;  // Top edge.
    float w = 0.0f;  // Width.
    float h = 0.0f;  // Height.
};

// How a text is drawn: the game decides the font, size and colour of each.
enum class TextStyle : std::uint8_t {
    TITLE,     // A screen's title.
    HEADING,   // A table's column headings and its page count.
    NORMAL,    // A row, a label, a button not under the cursor.
    FOCUSED,   // The component under the cursor (the game's pulsing colour).
    DISABLED,  // A component or row that refuses Cross.
};

// The menu sounds a screen plays.
enum class Sound : std::uint8_t {
    MOVE,    // The cursor moved.
    SELECT,  // A choice was made.
    BACK,    // The screen was left.
    DENIED,  // A disabled component was chosen.
};

// The parts of a modal screen's panel, each in a colour of the game's.
enum class PanelPart : std::uint8_t {
    FILL,  // The panel's field.
    EDGE,  // Its edge, a pixel wide.
};

// The game's look, as the screens draw it: one draw layer of text, filled rectangles, the game's
// images and its highlight bar, which a game implements with its own drawing.
//
// The binding owns it and keeps it alive while a screen draws; the screens call it on the game's
// thread, inside the game's frame, so what it draws lands in the game's own 2D list.
class Skin {
public:
    virtual ~Skin() = default;

    // Returns the height of one line of text, the pitch of a table's rows.
    virtual float line_height() const = 0;

    // Returns how wide `text` is drawn in `style`.
    virtual float text_width(std::string_view text, TextStyle style) const = 0;

    // Draws `text` with its top left at (`x`, `y`). The button tokens of the mod SDK (`\x10`
    // Cross ...) are drawn as the game's icons.
    virtual void text(float x, float y, std::string_view text, TextStyle style) = 0;

    // Draws the game's highlight bar over `rect`, behind the text drawn after it.
    virtual void highlight(const Rect& rect) = 0;

    // Fills `rect` with `rgba` (0xAABBGGRR, the games' 0x80 alpha is full).
    virtual void rect(const Rect& rect, std::uint32_t rgba) = 0;

    // Draws the game's image `sprite` (the skin's own numbering) stretched over `rect`.
    virtual void sprite(const Rect& rect, int sprite) = 0;

    // Returns the colour the game gives `part` of a modal screen's panel.
    virtual std::uint32_t colour(PanelPart part) const = 0;

    // Plays a menu sound.
    virtual void sound(Sound sound) = 0;
};

}  // namespace openrac::mpui

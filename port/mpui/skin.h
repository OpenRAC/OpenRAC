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

// The game's look, as the screens draw it.
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

    // Draws a panel behind a screen that has no game screen under it.
    virtual void panel(const Rect& rect) = 0;

    // Draws the marks that say a table has rows above (`up`) or below (`down`) the ones shown.
    virtual void scroll_marks(const Rect& table, bool up, bool down) = 0;

    // Plays a menu sound.
    virtual void sound(Sound sound) = 0;
};

}  // namespace openrac::mpui

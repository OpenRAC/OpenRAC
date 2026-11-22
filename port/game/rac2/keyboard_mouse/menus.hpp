// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Going Commando's own menus under the mouse, apart from the game so they are tested alone: menus
// by the texts the game drew, its grids and Quick Select ring followed by their selection, and the
// vendor's strip. The parts every game shares are in game/common/keyboard_mouse/kbm.hpp.
#ifndef KBM_RAC2_MENUS_HPP
#define KBM_RAC2_MENUS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "game/common/keyboard_mouse/kbm.hpp"

namespace kbm {

// ---- Menus by their texts (Going Commando) ----
//
// A menu whose items are texts the game drew this frame: the text under the pointer, the list it is
// in (texts of its height one above the other, centred alike, or side by side on its line) and the
// list's selected item, drawn in a colour of its own while the rest share one.

inline constexpr int kTextListMax = 128;

struct MenuText {
    std::uint32_t element = 0;  // the game's text element, 0 for a plain print (in no list)
    std::uint32_t colour = 0;   // ABGR, alpha 0x80 full
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    int cell = -1;  // its first button icon's cell (key_prompts.h), -1 none

    bool shown() const { return (colour >> 24) != 0 && x1 > x0 && y1 > y0; }

    float cx() const { return (x0 + x1) * 0.5f; }

    float cy() const { return (y0 + y1) * 0.5f; }
};

struct TextNav {
    int hit = -1;            // the text under the pointer
    int selected = -1;       // its list's selected text, -1 unknown
    std::uint16_t step = 0;  // the d-pad's step from the selected text toward the hit one, 0 there
    int steps = 0;           // how many of them: the items between, in the list's order
    bool scrolls = false;    // a list that scrolls round its middle (the items move, not the frame)
};

// The pointer's margin round a text (assumed: the rows' boxes are taller than their letters).
inline constexpr float kTextMarginX = 6.0f;
inline constexpr float kTextMarginY = 5.0f;

// A list's selected item among its texts in order: the one whose colour is its own while the rest
// share one; else, in a list that scrolls round its middle (an odd count, the colours alike from
// both ends, not all one: the ends faded), the middle one (`scrolls` set). -1 when neither.
inline int selected_text(const MenuText* t, const int* list, int count, bool* scrolls) {
    for (int a = 0; a < count; ++a) {
        int same = 0;
        for (int b = 0; b < count; ++b) {
            same += t[list[b]].colour == t[list[a]].colour ? 1 : 0;
        }
        // Its colour is its own: the selected one if the rest share theirs.
        if (same == 1) {
            const std::uint32_t rest = t[list[a == 0 ? 1 : 0]].colour;
            bool shared = true;
            for (int b = 0; b < count; ++b) {
                shared = shared && (b == a || t[list[b]].colour == rest);
            }
            if (shared) {
                return list[a];
            }
            break;
        }
    }

    // Faded ends round a middle.
    if (count % 2 == 1) {
        bool mirrored = true;
        bool one = true;
        for (int i = 0; i < count; ++i) {
            mirrored = mirrored && t[list[i]].colour == t[list[count - 1 - i]].colour;
            one = one && t[list[i]].colour == t[list[0]].colour;
        }
        if (mirrored && !one) {
            *scrolls = true;
            return list[count / 2];
        }
    }
    return -1;
}

inline TextNav text_nav(const MenuText* t, int n, float px, float py) {
    TextNav nav;

    // The first shown text the pointer is on.
    for (int i = 0; i < n && nav.hit < 0; ++i) {
        if (t[i].shown() && px >= t[i].x0 - kTextMarginX && px <= t[i].x1 + kTextMarginX
            && py >= t[i].y0 - kTextMarginY && py <= t[i].y1 + kTextMarginY) {
            nav.hit = i;
        }
    }

    // On no text: nothing to point at.
    if (nav.hit < 0) {
        return nav;
    }

    const MenuText& h = t[nav.hit];
    const float height = h.y1 - h.y0;
    int column[kTextListMax];
    int row[kTextListMax];
    int in_column = 0;
    int in_row = 0;

    // The texts of its height in its column (centred alike) and on its line.
    for (int i = 0; i < n; ++i) {
        // Only the game's shown list texts (an element, or a plain print) of the same height.
        if (!t[i].shown() || t[i].element == 0 || std::abs((t[i].y1 - t[i].y0) - height) > 1.0f) {
            continue;
        }

        if (std::abs(t[i].cx() - h.cx()) <= 4.0f && in_column < kTextListMax) {
            column[in_column++] = i;
        }

        if (std::abs(t[i].y0 - h.y0) <= 3.0f && in_row < kTextListMax) {
            row[in_row++] = i;
        }
    }

    const bool vertical = in_column >= in_row;
    int* list = vertical ? column : row;
    const int count = vertical ? in_column : in_row;

    // Fewer than three: no colour of its own to tell the selected one by.
    if (count < 3) {
        return nav;
    }

    // In order down or across.
    std::sort(list, list + count, [&](int a, int b) {
        return vertical ? t[a].cy() < t[b].cy() : t[a].cx() < t[b].cx();
    });

    nav.selected = selected_text(t, list, count, &nav.scrolls);

    // The selected one known and not the hit one: steps toward it.
    if (nav.selected >= 0 && nav.selected != nav.hit) {
        const MenuText& s = t[nav.selected];
        nav.step = vertical ? (h.cy() < s.cy() ? kUp : kDown) : (h.cx() < s.cx() ? kLeft : kRight);
        int at_hit = 0, at_selected = 0;
        for (int i = 0; i < count; ++i) {
            at_hit = list[i] == nav.hit ? i : at_hit;
            at_selected = list[i] == nav.selected ? i : at_selected;
        }
        nav.steps = std::abs(at_hit - at_selected);
    }

    return nav;
}

// ---- Going Commando's grids: following the selection frame ----
//
// A grid (weapons, gadgets, items, Quick Select) draws its cells with one GUI sprite element moved
// from cell to cell, and its selection frame with another drawn once. The frame's element is not
// known up front: the first step pressed shows it (the element drawn once that moved by a cell).
// A click on a cell steps the frame there, a step at a time, watching it move, then confirms.

struct GuiSprite {
    std::uint32_t element = 0;  // the sprite element (its guest address)
    float x = 0.0f, y = 0.0f;   // its centre, game pixels
    float frame = 0.0f;         // its shape's frame (+0x38: a ring's highlight's slot)
};

inline constexpr int kGridSpritesMax = 256;
// How near a cell's centre a click must be, and how near the frame counts as there (pixels).
inline constexpr float kGridCellReach = 30.0f;
inline constexpr float kGridThere = 6.0f;
// Ticks to wait for the frame to move after a step, and steps at most for one click.
inline constexpr int kGridWaitTicks = 14;
inline constexpr int kGridMaxSteps = 40;

class GridSeek {
public:
    void reset() { *this = GridSeek{}; }

    bool active() const { return m_active; }

    bool known() const { return m_cursor != 0; }

    // The button let go on the cell it went to: true when the frame is there already, for the
    // caller to confirm now; on its way, it confirms when there.
    bool confirm() {
        if (m_active) {
            m_confirm = true;
            return false;
        }
        return m_arrived;
    }

    // A click (confirm) or the pointer resting (no confirm) at (px, py): true when it is on a
    // grid's cell, which the frame then goes to.
    bool seek(const GuiSprite* s, int n, float px, float py, bool confirm) {
        float tx = 0.0f, ty = 0.0f;
        if (!cell_at(s, n, px, py, &tx, &ty)) {
            return false;
        }
        // Already on its way there: nothing new.
        if (m_active && std::abs(tx - m_tx) < 1.0f && std::abs(ty - m_ty) < 1.0f) {
            m_confirm = m_confirm || confirm;
            return true;
        }
        m_active = true;
        m_arrived = false;
        m_tx = tx;
        m_ty = ty;
        m_confirm = confirm;
        m_steps = 0;
        m_blocked = 0;
        m_probe = 0;
        m_wait = 0;
        return true;
    }

    // A tick with the last frame's sprites: the pad button to press now (0 none). `idle`: the
    // last press is over.
    std::uint16_t update(const GuiSprite* s, int n, bool idle) {
        if (!m_active) {
            return 0;
        }

        // A step out: wait for the frame to move and settle.
        if (m_wait > 0) {
            --m_wait;
            if (!settled(s, n)) {
                return 0;
            }
        }

        if (!idle) {
            return 0;
        }

        float hx = 0.0f, hy = 0.0f;
        // The frame not known: a probe step shows it.
        if (m_cursor == 0 || !once(s, n, m_cursor, &hx, &hy)) {
            m_cursor = 0;
            return probe(s, n);
        }

        const float dx = m_tx - hx, dy = m_ty - hy;
        // There: confirm if clicked.
        if (std::abs(dx) <= kGridThere && std::abs(dy) <= kGridThere) {
            m_active = false;
            m_arrived = !m_confirm;
            return m_confirm ? kCross : 0;
        }

        const bool across = std::abs(dx) >= std::abs(dy);
        int axis = across ? 1 : 2;
        // That way does not move it: the other, while it has a way to go.
        if ((m_blocked & axis) != 0) {
            axis = axis == 1 ? 2 : 1;
            const float other = axis == 1 ? dx : dy;
            if ((m_blocked & axis) != 0 || std::abs(other) <= kGridThere) {
                m_active = false;
                return 0;
            }
        }

        if (++m_steps > kGridMaxSteps) {
            m_active = false;
            return 0;
        }

        const std::uint16_t step =
            axis == 1 ? (dx > 0.0f ? kRight : kLeft) : (dy > 0.0f ? kDown : kUp);
        start(s, n, axis);
        return step;
    }

private:
    // The nearest cell of a grid to (px, py): a sprite element drawn three times or more.
    static bool cell_at(const GuiSprite* s, int n, float px, float py, float* tx, float* ty) {
        float best = kGridCellReach * kGridCellReach;
        bool found = false;
        for (int i = 0; i < n; ++i) {
            const float d = (s[i].x - px) * (s[i].x - px) + (s[i].y - py) * (s[i].y - py);
            if (std::abs(s[i].x - px) > kGridCellReach || std::abs(s[i].y - py) > kGridCellReach
                || d >= best || count(s, n, s[i].element) < 3) {
                continue;
            }
            best = d;
            *tx = s[i].x;
            *ty = s[i].y;
            found = true;
        }
        return found;
    }

    static int count(const GuiSprite* s, int n, std::uint32_t element) {
        int c = 0;
        for (int i = 0; i < n; ++i) {
            c += s[i].element == element ? 1 : 0;
        }
        return c;
    }

    // An element drawn exactly once, and where.
    static bool once(const GuiSprite* s, int n, std::uint32_t element, float* x, float* y) {
        int c = 0;
        for (int i = 0; i < n; ++i) {
            if (s[i].element == element) {
                *x = s[i].x;
                *y = s[i].y;
                ++c;
            }
        }
        return c == 1;
    }

    // A step pressed: where every element drawn once was.
    void start(const GuiSprite* s, int n, int axis) {
        m_before_count = 0;
        for (int i = 0; i < n && m_before_count < kBeforeMax; ++i) {
            if (count(s, n, s[i].element) == 1) {
                m_before[static_cast<std::size_t>(m_before_count++)] = s[i];
            }
        }
        m_axis = axis;
        m_wait = kGridWaitTicks;
        m_moved = false;
        m_last_x = -1.0f;
        m_last_y = -1.0f;
    }

    // Still waiting for the step's move: false. Once the frame has moved and stays put, true; a
    // step that moved nothing marks its way blocked.
    bool settled(const GuiSprite* s, int n) {
        // The frame not known yet: the element drawn once that moved a cell along the step.
        if (m_cursor == 0) {
            learn(s, n);
            if (m_cursor == 0) {
                if (m_wait == 0) {
                    m_blocked |= m_axis;
                }
                return m_wait == 0;
            }
        }

        float x = 0.0f, y = 0.0f;
        if (!once(s, n, m_cursor, &x, &y)) {
            return m_wait == 0;
        }
        const bool still = std::abs(x - m_last_x) < 0.5f && std::abs(y - m_last_y) < 0.5f;
        m_last_x = x;
        m_last_y = y;
        if (!m_moved) {
            for (int i = 0; i < m_before_count; ++i) {
                const GuiSprite& b = m_before[static_cast<std::size_t>(i)];
                m_moved = m_moved
                          || (b.element == m_cursor
                              && (std::abs(b.x - x) >= 2.0f || std::abs(b.y - y) >= 2.0f));
            }
        }
        if (m_moved && still) {
            m_wait = 0;
            return true;
        }
        // No move in time: that way is blocked (an edge).
        if (m_wait == 0 && !m_moved) {
            m_blocked |= m_axis;
        }
        return m_wait == 0;
    }

    // The element drawn once, before and now, that moved along the step's axis (either way: a
    // grid wraps round at its edges) onto a grid's cell.
    void learn(const GuiSprite* s, int n) {
        for (int i = 0; i < m_before_count; ++i) {
            const GuiSprite& b = m_before[static_cast<std::size_t>(i)];
            float x = 0.0f, y = 0.0f;
            if (!once(s, n, b.element, &x, &y)) {
                continue;
            }
            const float along = m_axis == 1 ? std::abs(x - b.x) : std::abs(y - b.y);
            const float off = m_axis == 1 ? std::abs(y - b.y) : std::abs(x - b.x);
            if (along < 4.0f || off >= along) {
                continue;
            }
            // It sits on a cell now (within the frame's reach of one).
            float cx = 0.0f, cy = 0.0f;
            if (cell_at(s, n, x, y, &cx, &cy) && std::abs(cx - x) <= kGridThere
                && std::abs(cy - y) <= kGridThere) {
                m_cursor = b.element;
                m_blocked = 0;
                m_moved = true;
                m_last_x = -1.0f;
                return;
            }
        }
    }

    // Where the frame likely is: the one element drawn once that sits on a cell.
    static void guess(const GuiSprite* s, int n, float* x, float* y) {
        int found = 0;
        for (int i = 0; i < n; ++i) {
            float cx = 0.0f, cy = 0.0f;
            if (count(s, n, s[i].element) == 1 && cell_at(s, n, s[i].x, s[i].y, &cx, &cy)
                && std::abs(cx - s[i].x) <= kGridThere && std::abs(cy - s[i].y) <= kGridThere) {
                *x = s[i].x;
                *y = s[i].y;
                ++found;
            }
        }
        if (found != 1) {
            *x = 256.0f;
            *y = 208.0f;
        }
    }

    // The frame not known: steps toward the cell, then away, across and down, until one shows it.
    std::uint16_t probe(const GuiSprite* s, int n) {
        static constexpr int kProbes = 4;
        if (m_probe >= kProbes) {
            m_active = false;
            return 0;
        }
        const int p = m_probe++;
        const bool across = p < 2;
        const bool toward = (p % 2) == 0;
        // From the frame's likely place (the one element drawn once on a cell), else the middle.
        float fx = 256.0f, fy = 208.0f;
        guess(s, n, &fx, &fy);
        const float d = across ? m_tx - fx : m_ty - fy;
        const bool positive = (d >= 0.0f) == toward;
        const std::uint16_t step = across ? (positive ? kRight : kLeft) : (positive ? kDown : kUp);
        start(s, n, across ? 1 : 2);
        return step;
    }

    static constexpr int kBeforeMax = 64;
    bool m_active = false;
    bool m_confirm = false;
    bool m_arrived = false;  // the last seek ended on its cell, not yet confirmed
    float m_tx = 0.0f, m_ty = 0.0f;
    std::uint32_t m_cursor = 0;
    int m_steps = 0;
    int m_blocked = 0;  // 1 across, 2 down: steps that way moved nothing
    int m_probe = 0;
    int m_wait = 0;
    int m_axis = 0;
    bool m_moved = false;
    float m_last_x = -1.0f, m_last_y = -1.0f;
    std::array<GuiSprite, kBeforeMax> m_before{};
    int m_before_count = 0;
};

// ---- Going Commando's Quick Select ring ----
//
// The Quick Select page draws its ring of eight slots as sprites all centred on the ring's middle,
// its highlight among them: the highlight's frame (+0x38) is its slot, 0 the top right one,
// going round clockwise (measured). Cross on the grid's weapon picks it up; then R1 moves the highlight one slot on, L1
// one back, and Cross puts the weapon there. The highlight's element is found by the first R1
// (the element at the middle whose frame moved). A click on a slot (or the grid's weapon dragged
// onto it) does all of that.

inline constexpr int kRingSlots = 8;
inline constexpr int kRingPieces = 8;       // sprites drawn at one place that make a ring
inline constexpr float kRingInner = 45.0f;  // a slot's distance from the middle (pixels)
inline constexpr float kRingOuter = 115.0f;

class RingSeek {
public:
    void reset() { *this = RingSeek{}; }

    bool active() const { return m_active; }

    // The ring's slot at (px, py), -1 for none (no ring drawn, or off its slots).
    static int slot_at(const GuiSprite* s, int n, float px, float py) {
        float cx = 0.0f, cy = 0.0f;
        if (!middle(s, n, &cx, &cy)) {
            return -1;
        }
        const float dx = px - cx, dy = py - cy;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d < kRingInner || d > kRingOuter) {
            return -1;
        }
        // Clockwise from the top, in eighths; the slots count from the top right one.
        const float turn = std::atan2(dx, -dy) / (2.0f * 3.14159265f);
        const int k = static_cast<int>(std::lround(turn * kRingSlots)) - 1;
        return ((k % kRingSlots) + kRingSlots) % kRingSlots;
    }

    // Pick the grid's weapon up, take the highlight to `slot`, then put it there.
    void seek(int slot) {
        m_active = true;
        m_target = slot;
        m_wait = 0;
        m_steps = 0;
        m_picked = false;
    }

    // A tick with the last frame's sprites: the pad button to press now (0 none).
    std::uint16_t update(const GuiSprite* s, int n, bool idle) {
        if (!m_active) {
            return 0;
        }

        // A press out: wait for the highlight's frame to change.
        if (m_wait > 0) {
            --m_wait;
            if (!moved(s, n) && m_wait > 0) {
                return 0;
            }
            // Nothing moved in time: give up.
            if (m_wait == 0 && !moved(s, n)) {
                m_active = false;
                return 0;
            }
            m_wait = 0;
        }

        if (!idle) {
            return 0;
        }

        // First the weapon picked up (the ring then follows R1 and L1), and a moment for it.
        if (!m_picked) {
            m_picked = true;
            m_pause = kRingPickTicks;
            return kCross;
        }
        if (m_pause > 0) {
            --m_pause;
            return 0;
        }

        int at = -1;
        // Not known: R1 shows it.
        if (m_cursor == 0 || (at = slot_of(s, n, m_cursor)) < 0) {
            m_cursor = 0;
            return press(s, n, kR1);
        }

        if (at == m_target) {
            m_active = false;
            return kCross;
        }
        if (++m_steps > kRingSlots) {
            m_active = false;
            return 0;
        }
        const int ahead = ((m_target - at) % kRingSlots + kRingSlots) % kRingSlots;
        return press(s, n, ahead <= kRingSlots / 2 ? kR1 : kL1);
    }

private:
    // The ring's middle: a place where kRingPieces elements or more are drawn.
    static bool middle(const GuiSprite* s, int n, float* cx, float* cy) {
        for (int i = 0; i < n; ++i) {
            int at = 0;
            for (int j = 0; j < n; ++j) {
                at += s[j].x == s[i].x && s[j].y == s[i].y ? 1 : 0;
            }
            if (at >= kRingPieces) {
                *cx = s[i].x;
                *cy = s[i].y;
                return true;
            }
        }
        return false;
    }

    // An element's slot by its frame (a whole number, 0 to 7), -1 when not drawn or not one.
    static int slot_of(const GuiSprite* s, int n, std::uint32_t element) {
        for (int i = 0; i < n; ++i) {
            if (s[i].element == element) {
                const float f = s[i].frame;
                const int k = static_cast<int>(std::lround(f));
                return std::abs(f - static_cast<float>(k)) < 0.01f && k >= 0 && k < kRingSlots ? k
                                                                                               : -1;
            }
        }
        return -1;
    }

    // A press: the frames of the elements at the middle before it.
    std::uint16_t press(const GuiSprite* s, int n, std::uint16_t button) {
        float cx = 0.0f, cy = 0.0f;
        m_before_count = 0;
        if (!middle(s, n, &cx, &cy)) {
            m_active = false;
            return 0;
        }
        for (int i = 0; i < n && m_before_count < kBeforeMax; ++i) {
            if (s[i].x == cx && s[i].y == cy) {
                m_before[static_cast<std::size_t>(m_before_count++)] = s[i];
            }
        }
        m_wait = kGridWaitTicks;
        return button;
    }

    // The highlight's frame changed since the press (and, not known yet, which element it is).
    bool moved(const GuiSprite* s, int n) {
        for (int i = 0; i < m_before_count; ++i) {
            const GuiSprite& b = m_before[static_cast<std::size_t>(i)];
            if (m_cursor != 0 && b.element != m_cursor) {
                continue;
            }
            const int was = slot_of(m_before.data(), m_before_count, b.element);
            const int now = slot_of(s, n, b.element);
            if (was >= 0 && now >= 0 && now != was) {
                m_cursor = b.element;
                return true;
            }
        }
        return false;
    }

    static constexpr int kBeforeMax = 32;
    static constexpr int kRingPickTicks = 8;
    bool m_active = false;
    bool m_picked = false;
    int m_pause = 0;
    int m_target = 0;
    std::uint32_t m_cursor = 0;
    int m_wait = 0;
    int m_steps = 0;
    std::array<GuiSprite, kBeforeMax> m_before{};
    int m_before_count = 0;
};

// ---- Going Commando's vendor ----

// Its strip of item icons (40 pixels, 56 apart, plain quads) keeps the selected one in the middle
// of the screen and slides under it. Above the strip the panel shows the item: Cross buys it.
inline constexpr float kGcStripStep = 56.0f;
inline constexpr int kGcNoIcon = 1000;

struct StripIcon {
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;

    float cx() const { return (x0 + x1) * 0.5f; }
};

// The d-pad's steps from the selected icon (the one nearest the screen's middle) to the one at
// (px, py), right for more; kGcNoIcon for no icon there.
inline int gc_strip_steps(const StripIcon* icons, int count, float px, float py) {
    int hit = -1;
    int middle = -1;
    for (int i = 0; i < count; ++i) {
        const StripIcon& c = icons[i];
        if (px >= c.x0 && px < c.x1 && py >= c.y0 && py < c.y1) {
            hit = i;
        }
        if (middle < 0
            || std::abs(c.cx() - kScreenWidth * 0.5f)
                   < std::abs(icons[middle].cx() - kScreenWidth * 0.5f)) {
            middle = i;
        }
    }
    if (hit < 0) {
        return kGcNoIcon;
    }
    return static_cast<int>(std::lround((icons[hit].cx() - icons[middle].cx()) / kGcStripStep));
}

// Whether (px, py) is on the item's panel (measured on the screen).
inline bool gc_vendor_panel(float px, float py) {
    return px >= 146.0f && px <= 370.0f && py >= 99.0f && py <= 245.0f;
}

}  // namespace kbm

#endif  // KBM_RAC2_MENUS_HPP

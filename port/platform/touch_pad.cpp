// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/touch_pad.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <optional>
#include <utility>

namespace openrac::platform {
namespace {

// A button answers a little outside its outline: thumbs are wider than the drawing.
constexpr float kButtonReach = 0.025f;
// A stick is picked up a little outside its circle too, and its knob goes to its edge.
constexpr float kStickReach = 1.5f;
// Where a thumb brings the left stick to it: the lower left, below the d-pad, left of this.
constexpr float kStickZoneTop = 0.42f;
constexpr float kStickZoneRight = 0.42f;
// Where a finger turns the camera: the right of the screen, from this across, off the buttons.
constexpr float kLookZoneLeft = 0.5f;
// The camera: an invisible stick under the finger that lands there. Pushed this many window
// heights from where the finger landed, it is pushed all the way (at the pad's own speed; the
// player's speed shortens or lengthens the push), and it keeps turning the camera as long as the
// finger stays out there, as a stick held over does. Pushed further, its centre follows the
// finger, so that a finger coming back turns the camera the other way at once. Within kLookDead
// of the centre it is still; past it the push starts at kLookFloor, past the game's dead zone, so
// that a small push turns the camera slowly rather than not at all.
constexpr float kLookReach = 0.1f;
constexpr float kLookDead = 0.12f;
constexpr float kLookFloor = 0.22f;
// A finger held on a "stick" action pushes the stick all the way once it has slid this far from
// where it landed, in window heights.
constexpr float kSteerDead = 0.02f;
// How far the player may move a group from its place, in window heights.
constexpr float kMaxOffset = 2.0f;
// A group's centre keeps this far inside the window.
constexpr float kInside = 0.04f;
// How near a line a dragged group's centre snaps to it, in window heights.
constexpr float kSnap = 0.025f;

using Kind = TouchControl::Kind;

// A control of `kind` in `group`, its radius (in window heights at scale 1) and stretch; placed later.
TouchControl make_control(Kind kind, Button button, TouchGroup group, float radius, float stretch) {
    TouchControl c;
    c.kind = kind;
    c.button = button;
    c.group = group;
    c.radius = radius;
    c.stretch = stretch;
    return c;
}

constexpr const char* kGroupNames[] = {"stick", "dpad",   "face",  "l1",   "l2",   "r1",
                                       "r2",    "select", "start", "menu", "arc0", "arc1",
                                       "arc2",  "arc3",   "arc4",  "arc5", "arc6", "arc7",
                                       "left0", "left1",  "left2", "top0", "top1", "hint0",
                                       "hint1", "hint2"};
static_assert(std::size(kGroupNames) == kTouchGroups, "a name for each TouchGroup");

}  // namespace

TouchPad::TouchPad() {
    // The left thumb moves Ratchet (the left stick, the d-pad above it); the right one jumps and
    // attacks (the face buttons, all of one size as on the pad) and turns the camera on the
    // screen around them; the shoulders along the top, under the index fingers. Each group's
    // centre from the nearer edge, its controls around it; in window heights at scale 1.
    using From = Place::From;
    using G = TouchGroup;
    const auto group = [this](G g, From from, float offset, float y) {
        m_places[static_cast<std::size_t>(g)] = Place{from, offset, y};
    };
    group(G::Stick, From::Left, 0.27f, 0.69f);
    group(G::Dpad, From::Left, 0.20f, 0.36f);
    group(G::Face, From::Right, 0.26f, 0.64f);
    // The shoulders a little below the top, where the game draws its health and bolts.
    group(G::L2, From::Left, 0.13f, 0.15f);
    group(G::L1, From::Left, 0.36f, 0.15f);
    group(G::R1, From::Right, 0.36f, 0.15f);
    group(G::R2, From::Right, 0.13f, 0.15f);
    group(G::Select, From::Middle, -0.16f, 0.075f);
    group(G::Start, From::Middle, 0.16f, 0.075f);
    group(G::Menu, From::Middle, 0.0f, 0.075f);
    // The Arc style: the corner first, under the resting right thumb; then by how far the thumb
    // has to go, the low ones before the high ones (a thumb reaches across more easily than up).
    group(G::Arc0, From::Right, 0.185f, 0.75f);
    group(G::Arc1, From::Right, 0.465f, 0.83f);
    group(G::Arc2, From::Right, 0.32f, 0.58f);
    group(G::Arc3, From::Right, 0.165f, 0.485f);
    group(G::Arc4, From::Right, 0.64f, 0.69f);
    group(G::Arc5, From::Right, 0.10f, 0.29f);
    group(G::Arc6, From::Right, 0.62f, 0.45f);
    group(G::Arc7, From::Right, 0.33f, 0.33f);
    // Beside the stick, above it, for the left thumb (what the shoulders under the left index
    // finger did on the pad: looking, strafing).
    group(G::Left0, From::Left, 0.18f, 0.40f);
    group(G::Left1, From::Left, 0.18f, 0.25f);
    group(G::Left2, From::Left, 0.36f, 0.25f);
    group(G::Top0, From::Middle, -0.20f, 0.075f);
    group(G::Top1, From::Middle, 0.20f, 0.075f);
    // A hint's button, where the eye is (left of the arc, above the middle of the right half).
    group(G::Hint0, From::Right, 0.84f, 0.52f);
    group(G::Hint1, From::Right, 0.84f, 0.32f);
    group(G::Hint2, From::Right, 1.02f, 0.42f);
    rebuild();
}

void TouchPad::classic_controls() {
    using G = TouchGroup;

    struct Item {
        Kind kind;
        Button button;
        G group;
        float dx, dy, radius, stretch;
    };

    constexpr float kFace = 0.105f;  // the face buttons' distance from their diamond's centre
    constexpr float kArm = 0.078f;   // the d-pad's arms' from its centre
    const Item items[] = {
        {Kind::LeftStick, Button::Cross, G::Stick, 0.0f, 0.0f, 0.15f, 1.0f},
        {Kind::Button, Button::Up, G::Dpad, 0.0f, -kArm, 0.052f, 1.0f},
        {Kind::Button, Button::Down, G::Dpad, 0.0f, kArm, 0.052f, 1.0f},
        {Kind::Button, Button::Left, G::Dpad, -kArm, 0.0f, 0.052f, 1.0f},
        {Kind::Button, Button::Right, G::Dpad, kArm, 0.0f, 0.052f, 1.0f},
        {Kind::Button, Button::Triangle, G::Face, 0.0f, -kFace, 0.07f, 1.0f},
        {Kind::Button, Button::Cross, G::Face, 0.0f, kFace, 0.07f, 1.0f},
        {Kind::Button, Button::Square, G::Face, -kFace, 0.0f, 0.07f, 1.0f},
        {Kind::Button, Button::Circle, G::Face, kFace, 0.0f, 0.07f, 1.0f},
        {Kind::Button, Button::L2, G::L2, 0.0f, 0.0f, 0.045f, 1.75f},
        {Kind::Button, Button::L1, G::L1, 0.0f, 0.0f, 0.045f, 1.75f},
        {Kind::Button, Button::R1, G::R1, 0.0f, 0.0f, 0.045f, 1.75f},
        {Kind::Button, Button::R2, G::R2, 0.0f, 0.0f, 0.045f, 1.75f},
        {Kind::Button, Button::Select, G::Select, 0.0f, 0.0f, 0.032f, 1.6f},
        {Kind::Button, Button::Start, G::Start, 0.0f, 0.0f, 0.032f, 1.6f},
        // The options button as large as the Arc style's.
        {Kind::Menu, Button::Cross, G::Menu, 0.0f, 0.0f, 0.05f, 1.0f},
    };
    for (const Item& item : items) {
        TouchControl c =
            make_control(item.kind, item.button, item.group, item.radius, item.stretch);
        c.buttons = item.kind == Kind::Button ? static_cast<std::uint16_t>(item.button) : 0;
        m_controls.push_back(c);
        m_members.push_back(Member{item.dx, item.dy, item.radius});
    }
}

void TouchPad::arc_controls() {
    using G = TouchGroup;
    // The stick (a little smaller) and the options button (larger) as in the Classic style, or a
    // d-pad in the stick's place; the actions in the style's places, each the size it asks for.
    if (m_dpad) {
        constexpr float kArm = 0.075f;
        constexpr float kArmRadius = 0.05f;

        const struct {
            Button button;
            float dx, dy;
        } arms[] = {
            {Button::Up, 0.0f, -kArm},
            {Button::Down, 0.0f, kArm},
            {Button::Left, -kArm, 0.0f},
            {Button::Right, kArm, 0.0f},
        };

        for (const auto& arm : arms) {
            TouchControl c = make_control(Kind::Button, arm.button, G::Stick, kArmRadius, 1.0f);
            c.buttons = static_cast<std::uint16_t>(arm.button);
            m_controls.push_back(c);
            m_members.push_back(Member{arm.dx, arm.dy, kArmRadius});
        }
    } else {
        m_controls.push_back(make_control(Kind::LeftStick, Button::Cross, G::Stick, 0.135f, 1.0f));
        m_members.push_back(Member{0.0f, 0.0f, 0.135f});
    }
    m_controls.push_back(make_control(Kind::Menu, Button::Cross, G::Menu, 0.05f, 1.0f));
    m_members.push_back(Member{0.0f, 0.0f, 0.05f});
    constexpr G kArc[] = {G::Arc0, G::Arc1, G::Arc2, G::Arc3, G::Arc4, G::Arc5, G::Arc6, G::Arc7};
    constexpr G kLeft[] = {G::Left0, G::Left1, G::Left2};
    constexpr G kTop[] = {G::Top0, G::Top1};
    constexpr G kHint[] = {G::Hint0, G::Hint1, G::Hint2};
    std::size_t left = 0, top = 0, hint = 0;
    const auto up = [&](const TouchAction& a) {
        return a.when.empty() || std::find(m_hints.begin(), m_hints.end(), a.when) != m_hints.end();
    };
    const auto has = [&](const std::string& id) {
        return std::any_of(m_actions.begin(), m_actions.end(), [&](const TouchAction& b) {
            return b.id == id;
        });
    };
    // What shows: one that stands in for another takes that one's place, as large.
    std::vector<TouchAction> shown;
    for (const TouchAction& own : m_actions) {
        if (!up(own) || (!own.instead.empty() && has(own.instead))) {
            continue;
        }
        const auto stand_in =
            std::find_if(m_actions.begin(), m_actions.end(), [&](const TouchAction& b) {
                return b.instead == own.id && up(b);
            });
        TouchAction a = stand_in != m_actions.end() ? *stand_in : own;
        a.size = own.size;
        a.side = own.side;
        a.at = own.at;
        a.when = own.when;
        a.wide = own.wide;
        shown.push_back(std::move(a));
    }
    // The arc: those with their own place first, so a button stays put from context to context;
    // the rest in the free places, in order.
    std::array<bool, std::size(kArc)> taken{};
    std::vector<G> arc_of(shown.size(), G::Count);
    for (std::size_t i = 0; i < shown.size(); ++i) {
        const int at = shown[i].at;
        if (shown[i].side == TouchSide::Arc && at >= 0 && !taken[static_cast<std::size_t>(at)]) {
            taken[static_cast<std::size_t>(at)] = true;
            arc_of[i] = kArc[at];
        }
    }
    for (std::size_t i = 0, free = 0; i < shown.size(); ++i) {
        if (shown[i].side != TouchSide::Arc || arc_of[i] != G::Count) {
            continue;
        }
        while (free < std::size(kArc) && taken[free]) {
            ++free;
        }
        if (free < std::size(kArc)) {
            taken[free] = true;
            arc_of[i] = kArc[free];
        }
    }
    for (std::size_t i = 0; i < shown.size(); ++i) {
        const TouchAction& a = shown[i];
        G g = G::Count;
        if (a.side == TouchSide::Hint) {
            g = hint < std::size(kHint) ? kHint[hint++] : G::Count;
        } else if (a.side == TouchSide::Left) {
            g = left < std::size(kLeft) ? kLeft[left++] : G::Count;
        } else if (a.side == TouchSide::Top) {
            g = top < std::size(kTop) ? kTop[top++] : G::Count;
        } else {
            g = arc_of[i];
        }
        if (g == G::Count) {
            continue;  // more than the style has places for: the first ones, the most used, stay
        }
        float radius = a.size == TouchSize::Large   ? 0.113f
                       : a.size == TouchSize::Small ? 0.070f
                                                    : 0.083f;
        float stretch = a.wide ? 1.4f : 1.0f;
        if (a.side == TouchSide::Top) {
            radius = 0.036f;
            stretch = 1.6f;
        }
        TouchControl c = make_control(Kind::Button, Button::Cross, g, radius, stretch);
        c.buttons = a.press != 0 ? a.press : a.then;
        c.then = a.press != 0 ? a.then : 0;
        for (std::uint16_t bit = 1; bit != 0; bit = static_cast<std::uint16_t>(bit << 1)) {
            if ((c.buttons & bit) != 0) {
                c.button = static_cast<Button>(bit);
                break;
            }
        }
        c.toggle = a.toggle;
        c.stick = a.stick;
        c.action = a.id;
        c.icon = a.icon;
        m_controls.push_back(std::move(c));
        m_members.push_back(Member{0.0f, 0.0f, radius});
    }
}

void TouchPad::rebuild() {
    // A thumb on the stick keeps it where it is: its index and place come over to the new set.
    std::optional<std::pair<float, float>> stick;
    for (std::size_t i = 0; i < m_controls.size(); ++i) {
        if (m_controls[i].kind == Kind::LeftStick && stick_taken(i)) {
            stick = std::pair{m_controls[i].x, m_controls[i].y};
        }
    }
    m_controls.clear();
    m_members.clear();
    if (m_style == TouchStyle::Arc) {
        arc_controls();
    } else {
        classic_controls();
    }
    m_held_since.assign(m_controls.size(), 0);
    // The stick is the first control when there is one (the stick's fingers keep its index; with a
    // d-pad in its place they hold nothing until they lift). A finger steering an action lets go.
    const bool has_stick = !m_controls.empty() && m_controls[0].kind == Kind::LeftStick;
    for (Finger& f : m_fingers) {
        if (f.role == Role::Stick) {
            f.stick = has_stick ? 0 : -1;
        } else if (f.role == Role::Steer) {
            f.role = Role::None;
            f.stick = -1;
        }
    }
    std::erase_if(m_toggled, [this](const std::string& id) {
        return std::none_of(m_controls.begin(), m_controls.end(), [&](const TouchControl& c) {
            return c.toggle && c.action == id;
        });
    });
    layout_controls();
    if (stick && has_stick) {
        m_controls[0].x = stick->first;
        m_controls[0].y = stick->second;
        m_controls[0].radius =
            m_members[0].radius * m_scale * m_sizes[static_cast<std::size_t>(TouchGroup::Stick)];
    }
}

void TouchPad::set_style(TouchStyle style) {
    if (style != m_style) {
        m_style = style;
        m_toggled.clear();
        rebuild();
    }
}

void TouchPad::set_actions(
    std::vector<TouchAction> actions, std::vector<std::string> hints, bool dpad
) {
    const bool now_same = actions == m_actions && hints == m_hints && dpad == m_dpad;
    if (now_same && !m_waiting) {
        return;
    }
    m_next_actions = std::move(actions);
    m_next_hints = std::move(hints);
    m_next_dpad = dpad;
    m_waiting = !now_same;
    take_waiting();
}

bool TouchPad::button_held() const {
    for (const Finger& f : m_fingers) {
        if (f.role == Role::Steer) {
            return true;
        }
        if (f.role != Role::None) {
            continue;
        }
        for (const TouchControl& c : m_controls) {
            if (c.kind == Kind::Button && distance(c, f.x, f.y) <= c.radius + kButtonReach) {
                return true;
            }
        }
    }
    return false;
}

void TouchPad::take_waiting() {
    if (!m_waiting || button_held()) {
        return;
    }
    m_waiting = false;
    m_actions = std::move(m_next_actions);
    m_hints = std::move(m_next_hints);
    m_dpad = m_next_dpad;
    m_next_actions.clear();
    m_next_hints.clear();
    if (m_style == TouchStyle::Arc) {
        rebuild();
    }
}

void TouchPad::set_aspect(float aspect) {
    if (aspect > 0.0f && aspect != m_aspect) {
        m_aspect = aspect;
        layout_controls();
    }
}

void TouchPad::set_safe_area(float left, float right) {
    left = std::max(left, 0.0f);
    right = std::max(right, 0.0f);
    if (left != m_safe_left || right != m_safe_right) {
        m_safe_left = left;
        m_safe_right = right;
        layout_controls();
    }
}

void TouchPad::set_scale(float scale) {
    scale = std::isfinite(scale) ? std::clamp(scale, kMinScale, kMaxScale) : kDefaultScale;
    if (scale != m_scale) {
        m_scale = scale;
        layout_controls();
    }
}

void TouchPad::set_look_speed(float speed) {
    m_look_speed = std::isfinite(speed) ? std::clamp(speed, 0.25f, 3.0f) : 1.0f;
}

void TouchPad::set_layout(std::string_view text) {
    m_offsets = {};
    m_sizes = filled_sizes();
    std::size_t at = 0;
    while (at <= text.size()) {
        const std::size_t end = std::min(text.find(',', at), text.size());
        const std::string item(text.substr(at, end - at));
        char name[16] = {};
        float dx = 0.0f;
        float dy = 0.0f;
        float size = 1.0f;
        char after = 0;
        const int read =
            std::sscanf(item.c_str(), " %15s %f %f %f %c", name, &dx, &dy, &size, &after);
        if ((read == 3 || read == 4) && std::isfinite(dx) && std::isfinite(dy)
            && std::isfinite(size)) {
            for (std::size_t g = 0; g < kTouchGroups; ++g) {
                if (std::string_view(name) == kGroupNames[g]) {
                    m_offsets[g] =
                        {std::clamp(dx, -kMaxOffset, kMaxOffset),
                         std::clamp(dy, -kMaxOffset, kMaxOffset)};
                    m_sizes[g] = std::clamp(size, kMinSize, kMaxSize);
                    break;
                }
            }
        }
        at = end + 1;
    }
    layout_controls();
}

std::string TouchPad::layout() const {
    std::string text;
    for (std::size_t g = 0; g < kTouchGroups; ++g) {
        const auto [dx, dy] = m_offsets[g];
        const float size = m_sizes[g];
        if (dx != 0.0f || dy != 0.0f || size != 1.0f) {
            text +=
                std::format("{}{} {:.4f} {:.4f}", text.empty() ? "" : ", ", kGroupNames[g], dx, dy);
            if (size != 1.0f) {
                text += std::format(" {:.2f}", size);
            }
        }
    }
    return text;
}

void TouchPad::reset_layout() {
    m_offsets = {};
    m_sizes = filled_sizes();
    layout_controls();
}

float TouchPad::selected_size() const {
    return m_selected != TouchGroup::Count ? m_sizes[static_cast<std::size_t>(m_selected)] : 1.0f;
}

void TouchPad::step_selected_size(int steps) {
    if (m_selected == TouchGroup::Count) {
        return;
    }
    float& size = m_sizes[static_cast<std::size_t>(m_selected)];
    // Counted in steps, so that ten of them come back to 1 exactly.
    const float stepped = std::round(size / kSizeStep + static_cast<float>(steps)) * kSizeStep;
    size = std::clamp(stepped, kMinSize, kMaxSize);
    layout_controls();
}

void TouchPad::set_editing(bool on) {
    if (on == m_editing) {
        return;
    }
    m_editing = on;
    m_selected = TouchGroup::Count;
    m_fingers.clear();  // a finger that came down as one thing is not the other's
    m_guide_x = m_guide_y = -1.0f;
    m_look_x = m_look_y = 0.0f;
    m_visible = m_visible || on;
    layout_controls();
}

void TouchPad::anchor(TouchGroup group, float& across, float& down) const {
    const auto g = static_cast<std::size_t>(group);
    const Place& p = m_places[g];
    switch (p.from) {
        case Place::From::Left:
            across = m_safe_left + p.offset * m_scale;
            break;
        case Place::From::Middle:
            across = m_aspect * 0.5f + p.offset * m_scale;
            break;
        case Place::From::Right:
            across = m_aspect - m_safe_right - p.offset * m_scale;
            break;
    }
    // Toward the nearer of the top and the bottom as the controls get smaller.
    down = p.y < 0.5f ? p.y * m_scale : 1.0f - (1.0f - p.y) * m_scale;
    across = std::clamp(across + m_offsets[g][0], kInside, std::max(kInside, m_aspect - kInside));
    down = std::clamp(down + m_offsets[g][1], kInside, 1.0f - kInside);
}

void TouchPad::snap(TouchGroup group) {
    const auto g = static_cast<std::size_t>(group);
    float across = 0.0f;
    float down = 0.0f;
    anchor(group, across, down);
    float to_x = across;
    float to_y = down;
    float best_x = kSnap;
    float best_y = kSnap;
    const auto near = [](float at, float line, float& best, float& to) {
        if (std::abs(line - at) < best) {
            best = std::abs(line - at);
            to = line;
        }
    };
    // Where it was first placed.
    const std::array<float, 2> moved = m_offsets[g];
    m_offsets[g] = {};
    float home_x = 0.0f;
    float home_y = 0.0f;
    anchor(group, home_x, home_y);
    m_offsets[g] = moved;
    near(across, home_x, best_x, to_x);
    near(down, home_y, best_y, to_y);
    near(across, m_aspect * 0.5f, best_x, to_x);
    for (std::size_t o = 0; o < kTouchGroups; ++o) {
        if (o == g || (static_cast<TouchGroup>(o) == TouchGroup::Menu && !m_menu_button)
            || std::none_of(m_controls.begin(), m_controls.end(), [o](const TouchControl& c) {
                   return static_cast<std::size_t>(c.group) == o;
               })) {
            continue;  // only the groups the style shows now
        }
        float other_x = 0.0f;
        float other_y = 0.0f;
        anchor(static_cast<TouchGroup>(o), other_x, other_y);
        near(across, other_x, best_x, to_x);
        near(across, m_aspect - other_x, best_x, to_x);
        near(down, other_y, best_y, to_y);
    }
    m_offsets[g] =
        {std::clamp(moved[0] + to_x - across, -kMaxOffset, kMaxOffset),
         std::clamp(moved[1] + to_y - down, -kMaxOffset, kMaxOffset)};
    m_guide_x = best_x < kSnap ? to_x / m_aspect : -1.0f;
    m_guide_y = best_y < kSnap ? to_y : -1.0f;
}

void TouchPad::place(std::size_t index) {
    TouchControl& c = m_controls[index];
    const Member& m = m_members[index];
    float across = 0.0f;
    float down = 0.0f;
    anchor(c.group, across, down);
    const float size = m_scale * m_sizes[static_cast<std::size_t>(c.group)];
    c.x = (across + m.dx * size) / m_aspect;
    c.y = down + m.dy * size;
    c.radius = m.radius * size;
}

bool TouchPad::stick_taken(std::size_t index) const {
    return std::any_of(m_fingers.begin(), m_fingers.end(), [index](const Finger& f) {
        return f.role == Role::Stick && f.stick == static_cast<int>(index);
    });
}

void TouchPad::layout_controls() {
    for (std::size_t i = 0; i < m_controls.size(); ++i) {
        // A stick a thumb holds stays under it.
        if (!stick_taken(i)) {
            place(i);
        }
    }
}

void TouchPad::bring_stick(std::size_t index, float x, float y) {
    TouchControl& c = m_controls[index];
    const float r = c.radius;
    const float across = std::clamp(x * m_aspect, m_safe_left + r, m_aspect * kStickZoneRight + r);
    c.x = across / m_aspect;
    c.y = std::clamp(y, kStickZoneTop, 1.0f - r);
}

float TouchPad::distance(const TouchControl& c, float x, float y) const {
    // Across the window in window heights: x times the aspect. A pill's straight middle counts as
    // its centre.
    const float straight = c.radius * (c.stretch - 1.0f);
    const float dx = std::max(std::abs((x - c.x) * m_aspect) - straight, 0.0f);
    const float dy = y - c.y;
    return std::sqrt(dx * dx + dy * dy);
}

TouchPad::Finger* TouchPad::find(std::uint64_t id) {
    const auto it = std::find_if(m_fingers.begin(), m_fingers.end(), [id](const Finger& f) {
        return f.id == id;
    });
    return it != m_fingers.end() ? &*it : nullptr;
}

void TouchPad::finger_down(std::uint64_t finger, float x, float y) {
    m_visible = true;
    m_touched = true;
    if (Finger* old = find(finger)) {
        finger_up(old->id);
    }
    Finger f{finger, x, y};
    if (m_editing) {
        // The group under the finger, the nearest if two are close; nothing beside them.
        float nearest = 0.0f;
        for (const TouchControl& c : m_controls) {
            if (c.kind == Kind::Menu && !m_menu_button) {
                continue;
            }
            const float d = distance(c, x, y) - c.radius - kButtonReach;
            if (d <= 0.0f && (f.role != Role::Drag || d < nearest)) {
                f.role = Role::Drag;
                f.group = c.group;
                nearest = d;
            }
        }
        if (f.role == Role::Drag) {
            m_selected = f.group;
            f.from_x = x;
            f.from_y = y;
            f.offset = m_offsets[static_cast<std::size_t>(f.group)];
        }
        m_fingers.push_back(f);
        return;
    }
    bool on_button = false;
    for (std::size_t i = 0; i < m_controls.size(); ++i) {
        const TouchControl& c = m_controls[i];
        if (c.kind == Kind::Menu) {
            if (m_menu_button && distance(c, x, y) <= c.radius + kButtonReach) {
                m_menu_pressed = true;
                on_button = true;
            }
            continue;
        }
        if (c.kind == Kind::Button) {
            const bool on = distance(c, x, y) <= c.radius + kButtonReach;
            if (on && c.then != 0 && i < m_held_since.size() && m_held_since[i] == 0) {
                // From now: a tap that lifts before the next frame still plays the combination.
                m_held_since[i] = std::max<std::uint64_t>(m_now_ms, 1);
            }
            if (on && c.toggle) {
                // A tap turns it on or off (a finger slid onto it changes nothing).
                const auto it = std::find(m_toggled.begin(), m_toggled.end(), c.action);
                if (it != m_toggled.end()) {
                    m_toggled.erase(it);
                } else {
                    m_toggled.push_back(c.action);
                }
            }
            if (on && c.stick && f.role == Role::None) {
                // Held by this finger wherever it slides, the slide pushing the stick.
                f.role = Role::Steer;
                f.stick = static_cast<int>(i);
                f.from_x = x;
                f.from_y = y;
            }
            on_button = on_button || on;
            continue;
        }
        if (!stick_taken(i) && distance(c, x, y) <= c.radius * kStickReach) {
            f.role = Role::Stick;
            f.stick = static_cast<int>(i);
            break;
        }
    }
    if (f.role == Role::None && !on_button) {
        if (y >= kStickZoneTop && x <= kStickZoneRight) {
            for (std::size_t i = 0; i < m_controls.size(); ++i) {
                if (m_controls[i].kind == Kind::LeftStick && !stick_taken(i)) {
                    bring_stick(i, x, y);
                    f.role = Role::Stick;
                    f.stick = static_cast<int>(i);
                    break;
                }
            }
        } else if (x >= kLookZoneLeft) {
            f.role = Role::Look;
            f.from_x = x;
            f.from_y = y;
        }
    }
    m_fingers.push_back(f);
}

void TouchPad::finger_move(std::uint64_t finger, float x, float y) {
    Finger* f = find(finger);
    if (f == nullptr) {
        return;
    }
    f->x = x;
    f->y = y;
    if (f->role == Role::Look) {
        push_look(*f);
    } else if (f->role == Role::Drag) {
        const auto g = static_cast<std::size_t>(f->group);
        m_offsets[g] =
            {std::clamp(f->offset[0] + (x - f->from_x) * m_aspect, -kMaxOffset, kMaxOffset),
             std::clamp(f->offset[1] + (y - f->from_y), -kMaxOffset, kMaxOffset)};
        snap(f->group);
        layout_controls();
    }
}

void TouchPad::finger_up(std::uint64_t finger) {
    std::erase_if(m_fingers, [finger](const Finger& f) { return f.id == finger; });
    m_guide_x = m_guide_y = -1.0f;
    // The camera stops with the finger lifted, or follows another still on it.
    m_look_x = m_look_y = 0.0f;
    for (Finger& f : m_fingers) {
        if (f.role == Role::Look) {
            push_look(f);
        }
    }
    layout_controls();  // a stick let go goes home
    take_waiting();
}

void TouchPad::release_all() {
    m_fingers.clear();
    m_guide_x = m_guide_y = -1.0f;
    m_look_x = m_look_y = 0.0f;
    layout_controls();
    take_waiting();
}

void TouchPad::set_menu_button(bool on) {
    m_menu_button = on;
    if (!on) {
        m_menu_pressed = false;
    }
}

bool TouchPad::take_menu_press() {
    const bool pressed = m_menu_pressed;
    m_menu_pressed = false;
    return pressed;
}

void TouchPad::idle(std::uint64_t now_ms) {
    m_now_ms = now_ms;
    m_held_since.resize(m_controls.size(), 0);
    for (std::size_t i = 0; i < m_controls.size(); ++i) {
        if (!held(i) && !combo_running(i)) {
            m_held_since[i] = 0;
        } else if (m_held_since[i] == 0) {
            m_held_since[i] = std::max<std::uint64_t>(now_ms, 1);
        }
    }
    if (m_touched || !m_fingers.empty() || m_editing) {
        m_touched = false;
        m_last_touch_ms = now_ms;
        m_visible = m_visible || m_editing;
    } else if (m_visible && now_ms - m_last_touch_ms >= kIdleHideMs) {
        m_visible = false;
    }
}

void TouchPad::push_look(Finger& f) {
    // In window heights across and down from the stick's centre, where the finger landed.
    const float reach = kLookReach / m_look_speed;
    float dx = (f.x - f.from_x) * m_aspect;
    float dy = f.y - f.from_y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length > reach) {
        // Out past the edge: the centre is dragged after the finger.
        const float excess = (length - reach) / length;
        f.from_x += dx * excess / m_aspect;
        f.from_y += dy * excess;
        dx -= dx * excess;
        dy -= dy * excess;
    }
    m_look_x = dx / reach;
    m_look_y = dy / reach;
}

void TouchPad::look(float& x, float& y) const {
    x = 0.0f;
    y = 0.0f;
    const float length = std::sqrt(m_look_x * m_look_x + m_look_y * m_look_y);
    if (length <= kLookDead) {
        return;
    }
    const float out =
        kLookFloor
        + (1.0f - kLookFloor) * (std::min(length, 1.0f) - kLookDead) / (1.0f - kLookDead);
    x = m_look_x / length * out;
    y = m_look_y / length * out;
}

bool TouchPad::held(std::size_t index) const {
    if (index >= m_controls.size()) {
        return false;
    }
    const TouchControl& c = m_controls[index];
    if (c.kind == Kind::Menu && !m_menu_button) {
        return false;
    }
    return std::any_of(m_fingers.begin(), m_fingers.end(), [&](const Finger& f) {
        switch (f.role) {
            case Role::Drag:
                return f.group == c.group;  // lit while it is being moved
            case Role::Stick:
                return f.stick == static_cast<int>(index);
            case Role::Look:
                return false;
            case Role::Steer:
                return f.stick == static_cast<int>(index);
            case Role::None:
                break;
        }
        return c.kind != Kind::LeftStick && !m_editing
               && distance(c, f.x, f.y) <= c.radius + kButtonReach;
    });
}

bool TouchPad::combo_running(std::size_t index) const {
    if (index >= m_controls.size() || index >= m_held_since.size()) {
        return false;
    }
    const std::uint64_t since = m_held_since[index];
    return m_controls[index].then != 0 && since != 0
           && m_now_ms < since + kThenDelayMs + kThenHoldMs;
}

bool TouchPad::toggled(std::size_t index) const {
    return index < m_controls.size() && m_controls[index].toggle
           && std::find(m_toggled.begin(), m_toggled.end(), m_controls[index].action)
                  != m_toggled.end();
}

void TouchPad::knob(std::size_t index, float& dx, float& dy) const {
    dx = 0.0f;
    dy = 0.0f;
    if (index >= m_controls.size()) {
        return;
    }
    const TouchControl& c = m_controls[index];
    for (const Finger& f : m_fingers) {
        if (f.role == Role::Stick && f.stick == static_cast<int>(index)) {
            dx = (f.x - c.x) * m_aspect;
            dy = f.y - c.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            if (length > c.radius) {
                dx *= c.radius / length;
                dy *= c.radius / length;
            }
            return;
        }
    }
}

void TouchPad::apply(Pad& pad, const StickDeadZone& dead_zone) const {
    if (m_editing) {
        return;  // the fingers move the controls, not Ratchet
    }
    const auto press = [&pad](std::uint16_t bits) {
        for (std::uint16_t bit = 1; bit != 0; bit = static_cast<std::uint16_t>(bit << 1)) {
            if ((bits & bit) != 0) {
                pad.set(static_cast<Button>(bit), true);
            }
        }
    };
    for (std::size_t i = 0; i < m_controls.size(); ++i) {
        const TouchControl& c = m_controls[i];
        if (c.toggle ? !toggled(i) : !held(i) && !combo_running(i)) {
            continue;
        }
        if (c.kind == Kind::Menu) {
            continue;  // the port's, not the game's
        }
        if (c.kind == Kind::Button) {
            press(c.buttons);
            // A combination's second press, a moment after its first (only while it is held).
            const std::uint64_t since = i < m_held_since.size() ? m_held_since[i] : 0;
            if (c.then != 0 && since != 0 && m_now_ms - since >= kThenDelayMs) {
                press(c.then);
            }
            continue;
        }
        float dx = 0.0f;
        float dy = 0.0f;
        knob(i, dx, dy);
        // The knob stays in its circle: squared as the DualShock 2's (pad.h), so a thumb pushed to
        // a diagonal runs.
        dx /= c.radius;
        dy /= c.radius;
        square_stick(dx, dy);
        stick_pair_bytes(dx, dy, dead_zone, pad.left_x, pad.left_y);
    }
    // A finger steering an action: the stick all the way toward its slide, once it has slid.
    for (const Finger& f : m_fingers) {
        if (f.role != Role::Steer || !held(static_cast<std::size_t>(f.stick))) {
            continue;
        }
        float dx = (f.x - f.from_x) * m_aspect;
        float dy = f.y - f.from_y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length >= kSteerDead) {
            dx /= length;
            dy /= length;
            square_stick(dx, dy);
            stick_pair_bytes(dx, dy, dead_zone, pad.left_x, pad.left_y);
        }
    }
    float lx = 0.0f;
    float ly = 0.0f;
    look(lx, ly);
    if (lx != 0.0f || ly != 0.0f) {
        // Camera speed above 100%: a full sideways swipe also turns faster, like the stick.
        pad.camera_speed = std::fabs(lx) >= 0.999f ? std::max(1.0f, m_look_speed) : 1.0f;
        square_stick(lx, ly);
        stick_pair_bytes(lx, ly, dead_zone, pad.right_x, pad.right_y);
    }
}

}  // namespace openrac::platform

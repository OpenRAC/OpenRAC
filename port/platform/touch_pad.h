// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A DualShock 2 on a touch screen (Android phones and tablets; any touch screen SDL reports): the
// left stick, the d-pad, the four face buttons, the shoulders, Select and Start, laid out for a
// picture held sideways; on Android, between Select and Start, the button that opens the port's
// options (game/common/debug_overlay.h, the drawer). Fingers come in as SDL gives them (0..1
// across and down the window) and the pad's state goes out as a Pad's (pad.h), added to what the
// keyboard and a gamepad give port 0.
//
// A stick is held by the finger that touched inside it, and follows it until it lifts, even
// outside the circle; the left stick also comes to the thumb that lands anywhere in the lower
// left of the screen, and goes home when it lifts. A button is held while any finger is on it, so
// a thumb slid from one to the next presses the next. The right stick is not drawn: a finger that
// lands on the right half of the screen away from the buttons holds an invisible one centred where
// it landed, and turns the camera for as long as it is pushed away from there (look()).
//
// The controls keep their distance from the edges in window heights, whatever the screen's shape,
// inside its safe area (a camera's notch). The player makes them smaller or larger (set_scale(),
// which brings them nearer the edges too) and moves them (set_editing(): a finger drags the group
// it lands on); where each group was moved to is a line of text for the settings file (layout()).
// Nothing here touches SDL or GL: input.cpp feeds it events and the frontend draws controls()
// (renderer/ui/touch_overlay.h).
//
// That is the Classic style. In the Arc style the buttons are actions (touch_actions.h): an icon
// each, sized by how often it is used, the largest in the corner under the right thumb and the
// rest on an arc around it, and they change with what the game is doing (set_actions(), from the
// game's touch context). A button under a finger stays until the finger lifts: the new set comes
// in then, so a press is never lost to a change of context.

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "platform/pad.h"
#include "platform/touch_actions.h"

namespace openrac::platform {

// The parts of the pad that move together when the player rearranges it.
enum class TouchGroup {
    Stick,
    Dpad,
    Face,
    L1,
    L2,
    R1,
    R2,
    Select,
    Start,
    Menu,
    // The Arc style's places: the arc under the right thumb (the first the corner), beside the
    // stick, along the top, and where a button the game shows for a moment (a hint's) comes up.
    Arc0,
    Arc1,
    Arc2,
    Arc3,
    Arc4,
    Arc5,
    Arc6,
    Arc7,
    Left0,
    Left1,
    Left2,
    Top0,
    Top1,
    Hint0,
    Hint1,
    Hint2,
    Count
};

enum class TouchStyle {
    Classic,  // the DualShock 2's buttons
    Arc       // the game's actions, as icons
};

inline constexpr std::size_t kTouchGroups = static_cast<std::size_t>(TouchGroup::Count);

struct TouchControl {
    enum class Kind {
        LeftStick,
        Button,
        Menu
    };
    Kind kind = Kind::Button;
    Button button = Button::Cross;  // for Kind::Button
    TouchGroup group = TouchGroup::Face;
    // The centre: x across the window (0..1), y down it (0..1); the radius in window heights.
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.0f;
    // Half the width over the radius: 1 a circle, more a pill (the shoulders, Select and Start, a
    // wide action).
    float stretch = 1.0f;
    // The Arc style's: the buttons it holds (`button` is the first of them), those it adds a moment
    // later, its action and icon (touch_actions.h); empty icon for the Classic style's controls.
    std::uint16_t buttons = 0;
    std::uint16_t then = 0;
    bool toggle = false;
    bool stick = false;  // a finger held on it pushes the left stick the way it slides
    std::string action;
    std::string icon;
};

class TouchPad {
public:
    TouchPad();

    // The window's width over its height: the controls keep their shape at any size.
    void set_aspect(float aspect);

    float aspect() const { return m_aspect; }

    // Classic (the default) or Arc; Arc shows the actions set_actions() gives, Classic ignores them.
    void set_style(TouchStyle style);

    TouchStyle style() const { return m_style; }

    // The actions of the game's context now, and the hints up (an action with a `when` shows only
    // while its hint is among them). Taken in at once when no finger is on a button, else when the
    // last such finger lifts. The same list again changes nothing. `dpad` puts the d-pad in the
    // stick's place (a menu's).
    void set_actions(
        std::vector<TouchAction> actions, std::vector<std::string> hints = {}, bool dpad = false
    );

    // Whether set_actions() has a set waiting for a finger to lift.
    bool actions_waiting() const { return m_waiting; }

    // A combination's second press (TouchAction::then) comes this long after its first. A tap
    // shorter than that still plays the whole combination: both presses are held until
    // kThenDelayMs + kThenHoldMs after the finger landed.
    static constexpr std::uint64_t kThenDelayMs = 50;
    static constexpr std::uint64_t kThenHoldMs = 50;

    // Where the screen cannot be seen or touched at the left and right edges, in window heights.
    void set_safe_area(float left, float right);

    // The controls' size against the pad's own (1), held to kMinScale..kMaxScale. Smaller ones sit
    // nearer the edges, so more of the picture shows.
    static constexpr float kMinScale = 0.5f;
    static constexpr float kMaxScale = 1.5f;
    static constexpr float kDefaultScale = 0.75f;
    void set_scale(float scale);

    float scale() const { return m_scale; }

    // How short a push turns the camera all the way (1: the pad's own; 2 half as far), held to
    // 0.25..3.
    void set_look_speed(float speed);

    // Where the player moved each group, from its own place, in window heights, and how large
    // they made it (1 its own size, left out when 1): "dpad 0.02 -0.05, face 0 0 1.2, ..."
    // (layout() writes it). Unknown names and numbers are left out.
    void set_layout(std::string_view text);
    std::string layout() const;
    void reset_layout();

    // The group a finger last took hold of while editing, for its size to be changed; Count for
    // none.
    TouchGroup selected() const { return m_selected; }

    // The selected group's size: 1 its own, kMinSize..kMaxSize, in steps of kSizeStep.
    static constexpr float kMinSize = 0.6f;
    static constexpr float kMaxSize = 1.6f;
    static constexpr float kSizeStep = 0.1f;
    float selected_size() const;
    void step_selected_size(int steps);

    // While editing, a finger drags the group it lands on and the game gets nothing; the pad stays
    // shown. A dragged group snaps into line with what it comes near (snap()).
    void set_editing(bool on);

    bool editing() const { return m_editing; }

    // The lines the group being dragged has snapped to, for the editing screen to show: across
    // (0..1 of the window's width) and down (0..1 of its height); below 0, none.
    float guide_x() const { return m_guide_x; }

    float guide_y() const { return m_guide_y; }

    void finger_down(std::uint64_t finger, float x, float y);
    void finger_move(std::uint64_t finger, float x, float y);
    void finger_up(std::uint64_t finger);

    // Every finger lifted (the window lost focus, the app went to the background).
    void release_all();

    // The options button (Kind::Menu): there only where the window has a drawer to open with it.
    void set_menu_button(bool on);

    bool menu_button() const { return m_menu_button; }

    // True once for each press of the options button.
    bool take_menu_press();

    // Adds what the fingers hold to `pad`: buttons pressed, the left stick's axes and the camera's
    // (the right stick) replacing the pad's while they are held, past the game's `dead_zone` (pad.h).
    void apply(Pad& pad, const StickDeadZone& dead_zone = {}) const;

    // Hidden at first; shown once the screen has been touched, until another device is used
    // (hide()) or no finger has touched it for kIdleHideMs (idle()).
    bool visible() const { return m_visible; }

    void hide() { m_visible = m_editing; }

    // Once a frame, with SDL's clock: puts the pad away when the screen has been left alone, and
    // times the combinations' second presses.
    static constexpr std::uint64_t kIdleHideMs = 10000;
    void idle(std::uint64_t now_ms);

    std::span<const TouchControl> controls() const { return m_controls; }

    // Whether control `index` is held now, and where a stick's knob is (offset from its centre in
    // window heights).
    bool held(std::size_t index) const;
    // Whether a toggle (TouchAction::toggle) is on: it holds its buttons with no finger on it.
    bool toggled(std::size_t index) const;
    void knob(std::size_t index, float& dx, float& dy) const;

    // The camera as the fingers turn it: -1..1 across and down, 0 with no finger on it.
    void look(float& x, float& y) const;

private:
    enum class Role {
        None,
        Stick,
        Look,
        Drag,
        Steer  // holds a "stick" action wherever it slides (TouchControl::stick)
    };

    struct Finger {
        std::uint64_t id = 0;
        float x = 0.0f;
        float y = 0.0f;
        Role role = Role::None;
        int stick = -1;  // Role::Stick, Role::Steer: the index of the control it holds
        // Role::Look: the invisible stick's centre. Role::Drag, Role::Steer: where it landed (and
        // the group's offset then).
        float from_x = 0.0f;
        float from_y = 0.0f;
        TouchGroup group = TouchGroup::Stick;
        std::array<float, 2> offset{};
    };

    // Where a group lives: its centre's distance from an edge (or from the middle), in window
    // heights, and its height down the window. Its controls are placed around it.
    struct Place {
        enum class From { Left, Middle, Right };
        From from = From::Left;
        float offset = 0.0f;
        float y = 0.0f;
    };

    // Distance from a point to control `c`'s outline (0 at its centre for a circle), in window
    // heights.
    float distance(const TouchControl& c, float x, float y) const;
    Finger* find(std::uint64_t id);
    bool stick_taken(std::size_t index) const;
    // The group's centre for the window's shape, the scale and the player's offset, kept on the
    // screen; in window heights across and down.
    void anchor(TouchGroup group, float& across, float& down) const;
    // Puts control `index` at its place.
    void place(std::size_t index);
    void layout_controls();
    // The camera from look finger `f` (m_look_x, m_look_y), its stick's centre dragged after it
    // past the stick's reach.
    void push_look(Finger& f);
    // Moves dragged group `group` onto the nearest line within kSnap of its centre, across and down
    // apart: where it was first placed, the window's middle, another group's centre, or the mirror
    // of another's across the middle (L1 opposite R1).
    void snap(TouchGroup group);
    // Brings the left stick under a thumb at (x, y), kept on the screen.
    void bring_stick(std::size_t index, float x, float y);
    // Makes the controls of the style and actions now (the stick's place kept while a thumb holds it).
    void rebuild();
    void classic_controls();
    void arc_controls();
    // Whether combination `index`, tapped and let go, still plays its presses.
    bool combo_running(std::size_t index) const;
    // Whether a finger holds a button (not the stick, the camera or a drag).
    bool button_held() const;
    // Takes in the waiting set when no finger holds a button.
    void take_waiting();

    std::vector<TouchControl> m_controls;

    // Each control's offset from its group's centre and its radius, in window heights at scale 1.
    struct Member {
        float dx = 0.0f;
        float dy = 0.0f;
        float radius = 0.0f;
    };

    std::vector<Member> m_members;
    std::array<Place, kTouchGroups> m_places{};
    std::array<std::array<float, 2>, kTouchGroups> m_offsets{};
    std::array<float, kTouchGroups> m_sizes = filled_sizes();
    TouchGroup m_selected = TouchGroup::Count;

    static constexpr std::array<float, kTouchGroups> filled_sizes() {
        std::array<float, kTouchGroups> sizes{};
        sizes.fill(1.0f);
        return sizes;
    }
    float m_safe_left = 0.0f;
    float m_safe_right = 0.0f;
    float m_scale = kDefaultScale;
    float m_look_speed = 1.0f;
    // The camera's push now (push_look), before look()'s dead zone and floor.
    float m_look_x = 0.0f;
    float m_look_y = 0.0f;
    std::vector<Finger> m_fingers;
    float m_aspect = 16.0f / 9.0f;
    bool m_visible = false;
    bool m_editing = false;
    float m_guide_x = -1.0f;
    float m_guide_y = -1.0f;
    // A finger came down since the last idle(): it counts even if it lifted within the frame.
    bool m_touched = false;
    std::uint64_t m_last_touch_ms = 0;
    bool m_menu_button = false;
    bool m_menu_pressed = false;
    TouchStyle m_style = TouchStyle::Classic;
    std::vector<TouchAction> m_actions;
    std::vector<std::string> m_hints;
    std::vector<TouchAction> m_next_actions;
    std::vector<std::string> m_next_hints;
    bool m_dpad = false;
    bool m_next_dpad = false;
    bool m_waiting = false;
    // The actions a tap has turned on (TouchAction::toggle), by id.
    std::vector<std::string> m_toggled;
    // When each control started being held (SDL's clock; 0 not held), and the clock's last read.
    std::vector<std::uint64_t> m_held_since;
    std::uint64_t m_now_ms = 0;
};

}  // namespace openrac::platform

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The on-screen pad (platform/touch_pad.h): fingers on its controls, as SDL reports them, become
// the pad's buttons, its stick and the camera; the player resizes and moves the controls.

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "check.h"
#include "platform/touch_actions.h"
#include "platform/touch_pad.h"

namespace {

using namespace openrac::platform;

const TouchControl& control(
    const TouchPad& touch, TouchControl::Kind kind, Button button = Button::Cross
) {
    for (const TouchControl& c : touch.controls()) {
        if (c.kind == kind && (kind != TouchControl::Kind::Button || c.button == button)) {
            return c;
        }
    }
    std::fprintf(stderr, "no such control\n");
    return touch.controls()[0];
}

Pad read(const TouchPad& touch) {
    Pad pad;
    touch.apply(pad);
    return pad;
}

const TouchControl* action(const TouchPad& touch, std::string_view id) {
    for (const TouchControl& c : touch.controls()) {
        if (c.action == id) {
            return &c;
        }
    }
    return nullptr;
}

TouchAction act(std::string id, std::uint16_t press, TouchSize size = TouchSize::Medium) {
    TouchAction a;
    a.icon = id;
    a.id = std::move(id);
    a.press = press;
    a.size = size;
    return a;
}

constexpr auto bit = [](Button b) {
    return static_cast<std::uint16_t>(b);
};

// The Arc style: the game's actions as buttons, in the corner first, changing with its context.
void arc_style() {
    TouchPad touch;
    touch.set_aspect(2.0f);
    std::vector<TouchAction> foot = {
        act("jump", bit(Button::Cross), TouchSize::Large),
        act("fire", bit(Button::Circle)),
        act("dash", bit(Button::R1), TouchSize::Small),
    };
    foot[2].then = bit(Button::Cross);
    TouchAction swing = act("swing", bit(Button::Circle));
    swing.when = "swing_target";
    swing.side = TouchSide::Hint;
    foot.push_back(swing);
    touch.set_actions(foot);
    CHECK(action(touch, "jump") == nullptr);  // the Classic style shows the DualShock's buttons
    touch.set_style(TouchStyle::Arc);
    const TouchControl* jump = action(touch, "jump");
    const TouchControl* fire = action(touch, "fire");
    CHECK(jump != nullptr && fire != nullptr && action(touch, "swing") == nullptr);
    // The first and largest in the corner, at the lower right.
    CHECK(jump->x > fire->x && jump->radius > fire->radius && jump->y > 0.6f);
    CHECK(read(touch).buttons == Pad{}.buttons);
    touch.finger_down(1, jump->x, jump->y);
    CHECK(read(touch).held(Button::Cross) && !read(touch).held(Button::Circle));
    touch.finger_up(1);

    // A combination: its first button at once, the second a moment later, both while held.
    const TouchControl* dash = action(touch, "dash");
    CHECK(dash != nullptr);
    touch.idle(1000);
    touch.finger_down(2, dash->x, dash->y);
    touch.idle(1016);
    CHECK(read(touch).held(Button::R1) && !read(touch).held(Button::Cross));
    touch.idle(1016 + TouchPad::kThenDelayMs);
    CHECK(read(touch).held(Button::R1) && read(touch).held(Button::Cross));
    touch.finger_up(2);
    touch.idle(1100);
    CHECK(!read(touch).held(Button::R1) && !read(touch).held(Button::Cross));
    // A tap quicker than the delay still plays the whole combination, then lets go.
    touch.idle(2000);
    touch.finger_down(4, dash->x, dash->y);
    touch.finger_up(4);
    touch.idle(2016);
    CHECK(read(touch).held(Button::R1) && !read(touch).held(Button::Cross));
    touch.idle(2000 + TouchPad::kThenDelayMs);
    CHECK(read(touch).held(Button::R1) && read(touch).held(Button::Cross));
    touch.idle(2000 + TouchPad::kThenDelayMs + TouchPad::kThenHoldMs);
    CHECK(!read(touch).held(Button::R1) && !read(touch).held(Button::Cross));

    // A hint brings its button up; with no finger on a button the change is at once.
    touch.set_actions(foot, {"swing_target"});
    CHECK(!touch.actions_waiting() && action(touch, "swing") != nullptr);

    // Another context while a finger holds a button: the old set stays until it lifts.
    jump = action(touch, "jump");
    touch.finger_down(3, jump->x, jump->y);
    touch.set_actions({act("boost", bit(Button::Cross), TouchSize::Large)});
    CHECK(touch.actions_waiting() && action(touch, "jump") != nullptr);
    CHECK(read(touch).held(Button::Cross));
    touch.finger_up(3);
    CHECK(
        !touch.actions_waiting() && action(touch, "jump") == nullptr
        && action(touch, "boost") != nullptr
    );

    // A thumb on the stick keeps it through a change, where it is.
    TouchControl stick = touch.controls()[0];
    CHECK(stick.kind == TouchControl::Kind::LeftStick);
    touch.finger_down(4, stick.x, stick.y);
    touch.finger_move(4, stick.x + 0.3f, stick.y);
    touch.set_actions(foot);
    CHECK(action(touch, "jump") != nullptr && read(touch).left_x == 0xFF);
    touch.finger_up(4);

    // A toggle: a tap holds it, another lets go.
    TouchAction look = act("look", bit(Button::L1), TouchSize::Small);
    look.side = TouchSide::Left;
    look.toggle = true;
    touch.set_actions({look});
    const TouchControl* l = action(touch, "look");
    CHECK(l != nullptr && l->x < 0.5f);
    touch.finger_down(5, l->x, l->y);
    touch.finger_up(5);
    CHECK(
        read(touch).held(Button::L1)
        && touch.toggled(static_cast<std::size_t>(l - touch.controls().data()))
    );
    touch.finger_down(5, l->x, l->y);
    touch.finger_up(5);
    CHECK(!read(touch).held(Button::L1));

    // Back to the Classic style: the DualShock's buttons again.
    touch.set_style(TouchStyle::Classic);
    CHECK(action(touch, "look") == nullptr && touch.controls().size() == 16);
}

// The sets, as the game's files and mods write them.
void action_sets() {
    TouchActionSets sets;
    std::string error;
    CHECK(sets.read(
        R"({"contexts": {
        "base": {"actions": [{"id": "map", "icon": "map", "press": ["select"], "side": "top"}]},
        "foot": {"extends": "base", "actions": [
            {"id": "jump", "icon": "jump", "press": ["cross"], "size": "large"},
            {"id": "dash", "icon": "dash", "press": ["r1"], "then": ["cross"], "size": "small"}]}}})",
        "game",
        error
    ));
    std::vector<TouchAction> foot = sets.actions("foot");
    CHECK(foot.size() == 3 && foot[0].id == "map" && foot[0].side == TouchSide::Top);
    CHECK(foot[1].press == bit(Button::Cross) && foot[1].size == TouchSize::Large);
    CHECK(foot[2].press == bit(Button::R1) && foot[2].then == bit(Button::Cross));
    // A mod adds a button and changes another; a bad set changes nothing.
    CHECK(sets.read(
        R"({"contexts": {"foot": {"actions": [
            {"id": "board", "icon": "board", "press": ["up"]},
            {"id": "jump", "icon": "jump", "press": ["cross"], "size": "medium"}]}}})",
        "mod",
        error
    ));
    foot = sets.actions("foot");
    CHECK(foot.size() == 4 && foot[1].size == TouchSize::Medium && foot[3].id == "board");
    CHECK(!sets.read(
        R"({"contexts": {"foot": {"actions": [{"id": "x", "icon": "x", "press": ["z"]}]}}})",
        "bad",
        error
    ));
    CHECK(error.find("no button") != std::string::npos);
    CHECK(sets.actions("foot").size() == 4);
    // A context of its own, replacing.
    CHECK(sets.read(
        R"({"contexts": {"foot": {"replace": true, "actions": [
            {"id": "x", "icon": "x", "press": ["cross"]}]}}})",
        "mod2",
        error
    ));
    CHECK(sets.actions("foot").size() == 1 && sets.actions("nothing").empty());
    // A loop of extends ends.
    CHECK(sets.read(
        R"({"contexts": {"a": {"extends": "b", "actions": []}, "b": {"extends": "a", "actions": []}}})",
        "loop",
        error
    ));
    CHECK(sets.actions("a").empty());
    // A menu's d-pad and a steering action, taken by the contexts that extend them.
    CHECK(sets.read(
        R"({"contexts": {"menu": {"dpad": true, "actions": [
            {"id": "quick", "icon": "quick", "press": ["triangle"], "stick": true}]},
            "vendor": {"extends": "menu", "actions": []}}})",
        "menus",
        error
    ));
    CHECK(sets.dpad("menu") && sets.dpad("vendor") && !sets.dpad("a") && !sets.dpad("nothing"));
    CHECK(sets.actions("vendor").size() == 1 && sets.actions("vendor")[0].stick);
}

// A finger held on a "stick" action pushes the left stick the way it slides, and keeps the action
// held off its button; a menu's context shows the d-pad in the stick's place.
void steer_and_dpad() {
    TouchPad touch;
    touch.set_aspect(2.0f);
    touch.set_style(TouchStyle::Arc);
    TouchAction quick = act("quick", bit(Button::Triangle));
    quick.stick = true;
    touch.set_actions({act("jump", bit(Button::Cross), TouchSize::Large), quick});
    const TouchControl* q = action(touch, "quick");
    CHECK(q != nullptr && q->stick);
    touch.finger_down(1, q->x, q->y);
    Pad pad = read(touch);
    CHECK(pad.held(Button::Triangle) && pad.left_x == Pad{}.left_x && pad.left_y == Pad{}.left_y);
    // Slid left, far off the button: still held, the stick all the way left.
    touch.finger_move(1, q->x - 0.2f, q->y);
    pad = read(touch);
    CHECK(pad.held(Button::Triangle) && pad.left_x < 0x20 && pad.left_y == Pad{}.left_y);
    // The set waits for it to lift, as for any finger on a button.
    touch.set_actions({act("ok", bit(Button::Cross), TouchSize::Large)}, {}, true);
    CHECK(touch.actions_waiting());
    touch.finger_up(1);
    CHECK(!touch.actions_waiting() && !read(touch).held(Button::Triangle));
    // The d-pad: no stick, four arms where it was.
    bool stick = false;
    for (const TouchControl& c : touch.controls()) {
        stick = stick || c.kind == TouchControl::Kind::LeftStick;
    }
    CHECK(!stick);
    const TouchControl& up = control(touch, TouchControl::Kind::Button, Button::Up);
    const TouchControl& down = control(touch, TouchControl::Kind::Button, Button::Down);
    CHECK(up.group == TouchGroup::Stick && up.y < down.y && up.x < 0.5f);
    touch.finger_down(2, up.x, up.y);
    CHECK(read(touch).held(Button::Up) && !read(touch).held(Button::Down));
    touch.finger_up(2);
    // Back in play: the stick again.
    touch.set_actions({act("jump", bit(Button::Cross), TouchSize::Large)});
    CHECK(touch.controls()[0].kind == TouchControl::Kind::LeftStick);
}

// A "press triangle" prompt's button stands in for quick select while its hint is up.
void stand_in() {
    TouchPad touch;
    touch.set_aspect(2.0f);
    touch.set_style(TouchStyle::Arc);
    TouchAction quick = act("quick", bit(Button::Triangle));
    quick.stick = true;
    TouchAction enter = act("enter", bit(Button::Triangle), TouchSize::Small);
    enter.when = "board";
    enter.instead = "quick";
    const std::vector<TouchAction> foot{
        act("jump", bit(Button::Cross), TouchSize::Large), quick, enter
    };
    touch.set_actions(foot);
    const TouchControl* q = action(touch, "quick");
    CHECK(q != nullptr && action(touch, "enter") == nullptr);
    const float x = q->x, y = q->y, r = q->radius;
    touch.set_actions(foot, {"board"});
    const TouchControl* e = action(touch, "enter");
    CHECK(action(touch, "quick") == nullptr && e != nullptr);
    CHECK(e->x == x && e->y == y && e->radius == r && !e->stick && e->group != TouchGroup::Hint0);
    TouchActionSets sets;
    std::string error;
    CHECK(!sets.read(
        R"({"contexts": {"foot": {"actions": [
        {"id": "a", "icon": "a", "press": ["triangle"], "instead": "quick"}]}}})",
        "t",
        error
    ));
}

// An action with its own arc place keeps it whatever comes before it; the rest fill the free ones.
void own_place() {
    TouchPad touch;
    touch.set_aspect(2.0f);
    touch.set_style(TouchStyle::Arc);
    TouchAction jump = act("jump", bit(Button::Cross), TouchSize::Large);
    jump.at = 0;
    TouchAction wrench = act("wrench", bit(Button::Square));
    wrench.at = 4;
    touch.set_actions({act("fire", bit(Button::Circle)), wrench, jump});
    CHECK(action(touch, "jump")->group == TouchGroup::Arc0);
    CHECK(action(touch, "wrench")->group == TouchGroup::Arc4);
    CHECK(action(touch, "fire")->group == TouchGroup::Arc1);
    TouchActionSets sets;
    std::string error;
    CHECK(sets.read(
        R"({"contexts": {"a": {"actions": [
        {"id": "a", "icon": "a", "press": ["cross"], "at": 7}]}}})",
        "t",
        error
    ));
    CHECK(sets.actions("a").at(0).at == 7);
    CHECK(!sets.read(
        R"({"contexts": {"a": {"actions": [
        {"id": "a", "icon": "a", "press": ["cross"], "at": 8}]}}})",
        "t",
        error
    ));
    CHECK(!sets.read(
        R"({"contexts": {"a": {"actions": [
        {"id": "a", "icon": "a", "press": ["cross"], "at": "x"}]}}})",
        "t",
        error
    ));
}

}  // namespace

int main() {
    TouchPad touch;
    touch.set_aspect(16.0f / 9.0f);
    CHECK(!touch.visible());
    CHECK(read(touch).buttons == Pad{}.buttons);

    // Cross, held and let go.
    const TouchControl& cross = control(touch, TouchControl::Kind::Button, Button::Cross);
    touch.finger_down(1, cross.x, cross.y);
    CHECK(touch.visible());
    CHECK(read(touch).held(Button::Cross));
    CHECK(!read(touch).held(Button::Triangle));
    touch.finger_up(1);
    CHECK(!read(touch).held(Button::Cross));

    // The left stick pushed right to its edge and beyond: full right, centre vertically.
    const TouchControl& left = control(touch, TouchControl::Kind::LeftStick);
    touch.finger_down(2, left.x, left.y);
    touch.finger_move(2, left.x + 0.5f, left.y);
    Pad pad = read(touch);
    CHECK(pad.left_x == 0xFF);
    CHECK(pad.left_y >= 0x7F && pad.left_y <= 0x81);
    CHECK(pad.right_x == kStickCentre);
    // Pushed to a diagonal, the round stick reaches the corner, as a DualShock 2's does.
    touch.finger_move(2, left.x + 0.5f, left.y + 0.5f * touch.aspect());
    pad = read(touch);
    CHECK(pad.left_x == 0xFF && pad.left_y == 0xFF);
    touch.finger_move(2, left.x + 0.5f, left.y);
    // A second finger on Square while the stick is held: both count.
    const TouchControl& square = control(touch, TouchControl::Kind::Button, Button::Square);
    touch.finger_down(3, square.x, square.y);
    pad = read(touch);
    CHECK(pad.held(Button::Square));
    CHECK(pad.left_x == 0xFF);
    // The stick's finger dragged up over the d-pad keeps the stick (full up), presses no button.
    touch.finger_move(2, left.x, 0.0f);
    pad = read(touch);
    CHECK(pad.left_y == 0x00);
    CHECK(!pad.held(Button::Up));

    // Everything lifted (the app went to the background).
    touch.release_all();
    pad = read(touch);
    CHECK(pad.left_x == kStickCentre && pad.left_y == kStickCentre);
    CHECK(!pad.held(Button::Square));

    // Away from every control: nothing.
    touch.finger_down(4, 0.5f, 0.5f);
    CHECK(read(touch).buttons == Pad{}.buttons);
    touch.hide();
    CHECK(!touch.visible());

    // A thumb in the lower left, away from the stick, brings the stick to it; lifted, it goes home.
    TouchPad floating;
    floating.set_aspect(2.0f);
    const TouchControl& stick = control(floating, TouchControl::Kind::LeftStick);
    const float home_x = stick.x;
    const float home_y = stick.y;
    floating.finger_down(7, 0.32f, 0.55f);
    CHECK(stick.x == 0.32f && stick.y == 0.55f);
    floating.finger_move(7, 0.32f, 0.40f);
    CHECK(read(floating).left_y < 0x40);
    floating.finger_up(7);
    CHECK(stick.x == home_x && stick.y == home_y);
    // In the middle of the screen it does not.
    floating.finger_down(8, 0.6f, 0.6f);
    CHECK(stick.x == home_x && read(floating).left_x == kStickCentre);
    floating.finger_up(8);

    // The controls keep their distance from the edges, inside the safe area.
    const TouchControl& circle = control(floating, TouchControl::Kind::Button, Button::Circle);
    const float circle_x = circle.x;
    floating.set_safe_area(0.0f, 0.1f);
    CHECK(std::abs((circle_x - circle.x) * 2.0f - 0.1f) < 1e-5f);
    floating.set_aspect(2.4f);
    CHECK(std::abs((1.0f - circle.x) * 2.4f - (1.0f - circle_x) * 2.0f - 0.1f) < 1e-5f);

    // Left alone, the pad goes away; a tap that lifts within the frame still counts as a touch.
    TouchPad idle;
    idle.idle(1000);
    CHECK(!idle.visible());
    idle.finger_down(5, 0.5f, 0.5f);
    idle.finger_up(5);
    idle.idle(50000);
    CHECK(idle.visible());
    idle.idle(50000 + TouchPad::kIdleHideMs - 1);
    CHECK(idle.visible());
    idle.idle(50000 + TouchPad::kIdleHideMs);
    CHECK(!idle.visible());
    // A finger held still keeps it.
    idle.finger_down(6, 0.5f, 0.5f);
    idle.idle(90000);
    idle.idle(200000);
    CHECK(idle.visible());

    // The options button: nothing until the window has a drawer to open, then a press of its own,
    // none of the game's buttons, and no stick brought under it.
    TouchPad menu;
    menu.set_aspect(16.0f / 9.0f);
    const TouchControl& options = control(menu, TouchControl::Kind::Menu);
    menu.finger_down(1, options.x, options.y);
    CHECK(!menu.take_menu_press());
    menu.finger_up(1);
    menu.set_menu_button(true);
    menu.finger_down(2, options.x, options.y);
    CHECK(menu.take_menu_press());
    CHECK(!menu.take_menu_press());
    const Pad pressed = read(menu);
    CHECK(pressed.buttons == Pad{}.buttons);
    CHECK(pressed.left_x == Pad{}.left_x && pressed.left_y == Pad{}.left_y);
    for (std::size_t i = 0; i < menu.controls().size(); ++i) {
        if (menu.controls()[i].kind == TouchControl::Kind::Menu) {
            CHECK(menu.held(i));
        }
    }
    // Clear of Select and Start beside it.
    const TouchControl& select = control(menu, TouchControl::Kind::Button, Button::Select);
    const TouchControl& start = control(menu, TouchControl::Kind::Button, Button::Start);
    const float gap =
        (start.x - options.x) * menu.aspect() - start.radius * start.stretch - options.radius;
    CHECK(gap > 0.05f * menu.scale());
    CHECK(select.x < options.x && options.x < start.x);

    // No right stick drawn: a finger on the right of the screen, off the buttons, holds an
    // invisible one centred where it landed. Pushed, the camera turns for as long as it is held
    // there, as a stick held over does; it stops when the finger lifts.
    TouchPad camera;
    camera.set_aspect(2.0f);
    for (const TouchControl& c : camera.controls()) {
        CHECK(c.kind != TouchControl::Kind::Button || c.button != Button::R3);
    }
    camera.idle(1000);
    camera.finger_down(1, 0.6f, 0.4f);
    camera.idle(1016);
    CHECK(read(camera).right_x == kStickCentre);
    camera.finger_move(1, 0.6f + 0.002f, 0.4f);  // a tremble: inside the dead zone
    CHECK(read(camera).right_x == kStickCentre);
    camera.finger_move(1, 0.6f + 0.06f, 0.4f);  // right, 0.12 heights: all the way
    for (int frame = 1; frame <= 60; ++frame) {
        camera.idle(1016 + 16 * static_cast<std::uint64_t>(frame));  // held there a second
    }
    pad = read(camera);
    CHECK(pad.right_x == 0xFF);
    CHECK(pad.right_y >= 0x7E && pad.right_y <= 0x82);
    CHECK(pad.buttons == Pad{}.buttons);
    // Half as far: about half way, past the game's dead zone.
    camera.finger_move(1, 0.6f + 0.025f, 0.4f);
    pad = read(camera);
    CHECK(pad.right_x > 0xA8 && pad.right_x < 0xE8);
    // Past the edge the centre follows the finger, so that coming back turns the camera the
    // other way at once.
    camera.finger_move(1, 0.9f, 0.4f);
    camera.finger_move(1, 0.84f, 0.4f);
    CHECK(read(camera).right_x < kStickCentre);
    camera.finger_move(1, 0.84f, 0.35f);
    CHECK(read(camera).right_y < kStickCentre);
    camera.finger_up(1);
    CHECK(read(camera).right_x == kStickCentre && read(camera).right_y == kStickCentre);
    // The player's camera speed: twice the speed, half the push for all the way.
    camera.set_look_speed(2.0f);
    camera.finger_down(4, 0.6f, 0.4f);
    camera.finger_move(4, 0.6f + 0.03f, 0.4f);
    CHECK(read(camera).right_x == 0xFF);
    // A full sideways swipe above 100% also turns the camera faster; at 100% it does not.
    camera.finger_move(4, 0.6f + 0.2f, 0.4f);
    CHECK(read(camera).camera_speed == 2.0f);
    camera.set_look_speed(1.0f);
    camera.finger_move(4, 0.6f + 0.4f, 0.4f);
    CHECK(read(camera).camera_speed == 1.0f);
    camera.finger_up(4);
    // A finger that lands on a button presses it and does not turn the camera, even dragged.
    const TouchControl& triangle = control(camera, TouchControl::Kind::Button, Button::Triangle);
    camera.finger_down(2, triangle.x, triangle.y);
    camera.idle(2000);
    camera.finger_move(2, triangle.x - 0.01f, triangle.y);
    camera.idle(2016);
    pad = read(camera);
    CHECK(pad.held(Button::Triangle));
    CHECK(pad.right_x == kStickCentre);
    camera.finger_up(2);
    // The left thumb on the stick and the right one on the camera at once.
    const TouchControl& walk = control(camera, TouchControl::Kind::LeftStick);
    camera.finger_down(3, walk.x, walk.y);
    camera.finger_move(3, walk.x, walk.y - 0.5f);
    camera.finger_down(4, 0.7f, 0.5f);
    camera.idle(3000);
    camera.finger_move(4, 0.7f, 0.55f);
    camera.idle(3016);
    pad = read(camera);
    CHECK(pad.left_y == 0x00);
    CHECK(pad.right_y > kStickCentre);
    camera.release_all();

    // No control covers another's, up to the pad's own size and on any screen's shape; the shoulders sit
    // below the top edge, where the game draws its health and bolts.
    for (const float aspect : {4.0f / 3.0f, 16.0f / 9.0f, 2.0f, 2.4f}) {
        for (const float scale : {TouchPad::kMinScale, TouchPad::kDefaultScale, 1.0f}) {
            TouchPad shape;
            shape.set_aspect(aspect);
            shape.set_scale(scale);
            shape.set_menu_button(true);
            const auto all = shape.controls();
            for (std::size_t i = 0; i < all.size(); ++i) {
                for (std::size_t j = i + 1; j < all.size(); ++j) {
                    const TouchControl& a = all[i];
                    const TouchControl& b = all[j];
                    if (a.group == b.group) {
                        continue;  // a group's own controls are laid out together
                    }
                    const float dx = std::abs(a.x - b.x) * aspect;
                    const float dy = std::abs(a.y - b.y);
                    const bool round = a.stretch == 1.0f && b.stretch == 1.0f;
                    const bool apart = round ? std::hypot(dx, dy) >= a.radius + b.radius
                                             : dx >= a.radius * a.stretch + b.radius * b.stretch
                                                   || dy >= a.radius + b.radius;
                    CHECK(apart);
                }
            }
            const TouchControl& l1 = control(shape, TouchControl::Kind::Button, Button::L1);
            CHECK(l1.y - l1.radius > 0.08f * scale);
        }
    }

    // Smaller controls sit nearer the edges; the default is smaller than the pad's own.
    TouchPad sized;
    sized.set_aspect(16.0f / 9.0f);
    CHECK(sized.scale() == TouchPad::kDefaultScale && TouchPad::kDefaultScale < 1.0f);
    const TouchControl& face = control(sized, TouchControl::Kind::Button, Button::Cross);
    const float small_radius = face.radius;
    const float small_y = face.y;
    sized.set_scale(1.0f);
    CHECK(face.radius > small_radius);
    CHECK(face.y < small_y);  // further up from the bottom edge
    sized.set_scale(9.0f);
    CHECK(sized.scale() == TouchPad::kMaxScale);
    sized.set_scale(0.0f);
    CHECK(sized.scale() == TouchPad::kMinScale);

    // Editing: a finger drags the group it lands on, the game gets nothing, and the layout is a
    // line of text that brings it back.
    TouchPad edit;
    edit.set_aspect(2.0f);
    const TouchControl& up = control(edit, TouchControl::Kind::Button, Button::Up);
    const TouchControl& down = control(edit, TouchControl::Kind::Button, Button::Down);
    const float up_x = up.x;
    const float up_y = up.y;
    const float gap_y = down.y - up.y;
    edit.set_editing(true);
    CHECK(edit.visible());
    // It stays shown while the player arranges it (another device used, the screen left alone),
    // and a finger on the empty right half turns no camera.
    edit.hide();
    edit.idle(1000);
    edit.idle(1000 + 2 * TouchPad::kIdleHideMs);
    CHECK(edit.visible());
    edit.finger_down(2, 0.6f, 0.45f);
    edit.finger_move(2, 0.7f, 0.5f);
    edit.idle(1000 + 2 * TouchPad::kIdleHideMs + 16);
    float look_x = 1.0f;
    float look_y = 1.0f;
    edit.look(look_x, look_y);
    CHECK(look_x == 0.0f && look_y == 0.0f);
    CHECK(read(edit).right_x == Pad{}.right_x);
    edit.finger_up(2);
    edit.finger_down(1, up.x, up.y);
    CHECK(read(edit).buttons == Pad{}.buttons);
    edit.finger_move(1, up.x + 0.1f, up.y + 0.05f);
    CHECK(std::abs(up.x - (up_x + 0.1f)) < 1e-5f && std::abs(up.y - (up_y + 0.05f)) < 1e-5f);
    CHECK(std::abs(down.y - up.y - gap_y) < 1e-5f);  // the whole d-pad moved
    CHECK(read(edit).buttons == Pad{}.buttons);
    // The group a finger took hold of has its own size, its arms kept in proportion.
    CHECK(edit.selected() == TouchGroup::Dpad);
    const float up_radius = up.radius;
    edit.step_selected_size(2);
    CHECK(std::abs(edit.selected_size() - 1.2f) < 1e-5f);
    CHECK(std::abs(up.radius - up_radius * 1.2f) < 1e-5f);
    CHECK(std::abs(down.y - up.y - gap_y * 1.2f) < 1e-5f);
    edit.step_selected_size(100);
    CHECK(edit.selected_size() == TouchPad::kMaxSize);
    edit.step_selected_size(-4);
    edit.finger_up(1);
    edit.set_editing(false);
    CHECK(edit.selected() == TouchGroup::Count);
    const std::string text = edit.layout();
    CHECK(text.starts_with("dpad 0.2000 0.0500"));
    TouchPad again;
    again.set_aspect(2.0f);
    again.set_layout(text);
    const TouchControl& up_again = control(again, TouchControl::Kind::Button, Button::Up);
    CHECK(std::abs(up_again.x - up.x) < 1e-4f && std::abs(up_again.y - up.y) < 1e-4f);
    CHECK(std::abs(up_again.radius - up.radius) < 1e-5f);
    CHECK(text.starts_with("dpad 0.2000 0.0500 1.20"));
    again.set_layout("dpad nonsense, face 0.1, nothing 1 2, stick 0.01 -0.02, face 0 0 9");
    CHECK(again.layout() == "stick 0.0100 -0.0200, face 0.0000 0.0000 1.60");
    again.reset_layout();
    CHECK(again.layout().empty());
    CHECK(std::abs(up_again.x - up_x) < 1e-5f);

    // Snapping: a dragged group near another's line, or its own place, lands on it, and the line
    // is there to show; a little further it goes where the finger takes it.
    TouchPad snapping;
    snapping.set_aspect(2.0f);
    snapping.set_editing(true);
    const TouchControl& left_stick = control(snapping, TouchControl::Kind::LeftStick);
    const TouchControl& dpad = control(snapping, TouchControl::Kind::Button, Button::Up);
    const float dpad_x = dpad.x;
    const float dpad_y = dpad.y;
    snapping.finger_down(1, dpad.x, dpad.y);
    snapping.finger_move(1, left_stick.x + 0.005f, dpad_y);
    CHECK(std::abs(dpad.x - left_stick.x) < 1e-5f);  // in line with the stick
    CHECK(std::abs(snapping.guide_x() - left_stick.x) < 1e-5f);
    snapping.finger_move(1, dpad_x + 0.006f, dpad_y + 0.01f);
    CHECK(std::abs(dpad.x - dpad_x) < 1e-5f && std::abs(dpad.y - dpad_y) < 1e-5f);  // back home
    snapping.finger_move(1, dpad_x - 0.04f, dpad_y);
    CHECK(std::abs(dpad.x - (dpad_x - 0.04f)) < 1e-5f && snapping.guide_x() < 0.0f);
    snapping.finger_up(1);
    CHECK(snapping.guide_x() < 0.0f && snapping.guide_y() < 0.0f);

    arc_style();
    action_sets();
    steer_and_dpad();
    stand_in();
    own_place();
    return openrac::test::result();
}

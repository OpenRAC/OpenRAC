// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu's Controls tab (menu_catalog.h's Controls entries): what each of the pad's controls is
// pressed from on a controller, changed by pressing the new button, and the sticks', the
// triggers' and the players' settings, as rows a controller, the keyboard or a pointer moves
// through. It edits the running game's controls file or the global one (controls_store.h); each
// change is written and applied at once, and a press held as it is bound waits for its release
// before it presses (platform/bindings.h's Latch).

#include <algorithm>
#include <cctype>
#include <format>
#include <imgui.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "controls_store.h"
#include "debug/menu_ui.h"
#include "frontend.h"
#include "platform/bindings.h"
#include "platform/input.h"
#include "platform/players.h"
#include "settings.h"

namespace openrac::frontend::debug_overlay {
namespace {

using controls::Scope;
using menu::Id;
using platform::Binding;
using platform::PadChord;

// A capture gives up after this long without a press, in seconds: a controller alone can always
// get out of it.
constexpr double kCaptureTimeout = 6.0;

// A controller's light while a button is pressed on it: the launcher's --ok.
constexpr ImU32 kLit = ui_colors::rgba(0x8f, 0xd6, 0x94);

// The controls' names as the rows show them, by settings key (platform::binding_keys()).
struct Label {
    const char* key;
    const char* text;
};

constexpr Label kTargetLabels[] = {
    {"bind_cross", "Cross"},
    {"bind_circle", "Circle"},
    {"bind_square", "Square"},
    {"bind_triangle", "Triangle"},
    {"bind_l1", "L1"},
    {"bind_r1", "R1"},
    {"bind_l2", "L2"},
    {"bind_r2", "R2"},
    {"bind_l3", "L3 (left stick click)"},
    {"bind_r3", "R3 (right stick click)"},
    {"bind_start", "Start"},
    {"bind_select", "Select"},
    {"bind_up", "D-pad up"},
    {"bind_down", "D-pad down"},
    {"bind_left", "D-pad left"},
    {"bind_right", "D-pad right"},
    {"bind_lstick_up", "Left stick up"},
    {"bind_lstick_down", "Left stick down"},
    {"bind_lstick_left", "Left stick left"},
    {"bind_lstick_right", "Left stick right"},
    {"bind_rstick_up", "Right stick up"},
    {"bind_rstick_down", "Right stick down"},
    {"bind_rstick_left", "Right stick left"},
    {"bind_rstick_right", "Right stick right"},
};

// The pad inputs' names: by position, with what the common pads print on them.
constexpr Label kInputLabels[] = {
    {"PadSouth", "South (A, Cross)"},
    {"PadEast", "East (B, Circle)"},
    {"PadWest", "West (X, Square)"},
    {"PadNorth", "North (Y, Triangle)"},
    {"PadBack", "Back / Select / Share"},
    {"PadGuide", "Guide / Home"},
    {"PadStart", "Start / Options"},
    {"PadLeftStick", "Left stick click"},
    {"PadRightStick", "Right stick click"},
    {"PadLeftShoulder", "Left shoulder"},
    {"PadRightShoulder", "Right shoulder"},
    {"PadDpadUp", "D-pad up"},
    {"PadDpadDown", "D-pad down"},
    {"PadDpadLeft", "D-pad left"},
    {"PadDpadRight", "D-pad right"},
    {"PadMisc1", "Misc (mic, capture, share)"},
    {"PadRightPaddle1", "Right paddle 1"},
    {"PadLeftPaddle1", "Left paddle 1"},
    {"PadRightPaddle2", "Right paddle 2"},
    {"PadLeftPaddle2", "Left paddle 2"},
    {"PadTouchpad", "Touchpad click"},
    {"PadLeftTrigger", "Left trigger"},
    {"PadRightTrigger", "Right trigger"},
    {"PadLeftStickUp", "Left stick up"},
    {"PadLeftStickDown", "Left stick down"},
    {"PadLeftStickLeft", "Left stick left"},
    {"PadLeftStickRight", "Left stick right"},
    {"PadRightStickUp", "Right stick up"},
    {"PadRightStickDown", "Right stick down"},
    {"PadRightStickLeft", "Right stick left"},
    {"PadRightStickRight", "Right stick right"},
};

std::string label_of(const char* key, std::span<const Label> table) {
    for (const Label& l : table) {
        if (std::string_view(key) == l.key) {
            return l.text;
        }
    }
    return key;
}

std::string target_label(int target) {
    return label_of(platform::key_of_target(target), kTargetLabels);
}

std::string input_label(const Binding& b) {
    return label_of(platform::binding_name(b), kInputLabels);
}

std::string chord_label(const PadChord& chord) {
    if (chord.with.kind == Binding::Kind::None) {
        return input_label(chord.input);
    }
    return input_label(chord.with) + " + " + input_label(chord.input);
}

Scope scope() {
    return s.controls.global ? Scope::Global : Scope::Game;
}

platform::Layer scope_layer() {
    if (s.controls.player >= 0) {
        return s.controls.global ? platform::Layer::GlobalPlayer : platform::Layer::GamePlayer;
    }
    return s.controls.global ? platform::Layer::Global : platform::Layer::Game;
}

// A key as the tab writes it: the player's own (p2.bind_cross) while one player is chosen.
std::string key_for(std::string_view key) {
    return s.controls.player >= 0 ? platform::player_prefix(s.controls.player) + std::string(key)
                                  : std::string(key);
}

// Whether the file the tab changes can be written now: not a newer program's, no press awaited.
bool writable(const controls::Store& store) {
    return !store.file(scope()).read_only() && s.controls.capture_target < 0;
}

// The pad inputs a target has now for the player chosen (or every player), wherever they came
// from.
std::vector<PadChord> pad_of(const controls::Store& store, int target) {
    const auto& slot = store.bindings(s.controls.player).slots[static_cast<std::size_t>(target)];
    return {slot.pad.begin(), slot.pad.begin() + slot.pad_count};
}

// Writes `pad` as the target's pad inputs in the tab's file, keeping the keys that file names for
// it (the keyboard is not changed here).
void write_pad(controls::Store& store, int target, const std::vector<PadChord>& pad) {
    const std::string key = key_for(platform::key_of_target(target));
    platform::Bindings::Slot named;
    named.key_count = -1;
    if (const auto value = store.file(scope()).get(key)) {
        platform::read_binding(key, *value, target, named);
    }
    platform::Bindings::Slot slot;
    slot.key_count = std::max(named.key_count, 0);
    slot.keys = named.keys;
    slot.pad_count = static_cast<int>(std::min<std::size_t>(pad.size(), platform::kMostOfAKind));
    std::copy_n(pad.begin(), slot.pad_count, slot.pad.begin());
    std::string value = platform::binding_value(slot);
    if (slot.pad_count == 0 && slot.key_count == 0) {
        value = platform::kPadNone;  // no pad input, the keys as they were
    }
    if (!store.set(scope(), key, value)) {
        s.controls.message = "Could not write " + store.path(scope()).string();
    }
}

// The other targets a lone pad input presses.
std::vector<int> also_bound(const controls::Store& store, int target, const Binding& input) {
    std::vector<int> out;
    for (int t = 0; t < platform::kTargetCount; ++t) {
        if (t == target) {
            continue;
        }
        for (const PadChord& chord : pad_of(store, t)) {
            if (chord.with.kind == Binding::Kind::None && chord.input == input) {
                out.push_back(t);
                break;
            }
        }
    }
    return out;
}

void start_capture(platform::Input& input, int target, ControlsMenu::Mode mode) {
    s.controls.capture_target = target;
    s.controls.mode = mode;
    s.controls.message.clear();
    s.controls.clash_target = -1;
    s.controls.capture_since = ImGui::GetTime();
    input.begin_capture();
}

// A press brought by the capture under way: a chord's first waits for its second; the last is
// written as the target's.
void captured(controls::Store& store, platform::Input& input, const Binding& got) {
    ControlsMenu& w = s.controls;
    if (w.mode == ControlsMenu::Mode::ChordFirst) {
        w.chord_with = got;
        w.mode = ControlsMenu::Mode::ChordSecond;
        w.capture_since = ImGui::GetTime();
        // The first is held while the second is pressed: only a new press counts.
        input.begin_capture();
        return;
    }
    input.end_capture();
    const int target = w.capture_target;
    const std::string what = target_label(target);
    w.capture_target = -1;
    PadChord chord{got, {}};
    if (w.mode == ControlsMenu::Mode::ChordSecond) {
        chord.with = w.chord_with;
    }
    std::vector<PadChord> pad;
    if (w.mode != ControlsMenu::Mode::Replace) {
        pad = pad_of(store, target);
    }
    if (std::find(pad.begin(), pad.end(), chord) == pad.end()) {
        if (pad.size() >= static_cast<std::size_t>(platform::kMostOfAKind)) {
            w.message =
                std::format("{} has {} already, the most it holds", what, platform::kMostOfAKind);
            return;
        }
        pad.push_back(chord);
    }
    write_pad(store, target, pad);
    w.message = std::format("{}: {}", what, chord_label(chord));
    // The press alone on another control too: both would press. Offered to be taken from it.
    if (chord.with.kind == Binding::Kind::None) {
        const std::vector<int> others = also_bound(store, target, chord.input);
        if (!others.empty()) {
            w.clash_from = target;
            w.clash_target = others.front();
            w.clash_input = chord.input;
        }
    }
}

// A line of the card in `ink`, not pressed.
void line(const std::string& text, ImU32 ink) {
    menu_note(text.c_str(), ink);
}

// A row with a value at its right in the accent's gold (or `ink`): true when pressed.
bool value_row(const std::string& label, const std::string& value, ImU32 ink = 0) {
    const ImU32 shown = ink != 0 ? ink : ui().gold;
    return menu_row(label.c_str(), ui().text, [&](ImDrawList* list, ImVec2 right) {
        const ImVec2 size = ImGui::CalcTextSize(value.c_str());
        list->AddText(ImVec2(right.x - size.x, right.y - size.y * 0.5f), shown, value.c_str());
    });
}

// A row of a control's own (change, add, remove): set in, dimmer than the control's.
bool sub_row(const std::string& label, ImU32 ink) {
    return menu_row(("      " + label).c_str(), ink);
}

// ---- Panels ----

// Under the control being changed: the press awaited (with Cancel), or a clash to settle.
void capture_rows(controls::Store& store, platform::Input& input) {
    ControlsMenu& w = s.controls;
    if (w.capture_target >= 0) {
        const std::string what = target_label(w.capture_target);
        const int left = std::max(
            0, static_cast<int>(kCaptureTimeout - (ImGui::GetTime() - w.capture_since) + 0.99)
        );
        line(
            w.mode == ControlsMenu::Mode::ChordFirst
                ? std::format("{}: hold the combo's first button", what)
            : w.mode == ControlsMenu::Mode::ChordSecond
                ? std::format("{}: keep {} held, press the second", what, input_label(w.chord_with))
                : std::format("{}: press a button on the controller", what),
            ui().amber
        );
        line(std::format("The controller is ignored until then. Stops in {} s.", left), ui().dim);
        if (menu_row("Cancel")) {
            input.end_capture();
            w.capture_target = -1;
        }
        note_hover(static_cast<int>(Id::ControlsStatus));
        return;
    }
    if (w.clash_target >= 0) {
        line(
            std::format(
                "{} also presses {}.", input_label(w.clash_input), target_label(w.clash_target)
            ),
            ui().amber
        );
        if (menu_row(std::format("Take it from {}", target_label(w.clash_target)).c_str())) {
            std::vector<PadChord> pad = pad_of(store, w.clash_target);
            std::erase(pad, PadChord{w.clash_input, {}});
            write_pad(store, w.clash_target, pad);
            w.clash_target = -1;
        }
        note_hover(static_cast<int>(Id::ControlsStatus));
        if (menu_row("Keep both")) {
            w.clash_target = -1;
        }
        note_hover(static_cast<int>(Id::ControlsStatus));
    }
}

// The tab's state at its top: a file it cannot write, or what the last change said.
void status_panel(controls::Store& store) {
    if (store.file(scope()).read_only()) {
        line(
            std::format(
                "{} was written by a newer OpenRAC: read, not changed.",
                store.path(scope()).filename().string()
            ),
            ui().err
        );
    }
    const std::string& said = s.controls.message;
    line(said.empty() ? "Changes are saved and used at once." : said, ui().soft);
}

// Each of the pad's controls with what presses it; one picked opens its own rows.
void bindings_panel(controls::Store& store, platform::Input& input) {
    ControlsMenu& w = s.controls;
    const bool can = writable(store);
    for (const char* const* key = platform::binding_keys(); *key != nullptr; ++key) {
        const int target = platform::target_of_key(*key);
        const std::vector<PadChord> pad = pad_of(store, target);
        const bool here =
            store.origins(w.player).pad[static_cast<std::size_t>(target)] == scope_layer();
        ImGui::PushID(target);
        const bool pressed =
            menu_row(target_label(target).c_str(), ui().text, [&](ImDrawList* list, ImVec2 right) {
                // Right to left: each input, a lone one another control has too marked in amber;
                // a dot when it is set in the file changed here.
                float x = right.x;
                if (pad.empty()) {
                    const ImVec2 size = ImGui::CalcTextSize("nothing");
                    list->AddText(ImVec2(x - size.x, right.y - size.y * 0.5f), ui().err, "nothing");
                    x -= size.x;
                }
                for (std::size_t i = pad.size(); i-- > 0;) {
                    const bool shared = pad[i].with.kind == Binding::Kind::None
                                        && !also_bound(store, target, pad[i].input).empty();
                    const std::string text =
                        chord_label(pad[i]) + (i + 1 < pad.size() ? "  /  " : "");
                    const ImVec2 size = ImGui::CalcTextSize(text.c_str());
                    list->AddText(
                        ImVec2(x - size.x, right.y - size.y * 0.5f),
                        shared ? ui().amber : ui().gold,
                        text.c_str()
                    );
                    x -= size.x;
                }
                if (here) {
                    list->AddCircleFilled(ImVec2(x - dp(10.0f), right.y), dp(3.0f), ui().brand);
                }
            });
        note_hover(static_cast<int>(Id::ControllerBindings));
        if (pressed) {
            w.open_target = w.open_target == target ? -1 : target;
        }
        // The press awaited for this control, or the clash it made, in place of its own rows.
        const int asking = w.capture_target >= 0 ? w.capture_target
                           : w.clash_target >= 0 ? w.clash_from
                                                 : -1;
        if (asking == target) {
            capture_rows(store, input);
        } else if (w.open_target == target) {
            const auto act = [&](const char* label, ControlsMenu::Mode mode) {
                if (sub_row(label, ui().soft) && can) {
                    start_capture(input, target, mode);
                }
                note_hover(static_cast<int>(Id::ControllerBindings));
            };
            act("Replace", ControlsMenu::Mode::Replace);
            act("Add a button", ControlsMenu::Mode::Add);
            act("Add a 2-button combo", ControlsMenu::Mode::ChordFirst);
            for (std::size_t i = 0; i < pad.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                if (sub_row("Remove " + chord_label(pad[i]), ui().soft) && can) {
                    std::vector<PadChord> rest = pad;
                    rest.erase(rest.begin() + static_cast<std::ptrdiff_t>(i));
                    write_pad(store, target, rest);
                }
                note_hover(static_cast<int>(Id::ControllerBindings));
                ImGui::PopID();
            }
            if (store.file(scope()).get(key_for(*key))) {
                if (sub_row("Reset to default", ui().soft) && can) {
                    store.erase(scope(), key_for(*key));
                }
                note_hover(static_cast<int>(Id::ControllerBindings));
            }
        }
        ImGui::PopID();
    }
}

// A whole number of the controller's settings, moved in steps of `step` within [low, high].
void tuning_choice(
    controls::Store& store,
    const char* label,
    const std::string& key,
    int value,
    int low,
    int high,
    int step,
    Id id
) {
    const int by = menu_choice(label, std::format("{}%", value), value > low, value < high);
    note_hover(static_cast<int>(id));
    if (by != 0 && writable(store)) {
        store.set(scope(), key, std::to_string(std::clamp(value + by * step, low, high)));
    }
}

// The square a stick moves in, with the stick as the controller gives it (grey) and as the game
// reads it after the tuning and the shaping (green), as a row of the card.
void stick_picture(platform::Input& input, int stick, const platform::Bindings& bindings) {
    const auto& devices = input.devices();
    const float size = dp(96.0f);
    // Without a controller, a row's height saying so.
    const float height = devices.empty() ? dp(s.menu_row_height) : size + dp(20.0f);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (s.card.rows++ > 0) {
        draw->AddLine(
            ImVec2(at.x + dp(16.0f), at.y), ImVec2(at.x + width - dp(16.0f), at.y), ui().line
        );
    }
    ImGui::Dummy(ImVec2(width, height));
    // The chosen player's controller (the first player's for every player), else the first.
    if (devices.empty()) {
        draw->AddText(
            ImVec2(at.x + dp(16.0f), at.y + height * 0.5f - ImGui::GetFontSize() * 0.5f),
            ui().dim,
            "No controller connected"
        );
        return;
    }
    std::size_t shown = 0;
    for (std::size_t d = 0; d < devices.size(); ++d) {
        if (devices[d].player == std::max(s.controls.player, 0)) {
            shown = d;
        }
    }
    const platform::PadInputs live = input.read_device(shown);
    draw->AddText(
        ImVec2(at.x + dp(16.0f), at.y + height * 0.5f - ImGui::GetFontSize() * 0.5f),
        ui().soft,
        devices[shown].name.c_str()
    );
    const ImVec2 a(at.x + width - dp(16.0f) - size, at.y + (height - size) * 0.5f);
    const ImVec2 mid(a.x + size * 0.5f, a.y + size * 0.5f);
    draw->AddRect(a, ImVec2(a.x + size, a.y + size), ui().line_strong, dp(6.0f));
    draw->AddCircle(mid, size * 0.5f, ui().line, 48);
    const auto axis = [&](int k) {
        return static_cast<float>(live.axes[static_cast<std::size_t>(2 * stick + k)]) / 32768.0f;
    };
    float x = axis(0);
    float y = axis(1);
    const auto spot = [&](float px, float py) {
        return ImVec2(mid.x + px * size * 0.5f, mid.y + py * size * 0.5f);
    };
    draw->AddCircleFilled(spot(x, y), dp(4.0f), ui().dim);
    platform::tune_stick(x, y, bindings.tuning.sticks[static_cast<std::size_t>(stick)]);
    platform::StickGate gate;
    platform::shape_stick(x, y, gate);
    draw->AddCircleFilled(spot(x, y), dp(4.0f), kLit);
}

// A stick's dead zone, outer edge, sensitivity and inversion, drawn as it reads now.
void stick_panel(controls::Store& store, platform::Input& input, int stick) {
    const Id id = stick == 0 ? Id::LeftStick : Id::RightStick;
    const char* side = stick == 0 ? "left" : "right";
    const platform::StickTuning& t =
        store.bindings(s.controls.player).tuning.sticks[static_cast<std::size_t>(stick)];
    ImGui::PushID(stick);
    tuning_choice(
        store, "Dead zone", key_for(std::format("pad_{}_deadzone", side)), t.deadzone, 0, 50, 1, id
    );
    tuning_choice(
        store, "Outer edge", key_for(std::format("pad_{}_outer", side)), t.outer, 50, 100, 1, id
    );
    tuning_choice(
        store,
        "Sensitivity",
        key_for(std::format("pad_{}_sensitivity", side)),
        t.sensitivity,
        25,
        300,
        5,
        id
    );
    const auto invert = [&](const char* label, const char* name, bool on) {
        if (menu_switch(label, on) && writable(store)) {
            store.set(scope(), key_for(std::format("pad_{}_{}", side, name)), on ? "0" : "1");
        }
        note_hover(static_cast<int>(id));
    };
    invert("Invert horizontal", "invert_x", t.invert_x);
    invert("Invert vertical", "invert_y", t.invert_y);
    stick_picture(input, stick, store.bindings(s.controls.player));
    if (menu_row("Reset this stick") && writable(store)) {
        for (const char* name : {"deadzone", "outer", "sensitivity", "invert_x", "invert_y"}) {
            store.erase(scope(), key_for(std::format("pad_{}_{}", side, name)));
        }
    }
    note_hover(static_cast<int>(id));
    ImGui::PopID();
}

// The controllers connected, each lit while a button is pressed on it, and the player it is.
void connected_panel(platform::Input& input) {
    const auto& devices = input.devices();
    if (devices.empty()) {
        line("None. Connect one and it shows here.", ui().dim);
        return;
    }
    for (std::size_t d = 0; d < devices.size(); ++d) {
        const platform::PadInputs now = input.read_device(d);
        bool active = std::any_of(now.buttons.begin(), now.buttons.end(), [](bool b) { return b; });
        for (const std::int16_t a : now.axes) {
            active = active || a > 16000 || a < -16000;
        }
        const std::string who = devices[d].player >= 0
                                    ? std::format("Player {}", devices[d].player + 1)
                                    : std::string("No player");
        ImGui::PushID(static_cast<int>(d));
        menu_row(devices[d].name.c_str(), ui().text, [&](ImDrawList* list, ImVec2 right) {
            const ImVec2 size = ImGui::CalcTextSize(who.c_str());
            list->AddText(
                ImVec2(right.x - size.x, right.y - size.y * 0.5f), ui().gold, who.c_str()
            );
            // A light that shows a press: which controller is which.
            list->AddCircleFilled(
                ImVec2(right.x - size.x - dp(14.0f), right.y),
                dp(4.0f),
                active ? kLit : ui_colors::fade(ui().dim, 0.5f)
            );
        });
        note_hover(static_cast<int>(Id::ConnectedControllers));
        ImGui::PopID();
    }
}

// A player's controller and keyboard (written to the global file, whichever the tab changes), and
// what the game reads from them now.
void player_panel(controls::Store& store, platform::Input& input, int p) {
    const Id id = p == 0 ? Id::PlayerOne : Id::PlayerTwo;
    const std::vector<std::string> wants = store.players().wants[static_cast<std::size_t>(p)];
    const auto has = [&wants](std::string_view name) {
        return std::find(wants.begin(), wants.end(), name) != wants.end();
    };
    // The keyboard: 0 not this player's, 1 while they have no controller, 2 always.
    int keyboard = has(platform::kKeyboardFallback) ? 1 : 0;
    if (has(platform::kKeyboard)) {
        keyboard = 2;
    }
    // The controller chosen: the first name that is not the keyboard.
    std::string chosen = "none";
    for (const std::string& name : wants) {
        if (name != platform::kKeyboard && name != platform::kKeyboardFallback) {
            chosen = name;
            break;
        }
    }
    std::vector<std::pair<std::string, std::string>> choices = {
        {"auto", "Next free controller"},
        {"none", "No controller"},
    };
    for (const platform::Device& device : input.devices()) {
        choices.emplace_back(device.id, device.name);
    }
    if (std::none_of(choices.begin(), choices.end(), [&](const auto& c) {
            return c.first == chosen;
        })) {
        choices.emplace_back(chosen, "A controller not connected now");
    }
    int at = 0;
    for (std::size_t i = 0; i < choices.size(); ++i) {
        if (choices[i].first == chosen) {
            at = static_cast<int>(i);
        }
    }
    const bool can = !store.file(Scope::Global).read_only();
    const auto write = [&](int with_keyboard, const std::string& controller) {
        std::vector<std::string> names;
        if (controller != "none") {
            names.push_back(controller);
        }
        if (with_keyboard > 0) {
            names.emplace_back(
                with_keyboard == 2 ? platform::kKeyboard : platform::kKeyboardFallback
            );
        }
        store.set(Scope::Global, platform::player_key(p), platform::player_value(names));
    };
    ImGui::PushID(p);
    const int count = static_cast<int>(choices.size());
    if (const int by = menu_choice(
            "Controller", choices[static_cast<std::size_t>(at)].second, count > 1, count > 1
        );
        by != 0 && can) {
        write(keyboard, choices[static_cast<std::size_t>((at + by + count) % count)].first);
    }
    note_hover(static_cast<int>(id));
    static constexpr const char* kKeyboardNames[] = {"No", "If no pad", "Always"};
    if (const int by =
            menu_choice("Keyboard", kKeyboardNames[keyboard], keyboard > 0, keyboard < 2);
        by != 0 && can) {
        write(std::clamp(keyboard + by, 0, 2), chosen);
    }
    note_hover(static_cast<int>(id));
    ImGui::PopID();
}

// What opens and closes the menu: a key and a controller button (settings.ini's menu_bar_key and
// menu_bar_button, read in debug_overlay.cpp).
void menu_open_panel() {
    const Id id = Id::MenuOpen;
    if (value_row(
            "Key",
            s.menu_key_capture ? std::format("press a key (Esc keeps {})", menu_bar_key_label())
                               : menu_bar_key_label(),
            s.menu_key_capture ? ui().amber : 0
        )) {
        s.menu_key_capture = true;
    }
    note_hover(static_cast<int>(id));
    if (settings::get().menu_bar_key != settings::Settings{}.menu_bar_key) {
        if (sub_row("Key back to the default", ui().soft)) {
            settings::get().menu_bar_key = settings::Settings{}.menu_bar_key;
            settings::save();
        }
        note_hover(static_cast<int>(id));
    }
    if (value_row(
            "Controller button",
            s.menu_button_capture
                ? std::format("press a button (Esc keeps {})", menu_bar_button_label())
                : menu_bar_button_label(),
            s.menu_button_capture ? ui().amber : 0
        )) {
        s.menu_button_capture = true;
    }
    note_hover(static_cast<int>(id));
    if (menu_bar_chord() || menu_bar_button() != SDL_GAMEPAD_BUTTON_INVALID) {
        if (sub_row("No button", ui().soft)) {
            settings::get().menu_bar_button = "none";
            settings::save();
        }
        note_hover(static_cast<int>(id));
    }
    if (settings::get().menu_bar_button != settings::Settings{}.menu_bar_button) {
        if (sub_row("Button back to the default", ui().soft)) {
            settings::get().menu_bar_button = settings::Settings{}.menu_bar_button;
            settings::save();
        }
        note_hover(static_cast<int>(id));
    }
}

}  // namespace

ControlsRow controls_row(Id id) {
    ControlsRow r;
    controls::Store* store = controls_store();
    platform::Input* input = frontend::input();
    if (store == nullptr || input == nullptr) {
        r.shown = false;
        return r;
    }
    const ControlsMenu& w = s.controls;
    const platform::PadTuning& tuning = store->bindings(w.player).tuning;
    switch (id) {
        case Id::ControlsFile:
            r.value =
                w.global ? std::string("Every game") : std::format("This game ({})", store->game());
            break;
        case Id::ControlsPlayer:
            r.value =
                w.player < 0 ? std::string("Every player") : std::format("Player {}", w.player + 1);
            break;
        case Id::ControlsPlayerReset:
            r.shown = w.player >= 0;
            r.label = std::format("Player {} as every player", w.player + 1);
            break;
        case Id::TriggerPoint: {
            const int percent = (static_cast<int>(tuning.trigger_threshold) * 100 + 127) / 255;
            r.value = std::format("{}%", percent);
            r.can_lower = percent > 1;
            r.can_raise = percent < 100;
            break;
        }
        case Id::FaceButtons:
            r.value = tuning.face_labels ? "By label (Nintendo pads)" : "By position";
            break;
        case Id::PadBackground:
            r.on = tuning.background;
            break;
        case Id::RumbleStrength:
            r.value = tuning.rumble.strength == 0 ? std::string("Off")
                                                  : std::format("{}%", tuning.rumble.strength);
            r.can_lower = tuning.rumble.strength > 0;
            r.can_raise = tuning.rumble.strength < 200;
            break;
        case Id::RumbleStyle:
            r.value =
                tuning.rumble.style == platform::RumbleStyle::Classic ? "Classic" : "Enhanced";
            break;
        case Id::RumbleTriggers:
            r.on = tuning.rumble.triggers;
            break;
        case Id::RumblePhone:
            r.on = tuning.rumble.phone;
            break;
        case Id::ControlsReset:
            r.label = w.confirm_reset ? std::string("Press again to reset")
                      : w.global      ? std::string("Reset every game's controls")
                                      : std::string("Reset this game's controls");
            break;
        default:
            break;
    }
    return r;
}

void controls_act(Id id, int by) {
    controls::Store* store = controls_store();
    platform::Input* input = frontend::input();
    if (store == nullptr || input == nullptr) {
        return;
    }
    ControlsMenu& w = s.controls;
    const platform::PadTuning& tuning = store->bindings(w.player).tuning;
    switch (id) {
        case Id::ControlsFile:
            w.global = !w.global;
            w.confirm_reset = false;
            break;
        case Id::ControlsPlayer:
            w.player = (w.player + 1 + by + platform::kPlayers + 1) % (platform::kPlayers + 1) - 1;
            w.open_target = -1;
            break;
        case Id::ControlsPlayerReset:
            if (writable(*store) && w.player >= 0) {
                store->reset_player(scope(), w.player);
                w.message =
                    std::format("Player {} reads every player's controls again", w.player + 1);
            }
            break;
        case Id::TriggerPoint:
            if (writable(*store)) {
                const int percent = (static_cast<int>(tuning.trigger_threshold) * 100 + 127) / 255;
                store->set(
                    scope(),
                    key_for("pad_trigger_threshold"),
                    std::to_string(std::clamp(percent + by, 1, 100))
                );
            }
            break;
        case Id::FaceButtons:
            if (writable(*store)) {
                store->set(
                    scope(), key_for("pad_face_buttons"), tuning.face_labels ? "position" : "labels"
                );
            }
            break;
        case Id::PadBackground:
            if (writable(*store)) {
                store->set(scope(), key_for("pad_background"), tuning.background ? "0" : "1");
            }
            break;
        case Id::RumbleStrength:
            if (writable(*store)) {
                // In tens: off, 10% .. 200%.
                const int strength = std::clamp(tuning.rumble.strength + 10 * by, 0, 200);
                store->set(scope(), key_for("pad_rumble"), std::to_string(strength));
            }
            break;
        case Id::RumbleStyle:
            if (writable(*store)) {
                store->set(
                    scope(),
                    key_for("pad_rumble_style"),
                    tuning.rumble.style == platform::RumbleStyle::Classic ? "enhanced" : "classic"
                );
            }
            break;
        case Id::RumbleTriggers:
            if (writable(*store)) {
                store->set(
                    scope(), key_for("pad_rumble_triggers"), tuning.rumble.triggers ? "0" : "1"
                );
            }
            break;
        case Id::RumblePhone:
            if (writable(*store)) {
                store->set(scope(), key_for("pad_rumble_phone"), tuning.rumble.phone ? "0" : "1");
            }
            break;
        case Id::PlayersReset:
            if (!store->file(Scope::Global).read_only()) {
                store->erase(Scope::Global, platform::player_key(0));
                store->erase(Scope::Global, platform::player_key(1));
            }
            break;
        case Id::ControlsReset:
            if (!writable(*store)) {
                break;
            }
            if (w.confirm_reset) {
                store->reset(scope());
                w.message = w.global ? "Every game's controls are the defaults again"
                                     : "This game's controls are those of every game again";
            }
            w.confirm_reset = !w.confirm_reset;
            break;
        default:
            break;
    }
}

void draw_controls_panel(Id id) {
    controls::Store* store = controls_store();
    platform::Input* input = frontend::input();
    if (store == nullptr || input == nullptr) {
        return;
    }
    switch (id) {
        case Id::ControlsStatus:
            status_panel(*store);
            break;
        case Id::ControllerBindings:
            bindings_panel(*store, *input);
            break;
        case Id::LeftStick:
            stick_panel(*store, *input, 0);
            break;
        case Id::RightStick:
            stick_panel(*store, *input, 1);
            break;
        case Id::ConnectedControllers:
            connected_panel(*input);
            break;
        case Id::PlayerOne:
            player_panel(*store, *input, 0);
            break;
        case Id::PlayerTwo:
            player_panel(*store, *input, 1);
            break;
        case Id::MenuOpen:
            menu_open_panel();
            break;
        default:
            break;
    }
}

void controls_frame() {
    platform::Input* input = frontend::input();
    controls::Store* store = controls_store();
    ControlsMenu& w = s.controls;
    if (input == nullptr || store == nullptr) {
        return;
    }
    // The menu gone, a capture under way ends: the pads are the game's again.
    if (!s.menu_open) {
        if (w.capture_target >= 0 || input->capturing()) {
            input->end_capture();
            w.capture_target = -1;
        }
        w.confirm_reset = false;
        s.menu_key_capture = false;
        s.menu_button_capture = false;
        return;
    }
    if (w.capture_target < 0) {
        return;
    }
    input->poll_capture();
    if (const std::optional<Binding> got = input->take_captured()) {
        captured(*store, *input, *got);
    } else if (ImGui::GetTime() - w.capture_since > kCaptureTimeout) {
        controls_cancel_capture();
        w.message = "No press came: nothing was changed";
    }
}

bool controls_capturing() {
    return s.controls.capture_target >= 0;
}

void controls_cancel_capture() {
    if (platform::Input* input = frontend::input()) {
        input->end_capture();
    }
    s.controls.capture_target = -1;
}

}  // namespace openrac::frontend::debug_overlay

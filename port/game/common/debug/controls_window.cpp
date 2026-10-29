// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The controls window (debug/overlay.h's controls_window()): what each of the pad's controls is
// pressed from on a controller, changed by pressing the new button, and the sticks', the
// triggers' and the players' settings. It edits the running game's controls file or the global
// one (controls_store.h); each change is written and applied at once, and a press held as it is
// bound waits for its release before it presses (platform/bindings.h's Latch).

#include <algorithm>
#include <cctype>
#include <format>
#include <imgui.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "controls_store.h"
#include "debug/overlay.h"
#include "frontend.h"
#include "platform/bindings.h"
#include "platform/input.h"
#include "platform/players.h"

namespace openrac::frontend::debug_overlay {
namespace {

using controls::Scope;
using platform::Binding;
using platform::PadChord;

// The controls' names as the window shows them, by settings key (platform::binding_keys()).
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

// The pad inputs' names as the window shows them: by position, with what the common pads print
// on them.
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

const char* layer_name(platform::Layer layer) {
    switch (layer) {
        case platform::Layer::Default:
            return "default";
        case platform::Layer::Global:
            return "all games";
        case platform::Layer::GlobalPlayer:
            return "all games, player";
        case platform::Layer::Game:
            return "this game";
        case platform::Layer::GamePlayer:
            return "this game, player";
    }
    return "";
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

// A key as the window writes it: the player's own (p2.bind_cross) while one player is chosen.
std::string key_for(std::string_view key) {
    return s.controls.player >= 0 ? platform::player_prefix(s.controls.player) + std::string(key)
                                  : std::string(key);
}

// The pad inputs a target has now for the player chosen (or every player), wherever they came
// from.
std::vector<PadChord> pad_of(const controls::Store& store, int target) {
    const auto& slot = store.bindings(s.controls.player).slots[static_cast<std::size_t>(target)];
    return {slot.pad.begin(), slot.pad.begin() + slot.pad_count};
}

// Writes `pad` as the target's pad inputs in the window's file, keeping the keys that file names
// for it (the keyboard is not changed here).
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

// The other targets a lone pad input presses, by their labels.
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

bool matches(const std::string& text, const char* filter) {
    if (filter[0] == '\0') {
        return true;
    }
    std::string a = text;
    std::string b = filter;
    const auto lower = [](std::string& v) {
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    };
    lower(a);
    lower(b);
    return a.find(b) != std::string::npos;
}

void start_capture(platform::Input& input, int target, ControlsWindow::Mode mode) {
    s.controls.capture_target = target;
    s.controls.mode = mode;
    s.controls.message.clear();
    s.controls.clash_target = -1;
    input.begin_capture();
}

// A capture under way: what it waits for, and what the press it brings does.
void capture_step(controls::Store& store, platform::Input& input) {
    ControlsWindow& w = s.controls;
    const std::string what = target_label(w.capture_target);
    ImGui::TextColored(
        kYellow,
        "%s",
        (w.mode == ControlsWindow::Mode::ChordFirst
             ? std::format("{}: hold the chord's first button on a controller...", what)
         : w.mode == ControlsWindow::Mode::ChordSecond
             ? std::format(
                   "{}: {} held; now press the chord's second...", what, input_label(w.chord_with)
               )
             : std::format("{}: press a button on a controller...", what))
            .c_str()
    );
    ImGui::TextDisabled("The game sees no controller until this is done. Esc cancels.");
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        input.end_capture();
        w.capture_target = -1;
        return;
    }
    const std::optional<Binding> got = input.take_captured();
    if (!got) {
        return;
    }
    if (w.mode == ControlsWindow::Mode::ChordFirst) {
        w.chord_with = *got;
        w.mode = ControlsWindow::Mode::ChordSecond;
        // The first is held while the second is pressed: only a new press counts.
        input.begin_capture();
        return;
    }
    input.end_capture();
    const int target = w.capture_target;
    w.capture_target = -1;
    PadChord chord{*got, {}};
    if (w.mode == ControlsWindow::Mode::ChordSecond) {
        chord.with = w.chord_with;
    }
    std::vector<PadChord> pad;
    if (w.mode != ControlsWindow::Mode::Replace) {
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
            w.clash_target = others.front();
            w.clash_input = chord.input;
        }
    }
}

// The list of the pad's controls, each with what presses it on a controller.
void bindings_tab(controls::Store& store, platform::Input& input) {
    ControlsWindow& w = s.controls;
    ImGui::SetNextItemWidth(dp(220));
    ImGui::InputTextWithHint("##filter", "Filter controls or buttons", w.filter, sizeof w.filter);
    if (w.clash_target >= 0) {
        ImGui::TextColored(
            kYellow,
            "%s also presses %s.",
            input_label(w.clash_input).c_str(),
            target_label(w.clash_target).c_str()
        );
        ImGui::SameLine();
        if (ImGui::SmallButton("Take it from there")) {
            std::vector<PadChord> pad = pad_of(store, w.clash_target);
            std::erase(pad, PadChord{w.clash_input, {}});
            write_pad(store, w.clash_target, pad);
            w.clash_target = -1;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Keep both")) {
            w.clash_target = -1;
        }
    }
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH
                                  | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable("bindings", 4, flags, ImVec2(0.0f, dp(380)))) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed, dp(130));
    ImGui::TableSetupColumn("Controller");
    ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthFixed, dp(75));
    ImGui::TableSetupColumn("##do", ImGuiTableColumnFlags_WidthFixed, dp(215));
    ImGui::TableHeadersRow();
    const bool locked = store.file(scope()).read_only() || w.capture_target >= 0;
    for (const char* const* key = platform::binding_keys(); *key != nullptr; ++key) {
        const int target = platform::target_of_key(*key);
        const std::vector<PadChord> pad = pad_of(store, target);
        std::string names = target_label(target);
        for (const PadChord& chord : pad) {
            names += " " + chord_label(chord);
        }
        if (!matches(names, w.filter)) {
            continue;
        }
        ImGui::PushID(target);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(target_label(target).c_str());

        ImGui::TableNextColumn();
        if (pad.empty()) {
            ImGui::TextColored(kRed, "nothing");
        }
        for (std::size_t i = 0; i < pad.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            if (i > 0) {
                ImGui::SameLine();
            }
            // A lone input another control has too is marked: both press together.
            const bool shared = pad[i].with.kind == Binding::Kind::None
                                && !also_bound(store, target, pad[i].input).empty();
            if (shared) {
                ImGui::TextColored(kYellow, "%s*", chord_label(pad[i]).c_str());
                if (ImGui::IsItemHovered()) {
                    std::string tip = "Also presses:";
                    for (const int other : also_bound(store, target, pad[i].input)) {
                        tip += " " + target_label(other);
                    }
                    ImGui::SetTooltip("%s", tip.c_str());
                }
            } else {
                ImGui::TextUnformatted(chord_label(pad[i]).c_str());
            }
            ImGui::SameLine(0.0f, dp(2));
            ImGui::BeginDisabled(locked);
            if (ImGui::SmallButton("x")) {
                std::vector<PadChord> rest = pad;
                rest.erase(rest.begin() + static_cast<std::ptrdiff_t>(i));
                write_pad(store, target, rest);
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        ImGui::TableNextColumn();
        const platform::Layer from = store.origins(w.player).pad[static_cast<std::size_t>(target)];
        ImGui::TextColored(from == scope_layer() ? kGreen : kDim, "%s", layer_name(from));

        ImGui::TableNextColumn();
        ImGui::BeginDisabled(locked);
        if (ImGui::SmallButton("Change")) {
            start_capture(input, target, ControlsWindow::Mode::Replace);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Add")) {
            start_capture(input, target, ControlsWindow::Mode::Add);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Chord")) {
            start_capture(input, target, ControlsWindow::Mode::ChordFirst);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!store.file(scope()).get(key_for(*key)));
        if (ImGui::SmallButton("Reset")) {
            store.erase(scope(), key_for(*key));
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// One whole number of the controller's settings: a slider written once, when let go (the value
// dragged is kept meanwhile, not the file's).
void tuning_slider(
    controls::Store& store,
    const char* label,
    const std::string& key,
    int value,
    int low,
    int high,
    const char* format
) {
    ControlsWindow& w = s.controls;
    int shown = w.drag_key == key ? w.drag_value : value;
    ImGui::SetNextItemWidth(dp(200));
    if (ImGui::SliderInt(label, &shown, low, high, format, ImGuiSliderFlags_AlwaysClamp)) {
        w.drag_key = key;
        w.drag_value = shown;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        store.set(scope(), key, std::to_string(shown));
        w.drag_key.clear();
    }
}

// The square a stick moves in, with the stick as the controller gives it (grey) and as the game
// reads it after the tuning and the shaping (green).
void stick_picture(
    const platform::PadInputs& inputs, int stick, const platform::Bindings& bindings
) {
    const float size = dp(110);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRect(at, ImVec2(at.x + size, at.y + size), IM_COL32(150, 150, 150, 255));
    draw->AddCircle(
        ImVec2(at.x + size / 2, at.y + size / 2), size / 2, IM_COL32(90, 90, 90, 255), 48
    );
    const auto axis = [&](int a) {
        return static_cast<float>(inputs.axes[static_cast<std::size_t>(2 * stick + a)]) / 32768.0f;
    };
    float x = axis(0);
    float y = axis(1);
    const auto spot = [&](float px, float py) {
        return ImVec2(at.x + (px + 1.0f) * size / 2, at.y + (py + 1.0f) * size / 2);
    };
    draw->AddCircleFilled(spot(x, y), dp(4), IM_COL32(160, 160, 160, 255));
    platform::tune_stick(x, y, bindings.tuning.sticks[static_cast<std::size_t>(stick)]);
    platform::StickGate gate;
    platform::shape_stick(x, y, gate);
    draw->AddCircleFilled(spot(x, y), dp(4), IM_COL32(90, 220, 120, 255));
    ImGui::Dummy(ImVec2(size, size));
}

// The sticks' dead zones, sensitivity and inversion, the triggers' threshold, the face buttons'
// labels, and whether the controllers count in the background.
void tuning_tab(controls::Store& store, platform::Input& input) {
    const platform::PadTuning& tuning = store.bindings(s.controls.player).tuning;
    const bool locked = store.file(scope()).read_only();
    // The stick drawn is the chosen player's controller's (the first player's for every player),
    // else the first connected.
    std::size_t shown = 0;
    const int whose = std::max(s.controls.player, 0);
    for (std::size_t d = 0; d < input.devices().size(); ++d) {
        if (input.devices()[d].player == whose) {
            shown = d;
        }
    }
    const platform::PadInputs live = input.read_device(shown);
    ImGui::BeginDisabled(locked);
    for (int stick = 0; stick < 2; ++stick) {
        const char* side = stick == 0 ? "left" : "right";
        const platform::StickTuning& t = tuning.sticks[static_cast<std::size_t>(stick)];
        ImGui::PushID(stick);
        ImGui::SeparatorText(stick == 0 ? "Left stick" : "Right stick");
        ImGui::BeginGroup();
        tuning_slider(
            store,
            "Dead zone",
            key_for(std::format("pad_{}_deadzone", side)),
            t.deadzone,
            0,
            50,
            "%d%%"
        );
        tuning_slider(
            store,
            "Outer edge",
            key_for(std::format("pad_{}_outer", side)),
            t.outer,
            50,
            100,
            "%d%%"
        );
        tuning_slider(
            store,
            "Sensitivity",
            key_for(std::format("pad_{}_sensitivity", side)),
            t.sensitivity,
            25,
            300,
            "%d%%"
        );
        bool invert_x = t.invert_x;
        if (ImGui::Checkbox("Invert horizontal", &invert_x)) {
            store.set(scope(), key_for(std::format("pad_{}_invert_x", side)), invert_x ? "1" : "0");
        }
        bool invert_y = t.invert_y;
        if (ImGui::Checkbox("Invert vertical", &invert_y)) {
            store.set(scope(), key_for(std::format("pad_{}_invert_y", side)), invert_y ? "1" : "0");
        }
        if (ImGui::SmallButton("Reset this stick")) {
            for (const char* name : {"deadzone", "outer", "sensitivity", "invert_x", "invert_y"}) {
                store.erase(scope(), key_for(std::format("pad_{}_{}", side, name)));
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine(0.0f, dp(24));
        stick_picture(live, stick, store.bindings(s.controls.player));
        ImGui::PopID();
    }
    ImGui::SeparatorText("Triggers and buttons");
    const int percent = (static_cast<int>(tuning.trigger_threshold) * 100 + 127) / 255;
    tuning_slider(
        store, "Trigger press point", key_for("pad_trigger_threshold"), percent, 1, 100, "%d%%"
    );
    int labels = tuning.face_labels ? 1 : 0;
    ImGui::SetNextItemWidth(dp(200));
    if (ImGui::Combo("Face buttons", &labels, "By position\0By label (Nintendo pads)\0")) {
        store.set(scope(), key_for("pad_face_buttons"), labels == 1 ? "labels" : "position");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "By label: on a pad with A on the right (Nintendo's), the button marked A is South."
        );
    }
    bool background = tuning.background;
    if (ImGui::Checkbox("Controllers work while another window is in front", &background)) {
        store.set(scope(), key_for("pad_background"), background ? "1" : "0");
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("The game has its own dead zone: the port adds none unless set here.");
}

// Which controller each player is: written to the global file, whichever file the window edits.
void players_tab(controls::Store& store, platform::Input& input) {
    const auto& devices = input.devices();
    ImGui::SeparatorText("Controllers connected");
    if (devices.empty()) {
        ImGui::TextDisabled("None. Connect one and it shows here.");
    }
    for (std::size_t d = 0; d < devices.size(); ++d) {
        const platform::PadInputs now = input.read_device(d);
        bool active = std::any_of(now.buttons.begin(), now.buttons.end(), [](bool b) { return b; });
        for (const std::int16_t a : now.axes) {
            active = active || a > 16000 || a < -16000;
        }
        // A light that shows a press: which controller is which.
        ImGui::TextColored(active ? kGreen : kDim, active ? "(*)" : "( )");
        ImGui::SameLine();
        ImGui::Text(
            "%s  %s",
            devices[d].name.c_str(),
            devices[d].player >= 0 ? std::format("player {}", devices[d].player + 1).c_str()
                                   : "no player"
        );
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", devices[d].id.c_str());
        }
    }
    const bool locked = store.file(Scope::Global).read_only();
    ImGui::BeginDisabled(locked);
    for (int p = 0; p < platform::kPlayers; ++p) {
        ImGui::PushID(p);
        ImGui::SeparatorText(std::format("Player {}", p + 1).c_str());
        std::vector<std::string> wants = store.players().wants[static_cast<std::size_t>(p)];
        const bool keyboard = std::find(wants.begin(), wants.end(), "keyboard") != wants.end();
        // The controller chosen: the first name that is not the keyboard.
        std::string chosen = "none";
        for (const std::string& name : wants) {
            if (name != "keyboard") {
                chosen = name;
                break;
            }
        }
        std::vector<std::pair<std::string, std::string>> choices = {
            {"auto", "Next free controller"},
            {"none", "No controller"},
        };
        for (const platform::Device& device : devices) {
            choices.emplace_back(device.id, device.name);
        }
        if (std::none_of(choices.begin(), choices.end(), [&](const auto& c) {
                return c.first == chosen;
            })) {
            choices.emplace_back(chosen, "A controller not connected now");
        }
        std::string shown = chosen;
        for (const auto& [id, name] : choices) {
            if (id == chosen) {
                shown = name;
            }
        }
        const auto write = [&](bool with_keyboard, const std::string& controller) {
            std::vector<std::string> names;
            if (with_keyboard) {
                names.emplace_back("keyboard");
            }
            if (controller != "none") {
                names.push_back(controller);
            }
            store.set(Scope::Global, platform::player_key(p), platform::player_value(names));
        };
        ImGui::SetNextItemWidth(dp(260));
        if (ImGui::BeginCombo("Controller", shown.c_str())) {
            for (const auto& [id, name] : choices) {
                if (ImGui::Selectable(name.c_str(), id == chosen)) {
                    write(keyboard, id);
                }
            }
            ImGui::EndCombo();
        }
        bool with_keyboard = keyboard;
        if (ImGui::Checkbox("Keyboard", &with_keyboard)) {
            write(with_keyboard, chosen);
        }
        // What the game reads from this player now.
        const platform::Pad& pad = input.pad(p);
        std::string held;
        for (std::size_t bit = 0; bit < kButtonNames.size(); ++bit) {
            if ((pad.buttons & (1u << bit)) == 0) {
                held += std::string(held.empty() ? "" : " ") + kButtonNames[bit];
            }
        }
        ImGui::TextDisabled(
            "Reads: %s  sticks %02X %02X  %02X %02X",
            held.empty() ? "nothing held" : held.c_str(),
            pad.left_x,
            pad.left_y,
            pad.right_x,
            pad.right_y
        );
        ImGui::PopID();
    }
    if (ImGui::Button("Players back to the defaults")) {
        store.erase(Scope::Global, platform::player_key(0));
        store.erase(Scope::Global, platform::player_key(1));
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("Players are the same in every game.");
}

}  // namespace

void controls_window() {
    controls::Store* store = controls_store();
    platform::Input* input = frontend::input();
    // Shut (its close box, F13, a menu) while waiting for a press: the pads are the game's again.
    if (!s.controls_window) {
        if (input != nullptr && input->capturing()) {
            input->end_capture();
        }
        s.controls.capture_target = -1;
        s.controls.confirm_reset = false;
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(dp(720), dp(560)), ImGuiCond_FirstUseEver);
    const bool open = ImGui::Begin("Controller bindings", &s.controls_window);
    if (!open || store == nullptr || input == nullptr) {
        ImGui::End();
        return;
    }
    ControlsWindow& w = s.controls;

    ImGui::TextUnformatted("Change:");
    ImGui::SameLine();
    if (ImGui::RadioButton(std::format("This game ({})", store->game()).c_str(), !w.global)) {
        w.global = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Every game", w.global)) {
        w.global = true;
    }
    ImGui::TextDisabled(
        w.global ? "Every game reads these, unless its own controls change them."
                 : "Only this game: what is not changed here comes from Every game."
    );
    // Whose controls: every player's, or one player's own over them.
    ImGui::TextUnformatted("For:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Every player", w.player < 0)) {
        w.player = -1;
    }
    for (int p = 0; p < platform::kPlayers; ++p) {
        ImGui::SameLine();
        if (ImGui::RadioButton(std::format("Player {}", p + 1).c_str(), w.player == p)) {
            w.player = p;
        }
    }
    if (w.player >= 0) {
        ImGui::SameLine();
        ImGui::BeginDisabled(store->file(scope()).read_only() || w.capture_target >= 0);
        if (ImGui::SmallButton(std::format("Player {} as every player", w.player + 1).c_str())) {
            store->reset_player(scope(), w.player);
            w.message = std::format("Player {} reads every player's controls again", w.player + 1);
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled(
            "Player %d's own controls: what is not changed here is every player's.", w.player + 1
        );
    }
    if (store->file(scope()).read_only()) {
        ImGui::TextColored(
            kRed,
            "%s was written by a newer OpenRAC: read, not changed.",
            store->path(scope()).filename().string().c_str()
        );
    }

    if (w.capture_target >= 0) {
        capture_step(*store, *input);
    } else if (!w.message.empty()) {
        ImGui::TextColored(kGreen, "%s", w.message.c_str());
    } else {
        ImGui::TextDisabled("Changes are saved and used at once.");
    }

    if (ImGui::BeginTabBar("controls")) {
        if (ImGui::BeginTabItem("Bindings")) {
            bindings_tab(*store, *input);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sticks and triggers")) {
            tuning_tab(*store, *input);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Players")) {
            players_tab(*store, *input);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    const char* what = w.global ? "Reset every game's controls" : "Reset this game's controls";
    ImGui::BeginDisabled(store->file(scope()).read_only() || w.capture_target >= 0);
    if (ImGui::Button(w.confirm_reset ? "Press again to reset" : what)) {
        if (w.confirm_reset) {
            store->reset(scope());
            w.message = w.global ? "Every game's controls are the defaults again"
                                 : "This game's controls are those of every game again";
        }
        w.confirm_reset = !w.confirm_reset;
    }
    ImGui::EndDisabled();
    if (w.confirm_reset && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        w.confirm_reset = false;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", store->path(scope()).string().c_str());
    ImGui::End();
}

}  // namespace openrac::frontend::debug_overlay

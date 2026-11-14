// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The in-game menu's entries (game/common/menu_catalog.h): every option the old menus had is still
// offered where it was, and the list is whole (a tab, a section, a name and a line of help each).

#include <set>
#include <string>
#include <string_view>

#include "check.h"
#include "game/common/menu_catalog.h"

namespace {

using namespace openrac::frontend::menu;

// What the menus offered before they were one list: the desktop's menu bar (Windows and Linux,
// debug/menu_bar.cpp) and the touch screen's drawer (debug/drawer.cpp), each item by its old name
// and the entry that does it now. Where the new menu does it another way, the line says how.
struct Old {
    std::uint8_t where;
    const char* name;
    Id now;
};

constexpr Old kOld[] = {
    // The menu bar: Game
    {kDesktop, "Full screen", Id::FullScreen},
    {kDesktop, "Frame counter", Id::FrameCounter},
    // "Hide these menus": the menu key (` by default) now opens and closes the menu itself.
    {kDesktop, "Exit game", Id::Quit},
    // Controls
    {kDesktop, "Controller bindings...", Id::ControllerBindings},
    // Graphics
    {kDesktop, "Picture", Id::Picture},
    {kDesktop, "HUD", Id::Hud},
    {kDesktop, "Render scale", Id::RenderScale},
    {kDesktop, "Anti-aliasing", Id::Msaa},
    {kDesktop, "Anisotropic filtering", Id::Anisotropy},
    // "Scenery detail" is a step of Draw distance now (Original, Unlimited).
    {kDesktop, "Scenery detail", Id::DrawDistance},
    {kDesktop, "Frame rate", Id::FrameRate},
    {kDesktop, "VSync", Id::VSync},
    {kDesktop, "Enhanced Settings", Id::EnhancedAdvanced},
    // Sound: "Volume up" and "Volume down" (F10, F9, which still work) are the Volume choice.
    {kDesktop, "Volume up", Id::Volume},
    {kDesktop, "Volume down", Id::Volume},
    // Developer
    {kDesktop, "Freecam: Enable Freecam", Id::Freecam},
    {kDesktop, "Freecam: Mode", Id::FreecamMode},
    {kDesktop, "Freecam: Inspect", Id::FreecamInspect},
    {kDesktop, "Freecam: Fly speed", Id::FreecamSpeed},
    {kDesktop, "Freecam: Reset to game camera", Id::FreecamReset},
    {kDesktop, "Freecam: Exit Freecam", Id::FreecamExit},
    {kDesktop, "Freecam: keys", Id::FreecamHelp},
    {kDesktop, "Debug overlay", Id::DebugOverlay},
    {kDesktop, "Moby picker", Id::MobyPicker},
    {kDesktop, "Object Viewer", Id::ObjectViewer},
    // "Pause" (F5, which still works): the menu pauses the game; Keep paused keeps it so.
    {kDesktop, "Pause", Id::KeepPaused},
    {kDesktop, "Step one tick", Id::StepTick},
    // "Slower" and "Faster" (F7, F8, which still work) are the Speed choice.
    {kDesktop, "Slower", Id::Speed},
    {kDesktop, "Faster", Id::Speed},
    {kDesktop, "Screenshot", Id::Screenshot},
    {kDesktop, "Record diagnostics: Gameplay recording", Id::RecordGameplay},
    {kDesktop, "Record diagnostics: Recording length", Id::RecordLength},
    {kDesktop, "Record diagnostics: Graphics sweep", Id::RecordSweep},
    {kDesktop, "Record diagnostics: Open recordings folder", Id::RecordFolder},
    {kDesktop, "Simulate slow CPU", Id::SlowCpu},
    {kDesktop, "Level select", Id::LevelSelect},
    // The mods' own menus (mods::developer_menus) are tabs of their own, not entries.

    // The drawer: "Resume" is its header's button, as before.
    {kTouch, "Picture", Id::Picture},
    {kTouch, "HUD", Id::Hud},
    {kTouch, "Render scale", Id::RenderScale},
    {kTouch, "Anti-aliasing", Id::Msaa},
    {kTouch, "Anisotropic filtering", Id::Anisotropy},
    {kTouch, "Scenery detail", Id::DrawDistance},
    {kTouch, "Frame rate", Id::FrameRate},
    {kTouch, "VSync", Id::VSync},
    {kTouch, "HDR", Id::Hdr},
    {kTouch, "HDR paper white", Id::HdrPaperWhite},
    {kTouch, "HDR status", Id::HdrStatus},
    {kTouch, "Frame counter", Id::FrameCounter},
    {kTouch, "Low-latency frames", Id::LowLatency},
    {kTouch, "Responsive controls", Id::ResponsiveControls},
    {kTouch, "Enhanced graphics: why it is off", Id::EnhancedStatus},
    {kTouch, "Enhanced graphics", Id::EnhancedGraphics},
    {kTouch, "Shadow quality", Id::ShadowQuality},
    {kTouch, "Shadow filtering", Id::ShadowFilter},
    {kTouch, "Occlusion quality", Id::OcclusionQuality},
    {kTouch, "Hierarchical depth (Hi-Z)", Id::OcclusionHiZ},
    {kTouch, "Texture upscaling", Id::TextureUpscaling},
    {kTouch, "Effects detail", Id::EffectsDetail},
    {kTouch, "Upscaling", Id::Upscaling},
    {kTouch, "Sharpening", Id::Sharpening},
    {kTouch, "Motion blur", Id::MotionBlur},
    {kTouch, "Motion blur quality", Id::MotionBlurQuality},
    {kTouch, "Color correction", Id::ColorCorrection},
    {kTouch, "Brightness", Id::Brightness},
    {kTouch, "Reset to defaults", Id::EnhancedReset},
    {kTouch, "Volume", Id::Volume},
    {kTouch, "Skip cutscenes", Id::SkipCutscenes},
    {kTouch, "Controller bindings", Id::ControllerBindings},
    {kTouch, "Action buttons", Id::TouchActionButtons},
    {kTouch, "Button size", Id::TouchButtonSize},
    {kTouch, "Camera speed", Id::TouchCameraSpeed},
    {kTouch, "Move the controls", Id::TouchMoveControls},
    {kTouch, "Game fixes", Id::GameFixes},
    {kTouch, "Debug overlay", Id::DebugOverlay},
    {kTouch, "Keep paused", Id::KeepPaused},
    {kTouch, "Step one tick", Id::StepTick},
    {kTouch, "Speed", Id::Speed},
    {kTouch, "Screenshot", Id::Screenshot},
    {kTouch, "Gameplay recording", Id::RecordGameplay},
    {kTouch, "Recording length", Id::RecordLength},
    {kTouch, "Graphics sweep", Id::RecordSweep},
    {kTouch, "Simulate slow CPU", Id::SlowCpu},
    {kTouch, "Level select", Id::LevelSelect},
    {kTouch, "Quit game", Id::Quit},
    // The controls window (F13, once debug/controls_window.cpp), now the Controls tab's rows,
    // offered on both kinds of screen; its Menu page only where a desktop's menu opens by a key.
    {kDesktop, "Change: this game / every game", Id::ControlsFile},
    {kTouch, "Change: this game / every game", Id::ControlsFile},
    {kDesktop, "For: every player / player 1 / player 2", Id::ControlsPlayer},
    {kTouch, "For: every player / player 1 / player 2", Id::ControlsPlayer},
    {kDesktop, "Player N as every player", Id::ControlsPlayerReset},
    {kDesktop, "Bindings: change, add, chord, remove, reset", Id::ControllerBindings},
    {kTouch, "Bindings: change, add, chord, remove, reset", Id::ControllerBindings},
    {kDesktop, "Left stick tuning", Id::LeftStick},
    {kDesktop, "Right stick tuning", Id::RightStick},
    {kTouch, "Left stick tuning", Id::LeftStick},
    {kTouch, "Right stick tuning", Id::RightStick},
    {kDesktop, "Trigger press point", Id::TriggerPoint},
    {kDesktop, "Face buttons", Id::FaceButtons},
    {kDesktop, "Controllers work while another window is in front", Id::PadBackground},
    {kDesktop, "Controllers connected", Id::ConnectedControllers},
    {kDesktop, "Player 1: controller, keyboard", Id::PlayerOne},
    {kDesktop, "Player 2: controller, keyboard", Id::PlayerTwo},
    {kTouch, "Player 1: controller, keyboard", Id::PlayerOne},
    {kDesktop, "Players back to the defaults", Id::PlayersReset},
    {kDesktop, "Menu: key and controller button", Id::MenuOpen},
    {kDesktop, "Reset this game's / every game's controls", Id::ControlsReset},
    {kTouch, "Reset this game's / every game's controls", Id::ControlsReset},
};

// Every option of the old menus is offered on the screens it was offered on.
void keeps_every_old_option() {
    for (const Old& old : kOld) {
        const Entry& e = entry(old.now);
        if (!offered(e, old.where)) {
            std::fprintf(
                stderr,
                "menu_catalog_test: \"%s\" (%s) is no longer offered there\n",
                old.name,
                old.where == kDesktop ? "desktop" : "touch"
            );
        }
        CHECK(offered(e, old.where));
    }
}

// Each Id once, in order; a tab, a name and help each; a section's entries together, the tabs in
// their order, so a menu drawing them in turn shows each section once.
void lists_each_entry_once_in_order() {
    const auto all = entries();
    CHECK(all.size() == static_cast<std::size_t>(kIds));
    std::set<std::string> sections_seen;
    std::string section;
    int tab = 0;
    for (std::size_t i = 0; i < all.size(); ++i) {
        const Entry& e = all[i];
        CHECK(static_cast<std::size_t>(e.id) == i);
        CHECK(&entry(e.id) == &e);
        CHECK(e.label != nullptr && *e.label != '\0');
        CHECK(e.help != nullptr && *e.help != '\0');
        CHECK(e.where != 0 && (e.where & ~kEverywhere) == 0);
        CHECK(static_cast<int>(e.tab) >= tab);
        tab = static_cast<int>(e.tab);
        const std::string here = std::string(tab_name(e.tab)) + "/" + e.section;
        if (here != section) {
            CHECK(!sections_seen.contains(here));
            sections_seen.insert(here);
            section = here;
        }
        if (e.tab != Tab::Program) {
            CHECK(*e.section != '\0');
        }
    }
    // Every tab but Program has something on both kinds of screen, so neither menu has an empty
    // tab (Controls: the bindings; Developer: the overlay).
    for (int t = 0; t < static_cast<int>(Tab::Program); ++t) {
        for (const std::uint8_t where : {kDesktop, kTouch}) {
            bool any = false;
            for (const Entry& e : all) {
                any = any || (static_cast<int>(e.tab) == t && offered(e, where));
            }
            CHECK(any);
        }
    }
}

// HDR's look sliders are on both kinds of screen (desktop menu and Android drawer), in HDR's
// section, as sliders.
void offers_hdr_look_everywhere() {
    for (const Id id : {Id::HdrHighlights, Id::HdrSdrHue, Id::HdrSdrChroma, Id::HdrBlowout}) {
        const Entry& e = entry(id);
        CHECK(offered(e, kDesktop) && offered(e, kTouch));
        CHECK(e.kind == Kind::Slider && std::string_view(e.section) == "HDR");
    }
}

}  // namespace

int main() {
    keeps_every_old_option();
    offers_hdr_look_everywhere();
    lists_each_entry_once_in_order();
    return openrac::test::result();
}

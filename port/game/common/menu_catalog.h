// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's in-game menu as data: every option, button and note, the tab and section it sits in,
// what it says about itself and on which kind of screen it is offered. The desktop's menu (a bar of
// tabs and a window, debug/menu_window.cpp) and the touch screen's drawer (debug/drawer.cpp) both
// draw this one list, so an option added here shows in both. What an entry does is
// debug/menu_items.cpp's, one `case` of a switch on its Id that the compiler checks is there.
//
// Nothing here draws or reads a setting: tests/menu_catalog_test.cpp checks the list on its own.

#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace openrac::frontend::menu {

// What an entry is.
enum class Kind : std::uint8_t {
    Switch,  // on or off, a switch at its right
    Choice,  // a value between two arrows, moved back and on
    Slider,  // a value adjusted along a bounded range
    Action,  // pressed: does something (opens a window, takes a screenshot, quits)
    Note,    // a line of text, nothing to press
    Fixes,   // the program's game fixes (fixes.h): a switch each, as many as the disc has
    Panel,   // rows of its own, as many as it has now (the controller's buttons, a stick, a player)
};

// The kinds of screen an entry is offered on. Desktop: a window with a keyboard and a mouse (the
// menu bar's tabs); Touch: a touch screen (Android, or OPENRAC_DRAWER=1: the drawer).
enum Where : std::uint8_t {
    kDesktop = 1,
    kTouch = 2,
    kEverywhere = kDesktop | kTouch,
};

// The menu's tabs, in their order along the bar (the drawer's sections follow it too). Program
// holds what is not an option (Quit), which each menu places itself.
enum class Tab : std::uint8_t {
    Graphics,
    Enhanced,
    Game,
    Controls,
    Developer,
    Program,
    Count
};

// Every entry, by name. A new option is a value here, a row in menu_catalog.cpp and a `case` in
// debug/menu_items.cpp; nothing else needs to change for both menus to show it.
enum class Id : std::uint8_t {
    // Graphics
    FullScreen,
    DisplayMode,
    Monitor,
    Picture,
    GameplayFov,
    Hud,
    RenderScale,
    Msaa,
    Anisotropy,
    DrawDistance,
    FrameRate,
    FrameCap,
    VSync,
    FrameCounter,
    Hdr,
    HdrPaperWhite,
    HdrPeak,
    HdrStatus,
    LowLatency,
    ResponsiveControls,
    GpuBreadcrumbs,
    // Enhanced
    EnhancedStatus,
    EnhancedGraphics,
    EnhancedPreset,
    ModernShadows,
    ShadowQuality,
    ShadowFilter,
    RimLight,
    RimShadowLift,
    BounceStrength,
    AmbientOcclusion,
    OcclusionQuality,
    OcclusionHiZ,
    TextureUpscaling,
    Upscaling,
    Sharpening,
    MotionBlur,
    MotionBlurQuality,
    ColorCorrection,
    Brightness,
    EnhancedReset,
    EnhancedAdvanced,
    // Game
    Volume,
    MuteInBackground,
    SkipCutscenes,
    GameFixes,
    // Controls
    ControlsFile,
    ControlsPlayer,
    ControlsPlayerReset,
    ControlsStatus,
    ControllerBindings,
    LeftStick,
    RightStick,
    TriggerPoint,
    FaceButtons,
    PadBackground,
    RumbleStrength,
    RumbleStyle,
    RumbleTriggers,
    RumblePhone,
    ConnectedControllers,
    PlayersReset,
    PlayerOne,
    PlayerTwo,
    MenuOpen,
    TouchActionButtons,
    TouchButtonSize,
    TouchCameraSpeed,
    TouchMoveControls,
    ControlsReset,
    // Developer
    DebugOverlay,
    MobyPicker,
    ObjectViewer,
    MeatProxyReview,
    LevelSelect,
    KeepPaused,
    StepTick,
    Speed,
    Screenshot,
    RecordGameplay,
    RecordLength,
    RecordSweep,
    RecordFolder,
    SlowCpu,
    Freecam,
    FreecamMode,
    FreecamInspect,
    FreecamSpeed,
    FreecamReset,
    FreecamExit,
    FreecamHelp,
    // Program
    Quit,
    Count
};

constexpr int kIds = static_cast<int>(Id::Count);
constexpr int kTabs = static_cast<int>(Tab::Count);

// One entry of the menu.
struct Entry {
    Id id;
    Tab tab;
    const char* section;  // its heading in the tab; entries of one section stand together
    Kind kind;
    std::uint8_t where;  // Where's bits
    const char* label;   // its name, as the player reads it (an entry may change it, items.cpp)
    const char* help;    // what it does and when to use it: the menu window's side, a tooltip
};

// Every entry, in the order the menus show them: by tab, then by section.
std::span<const Entry> entries();

// The entry `id`.
const Entry& entry(Id id);

// A tab's name on the bar ("Graphics").
const char* tab_name(Tab tab);

// Says whether `e` is offered on `where` (one of Where's bits).
constexpr bool offered(const Entry& e, std::uint8_t where) {
    return (e.where & where) != 0;
}

}  // namespace openrac::frontend::menu

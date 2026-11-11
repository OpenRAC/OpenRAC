// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The in-game menu's entries (menu_catalog.h).

#include "menu_catalog.h"

#include <array>

namespace openrac::frontend::menu {

namespace {

constexpr std::uint8_t D = kDesktop;
constexpr std::uint8_t T = kTouch;
constexpr std::uint8_t A = kEverywhere;

constexpr Kind kSwitch = Kind::Switch;
constexpr Kind kChoice = Kind::Choice;
constexpr Kind kAction = Kind::Action;
constexpr Kind kNote = Kind::Note;
constexpr Kind kPanel = Kind::Panel;

// clang-format off
constexpr Entry kEntries[] = {
    // ---- Graphics ----
    {Id::FullScreen, Tab::Graphics, "Display", kSwitch, D, "Full screen",
     "Fills the screen with the game's window, or gives the window back. F11 does the same."},
    {Id::DisplayMode, Tab::Graphics, "Display", kChoice, D, "Display mode",
     "Windowed, or borderless full screen at the display's own resolution."},
    {Id::Monitor, Tab::Graphics, "Display", kChoice, D, "Monitor",
     "The display the game goes full screen on: the window's own, or a display by its number."},
    {Id::MenuSize, Tab::Graphics, "Display", kChoice, A, "Menu size",
     "How large this menu and its text are. Auto sizes it to the screen; the steps make it "
     "smaller or larger from there."},
    {Id::Picture, Tab::Graphics, "Picture", kChoice, A, "Picture",
     "The picture's shape: the console's 4:3, a wide screen, or the whole window."},
    {Id::GameplayFov, Tab::Graphics, "Picture", Kind::Slider, A, "Field of View",
     "Adjusts how much of the world you see during gameplay. 100% is the original camera view."},
    {Id::Hud, Tab::Graphics, "Picture", kChoice, A, "HUD",
     "Where the HUD sits on a wide picture: in the 4:3 middle, as on a console, or out at the "
     "screen's edges."},
    {Id::RenderScale, Tab::Graphics, "Picture", kChoice, A, "Render scale",
     "How many pixels the 3D world is drawn with, against the window's. Lower it on a slow "
     "device; raise it for a sharper picture."},
    {Id::Msaa, Tab::Graphics, "Picture", kChoice, A, "Anti-aliasing",
     "Smooths the edges of the world's shapes. Each step costs the graphics card more."},
    {Id::Anisotropy, Tab::Graphics, "Picture", kChoice, A, "Anisotropic filtering",
     "Keeps floors and walls sharp when seen at a slant. Cheap on most graphics cards."},
    {Id::SceneryDetail, Tab::Graphics, "Picture", kChoice, A, "Scenery detail",
     "Full draws the scenery's most detailed models at every distance; Original swaps them for "
     "simpler ones far away, as the console did."},
    {Id::DrawDistance, Tab::Graphics, "Picture", kChoice, A, "Draw distance",
     "How far away things are still drawn. Original stops where the console did; the larger "
     "steps draw farther, and Unlimited draws everything in view. What the game hides from a "
     "place stays hidden."},
    {Id::FrameRate, Tab::Graphics, "Frames", kChoice, A, "Frame rate",
     "Original keeps the console's rate. The others draw extra frames between the game's ticks: "
     "as fast as the display, up to a cap, or without a limit. The game itself runs at its own "
     "speed either way."},
    // Desktop only: on Android the display's rate is the system's, and Frame rate's list of caps
    // is enough there.
    {Id::FrameCap, Tab::Graphics, "Frames", kChoice, D, "Frame cap",
     "The most frames a second, in steps of 5, while Frame rate is capped."},
    {Id::VSync, Tab::Graphics, "Frames", kChoice, A, "VSync",
     "Waits for the display before showing a frame, which stops tearing. Off shows frames as "
     "soon as they are drawn."},
    {Id::FrameCounter, Tab::Graphics, "Frames", kChoice, A, "Frame counter",
     "Shows the frames a second in a corner of the picture. F2 changes it too."},
    {Id::Hdr, Tab::Graphics, "HDR", kChoice, A, "HDR",
     "High dynamic range output, when the system has the display in HDR mode. Turned on from "
     "Off, it starts with the game's next start."},
    {Id::HdrPaperWhite, Tab::Graphics, "HDR", kChoice, A, "HDR paper white",
     "How bright the game's white is in HDR. System takes the brightness the system gives "
     "ordinary pictures."},
    {Id::HdrPeak, Tab::Graphics, "HDR", kChoice, A, "HDR peak",
     "The brightest that glows and explosions may go in HDR, also told to the display. Display "
     "takes what the display reports. Where the system sets the brightness (Android, macOS), "
     "it can only go lower."},
    {Id::HdrStatus, Tab::Graphics, "HDR", kNote, A, "HDR status",
     "What the display and the system give the game for HDR now."},
    {Id::LowLatency, Tab::Graphics, "Latency", kSwitch, A, "Low-latency frames",
     "Queues fewer frames for the display and holds none back, so a press shows sooner. Off "
     "keeps the timing players have learnt."},
    {Id::ResponsiveControls, Tab::Graphics, "Latency", kSwitch, A, "Responsive controls",
     "Reads the controller again just before each tick, and turns the camera between ticks "
     "with the right stick as it is now. Off keeps the timing players have learnt."},

    {Id::GpuBreadcrumbs, Tab::Graphics, "Diagnostics", kSwitch, A, "GPU breadcrumbs (restart)",
     "Records GPU progress to help diagnose graphics crashes. Restart the game to apply changes."},

    // ---- Enhanced ----
    {Id::EnhancedStatus, Tab::Enhanced, "Enhanced graphics", kNote, A, "Enhanced graphics",
     "Why the enhanced graphics cannot be changed now, and what to do about it."},
    {Id::EnhancedGraphics, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Enhanced graphics",
     "Bounced light, modern shadows and ambient occlusion over the original lighting. Off shows "
     "the game as the console drew it."},
    {Id::ModernShadows, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Modern shadows",
     "Shadows cast by the level's sun on everything, in place of the game's round blobs."},
    {Id::ShadowQuality, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Shadow quality",
     "The shadows' sharpness and reach. Higher costs the graphics card more."},
    {Id::RimLight, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Gameplay rim light",
     "A thin light along the edges of characters, so they stand out from the scenery."},
    {Id::RimShadowLift, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Rim shadow lift",
     "Lets the rim light show in shadow too."},
    {Id::BounceStrength, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Bounce strength",
     "How much light bounces off the level onto everything else."},
    {Id::AmbientOcclusion, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Ambient occlusion",
     "Darkens corners and creases where light hardly reaches."},
    {Id::OcclusionQuality, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Occlusion quality",
     "Quality traces more directions for smoother occlusion; Performance costs less."},
    {Id::OcclusionHiZ, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Hierarchical depth (Hi-Z)",
     "Uses a depth hierarchy for distant ambient-occlusion samples."},
    {Id::TextureUpscaling, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Texture upscaling",
     "Normal draws the world's textures as they are; Edge-aware enlarges them twice with sharp "
     "edges."},
    {Id::Upscaling, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Upscaling",
     "Draws fewer pixels and builds the full picture from several frames. The named upscaler is "
     "the one this device uses."},
    {Id::Sharpening, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Sharpening",
     "How much the upscaler sharpens the picture it builds."},
    {Id::MotionBlur, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Motion blur",
     "Blurs what moves fast across the picture."},
    {Id::ColorCorrection, Tab::Enhanced, "Enhanced graphics", kSwitch, A, "Color correction",
     "A filmic tone curve (AgX) for the enhanced lighting's brighter colours."},
    {Id::Brightness, Tab::Enhanced, "Enhanced graphics", kChoice, A, "Brightness",
     "The picture's brightness under color correction."},
    {Id::EnhancedReset, Tab::Enhanced, "Enhanced graphics", kAction, A, "Reset to defaults",
     "Puts every enhanced setting back as it came. Press twice: the first press only asks."},
    {Id::EnhancedAdvanced, Tab::Enhanced, "Advanced", kSwitch, D, "Advanced tuning window",
     "A window with every enhanced setting, the materials, the water and the diagnostics, for "
     "tuning a level by eye."},

    // ---- Game ----
    {Id::Volume, Tab::Game, "Sound", kChoice, A, "Volume",
     "The game's volume, in steps of 10%. F9 and F10 change it too."},
    {Id::MuteInBackground, Tab::Game, "Sound", kSwitch, D, "Mute in background",
     "Silences the game while another window has the focus."},
    {Id::SkipCutscenes, Tab::Game, "Game", kSwitch, A, "Skip cutscenes",
     "Start skips a cutscene, as it does on the console once the scene has been seen."},
    {Id::GameFixes, Tab::Game, "Game fixes", Kind::Fixes, A, "Game fixes",
     "What the port changes from the console. On is the port's fix; off gives the console's "
     "behaviour back, bugs included. The launcher's page for the game says more about each."},

    // ---- Controls ----
    {Id::ControlsFile, Tab::Controls, "Controller", kChoice, A, "Change the controls of",
     "This game: only it, and what is not changed here comes from every game. Every game: what "
     "each game reads unless its own controls change it."},
    {Id::ControlsPlayer, Tab::Controls, "Controller", kChoice, A, "For",
     "Every player, or player 1 or 2 alone: a player's own controls go over every player's."},
    {Id::ControlsPlayerReset, Tab::Controls, "Controller", kAction, A, "Player as every player",
     "Takes the player's own controls away: they read every player's again."},
    {Id::ControlsStatus, Tab::Controls, "Controller", kPanel, A, "Controller",
     "Changes are saved and used at once. A button held as it is bound presses its new control "
     "only once let go and pressed again."},
    {Id::ControllerBindings, Tab::Controls, "Buttons", kPanel, A, "Controller bindings",
     "What presses each of the pad's controls on a controller. Pick one to replace it, add a "
     "button or a 2-button combo, remove one or go back to the default. A mark: another control "
     "has that button too, and both press."},
    {Id::LeftStick, Tab::Controls, "Left stick", kPanel, A, "Left stick",
     "Dead zone, outer edge, sensitivity and inversion. The picture shows the stick as the "
     "controller gives it (grey) and as the game reads it (green). The game has its own dead "
     "zone: the port adds none unless set here."},
    {Id::RightStick, Tab::Controls, "Right stick", kPanel, A, "Right stick",
     "Dead zone, outer edge, sensitivity and inversion. The picture shows the stick as the "
     "controller gives it (grey) and as the game reads it (green)."},
    {Id::TriggerPoint, Tab::Controls, "Triggers and buttons", kChoice, A, "Trigger press point",
     "How far a trigger is pulled before it presses."},
    {Id::FaceButtons, Tab::Controls, "Triggers and buttons", kChoice, A, "Face buttons",
     "By position, or by label: on a pad with A on the right (Nintendo's), the button marked A "
     "is South."},
    {Id::PadBackground, Tab::Controls, "Triggers and buttons", kSwitch, D,
     "Controllers work in the background",
     "The controllers still play while another window is in front. Off: the game ignores them "
     "while it is alt-tabbed or minimised."},
    {Id::RumbleStrength, Tab::Controls, "Vibration", kChoice, A, "Vibration",
     "How strong the controller vibrates. Off: never."},
    {Id::RumbleStyle, Tab::Controls, "Vibration", kChoice, A, "Vibration style",
     "Enhanced: smoother and easier to feel on modern controllers. Classic: the game's raw "
     "values."},
    {Id::RumbleTriggers, Tab::Controls, "Vibration", kSwitch, A, "Vibration in the triggers",
     "Also vibrate the triggers, on controllers that have trigger motors (Xbox One and later)."},
    {Id::RumblePhone, Tab::Controls, "Vibration", kSwitch, T, "Vibrate the phone",
     "With touch controls, the phone or tablet vibrates."},
    {Id::ConnectedControllers, Tab::Controls, "Players", kPanel, A, "Controllers connected",
     "Each controller connected, lit while a button is pressed on it, and the player it is."},
    {Id::PlayersReset, Tab::Controls, "Players", kAction, A, "Players back to the defaults",
     "Each player takes the next free controller again. Players are the same in every game."},
    {Id::PlayerOne, Tab::Controls, "Player 1", kPanel, A, "Player 1",
     "Player 1's controller and keyboard. Players are the same in every game."},
    {Id::PlayerTwo, Tab::Controls, "Player 2", kPanel, A, "Player 2",
     "Player 2's controller and keyboard. Players are the same in every game."},
    {Id::MenuOpen, Tab::Controls, "Menu", kPanel, D, "Open the menu",
     "The key and the controller button that open and close this menu. Any controller's Select "
     "and Start pressed together do too. The key is kept by its place on the keyboard; Alt "
     "counts pressed by itself, so Alt+Return still goes full screen."},
    {Id::TouchActionButtons, Tab::Controls, "Touch controls", kSwitch, T, "Action buttons",
     "Buttons that show what they do (jump, fire, the ship's boost) and change with what the game "
     "is doing. Off: the DualShock's buttons."},
    {Id::TouchButtonSize, Tab::Controls, "Touch controls", kChoice, T, "Button size",
     "The on-screen buttons' size."},
    {Id::TouchCameraSpeed, Tab::Controls, "Touch controls", kChoice, T, "Camera speed",
     "How fast a finger dragged over the screen turns the camera."},
    {Id::TouchMoveControls, Tab::Controls, "Touch controls", kAction, T, "Edit the on-screen buttons",
     "Drag the on-screen buttons where your thumbs want them. The game waits meanwhile."},
    {Id::ControlsReset, Tab::Controls, "Reset", kAction, A, "Reset these controls",
     "This game's controls go back to every game's, or every game's to the defaults. Pressed "
     "twice: the first asks."},

    // ---- Developer ----
    {Id::DebugOverlay, Tab::Developer, "Tools", kSwitch, A, "Debug overlay",
     "The frame's timings, the renderer's passes, memory, mobys and the game's state. F3 shows "
     "and hides it."},
    {Id::MobyPicker, Tab::Developer, "Tools", kSwitch, D, "Moby picker",
     "Click a moby in the picture to select it and see its fields."},
    {Id::ObjectViewer, Tab::Developer, "Tools", kSwitch, D, "Object viewer",
     "Look at a picked object on its own: its model, bones, lights and materials."},
    {Id::MeatProxyReview, Tab::Developer, "Tools", kSwitch, D, "Meat Proxy Review",
     "Open saved freecam views and record feedback."},
    {Id::LevelSelect, Tab::Developer, "Tools", kAction, A, "Level select",
     "Goes to another level. F1 opens it too."},
    {Id::KeepPaused, Tab::Developer, "Time", kSwitch, A, "Keep paused",
     "The game waits while the menu is open; this keeps it paused once the menu is closed. F5 "
     "pauses and resumes in play."},
    {Id::StepTick, Tab::Developer, "Time", kAction, A, "Step one tick",
     "Runs the game for one tick and pauses it again. F6 does the same."},
    {Id::Speed, Tab::Developer, "Time", kChoice, A, "Speed",
     "Slows the game down, to half, a quarter and so on. F7 and F8 do the same."},
    {Id::Screenshot, Tab::Developer, "Capture", kAction, A, "Screenshot",
     "Saves the picture once the menu has closed. F12 takes one in play."},
    {Id::RecordGameplay, Tab::Developer, "Record diagnostics", kAction, A, "Gameplay recording",
     "Records while you play, for the length below. Use it to catch stutters or a slow place or "
     "moment. It starts a moment after the menu closes; opening the menu ends it, and its "
     "report is copied and saved. Ctrl+F3 starts and stops one too."},
    {Id::RecordLength, Tab::Developer, "Record diagnostics", kChoice, A, "Recording length",
     "How long a gameplay recording runs, or until it is stopped."},
    {Id::RecordSweep, Tab::Developer, "Record diagnostics", kAction, A, "Graphics sweep",
     "Stand still while it runs: it changes the graphics settings every few seconds, with "
     "VSync off, and measures what each one costs. Use it when the game is slow everywhere."},
    {Id::RecordFolder, Tab::Developer, "Record diagnostics", kAction, D, "Open recordings folder",
     "Opens the folder the recordings and their reports are saved in."},
    {Id::SlowCpu, Tab::Developer, "Performance", kChoice, A, "Simulate slow CPU",
     "Makes each frame's work take longer, as on a weaker device's processor, to test how the "
     "game copes."},
    {Id::Freecam, Tab::Developer, "Freecam", kSwitch, A, "Freecam",
     "A camera of your own, flown with the keyboard and mouse or a gamepad while the game goes "
     "on."},
    {Id::FreecamMode, Tab::Developer, "Freecam", kChoice, A, "Mode",
     "Free look flies anywhere; Culling inspection shows what the game's camera would draw."},
    {Id::FreecamInspect, Tab::Developer, "Freecam", kChoice, A, "Inspect",
     "Whose culling is shown: the main camera's, or one of the shadow cascades' (with modern "
     "shadows on)."},
    {Id::FreecamSpeed, Tab::Developer, "Freecam", kChoice, A, "Fly speed",
     "How fast the freecam flies, in the game's units a second."},
    {Id::FreecamReset, Tab::Developer, "Freecam", kAction, A, "Reset to game camera",
     "Puts the freecam where the game's camera is."},
    {Id::FreecamExit, Tab::Developer, "Freecam", kAction, A, "Exit freecam",
     "Gives the view back to the game and puts the freecam's settings back as they came."},
    {Id::FreecamHelp, Tab::Developer, "Freecam", kNote, A, "Freecam controls",
     "How to move the freecam, with the keyboard or the controller, whichever was used last."},

    // ---- Program ----
    {Id::Quit, Tab::Program, "", kAction, A, "Quit game",
     "Closes the game. Progress since the last save is lost."},
};
// clang-format on

constexpr std::array<const char*, kTabs> kTabNames =
    {"Graphics", "Enhanced", "Game", "Controls", "Developer", "Program"};

// The entries by Id, checked as the program is compiled: each Id once, in order.
constexpr bool in_order() {
    if (std::size(kEntries) != static_cast<std::size_t>(kIds)) {
        return false;
    }
    for (int i = 0; i < kIds; ++i) {
        if (static_cast<int>(kEntries[i].id) != i) {
            return false;
        }
    }
    return true;
}

static_assert(in_order(), "menu_catalog.cpp: one entry per Id, in the Id's order");

}  // namespace

std::span<const Entry> entries() {
    return kEntries;
}

const Entry& entry(Id id) {
    return kEntries[static_cast<int>(id)];
}

const char* tab_name(Tab tab) {
    return kTabNames[static_cast<std::size_t>(tab)];
}

}  // namespace openrac::frontend::menu

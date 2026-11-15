// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the debug overlay's files share, and nothing outside the overlay includes: its state, the
// constants and types the state is made of, and the helpers its panels draw with. debug_overlay.cpp
// implements debug_overlay.h with them; debug/overlay.cpp defines the state and the helpers, and
// each panel, and each of the parts the entry points draw (the drawer and its touch handling, the
// menu bar, the settings controls they share, the marks over the picture), is defined in the file
// of debug/ named for its subject.
//
// The state is file-global, as it was inside debug_overlay.cpp; it is to become an object passed
// to what needs it (CODING_CONVENTIONS.md, sections 11.4 and 24). The window's thread, the one
// that calls debug_overlay.h's functions, is the only one that reads or writes it.
//
// The game's state (host::GameState, host::LiveMoby: game/common/profile.h) comes through
// debug_overlay.h, and the overlay's other files reach it through that include rather than adding
// includes of their own.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "debug/object_viewer.h"
#include "debug_overlay.h"
#include "openrac/guest.h"
#include "platform/bindings.h"

namespace openrac::frontend::debug_overlay {

// One moby's bytes in main memory (the size of the game's moby struct).
constexpr std::uint32_t kMobySize = 0x100;

// The sections of the panel, as OPENRAC_OVERLAY names them.
enum Section : std::size_t {
    kFrame,
    kRenderer,
    kMemory,
    kMobys,
    kGame,
    kShaders,
    kSectionCount
};

constexpr std::array<const char*, kSectionCount> kSectionNames =
    {"frame", "renderer", "memory", "mobys", "game", "shaders"};

// Developer menu and renderer-panel controls share the same opt-in camera settings.
void camera_tools_controls();
void place_review_camera(int level, scene::Vec3 position, scene::Vec3 forward);
void meat_proxy_review_window(const Shown& shown);

inline renderer::Vec3 camera_forward(const renderer::Camera& camera) {
    return {-camera.view[2], -camera.view[6], -camera.view[10]};
}

// The pad's buttons by bit (active low in the report).
constexpr std::array<const char*, 16> kButtonNames =
    {"select",
     "L3",
     "R3",
     "start",
     "up",
     "right",
     "down",
     "left",
     "L2",
     "R2",
     "L1",
     "R1",
     "triangle",
     "circle",
     "cross",
     "square"};

// The main memory map: one cell per 128 KB.
constexpr std::uint32_t kCellBytes = 128 * 1024;

// One cell of the memory map (scan_ram): how much of it holds data, and whether it changed.
struct RamCell {
    std::uint16_t used_pages = 0;  // of 32 (4 KB each) holding anything but zero
    std::uint64_t sum = 0;         // a checksum of its bytes at the last scan
    bool changed = false;          // the checksum differs from the scan before
};

// A scripted key or click (OPENRAC_KEYS).
// Explicit held actions let native diagnostics exercise scene-camera and gizmo drags safely.
enum class ScriptedAction {
    Stroke,
    Click,
    Move,
    ButtonDown,
    ButtonUp,
    Wheel,
    KeyDown,
    KeyUp
};
struct ScriptedInput {
    std::uint64_t frame = 0;  // the window frame it happens at
    int held = 0;             // and that many held pictures later, while paused there
    ScriptedAction action = ScriptedAction::Stroke;
    std::uint8_t button = SDL_BUTTON_LEFT;
    SDL_Keycode key = 0;      // 0: a pointer or wheel action
    float x = 0.0f;           // a click's place in the window, in ImGui's coordinates
    float y = 0.0f;
    bool done = false;  // put in the event queue already
};

// The card a section of the drawer's rows is drawn on (card_begin).
struct Card {
    ImVec2 top;          // its top left corner, in the window
    float width = 0.0f;  // its width in pixels
    int rows = 0;        // the rows drawn on it so far
};

// A window a finger scrolls (drag_scroll): whether the finger down now has dragged it, so that it
// presses nothing, and how fast it goes on moving once the finger has let go, in pixels a second.
struct Scroll {
    bool dragged = false;
    float velocity = 0.0f;
};

// How the desktop's menu window moves (debug/menu_window.cpp): its rows' scroll easing to where
// the wheel or the focus sends it, the open tab's page coming in from the side it was picked on, and
// the help fading in when it changes.
struct MenuMotion {
    float scroll_target = 0.0f;  // where the rows are going, in pixels
    float scroll_shown = 0.0f;   // where they are now
    unsigned rows_window = 0;    // the rows' child window (ImGuiID), whose scroll this eases
    float page = 1.0f;           // the open tab's page, 0 just picked, 1 in place
    float page_from = 0.0f;      // -1 it comes from the left, 1 from the right
    int page_tab = -1;           // the tab the page is of
    float help = 1.0f;           // the help's text, 0 just changed, 1 shown
    int help_of = -2;            // the entry it is of (s.menu_hover)
    bool pad = false;            // the last input was a controller's: its buttons are named
    float quit_hold = 0.0f;      // seconds the controller's quit button has been held
    int last_tab = 0;            // the last of the bar's tabs open (not a button's page)
};

// The menu's Controls tab (debug/menu_controls.cpp): which file it changes, what is being captured,
// and what it last said.
struct ControlsMenu {
    bool global = false;      // the global file; else the running game's own
    int player = -1;          // the player whose own controls are changed (0, 1); -1 every player's
    int open_target = -1;     // the control whose own rows (change, add, remove) are open; -1 none
    int capture_target = -1;  // the target a press is being captured for; -1 none
    double capture_since = 0.0;  // when the capture began (ImGui's clock); it times out
    // What the captured press does: replaces the target's pad inputs, adds one, or is the first
    // or the second of a chord.
    enum class Mode {
        Replace,
        Add,
        ChordFirst,
        ChordSecond,
    } mode = Mode::Replace;
    platform::Binding chord_with;  // a chord's first press, once captured
    std::string message;           // what the last change said
    // A press just bound that another control also has alone: offered to be taken from it.
    int clash_target = -1;
    int clash_from = -1;  // the control just changed, under which the choice is shown
    platform::Binding clash_input;
    bool confirm_reset = false;  // "Reset" asked once; asked again, it resets
};

// Everything the overlay keeps between frames: what the environment asked for, what the last
// tick gave it, and what its panels, windows, menus and drawer were set to. One exists (s).
struct State {
    CameraDebugSettings
        camera_tools;           // Window-thread developer settings, never shared with worker.
    bool camera_reset = false;  // Pending reseed from the game camera.
    unsigned camera_alt = 0;    // Left/right Alt bits; releasing one must not release the other.
    bool camera_right_mouse = false;  // Right-click is an alternative mouse-look hold.
    float camera_look_x = 0,
          camera_look_y = 0;        // Mouse deltas consumed once per displayed snapshot.
    std::uint64_t camera_time = 0;  // SDL nanoseconds of the last input snapshot.
    bool camera_pad_chord = false;  // L3+R3 held on a gamepad: toggles freecam once per press.
    std::vector<ScriptedInput> script;
    std::optional<ImVec2> script_pointer;  // App-local pointer held by explicit input scripts.
    SDL_Keymod script_modifiers = SDL_KMOD_NONE;
    std::filesystem::path script_file;
    std::filesystem::file_time_type script_file_time{};
    std::uint64_t script_poll_at = 0;   // OPENRAC_KEYS, in order
    SDL_Window* window = nullptr;       // as last drawn into
    int held_pictures = 0;              // pictures of the tick held now
    bool wanted = false;                // shown from the start
    bool configured = false;            // the environment was read
    bool visible = false;               // the panel is shown (F3)
    bool initialised = false;           // the ImGui context exists
    bool gl_subset = false;  // drawn with the renderer's GL (imgui_gl.h): the window's is Vulkan's
    std::array<bool, kSectionCount> open_at_start = {true, false, false, false, false, false};

    // The last tick, as the window gave it.
    std::string game;
    std::span<const std::uint8_t> ram;
    int level = -1;
    host::GameState state;
    std::uint32_t draws = 0;
    std::uint64_t tick = 0;
    Tick counts;
    std::uint16_t buttons = 0xFFFF;
    std::uint8_t analog[4] = {128, 128, 128, 128};

    // Pause, step, slow motion.
    bool paused = false;       // the game is held: no tick runs
    bool step = false;         // one tick to run while paused
    double stretch = 1.0;      // a tick lasts this many of the game's (slow motion: 1 to 64)
    long long pause_at = -1;   // OPENRAC_PAUSE_AT: the tick to pause at; -1 none
    long long reload_at = -1;  // OPENRAC_SHADER_RELOAD: the tick to read the shaders again at
    int held_frames =
        0;  // frames to hold before going on by itself (a scripted pause); 0: for good

    // The renderer's passes hidden here, by name; wireframe.
    std::set<std::string> hidden;  // the passes' names (OPENRAC_HIDE_PASSES and the switches)
    bool wireframe = false;        // the world drawn as lines
    bool wireframe_set = false;    // GL's polygon mode is lines now, to be put back after the frame

    // Mobys.
    std::uint32_t selected = 0;  // address; 0 none
    bool follow_hero = false;    // "inspect": the hero is selected, whichever moby he is now
    bool material_settings = false;  // Unified authoring window, independent of Enhanced Settings.
    std::string material_preset;         // Stable preset id, revalidated after level changes.
    int material_library_preset = -1;
    std::optional<std::size_t> material_selection;  // Active slot for the preview.
    std::vector<std::size_t> material_extra_slots;
    std::weak_ptr<const scene::MaterialAssets> material_selection_assets;
    bool material_pick_pending = false;  // Resolve the latest picker result in the editor.
    std::string material_error;          // Persistent edit/save diagnostics.
    bool enhanced_settings = false;  // Independent Enhanced Settings window.
    bool meat_proxy_review = false;
    ControlsMenu controls;           // the menu's Controls tab
    bool inspector = false;      // the moby inspector window is open
    bool labels = false;         // the mobys' classes written in the picture
    bool near_hero = true;       // the list holds only the mobys within `radius` of the hero
    float radius = 30.0f;        // in the game's units
    scene::PickResult picker_result;
    double picker_export_at = -1;
    std::string picker_error;

    // Memory.
    bool memory_viewer = false;               // the memory viewer window is open
    std::uint32_t view_address = 0x00100000;  // the first address it shows
    bool view_words = false;                  // it shows words and floats, not bytes
    std::vector<RamCell> cells;               // the memory map, as scan_ram last measured it
    std::uint64_t scanned_at = ~0ull;         // the tick of that scan, divided by 25; ~0 never
    std::uint32_t used_pages = 0;             // the 4 KB pages holding data, in all the cells

    // Screenshots of the window, with the overlay.
    std::optional<std::uint64_t> shot_tick;  // write the next frame under this tick's name
    std::uint64_t shot_every = 0;  // OPENRAC_OVERLAY_SHOT=N:PREFIX
    std::string shot_prefix;       // the start of their file names
    std::uint64_t shot_from = 0;   // OPENRAC_SHOT_FROM: the first tick written
    int shots = 0;                 // written so far (at most 10,000)

    std::string message;       // what the last action said (a reload, a file written)
    bool alt_alone = false;    // Alt went down and no other key since: its release toggles the menu
    bool menu_key_capture = false;     // the next key pressed becomes the menu's (Controls tab)
    bool menu_button_capture = false;  // the next pad button becomes the menu's
    std::uint64_t focus_ns = 0;  // when the window last gained or lost the focus (SDL's clock)
    ObjectViewerState object_viewer;  // Session-only developer object studio.
    bool developer = false;  // the launcher's developer tools (OPENRAC_DEVELOPER): the level select
    std::string dump_path = "openrac_frames.jsonl";  // where the frames' measurements are written

    // The menu (menu_window() on a desktop, drawer() on a touch screen; debug/menu_ui.h).
    bool menu_open = false;         // open, or opening
    float menu_at = 0.0f;           // how far out: 0 shut, 1 open
    bool keep_paused = false;       // the pause the game is left in when it shuts
    bool menu_focus = false;        // given the keyboard and gamepad when it opens or changes tab
    bool menu_focus_first = false;  // the next row drawn takes the focus (debug/menu_ui.cpp)
    MenuMotion menu_motion;         // the desktop's window's scroll and transitions
    int menu_want_tab = -1;         // a menu::Tab to open the window at (F13: Controls); -1 none
    bool pad_locked = false;        // ImGui gets no gamepad: a binding's press until let go
    bool drew_last_frame = false;   // the overlay drew the frame before (ImGui's clock ran)
    Scroll menu_scroll;             // the finger's drag over the drawer's rows
    bool slider_held = false;       // a finger moving a slider sideways: the rows do not scroll
    bool shot_when_shut =
        false;  // a screenshot asked for in it, taken once it is out of the picture
    bool menu_confirm_reset = false;  // Enhanced "Reset" pressed once; pressed again, it resets
    bool quit_asked = false;          // Quit pressed in the drawer: its confirmation is shown
    bool quit_focus = false;          // the confirmation's first row takes the focus once
    int menu_tab = 0;                 // the desktop's open tab, by its place on the bar
    long long menu_open_at = -1;      // OPENRAC_MENU: the tick to open the menu at; -1 none
    int menu_hover = -1;              // the entry under the pointer or the focus (menu::Id), or -1
    float menu_row_height = 48.0f;    // a row's height in dp: a thumb's on a touch screen
    bool drawer_developer = false;    // the drawer's developer tools unfolded
    bool drawer_fixes = false;        // the drawer's game fixes unfolded
    bool drawer_controller = false;   // the controller's settings out beside the drawer
    float drawer_controller_at = 0.0f;     // how far out they are: 0 in, 1 out
    bool drawer_controller_focus = false;  // the gamepad's focus goes to them, or back, next frame
    Scroll drawer_controller_scroll;       // the finger's drag over their rows
    // The on-screen pad's layout being edited (touch_editing()): Reset asked for, the box of the
    // bar in the middle of the screen as last drawn, whose fingers are ImGui's, not the pad's, and
    // the lines a dragged group snapped to (set_touch_guides()).
    bool touch_editing = false;
    bool touch_reset = false;
    ImVec2 bar_min{};
    ImVec2 bar_max{};
    ImVec2 touch_guides{-1.0f, -1.0f};
    // The size of the control picked to be resized (set_touch_selected()), below 0 for none, and
    // the steps the bar asked to change it by (take_touch_size_steps()).
    float touch_selected = -1.0f;
    int touch_size_steps = 0;
    Card card;                      // the card being drawn on
    bool settings_changed = false;  // a setting changed in the drawer or the menus, not applied yet
    Scroll panel_scroll;            // the panel's drag_scroll
    // What eases from one state to the next (tween()), by ImGui's id.
    std::unordered_map<ImGuiID, float> tweens;
    float ui_scale = 1.0f;          // the screen's density, for rows a thumb can press
    // The drawer's logo in ImGui's font atlas (initialise()): its custom rectangle, -1 before it is
    // made, and where its name starts, in pixels from its left.
    int logo_rect = -1;
    int logo_name_x = 0;
    // Fingers on the screen as ImGui's pointer: the first one down, and those the drawer took from
    // the on-screen pad (their moves and lifts are the drawer's too).
    SDL_FingerID pointer = 0;
    bool pointer_down = false;
    std::vector<SDL_FingerID> taken;

    // The processor's load, measured each half second.
    double cpu_seconds = 0.0;  // the process's processor time at the last measure, in seconds
    double cpu_wall = 0.0;     // the frame's start then (common/stats.h's clock)
    double cpu_load = 0.0;     // the load between the last two: 1 is one core busy

    // The performance capture's length, as the drawer and the menu set it (0: until stopped).
    double capture_seconds = 30.0;
};

// The overlay's state: what the last tick gave it and what its panels and windows were set to.
// Defined in debug/overlay.cpp; only the window's thread touches it.
extern State s;

// The last second of frames, averaged (averages()): how many there were, their rates, and where
// their time went. Times are in milliseconds, per tick or per frame as marked.
struct Averages {
    int frames = 0, ticks = 0;              // the frames in the second, and the ticks among them
    double frame_ms = 0.0, worst_ms = 0.0;  // a frame's interval: the average, the longest
    double fps = 0.0, tps = 0.0;            // frames and ticks a second
    double game_ms = 0.0, scene_ms = 0.0, wait_ms = 0.0;  // per tick
    double prepare_ms = 0.0, render_ms = 0.0, present_ms = 0.0, overlay_ms = 0.0,
           swap_ms = 0.0;  // per frame
    double gpu_ms = -1.0;  // per frame, over the frames whose GPU times are back; -1 none
    int gpu_frames = 0;    // those frames
};

// The colours of the panels' text: a warning, a good result, a note, a selection.
extern const ImVec4 kRed;
extern const ImVec4 kGreen;
extern const ImVec4 kDim;
extern const ImVec4 kYellow;

// Returns `v`, a length in the overlay's units, in pixels: `v` on a 1080p screen, more on a
// dense or large one (s.ui_scale).
float dp(float v);

// Says whether the `size` bytes at `address` (a game address, its segment bits ignored) are all in
// the game's main memory as the last tick gave it.
bool in_ram(std::uint32_t address, std::uint32_t size);

// Returns the moby class names by class number, read once from the editor's table
// (openrac_moby_class_names, cmake/Games.cmake).
const std::unordered_map<int, std::string>& class_names();

// Returns the name of moby class `id`, or the empty string for a class the table does not name.
std::string class_name(int id);

// Returns the game function at `address` as the panels show it: its address and its name, "(no C)"
// for one without C, "none" for 0.
std::string function_name(std::uint32_t address);

// Returns the hero's moby (class 0) among the last tick's live mobys, or nullptr when there is none.
// The pointer lives until the next tick.
const host::LiveMoby* hero();

// Returns the selected moby (s.selected) among the last tick's live mobys, or nullptr when none is
// selected or it is gone. The pointer lives until the next tick.
const host::LiveMoby* selected_moby();

// Returns the distance between the positions of mobys `a` and `b`, in the game's units.
float distance(const host::LiveMoby& a, const host::LiveMoby& b);

// Returns `bytes` as megabytes with one decimal ("12.3 MB").
std::string megabytes(std::uint64_t bytes);

// Draws one row of a two-column table: `label`, then `value`. Inside an ImGui table only.
void row(const char* label, const std::string& value);

// ---- The panel's sections and the windows beside it ----

// Returns the averages of the last second of frames (common/stats.h's history): rates, times per
// tick and per frame, the GPU's time where it is measured; an empty Averages before the first
// frame. Defined in debug/frame.cpp.
Averages averages();

// Draws the frame section: the rates, the frame times' plots and where a frame's time goes, from
// `a` (averages()) and what the window measured (`shown`). Defined in debug/frame.cpp.
void frame_section(const Shown& shown, const Averages& a);

// Draws the renderer section: the GPU, the frame's counters and a table of the renderer's passes,
// each with a switch that hides it (s.hidden). Defined in debug/renderer.cpp.
void renderer_section(const Shown& shown);

// Measures the game's main memory for the memory map (s.cells, s.used_pages): the pages of each
// 128 KB cell that hold anything, and which cells changed. Does it at most once each 25 ticks, and
// not before the tick has given the whole 32 MB. Defined in debug/memory.cpp.
void scan_ram();

// Draws the memory section: the host's memory, the game's stack and the map of its main memory, a
// click on which opens the memory viewer there. Defined in debug/memory.cpp.
void memory_section();

// Draws the memory viewer window (s.memory_viewer), 256 bytes from s.view_address as bytes or as
// words and floats. Defined in debug/memory.cpp.
void memory_viewer_window();

// Draws the mobys section: the live mobys, those near the hero or all of them, nearest first; a
// click on one selects it and opens the inspector. Defined in debug/mobys.cpp.
void mobys_section();

// Draws the moby inspector window (s.inspector): the selected moby's fields, read from the game's
// memory. Defined in debug/mobys.cpp.
void inspector_window();

// Draws the game section: the level, the hero, the camera, the pad, the calls to functions without
// C, and the probes' switches. Defined in debug/game.cpp.
void game_section(const Shown& shown);

// Returns the folder the shaders are read again from: OPENRAC_SHADER_DIR when it is set, otherwise
// the port folder the build read them from (common/stats.h), empty when none was built in.
// Defined in debug/shaders.cpp.
std::string shader_root();

// Draws the shaders section: the GL programs and their state, with a button that reads them all
// again from shader_root() and one for each. Defined in debug/shaders.cpp.
void shaders_section();

// ---- The window's entry points' helpers (debug_overlay.cpp) ----

// Shows or hides the panel (s.visible), and has the frame statistics kept while it is shown.
// Defined in debug_overlay.cpp.
void set_visible(bool on);

// ---- The touch handling, the drawer, the menus and their controls ----

// Says whether the drawer is offered: always on Android/iOS, elsewhere when OPENRAC_DRAWER=1. With it
// the menus are the drawer's and the panel has no keys to name. Defined in debug/touch.cpp.
bool drawer_possible();

// Scrolls the window being drawn when a finger drags up or down it, and lets go of what the finger
// came down on, which is then not pressed; let go, the rows glide on and slow down (`scroll`).
// Defined in debug/touch.cpp.
void drag_scroll(Scroll& scroll);

// Starts or ends the editing of the on-screen pad's layout (touch_editing()): the menu shuts,
// and the game stays paused until it ends. Defined in debug/touch.cpp.
void edit_touch(bool on);

// Handles the drawer's events: Back and Guide open and shut it; fingers are ImGui's pointer, and
// those that land while it is open are its own, not the on-screen pad's. Returns true when `e` was
// the drawer's, which the game then does not see. Defined in debug/touch.cpp.
bool drawer_event(const SDL_Event& e);

// Slides the drawer towards s.menu_open with the launcher's easing, and draws it as far out as it
// is (s.menu_at): the menu's entries (menu_catalog.h) on cards, the developer tools folded away. A
// touch beside it or the gamepad's B shuts it. Defined in debug/drawer.cpp.
void drawer(const Shown& shown);

// Slides the desktop's menu towards s.menu_open: the bar of tabs from the window's top and the
// window with the open tab's entries and their help. Defined in debug/menu_window.cpp.
void menu_window(const Shown& shown);

// Draws the bar in the middle of the screen while the on-screen pad's layout is edited, to make
// the controls smaller or larger, put them back and finish, with the lines a dragged group snapped
// to. Defined in debug/drawer.cpp.
void touch_edit_bar();

// Says whether the desktop's menu (menu_window) is the menu: everywhere the drawer is not.
// Defined in debug/menu_bar.cpp.
bool menu_window_possible();

// Presses and releases `key` through SDL's queue, as the macOS menus do: a menu item does what the
// port's key for it does. Defined in debug/menu_bar.cpp.
void press(SDL_Keycode key);

// The key that opens and closes the menu, as the player reads it ("`", "Alt", "F4"), and its
// place on the keyboard (SDL_SCANCODE_UNKNOWN for Alt, which counts only pressed by itself).
// Defined in debug/menu_bar.cpp.
std::string menu_bar_key_label();
SDL_Scancode menu_bar_scancode();

// The controller button that does the same (SDL_GAMEPAD_BUTTON_INVALID for none and for Start+Select),
// whether it is Start+Select pressed together (the default, platform/menu_chord.h), and its name as
// the player reads it ("Start+Select", "R4", "none"). Defined in debug/menu_bar.cpp.
SDL_GamepadButton menu_bar_button();
bool menu_bar_chord();
std::string menu_bar_button_label();

// Opens or closes the menu. Defined in debug/menu_bar.cpp.
void toggle_menu();

// Moves setting `row` (options_menu.h) on by `by`, saves the settings and marks them changed for
// the window to apply (take_settings_changed). A picture shape of the player's own is the settings
// file's, so moving the aspect skips it. Defined in debug/controls.cpp.
void change_setting(int row, int by);

// Puts the Enhanced graphics settings and the session's material, water and fog tuning back to
// their defaults, for the Enhanced Settings window and the drawer. Defined in debug/controls.cpp.
void reset_enhanced();

// Draws the independent Enhanced Settings window with launcher-ordered main choices and advanced
// tuning, diagnostics and water sections; disabled where the level has no compatible probe data. A
// change is saved and marked for the window to apply. Defined in debug/controls.cpp.
void enhanced_settings_window();

// ---- The performance capture (common/perf_capture.h) ----

// Starts a capture of s.capture_seconds, or stops the one running, or calls off the one about to
// start. `after_drawer`: it starts three seconds after the drawer has gone. Defined in
// debug/capture.cpp.
void toggle_capture(bool after_drawer);

// A settings sweep: a capture that changes the graphics settings every few seconds (VSync off) and
// gives them back at its end, its report listing each step's frame time. Started like
// toggle_capture(true). Defined in debug/capture.cpp.
void toggle_settings_sweep();
double settings_sweep_seconds();

// The capture's length as the drawer shows it ("30 s", "Until stopped"), and moved on by `by`.
const char* capture_length_name();
void step_capture_length(int by);

// The simulated slow processor's factor as the drawer and the menu show it ("Off", "4x"), and moved
// on by `by` (common/slow_cpu.h; only where slow_cpu::available()).
std::string slow_cpu_name();
void step_slow_cpu(int by);

// Opens the folder the recordings are saved in (capture.cpp's capture_dir) in the system's file
// manager.
void open_capture_folder();

// Says whether a capture runs or is about to start (or one is due by script), so that the
// overlay is drawn for its badge; and whether one is counting down to its start.
bool capture_busy();
bool capture_pending();

// The notification of the capture ("REC 12 / 30 s"); empty when none runs.
std::string capture_badge();

// Once a picture, before the overlay is drawn: starts a capture whose time has come, ends one when
// the drawer opens, and writes, copies and announces a finished one. Defined in debug/capture.cpp.
void capture_frame(const Shown& shown);

// ---- What is drawn over the game's picture ----

// Draws the marks over the game's picture (`shown`): a ring round the selected moby and the
// mobys' labels (s.labels, within s.radius of the hero). The click picker and bottom
// details are separate in debug/picker.cpp.
void picture_marks(const Shown& shown);
// Draws unified material authoring controls after the window-thread handoff.
void material_settings_window(const Shown& shown);
void release_material_preview();
void picker_overlay(const Shown& shown);
// Reads one native input action; the returned comma begins the next frame/action pair.
Result<const char*> read_scripted_action(const char*, ScriptedInput&);
// Delivers due held/drag actions only inside the game, without moving the operating system cursor.
void refresh_scripted_pointer();
void run_script(std::uint64_t frame, int held);
void release_scripted_pointer(const SDL_Event& event);
// Initial inline actions, or an explicit @file whose append-only actions are polled only in diagnostics.
void configure_scripted_input(std::string_view text);
// Inspect the click selection without borrowing game graphics objects or moving its camera.
void select_object_viewer(const Shown& shown);
// Draw the developer-only studio after the picker has updated its selection.
void object_viewer_window(const Shown& shown);
// Release private studio resources while the graphics context and ImGui are still alive.
void release_object_viewer();

// Keeps the notifications of what stands (debug_overlay.h, Notification) up to date, whether the
// panel is shown or not: the pause and the speed the keys have set, a recording, a mod's line.
// Defined in debug/picture.cpp.
void status_notifications();

// Draws the notifications, over everything else (debug/notifications.cpp); says whether any is
// shown or going.
void draw_notifications();
// A press (click or tap) at `at`, in ImGui's coordinates: true when a card took it.
bool press_notification(ImVec2 at);
bool notifications_pending();

// The hint at the start: what opens the menu. Defined in
// debug/notifications.cpp.
void notify_menu_hint();

// Draws one line of text, formatted as std::format does.
template <typename... Args>
void text(std::format_string<Args...> fmt, Args&&... args) {
    ImGui::TextUnformatted(std::format(fmt, std::forward<Args>(args)...).c_str());
}

// Draws one line of text in `colour`, formatted as std::format does.
template <typename... Args>
void text_coloured(const ImVec4& colour, std::format_string<Args...> fmt, Args&&... args) {
    ImGui::PushStyleColor(ImGuiCol_Text, colour);
    ImGui::TextUnformatted(std::format(fmt, std::forward<Args>(args)...).c_str());
    ImGui::PopStyleColor();
}

// Returns the T at `address` in the game's main memory (its segment bits ignored), or a T of
// zeros when it is not all inside.
template <typename T>
T read(std::uint32_t address) {
    T value{};
    address = openrac_guest_ram_address_for_size(address, static_cast<std::uint32_t>(s.ram.size()));

    if (static_cast<std::size_t>(address) + sizeof(T) <= s.ram.size()) {
        std::memcpy(&value, s.ram.data() + address, sizeof(T));
    }

    return value;
}

}  // namespace openrac::frontend::debug_overlay

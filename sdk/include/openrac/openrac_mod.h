/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The interface between OpenRAC's native port and a native mod (docs/port/MODS.md).
 *
 * A mod is a shared library (.dylib, .so, .dll) that exports openrac_mod_query and openrac_mod_load,
 * and optionally openrac_mod_unload. The port opens it at start, before the game runs, asks
 * openrac_mod_query which interface it was built for, and calls openrac_mod_load with the table of
 * what the port offers (openrac_api). The mod uses nothing of the port but that table: it is built
 * against this header alone, and links against nothing of OpenRAC's.
 *
 * Versions. OPENRAC_MOD_API_MAJOR changes when anything in the table changes or goes: a mod built
 * for another major is refused. OPENRAC_MOD_API_MINOR grows when something is added at the end of
 * the table: a mod built for a minor above the port's is refused, one built for a lower minor loads
 * (and checks api->api_minor before it uses anything newer than what it was built for).
 *
 * Everything runs on the game's thread: load, the hooks, the callbacks, unload.
 *
 * This header is plain C (C99, and C++ through its extern "C" block): it is the ABI, and the C++
 * layer (openrac/mod.hpp) is built on it. It is laid out as: versions, helper macros, types,
 * the input state, the table (openrac_api), the entry points, then the helpers a mod uses.
 * Each item says which interface version added it ("Since 1.0" is the original table); a mod
 * checks api->api_minor before it uses anything newer than the minor it was built for.
 */
#ifndef OPENRAC_MOD_H
#define OPENRAC_MOD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------ *
 * Versions
 * ------------------------------------------------------------------------------------------ */

/* The interface's major version. Changes when anything in the table changes or goes. */
#define OPENRAC_MOD_API_MAJOR 1

/* The interface's minor version, which this header describes. It grows when
 * something is added at the end of the table. 1.1 added script mods (the table
 * is unchanged), 1.2 the keyboard and mouse, 1.3 the pointer over the game's
 * picture (openrac_input.pointer_*), 1.4 the graphics context, extra views of
 * the game and a Vulkan device made with the mod (a VR headset, docs/views.md),
 * 1.5 entries in the game's front-end menu with a page of the mod's own, 1.6 the
 * tick-boundary callback and warm in-session snapshot calls, 1.7 pages of the game's own menus
 * (its frames, list, text, prompt and sounds) and items of the title's main menu, 1.8 play
 * (multiplayer's calls): the game's phase and level, the hero's pose, figures that look like the
 * hero, a game started or a level travelled to from code, and the player's saves, 1.9 the world:
 * the level's mobys listed, driven by a mod, posed, killed and made, a gate around each moby's
 * update with the hero standing in for another player, hits watched and posted, eyes that keep
 * mobys running, and the bolts.
 */
#define OPENRAC_MOD_API_MINOR 9

/* The interface a mod is built for, what its openrac_mod_query reports: its manifest's "api" minor,
 * which the SDK's openrac_add_mod passes in, else this header's. A mod for 1.1 built with a newer
 * SDK thus still loads on a port of 1.1 (and uses nothing newer without checking api->api_minor). */
#ifndef OPENRAC_MOD_TARGET_MINOR
#define OPENRAC_MOD_TARGET_MINOR OPENRAC_MOD_API_MINOR
#endif

/* Marks an entry point of the mod's library as exported, whatever the build's default visibility.
 * Since 1.0. */
#if defined(_WIN32)
#define OPENRAC_MOD_EXPORT __declspec(dllexport)
#else
#define OPENRAC_MOD_EXPORT __attribute__((visibility("default")))
#endif

/* ------------------------------------------------------------------------------------------ *
 * Basic types and constants
 * ------------------------------------------------------------------------------------------ */

/* A game address: an offset into game memory (the console's address). Since 1.0. */
typedef uint32_t openrac_gaddr;

/* The port's handle for one loaded mod, passed back to the functions that are the mod's own (its
 * log lines, its settings, its hooks). Opaque; valid from openrac_mod_load until the program ends.
 * Since 1.0. */
typedef struct openrac_mod openrac_mod;

/* The levels of openrac_api.log, from the most to the least talkative. Since 1.0. */
#define OPENRAC_LOG_DEBUG 0
#define OPENRAC_LOG_INFO 1
#define OPENRAC_LOG_WARN 2
#define OPENRAC_LOG_ERROR 3

/* The results of openrac_api.hook. Since 1.0. */
#define OPENRAC_HOOK_OK 0
/* This port has no hook point of that name (on this disc). */
#define OPENRAC_HOOK_UNKNOWN (-1)
/* The hook point's signature is another than the one asked for. */
#define OPENRAC_HOOK_SIGNATURE (-2)
/* Hooks are taken in openrac_mod_load only. */
#define OPENRAC_HOOK_TOO_LATE (-3)
/* Another mod has it already: the views (on_views) and the Vulkan device
 * (setup_vulkan) are one mod's each. Since 1.4. */
#define OPENRAC_HOOK_TAKEN (-4)

/* ------------------------------------------------------------------------------------------ *
 * Reporting to the port
 * ------------------------------------------------------------------------------------------ */

/* What openrac_mod_query tells the port before it loads the mod. The port sets `size`; the mod
 * sets the rest (OPENRAC_MOD_DEFINE does). Since 1.0. */
typedef struct openrac_mod_info {
    /* sizeof(openrac_mod_info), set by the port. */
    uint32_t size;
    /* OPENRAC_MOD_API_MAJOR as the mod was built. */
    uint32_t api_major;
    /* OPENRAC_MOD_TARGET_MINOR as the mod was built. */
    uint32_t api_minor;
    /* The manifest's id: the port checks they agree. Static storage in the mod. */
    const char* id;
    /* The mod's version, as its manifest's. Static storage in the mod. */
    const char* version;
} openrac_mod_info;

/* ------------------------------------------------------------------------------------------ *
 * The keyboard and mouse (since 1.2)
 * ------------------------------------------------------------------------------------------ */

/* The number of entries of openrac_input.keys. A key is its USB HID usage, the same on every layout
 * and system (openrac/keys.h names them: "KeyW", "Space", "ShiftLeft"). Since 1.2. */
#define OPENRAC_KEY_COUNT 512

/* The key is down: a bit of an openrac_input.keys[k] entry. Since 1.2. */
#define OPENRAC_KEY_HELD 1u
/* The key went down since the last tick (a tap shorter than a tick too): a bit of an
 * openrac_input.keys[k] entry. Since 1.2. */
#define OPENRAC_KEY_PRESSED 2u

/* The bits of openrac_input.mouse_held and mouse_pressed. Since 1.2. */
#define OPENRAC_MOUSE_LEFT 1u
#define OPENRAC_MOUSE_MIDDLE 2u
#define OPENRAC_MOUSE_RIGHT 4u
/* The side button nearer the wrist (X1). */
#define OPENRAC_MOUSE_BACK 8u
/* The other side button (X2). */
#define OPENRAC_MOUSE_FORWARD 16u

/* A device of openrac_api.take_input: the port's own keyboard table stops driving pad 0. Since
 * 1.2. */
#define OPENRAC_INPUT_KEYBOARD 1u
/* A device of openrac_api.take_input: the mod uses the mouse in play. Reserved: the port has no
 * mouse use of its own in play. Since 1.2. */
#define OPENRAC_INPUT_MOUSE 2u

/* The keyboard and mouse as the port read them for the tick about to run: taken once a tick, after
 * the window's events and before the game reads its pads (so before the tick's pad reports).
 * Read it through openrac_api.input; it is the port's, and valid until the next tick.
 *
 * Since 1.2; the fields from `pointer_x` on are 1.3's, and exist only when `size` reaches them. */
typedef struct openrac_input {
    /* sizeof(openrac_input) as the port was built: a newer port's is larger. */
    uint32_t size;
    /* Counts the ticks: a new value, a new state. */
    uint32_t tick;
    /* By HID usage: OPENRAC_KEY_HELD | OPENRAC_KEY_PRESSED. */
    uint8_t keys[OPENRAC_KEY_COUNT];
    /* The OPENRAC_MOUSE_* buttons down now. */
    uint32_t mouse_held;
    /* The OPENRAC_MOUSE_* buttons that went down since the last tick. */
    uint32_t mouse_pressed;
    /* The mouse's motion since the last tick, in the system's pixels: raw (no pointer acceleration)
     * while captured; y grows downward. */
    float mouse_dx;
    float mouse_dy;
    /* The wheel since the last tick, in notches: x > 0 to the right, y > 0 away from the player
     * (up), whatever the system's scroll direction. */
    float wheel_x;
    float wheel_y;
    /* 1 when the game's window has the keyboard; while not, nothing is held. */
    uint8_t focused;
    /* 1 when the cursor is captured: hidden, kept in the window, motion relative. */
    uint8_t captured;
    /* 1 when the port's own overlay or menu has the input now (F3): leave it alone. */
    uint8_t overlay;
    /* Padding; zero. */
    uint8_t reserved;
    /* Since 1.3 (check `size` first): where the cursor is over the game's 4:3 picture, 0..1 across
     * and down (outside it below 0 or above 1), as the game's 2D screen of 512 by 416 pixels has
     * it: x * 512, y * 416 is the game's pixel. Meaningful while the cursor is not captured. */
    float pointer_x;
    float pointer_y;
    /* Since 1.3: 1 when the cursor is over the picture, in the focused window. */
    uint8_t pointer_inside;
    /* Padding; zero. */
    uint8_t reserved2[3];
} openrac_input;

/* ------------------------------------------------------------------------------------------
 * * The graphics context (since 1.4)
 * ------------------------------------------------------------------------------------------
 */

/* The values of openrac_graphics.backend. Since 1.4. */
/* No window yet, or none at all (a test, a server). */
#define OPENRAC_GRAPHICS_NONE 0
/* The desktop's OpenGL 4.1 core profile. */
#define OPENRAC_GRAPHICS_OPENGL 1
/* OpenGL ES 3.2 (Android, or OPENRAC_GL_API=es). */
#define OPENRAC_GRAPHICS_OPENGL_ES 2
/* The port's Vulkan renderer (OPENRAC_GL_API=vulkan). */
#define OPENRAC_GRAPHICS_VULKAN 3

/* What the port draws with, for a mod that hands the game's pictures to another
 * API (OpenXR). Read it through openrac_api.graphics; it is the port's, and
 * valid while the window is open. Vulkan handles are given as pointers
 * (VkInstance, VkPhysicalDevice, VkDevice and VkQueue are dispatchable
 * handles), so that this header needs no Vulkan header. Since 1.4. */
typedef struct openrac_graphics {
  /* sizeof(openrac_graphics) as the port was built: a newer port's is larger.
   */
  uint32_t size;
  /* An OPENRAC_GRAPHICS_* value. */
  uint32_t backend;
  /* OpenGL and OpenGL ES: the context's functions by name
   * (SDL_GL_GetProcAddress); null for Vulkan and without a window. The context
   * is current on the game's thread. */
  void *(*gl_proc)(const char *name);
  /* Vulkan: the backend's instance, physical device, device and graphics queue,
   * its family and its index in the family, and the vkGetInstanceProcAddr they
   * were made with; null and 0 otherwise. While a mod has views (on_views) the
   * port submits to the queue on the game's thread only, so a mod's own use of
   * it there (OpenXR's xrEndFrame) needs no lock. */
  void *vk_instance;
  void *vk_physical_device;
  void *vk_device;
  void *vk_queue;
  uint32_t vk_queue_family;
  uint32_t vk_queue_index;
  void *vk_get_instance_proc_addr;
} openrac_graphics;

/* ------------------------------------------------------------------------------------------
 * * Views of the game (since 1.4)
 * ------------------------------------------------------------------------------------------
 */

/* The most views a frame draws (openrac_view_source.begin's `max`). Since 1.4.
 */
#define OPENRAC_VIEW_MAX 4

/* The values of openrac_view.kind: what a view draws. Since 1.4. */
/* The game's whole picture: the world, with the 2D (HUD, text, menus, fades)
 * over it at the game's 4:3 shape in its middle. */
#define OPENRAC_VIEW_FULL 0
/* The world alone, without the 2D. */
#define OPENRAC_VIEW_WORLD 1
/* The 2D alone, over transparent black (alpha 0 where nothing is drawn): for a
 * panel. Give it the game's 4:3 shape; the eye pose and field of view are not
 * used. */
#define OPENRAC_VIEW_2D 2

/* One view the port draws in a frame besides the window's: an eye of a headset,
 * a panel. The mod fills the members up to `image_layout` in
 * openrac_view_source.begin; the port sets the ones after as it draws.
 * Since 1.4. */
typedef struct openrac_view {
  /* An OPENRAC_VIEW_* value. */
  uint32_t kind;
  /* The picture's size in pixels, each from 1 to 8192. */
  int32_t width;
  int32_t height;
  /* The eye relative to the game's camera, in the camera's view space (OpenXR's
   * and OpenGL's: X right, Y up, looking down -Z), in the game's units: a
   * position, and an orientation as a quaternion x, y, z, w. The identity (0 0
   * 0, 0 0 0 1) is the game's own camera; an all-zero orientation counts as the
   * identity. */
  float position[3];
  float orientation[4];
  /* The field of view as OpenXR's XrFovf gives it: the angles of its left,
   * right, top and bottom edges from straight ahead, in radians (left and down
   * negative). All zero: the game's own vertical one, at the view's shape. */
  float angle_left;
  float angle_right;
  float angle_up;
  float angle_down;
  /* Where the picture goes, in the backend's own terms (openrac_api.graphics),
   * copied as its bytes with no conversion of colour (an sRGB image gets the
   * game's picture as it is shown):
   * - OpenGL: `texture`, a GL_TEXTURE_2D of the port's context, RGBA8 or
   * SRGB8_ALPHA8, at least width by height (an OpenXR swapchain image);
   * - Vulkan: `image`, a VkImage of the port's device, VK_FORMAT_R8G8B8A8_UNORM
   * or _SRGB, exactly width by height, usable as a transfer destination, which
   * the port finds in `image_layout` (a VkImageLayout) and leaves in it. Zero
   * and null: the picture is drawn only into the port's own texture
   * (`picture`). */
  uint32_t texture;
  void *image;
  uint32_t image_layout;
  /* Set by the port: 1 when this frame drew the view anew; 0 when it showed the
   * last frame's picture again (a frame the port draws no world in, as the
   * pause menu's repeated frames), and then that picture is copied again. */
  uint32_t drawn;
  /* Set by the port: its own GL texture holding the view (with OpenGL), until
   * the next frame. */
  uint32_t picture;
  /* Padding; zero. */
  uint32_t reserved[3];
} openrac_view;

/* The functions a mod gives on_views. The port copies the structure; `user` is
 * passed back to each. Every function is called on the game's thread, with the
 * graphics context current. Since 1.4. */
typedef struct openrac_view_source {
  /* sizeof(openrac_view_source) as the mod was built. */
  uint32_t size;
  void *user;
  /* Called in each frame the port draws the game's scene (each refresh, between
   * the game's ticks too), after the window's picture is recorded: fills
   * views[0] to views[n - 1] (n at most `max`) and returns n; 0 draws none this
   * frame. The place for OpenXR's xrWaitFrame, xrBeginFrame, xrLocateViews and
   * xrAcquireSwapchainImage. Frames that show a movie or a still picture
   * instead of the scene do not call it. */
  int (*begin)(void *user, openrac_view *views, int max);
  /* Called once the frame that drew them is submitted to the GPU (after the
   * window's picture is shown), with the views `begin` gave and what the port
   * set in them: the place for xrReleaseSwapchainImage and xrEndFrame. Called
   * after every `begin`, even one that returned 0 or a frame that drew nothing.
   */
  void (*end)(void *user, const openrac_view *views, int count);
  /* Called once before the graphics context goes (the window closes): let go of
   * everything made with it (OpenXR's session and swapchains). Optional. */
  void (*closing)(void *user);
} openrac_view_source;

/* What a mod gives setup_vulkan so that the port's Vulkan device is one the mod
 * can share (OpenXR's XR_KHR_vulkan_enable2: xrCreateVulkanInstanceKHR,
 * xrGetVulkanGraphicsDevice2KHR, xrCreateVulkanDeviceKHR). The port copies the
 * structure; `user` is passed back to each. The functions are called when the
 * window opens, after openrac_mod_load, on the game's thread: the mod's
 * manifest says "vulkan_device": true so that it loads before the window.
 * Pointers stand for the Vulkan types named, and int32_t for VkResult.
 * Since 1.4. */
typedef struct openrac_vulkan_setup {
  /* sizeof(openrac_vulkan_setup) as the mod was built. */
  uint32_t size;
  void *user;
  /* Makes the instance from `create_info` (a const VkInstanceCreateInfo*, the
   * port's: the extensions and the API version it needs) in place of
   * vkCreateInstance, through `get_instance_proc_addr` (a
   * PFN_vkGetInstanceProcAddr), and sets `*instance`. Returns a VkResult;
   * anything but VK_SUCCESS (0) and the port draws with OpenGL instead.
   * Optional. */
  int32_t (*create_instance)(void *user, void *get_instance_proc_addr,
                             const void *create_info, void **instance);
  /* Returns the VkPhysicalDevice to draw on, of `instance`; null lets the port
   * choose. The port refuses a device it cannot draw with (and draws with
   * OpenGL instead). Optional. */
  void *(*physical_device)(void *user, void *instance);
  /* Makes the device from `create_info` (a const VkDeviceCreateInfo*, the
   * port's) on `physical_device` in place of vkCreateDevice, and sets
   * `*device`. Returns a VkResult, as create_instance. Optional. */
  int32_t (*create_device)(void *user, void *get_instance_proc_addr,
                           void *physical_device, const void *create_info,
                           void **device);
} openrac_vulkan_setup;

/* ------------------------------------------------------------------------------------------ *
 * Menu pages (since 1.5)
 * ------------------------------------------------------------------------------------------ */

/* The most entries the mods together add to the front-end menu (openrac_api.menu_entry).
 * Since 1.5. */
#define OPENRAC_MENU_ENTRIES_MAX 4

/* A menu page's own pixels: the game's 2D picture, 512 across and 448 down, (0, 0) its top left,
 * shown stretched over the whole window. Since 1.5. */
#define OPENRAC_MENU_WIDTH 512
#define OPENRAC_MENU_HEIGHT 448

/* A character of openrac_api.menu_text at size 1: 6 pixels across (a 5 by 7 glyph and a column
 * between), 8 down; at size n each is n times that. Letters are drawn as capitals; a character
 * the font lacks is a blank. Since 1.5. */
#define OPENRAC_MENU_CHAR_WIDTH 6
#define OPENRAC_MENU_CHAR_HEIGHT 8

/* A colour of a menu page's drawing, 0xRRGGBBAA: alpha 255 covers what is under, 0 leaves it.
 * Since 1.5. */
#define OPENRAC_RGBA(r, g, b, a)                                                                   \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

/* The pad's buttons in openrac_menu_frame.held and .pressed: pad 0's (the keyboard's keys the port
 * maps to it included), one bit each, set while down. Since 1.5. */
#define OPENRAC_PAD_SELECT 0x0001u
#define OPENRAC_PAD_L3 0x0002u
#define OPENRAC_PAD_R3 0x0004u
#define OPENRAC_PAD_START 0x0008u
#define OPENRAC_PAD_UP 0x0010u
#define OPENRAC_PAD_RIGHT 0x0020u
#define OPENRAC_PAD_DOWN 0x0040u
#define OPENRAC_PAD_LEFT 0x0080u
#define OPENRAC_PAD_L2 0x0100u
#define OPENRAC_PAD_R2 0x0200u
#define OPENRAC_PAD_L1 0x0400u
#define OPENRAC_PAD_R1 0x0800u
#define OPENRAC_PAD_TRIANGLE 0x1000u
#define OPENRAC_PAD_CIRCLE 0x2000u
#define OPENRAC_PAD_CROSS 0x4000u
#define OPENRAC_PAD_SQUARE 0x8000u

/* What a menu page's callback is given each frame it is open. The port's; valid during the call.
 * Since 1.5. */
typedef struct openrac_menu_frame {
    /* sizeof(openrac_menu_frame) as the port was built: a newer port's is larger. */
    uint32_t size;
    /* The OPENRAC_PAD_* buttons down now. */
    uint32_t held;
    /* The OPENRAC_PAD_* buttons that went down since the last call. */
    uint32_t pressed;
    /* 1 on the first call after the page opened, 0 after. */
    uint32_t opened;
} openrac_menu_frame;

/* What a menu page's callback returns: the page stays open, or closes and the game's menu it was
 * opened from comes back (as the game's own back button does). Since 1.5. */
#define OPENRAC_MENU_STAY 0
#define OPENRAC_MENU_CLOSE 1

/* A mod's menu page (openrac_api.menu_entry): called once a frame while the page is open, on the
 * game's thread, with the `user` pointer the mod gave. It draws the page with menu_rect and
 * menu_text (the page starts each frame black) and returns OPENRAC_MENU_STAY or
 * OPENRAC_MENU_CLOSE. While the page is open the game sees no button of pad 0. Since 1.5. */
typedef int (*openrac_menu_fn)(void* user, const openrac_menu_frame* frame);

/* ------------------------------------------------------------------------------------------ *
 * Pages of the game's own menus (since 1.7)
 *
 * A page here is drawn by the game's own front end: its frames, its title, its list in its font,
 * its footer of hints, its sounds. The mod says what is on it (openrac_api.ui_set) and hears what
 * the player does (openrac_ui_fn). Ratchet & Clank: a page opened from an item of the title's main
 * menu (openrac_api.ui_main_item) has the Options page's frames; a page opened from that one has
 * the Language page's, a level down, and opening another from there takes its place. Up and Down
 * move the cursor (over the items the cursor can take), Cross chooses, Left and Right step a
 * choice, Triangle goes back a level. A game whose front end the port does not know shows none.
 * ------------------------------------------------------------------------------------------ */

/* The most pages the mods together make, items a page lists, footer lines, choices an item steps
 * through, and characters a text keeps (longer ones are cut). Since 1.7. */
#define OPENRAC_UI_PAGES_MAX 16
#define OPENRAC_UI_ITEMS_MAX 12
#define OPENRAC_UI_FOOTER_MAX 3
#define OPENRAC_UI_CHOICES_MAX 64
#define OPENRAC_UI_TEXT_MAX 47

/* The most items the mods together add to the main menu (openrac_api.ui_main_item): Ratchet &
 * Clank's has room for two rows below its own three. Since 1.7. */
#define OPENRAC_UI_MAIN_ITEMS_MAX 2

/* The most items the mods together add to the pause menu's Options page (openrac_api.ui_pause_item).
 * Since 1.9. */
#define OPENRAC_UI_PAUSE_ITEMS_MAX 2

/* The main menu's own items, for openrac_api.ui_main_item's `after`. Since 1.7. */
#define OPENRAC_UI_NEW_GAME "new_game"
#define OPENRAC_UI_LOAD_GAME "load_game"
#define OPENRAC_UI_OPTIONS "options"

/* The pad's buttons as the game's font draws them, for a text: OPENRAC_UI_CROSS " Join". Since
 * 1.7. */
#define OPENRAC_UI_CROSS "\x10"
#define OPENRAC_UI_CIRCLE "\x11"
#define OPENRAC_UI_TRIANGLE "\x12"
#define OPENRAC_UI_SQUARE "\x13"

/* What an item is (openrac_ui_item.kind). Since 1.7. */
/* A line Cross chooses: OPENRAC_UI_CHOOSE. */
#define OPENRAC_UI_BUTTON 0u
/* A line with a value among `choices`: Left and Right step it back and on, Cross on (around the
 * ends), each an OPENRAC_UI_CHANGE. Shown as the label and the value. */
#define OPENRAC_UI_CHOICE 1u
/* A line of text (a status, a name): the cursor passes over it. */
#define OPENRAC_UI_TEXT 2u

/* An item's flags (openrac_ui_item.flags). Since 1.7. */
/* Drawn dim, as the game's locked entries; the cursor passes over it, and choosing it plays the
 * game's refusal, and no event. */
#define OPENRAC_UI_DISABLED 0x1u

/* A page's flags (openrac_api.ui_page). Since 1.7. */
/* Triangle comes to the mod as OPENRAC_UI_BACK instead of going back: the mod goes back itself
 * (openrac_api.ui_open with OPENRAC_UI_GO_BACK), for one that asks first. */
#define OPENRAC_UI_OWN_BACK 0x1u

/* One item of a page (openrac_ui_content.items). The mod's; read during openrac_api.ui_set.
 * Since 1.7. */
typedef struct openrac_ui_item {
    /* OPENRAC_UI_BUTTON, OPENRAC_UI_CHOICE or OPENRAC_UI_TEXT. */
    uint32_t kind;
    /* OPENRAC_UI_DISABLED, or 0. */
    uint32_t flags;
    /* What the line says (UTF-8: ASCII and the game's accented letters; the OPENRAC_UI_CROSS ...
     * glyphs). */
    const char* label;
    /* OPENRAC_UI_CHOICE: the values (`choice_count` of them, up to OPENRAC_UI_CHOICES_MAX) and
     * the one shown. Unused otherwise. */
    const char* const* choices;
    uint32_t choice_count;
    uint32_t choice;
} openrac_ui_item;

/* What a page shows (openrac_api.ui_set). The mod's; read during the call. Since 1.7. */
typedef struct openrac_ui_content {
    /* sizeof(openrac_ui_content) as the mod was built. */
    uint32_t size;
    /* The title, in the page's title frame. */
    const char* title;
    /* The list, `item_count` items (up to OPENRAC_UI_ITEMS_MAX). */
    const openrac_ui_item* items;
    uint32_t item_count;
    /* The footer's lines (up to OPENRAC_UI_FOOTER_MAX); none: the game's own hints (Select,
     * Exit, in the game's language). */
    const char* const* footer;
    uint32_t footer_count;
} openrac_ui_content;

/* What happened on a page (openrac_ui_event.kind). Since 1.7. */
/* The page came on the screen (from the main menu, ui_open, or back from a page under it). */
#define OPENRAC_UI_SHOW 0u
/* Each tick the page is on the screen, after the tick's other events. */
#define OPENRAC_UI_FRAME 1u
/* Cross on a button that is not disabled: `item`. */
#define OPENRAC_UI_CHOOSE 2u
/* A choice stepped: `item`, `value` its new choice. */
#define OPENRAC_UI_CHANGE 3u
/* Triangle on a page made with OPENRAC_UI_OWN_BACK. */
#define OPENRAC_UI_BACK 4u
/* The question of ui_confirm answered: `value` 1 for yes (Cross), 0 for no (Triangle, or the
 * page left), `tag` the one the mod gave. */
#define OPENRAC_UI_ANSWER 5u
/* The page left the screen. */
#define OPENRAC_UI_HIDE 6u

/* An event of a page (openrac_ui_fn). The port's; valid during the call. Since 1.7. */
typedef struct openrac_ui_event {
    /* sizeof(openrac_ui_event) as the port was built: a newer port's is larger. */
    uint32_t size;
    /* OPENRAC_UI_SHOW ... OPENRAC_UI_HIDE. */
    uint32_t kind;
    /* The item it is about (CHOOSE, CHANGE), else -1. */
    int32_t item;
    /* CHANGE: the new choice; ANSWER: 1 yes, 0 no; else 0. */
    int32_t value;
    /* ANSWER: the question's tag; else 0. */
    uint32_t tag;
    /* The item under the cursor. */
    int32_t cursor;
} openrac_ui_event;

/* A mod's page (openrac_api.ui_page): called on the game's thread with the `user` pointer the mod
 * gave, the page's id and each event. The calls of the table work from here (ui_set, ui_open, ...);
 * what they ask for happens after the call. Since 1.7. */
typedef void (*openrac_ui_fn)(void* user, int page, const openrac_ui_event* event);

/* openrac_api.ui_open's page that goes back a level, as Triangle does. Since 1.7. */
#define OPENRAC_UI_GO_BACK (-1)

/* The game's menu sounds (openrac_api.ui_sound). The port plays the cursor's, a choice's and a
 * refusal's itself. Since 1.7. */
#define OPENRAC_UI_SOUND_SELECT 0
#define OPENRAC_UI_SOUND_CURSOR 1
#define OPENRAC_UI_SOUND_DENIED 2
#define OPENRAC_UI_SOUND_OPEN 3
#define OPENRAC_UI_SOUND_PAGE 4

/* ------------------------------------------------------------------------------------------ *
 * Play: multiplayer's calls (since 1.8)
 *
 * What a mod that plays the game with others needs of it: what the game is doing (its phase and
 * level), the hero's pose to send, figures to show the others with (mobys that look like the hero,
 * driven by the mod: they never run the hero's logic, collide with nothing and are no target), a
 * new game or a save started from code and a level travelled to, and the player's saves. Ratchet &
 * Clank has them (PAL and NTSC-U); another game answers that it has none (OPENRAC_PLAY_NONE,
 * OPENRAC_HOOK_UNKNOWN).
 * ------------------------------------------------------------------------------------------ */

/* What the game is doing (openrac_play_status.phase). Since 1.8. */
/* The port knows nothing of this game's play: the play calls do nothing. */
#define OPENRAC_PLAY_NONE 0
/* The title and its menus: a game can be started (openrac_api.play_start). */
#define OPENRAC_PLAY_FRONT_END 1
/* Between two programs: a level loading, the flight between planets. */
#define OPENRAC_PLAY_LOADING 2
/* A level, the player in control: a level can be travelled to (openrac_api.play_travel). */
#define OPENRAC_PLAY_PLAYING 3
/* A level, a scene playing: a cutscene, a movie, a slideshow, the ship's landing or take-off. */
#define OPENRAC_PLAY_SCENE 4
/* A level, a menu over it: paused, a vendor, a dialog. */
#define OPENRAC_PLAY_MENU 5

/* What the game is doing (openrac_api.play_status). The port's; filled by the call. Since 1.8. */
typedef struct openrac_play_status {
    /* sizeof(openrac_play_status), set by the mod before the call. */
    uint32_t size;
    /* OPENRAC_PLAY_NONE ... OPENRAC_PLAY_MENU. */
    int32_t phase;
    /* The level played (the game's own number), or the one loading; -1 for none (the front end). */
    int32_t level;
    /* How many levels the game has (0 .. levels - 1). */
    int32_t levels;
    /* Counts the level programs loaded since the start: a new value, a new level (or the same one
     * loaded again), whose figures are gone. */
    uint32_t loads;
} openrac_play_status;

/* openrac_pose.flags. Since 1.8. */
/* The animation is blending into `seq` from another: `frame` is where it arrives. */
#define OPENRAC_POSE_BLENDING 0x1u

/* A pose of the hero, or of a figure (openrac_api.hero_pose, figure_pose). Units are the game's.
 * Since 1.8. */
typedef struct openrac_pose {
    /* sizeof(openrac_pose), set by the mod. */
    uint32_t size;
    /* The level the pose is in. */
    int32_t level;
    /* Where the body is drawn, its rotation (radians, the game's axes: z the yaw) and its scale. */
    float position[3];
    float rotation[3];
    float scale;
    /* How fast it moves, in game units a second. */
    float velocity[3];
    /* The hero's state (the game's own ids), its health, and the item it holds (-1 none). */
    int32_t state;
    int32_t health;
    int32_t item;
    /* The animation: the sequence of the hero's class, the key it is at, the key after, how far
     * from one to the other (0 to 1), and its speed (1 normal, 0 held). OPENRAC_POSE_* flags. */
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
    uint32_t flags;
} openrac_pose;

/* ------------------------------------------------------------------------------------------ *
 * The world: the level's mobys (since 1.9)
 *
 * For a mod that shares the world between games: one game runs it (its enemies, crates, doors,
 * platforms), the others drive their copies from it. A moby is named by its slot (its index in
 * the level's moby array, this game's own) and, when the level placed it, by its key (its record in
 * the level's file: the same moby has the same key in every game that loads the level, whatever
 * was killed before). A driven moby's own update no longer runs (its animation still plays and
 * its collision follows it): the mod poses it. The gate (on_moby_update) is asked around each
 * update the game runs; before one, the mod may have the hero stand in elsewhere for it (another
 * player: the enemy then sees, chases and hits that player's figure). Hits the game posts are told
 * to a mod (on_moby_hit), which may take them back, and a mod posts its own (moby_hit).
 * ------------------------------------------------------------------------------------------ */

/* openrac_moby.flags. Since 1.9. */
/* It has an update of its own (the game runs it near the camera). */
#define OPENRAC_MOBY_UPDATES 0x1u
/* A mod drives it (moby_drive): its update does not run. */
#define OPENRAC_MOBY_DRIVEN 0x2u
/* The level placed it: it has a key. */
#define OPENRAC_MOBY_PLACED 0x4u
/* Its class is a creature (an enemy, a person). */
#define OPENRAC_MOBY_CREATURE 0x8u
/* A placed moby the game deleted (killed, broken, picked up): it stays dead for the level load,
 * listed so a mod learns of the death from the game itself, never from a moby left out. */
#define OPENRAC_MOBY_DEAD 0x10u
/* An actor of the in-level scene playing (scene_playing): the game made it for the scene and
 * deletes it when the scene ends. Each game playing the scene has its own. */
#define OPENRAC_MOBY_SCENE 0x20u
/* Hidden: drawn by nothing however near, as the game hides one (its update did, or moby_hide),
 * or faded out (alpha 0). */
#define OPENRAC_MOBY_HIDDEN 0x40u
/* Drawn in the last frame (in view, within its draw distance, not hidden). */
#define OPENRAC_MOBY_DRAWN 0x80u

/* A moby of the level (moby_list), or the pose a mod gives one (moby_pose, moby_spawn: position,
 * rotation, scale and the animation are read). Since 1.9. */
typedef struct openrac_moby {
    /* sizeof(openrac_moby), set by the mod. */
    uint32_t size;
    /* Its slot in this game, its key (-1 for one the level did not place), its class. */
    int32_t slot;
    int32_t key;
    int32_t oclass;
    /* OPENRAC_MOBY_* flags, and its state (the game's own byte, below 0x80 while it lives). */
    uint32_t flags;
    uint32_t state;
    /* As openrac_pose's. */
    float position[3];
    float rotation[3];
    float scale;
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
} openrac_moby;

/* How the game deleted a moby of the level (moby_deletion). Since 1.9. */
typedef struct openrac_moby_deletion {
    /* sizeof(openrac_moby_deletion), set by the mod. */
    uint32_t size;
    /* The slot and class of the moby whose update deleted it (itself, or another: a spawner, a
     * manager), -1 for none (the game's own code outside any moby's update: a level's script, a
     * scene ending, the hero's items). */
    int32_t by;
    int32_t by_class;
    /* 1 when a mod deleted it (moby_kill). */
    int32_t by_mod;
    /* Its class then. */
    int32_t oclass;
    /* The level load it happened in (as openrac_play_status' loads). */
    uint32_t loads;
} openrac_moby_deletion;

/* How the last run list (the mobys whose update the game ran in a tick) treated a moby
 * (openrac_moby_run.ran). Since 1.9. */
/* Not looked at: no moby there, a dead one, or one the game never runs (mode bit 2). */
#define OPENRAC_MOBY_RUN_NONE (-1)
/* Left out: farther than its update distance from the camera and from every eye (play_eyes). */
#define OPENRAC_MOBY_RUN_FAR 0
/* Run: drawn in the last frame, its update distance always (255), within it of the camera, within
 * it of an eye, or as a member of a group another member of which was run. */
#define OPENRAC_MOBY_RUN_DRAWN 1
#define OPENRAC_MOBY_RUN_ALWAYS 2
#define OPENRAC_MOBY_RUN_NEAR 3
#define OPENRAC_MOBY_RUN_EYE 4
#define OPENRAC_MOBY_RUN_GROUP 5

/* What the last menu update did with the level's world (openrac_moby_run.menu). Since 1.9. */
/* No menu update in this level load yet. */
#define OPENRAC_RUN_MENU_NONE 0
/* A menu updated with the world held (play_overlay_pause off). */
#define OPENRAC_RUN_MENU_HELD 1
/* The pause menu's first tick: the level's camera saved, the world run with it. */
#define OPENRAC_RUN_MENU_FIRST 2
/* The pause menu: the level's camera saved on its first tick put up for the world's run. */
#define OPENRAC_RUN_MENU_LEVEL_CAMERA 3
/* The pause menu, with no level camera saved in this level load: the menu's camera stays up. */
#define OPENRAC_RUN_MENU_OWN_CAMERA 4
/* A freeze dialog: the world run with the camera up (no swap). */
#define OPENRAC_RUN_MENU_FREEZE 5

/* How the game ran the level's mobys in the last tick, and the moby in a slot (moby_run): for a mod
 * that reports why a moby stopped or left. Since 1.9. */
typedef struct openrac_moby_run {
    /* sizeof(openrac_moby_run), set by the mod. */
    uint32_t size;
    /* Run lists built of the level's mobys in this level load, and the camera the last one
     * measured the update distances from. */
    uint32_t runs;
    float camera[3];
    /* The menu updates seen in this level load (the pause menu's, a freeze dialog's), what the last
     * did (OPENRAC_RUN_MENU_*), and the level's camera the pause menu puts up for the world. */
    uint32_t menu_ticks;
    int32_t menu;
    float level_camera[3];
    /* The level's own game mode (Ratchet & Clank: 0 play, 3 paused, 4 a dialog, 5 a vendor...), and
     * which moby context is current (0 the level's, 1 the pause menu's own). */
    int32_t level_mode;
    int32_t context;
    /* The moby in `slot` (-1 or none: OPENRAC_MOBY_RUN_NONE and zeros): its update distance (255:
     * always), its draw distance, whether it was drawn in the last frame, how the last run list
     * treated it (OPENRAC_MOBY_RUN_*), and its distance from that run list's camera. */
    int32_t update_dist;
    int32_t draw_dist;
    int32_t drawn;
    int32_t ran;
    float distance;
} openrac_moby_run;

/* The gate's moments (openrac_moby_update_fn). Since 1.9. */
#define OPENRAC_MOBY_BEFORE 0
#define OPENRAC_MOBY_AFTER 1

/* The gate (openrac_api.on_moby_update): called with the `user` pointer the mod gave, before and
 * after the update of the moby in `slot` (not for one a mod drives). Before it, returns 1 to skip
 * the update (then no call after), 0 to run it. Since 1.9. */
typedef int (*openrac_moby_update_fn)(void* user, int slot, int phase);

/* A hit (openrac_api.on_moby_hit, moby_hit): to the moby in `victim`, from `attacker` (-1 none), of
 * the game's weapon or attack `mask`, `damage`, at `position` moving at `velocity`. Since 1.9. */
typedef struct openrac_hit {
    /* sizeof(openrac_hit), set by whoever fills it. */
    uint32_t size;
    int32_t victim;
    int32_t attacker;
    uint32_t mask;
    float damage;
    float position[3];
    float velocity[3];
} openrac_hit;

/* Told a hit the game posted (openrac_api.on_moby_hit); returns 1 to take it back (the victim
 * never reads it), 0 to leave it. Since 1.9. */
typedef int (*openrac_hit_fn)(void* user, const openrac_hit* hit);

/* The most eyes (openrac_api.play_eyes). Since 1.9. */
#define OPENRAC_EYES_MAX 8

/* The most figures the mods together have at once (openrac_api.figure_create). Since 1.8. */
#define OPENRAC_FIGURES_MAX 8

/* The pieces the hero wears and holds (openrac_api.hero_held, figure_held): the item in his hand
 * (Ratchet & Clank: the wrench, or the weapon or gadget equipped), and the others the game draws on
 * him (boots, helmet, the pack or Clank on his back). Since 1.9. */
#define OPENRAC_HELD_MAX 8

/* openrac_held_piece.flags. Since 1.9. */
/* Not drawn now (the game hides it). */
#define OPENRAC_HELD_HIDDEN 0x1u
/* Its animation is blending into `seq`, as OPENRAC_POSE_BLENDING. */
#define OPENRAC_HELD_BLENDING 0x2u
/* Off the body (a thrown wrench): at `position` and `rotation`, not on its joint. */
#define OPENRAC_HELD_LOOSE 0x4u

/* One piece: the game's slot it is worn in and which of the slot's mobys it is (0 or 1), the item
 * and the class of its moby, the body's joint it follows, OPENRAC_HELD_* flags, where it is (only
 * read for a loose piece), and its own animation (as openrac_pose's: a swing, a shot). Since 1.9. */
typedef struct openrac_held_piece {
    int32_t slot;
    int32_t part;
    int32_t item;
    int32_t oclass;
    int32_t joint;
    uint32_t flags;
    float position[3];
    float rotation[3];
    uint32_t seq;
    uint32_t frame;
    uint32_t next_frame;
    float anim_time;
    float anim_speed;
} openrac_held_piece;

/* What a body wears and holds: `count` pieces. Since 1.9. */
typedef struct openrac_held {
    /* sizeof(openrac_held), set by the mod. */
    uint32_t size;
    int32_t count;
    openrac_held_piece pieces[OPENRAC_HELD_MAX];
} openrac_held;

/* A shot the hero fired (openrac_api.hero_shots), or one a figure fires (figure_shot): the weapon
 * (the game's item id), where it starts, its velocity (game units a tick, as the game keeps it; for
 * a shot the game aims by angles, its direction) and the game's own aim angles (Ratchet & Clank's
 * Blaster: the heading's yaw and pitch, then the shot's first yaw and pitch). Since 1.9. */
typedef struct openrac_shot {
    /* sizeof(openrac_shot), set by whoever fills it. */
    uint32_t size;
    int32_t item;
    float position[3];
    float velocity[3];
    float aim[4];
} openrac_shot;

/* The most shots kept for hero_shots between two calls. Since 1.9. */
#define OPENRAC_SHOTS_MAX 16

/* A save of the player's (openrac_api.save_slot), read from the game's own save file on the
 * memory card. Since 1.8. */
typedef struct openrac_save_info {
    /* sizeof(openrac_save_info), set by the mod before the call. */
    uint32_t size;
    /* The level it continues in, its bolts, its gold bolts and its time played in seconds. */
    int32_t level;
    int32_t bolts;
    int32_t gold_bolts;
    int32_t seconds;
} openrac_save_info;

/* ------------------------------------------------------------------------------------------ *
 * Callbacks
 * ------------------------------------------------------------------------------------------ */

/* A function the port calls after each game tick (openrac_api.on_frame), with the `user` pointer
 * the mod gave. Since 1.0. */
typedef void (*openrac_frame_fn)(void* user);

/* Called after the game's vertical-blank handlers have run. Snapshot operations are only accepted
 * from this callback, at the port's tick boundary. Since 1.6. */
typedef void (*openrac_tick_boundary_fn)(void* user, uint32_t tick);

/* A function the port calls for each pad report (openrac_api.on_pad), with the `user` pointer the
 * mod gave. `report` is the report as the console's pad library hands it to the game (its bytes,
 * `size` of them), which the callback may change before the game reads it; `port` is the
 * controller's port, 0 or 1. Since 1.0. */
typedef void (*openrac_pad_fn)(void* user, int port, uint8_t* report, int size);

/* ------------------------------------------------------------------------------------------ *
 * The table the port offers
 * ------------------------------------------------------------------------------------------ */

/* What the port offers a mod. Filled by the port; valid for the life of the program. Every function
 * is called on the game's thread. Members from "1.2" on are at the end: a mod built for 1.1 never
 * sees them, and one built for 1.2 loads on a 1.2 port or newer. */
typedef struct openrac_api {
    /* sizeof(openrac_api) as the port was built: a newer port's is larger. Since 1.0. */
    uint32_t size;
    /* The port's interface version: at least what the mod was built for, else it is not loaded.
     * Since 1.0. */
    uint16_t api_major;
    uint16_t api_minor;

    /* The game's id, "rac1" (games/<game>/game.json). Since 1.0. */
    const char* game;
    /* The disc's version, "pal" or "ntsc". Since 1.0. */
    const char* version;
    /* OpenRAC's version, "0.9.0". Since 1.0. */
    const char* openrac_version;
    /* The platform, "macos-arm64", "linux-x86_64", "windows-x86_64", ... Since 1.0. */
    const char* platform;

    /* The base of game memory: game address a is memory + a. Fixed for the life of the program.
     * Since 1.0. */
    uint8_t* memory;

    /* Writes a line to the session's log, after the mod's id. `level` is an OPENRAC_LOG_* value.
     * Since 1.0. */
    void (*log)(openrac_mod* mod, int level, const char* text);

    /* Says why the mod refuses to load (status.json and the launcher show it). Only during
     * openrac_mod_load, which then returns non-zero. Since 1.0. */
    void (*fail)(openrac_mod* mod, const char* reason);

    /* Read the mod's settings (its manifest's "settings"; the values the launcher wrote). A key
     * the manifest does not declare, or of another type, gives 0, 0.0 or "". A string stays valid
     * for the life of the program. Since 1.0. */
    int (*setting_bool)(openrac_mod* mod, const char* key);
    int64_t (*setting_int)(openrac_mod* mod, const char* key);
    double (*setting_float)(openrac_mod* mod, const char* key);
    const char* (*setting_string)(openrac_mod* mod, const char* key);

    /* Takes the hook point `name` (games/<game>.h: its name and signature) for `replacement`, a
     * function of the hook point's signature, which `signature` spells. Sets `*original` to what
     * the replacement goes on to: the game's own function, or the replacement of a mod loaded
     * earlier, so the mod loaded last runs first. Returns OPENRAC_HOOK_OK or one of the other
     * OPENRAC_HOOK_* results. Only during openrac_mod_load. Since 1.0. */
    int (*hook)(
        openrac_mod* mod,
        const char* name,
        const char* signature,
        void* replacement,
        void** original
    );

    /* Calls `fn(user)` after each game tick, in load order. Returns 0, or non-zero when refused
     * (after openrac_mod_load). Only during openrac_mod_load. Since 1.0. */
    int (*on_frame)(openrac_mod* mod, openrac_frame_fn fn, void* user);

    /* Calls `fn(user, port, report, size)` for each pad report, before the game reads it, in load
     * order. Returns 0, or non-zero when refused (after openrac_mod_load). Only during
     * openrac_mod_load. Since 1.0. */
    int (*on_pad)(openrac_mod* mod, openrac_pad_fn fn, void* user);

    /* Returns a game function by its name in this disc's build ("func_001F9F90" on PAL,
     * "FUN_001f9dc8" on NTSC-U), as a host function to call with its signature; null when there is
     * none. Since 1.0. */
    void* (*function)(const char* name);

    /* Returns the function at a code address, in the level loaded now (a function pointer the game
     * keeps); null when there is none. Since 1.0. */
    void* (*function_at)(openrac_gaddr address);

    /* Return a global's address for the code loaded now: one of the executable's (guest.h,
     * OPENRAC_DATA), or of level `level`'s program (OPENRAC_LDATA). Ask each time: a level moves
     * its globals. Since 1.0. */
    openrac_gaddr (*relocate_data)(openrac_gaddr address);
    openrac_gaddr (*relocate_level_data)(int level, openrac_gaddr address);

    /* Returns the level whose program is loaded, -1 for none (the executable's). Since 1.0. */
    int (*loaded_level)(void);

    /* Gives `size` bytes (aligned to 16) of room on the game's stack for a call that takes the
     * address of a local, and keeps the stack as it was in `*saved`. Returns the room's address.
     * Pair each push with a stack_pop, innermost first. Since 1.0. */
    openrac_gaddr (*stack_push)(uint32_t size, openrac_gaddr* saved);

    /* Gives back what the matching stack_push took: `saved` is what it stored. Since 1.0. */
    void (*stack_pop)(openrac_gaddr saved);

    /* Returns the keyboard and mouse for the tick about to run. Never null: without a window (a
     * test, a server) nothing is held and nothing moves. Valid until the next tick. Since 1.2. */
    const openrac_input* (*input)(void);

    /* Takes `devices` (OPENRAC_INPUT_*) from the port: with OPENRAC_INPUT_KEYBOARD the port's own
     * keyboard table (WASD, Space, ...) no longer presses pad 0's buttons, and the mod presses
     * them through on_pad instead. A gamepad goes on as before; the port's window keys (F3, F11)
     * stay the port's. Returns 0, or OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.2. */
    int (*take_input)(openrac_mod* mod, uint32_t devices);

    /* Asks for the cursor captured (`capture` 1: hidden, held in the window, motion relative, as
     * a game's mouse look wants it) or let go (0). Any time; the latest asking of any mod counts.
     * The port lets it go anyway while the window is not focused or its overlay is open, and takes
     * it back when the reason is gone and the mod still asks. Since 1.2. */
    void (*capture_mouse)(openrac_mod* mod, int capture);

    /* Returns what the port draws with (openrac_graphics). Never null: before
     * the window opens, and without one, its backend is OPENRAC_GRAPHICS_NONE.
     * Any time. Since 1.4. */
    const openrac_graphics *(*graphics)(void);

    /* Asks the port to draw views of the game besides the window's picture each
     * frame, through `source` (openrac_view_source), with OpenGL and Vulkan.
     * While a mod has views, the Vulkan renderer draws on the game's thread.
     * Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_TAKEN when another mod has them,
     * OPENRAC_HOOK_UNKNOWN for a source without begin or end, or
     * OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.4. */
    int (*on_views)(openrac_mod *mod, const openrac_view_source *source);

    /* Has the port make its Vulkan device through `setup`
     * (openrac_vulkan_setup) when the window opens, so that the mod can share
     * it. Without Vulkan (OpenGL, a build without it) nothing is called.
     * Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_TAKEN when another mod has it,
     * OPENRAC_HOOK_UNKNOWN for a setup too small, or OPENRAC_HOOK_TOO_LATE
     * after openrac_mod_load. Since 1.4. */
    int (*setup_vulkan)(openrac_mod *mod, const openrac_vulkan_setup *setup);
    /* Adds an entry named `label` (ASCII, up to 31 characters) to the game's front-end menu, which
     * opens a page of the mod's own: `fn(user, frame)` each frame it is open (openrac_menu_fn).
     * Ratchet & Clank has it in the title's Options list, after the port's own entries (Video,
     * and Planets with developer tools). A game whose front end the port does not know shows
     * none. Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_UNKNOWN for no label or fn or past
     * OPENRAC_MENU_ENTRIES_MAX, or OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.5. */
    int (*menu_entry)(openrac_mod* mod, const char* label, openrac_menu_fn fn, void* user);

    /* Fills the rectangle of `width` by `height` pixels whose top left is (x, y) on the menu page
     * (OPENRAC_MENU_WIDTH by OPENRAC_MENU_HEIGHT) with `rgba` (OPENRAC_RGBA), blended by its
     * alpha; what is outside the page is left out. Only during the mod's own menu callback;
     * ignored otherwise. Since 1.5. */
    void (*menu_rect)(openrac_mod* mod, int x, int y, int width, int height, uint32_t rgba);

    /* Writes `text` (ASCII; a newline starts a line below) on the menu page with its first
     * character's top left at (x, y), each character OPENRAC_MENU_CHAR_WIDTH by
     * OPENRAC_MENU_CHAR_HEIGHT times `size` (1 to 8), in `rgba`. Only during the mod's own menu
     * callback; ignored otherwise. Since 1.5. */
    void (*menu_text)(openrac_mod* mod, int x, int y, int size, uint32_t rgba, const char* text);

    /* Calls `fn(user, tick)` after the game's vertical-blank handlers have run, the port's tick
     * boundary, the only place the savestate calls below are accepted (openrac_tick_boundary_fn).
     * Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_TOO_LATE for no fn or after openrac_mod_load.
     * Since 1.6. */
    int (*on_tick_boundary)(openrac_mod* mod, openrac_tick_boundary_fn fn, void* user);

    /* The byte size of this port build's warm, same-process snapshot; 0 where the running game
     * has none (Ratchet & Clank PAL and NTSC have one). Since 1.6. */
    size_t (*savestate_size)(void);

    /* Captures a snapshot into `out`, `size` being savestate_size(). Only inside on_tick_boundary;
     * returns 0 on success. The bytes are private to this process and build, not for a cold
     * restore or another version. Since 1.6. */
    int (*savestate_save)(void* out, size_t size);

    /* Restores a snapshot savestate_save made, under the same constraints; returns 0 on success.
     * It may refuse when the runtime allocated new guest statics or strings since the snapshot,
     * because translated call sites cache those addresses in process memory. Since 1.6. */
    int (*savestate_load)(const void* data, size_t size);

    /* Makes a page of the game's own menus (OPENRAC_UI_OWN_BACK or 0 in `flags`), with nothing on
     * it yet (ui_set), whose events go to `fn(user, page, event)` (openrac_ui_fn). Returns the
     * page's id (0 or more), OPENRAC_HOOK_UNKNOWN for no fn or past OPENRAC_UI_PAGES_MAX, or
     * OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.7. */
    int (*ui_page)(openrac_mod* mod, uint32_t flags, openrac_ui_fn fn, void* user);

    /* Adds an item named `label` to the title's main menu, after its own item `after`
     * (OPENRAC_UI_NEW_GAME, OPENRAC_UI_LOAD_GAME, OPENRAC_UI_OPTIONS), which opens the mod's
     * `page`. Two after the same item go in the order they were added. Returns OPENRAC_HOOK_OK,
     * OPENRAC_HOOK_UNKNOWN for an unknown `after`, no label, a page that is not the mod's, or past
     * OPENRAC_UI_MAIN_ITEMS_MAX, or OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.7. */
    int (*ui_main_item)(openrac_mod* mod, const char* after, const char* label, int page);

    /* Puts `content` on the mod's `page` (openrac_ui_content), in place of what it had: shown at
     * once if the page is on the screen. Any time. The cursor stays on its item (on the last when
     * there are fewer, and off a TEXT). Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a page
     * that is not the mod's or no content. Since 1.7. */
    int (*ui_set)(openrac_mod* mod, int page, const openrac_ui_content* content);

    /* While one of the mod's pages is on the screen: shows its `page`, a level under a page the
     * main menu opened or in place of one under it; a page that is on a level above goes back to
     * it. OPENRAC_UI_GO_BACK goes back a level, as Triangle does. Since 1.9 also while the title's
     * main menu itself is on the screen: shows the page one of the mod's main-menu items opens
     * (openrac_api.ui_main_item), as choosing the item does, so a mod brings the player back to
     * its page after a game it ended. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN when none
     * of the mod's pages (nor the main menu, for an item's page) is on the screen or the page is
     * not the mod's. Since 1.7. */
    int (*ui_open)(openrac_mod* mod, int page);

    /* While the mod's `page` is on the screen: asks `question` with the game's prompt box in place
     * of the list, the footer saying Cross is yes and Triangle no; the answer is an
     * OPENRAC_UI_ANSWER with `tag`. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN when the page
     * is not on the screen or another question is open. Since 1.7. */
    int (*ui_confirm)(openrac_mod* mod, int page, const char* question, uint32_t tag);

    /* Returns the item under the cursor of the mod's `page` (-1 for a page that is not the mod's),
     * after moving it to `item` when that is 0 or more and an item the cursor takes. Since 1.7. */
    int (*ui_cursor)(openrac_mod* mod, int page, int item);

    /* Plays one of the game's menu sounds (OPENRAC_UI_SOUND_*) while one of the mod's pages is on
     * the screen. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN otherwise. Since 1.7. */
    int (*ui_sound)(openrac_mod* mod, int sound);

    /* Fills `out` (its size set) with what the game is doing: its phase (OPENRAC_PLAY_*), its
     * level. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a game the port knows no play of
     * (phase OPENRAC_PLAY_NONE). Since 1.8. */
    int (*play_status)(openrac_play_status* out);

    /* The name of the game's level `level` ("Veldin"), or null for none. Since 1.8. */
    const char* (*level_name)(int level);

    /* Fills `out` (its size set) with the hero's pose as it is drawn now. Returns OPENRAC_HOOK_OK,
     * or OPENRAC_HOOK_UNKNOWN when there is no hero (no level, the hero not placed yet). Since
     * 1.8. */
    int (*hero_pose)(openrac_pose* out);

    /* Makes a figure in the level: a moby that looks like the hero, in his pose, drawn but never
     * running his logic, colliding with nothing and targeted by nothing, until figure_remove or the
     * level goes (a new play_status.loads). Returns its id (0 or more), or OPENRAC_HOOK_UNKNOWN
     * outside a level, past OPENRAC_FIGURES_MAX, or when the level has no room for a moby. Since
     * 1.8. */
    int (*figure_create)(openrac_mod* mod);

    /* Puts the mod's figure `id` in `pose`: where it is, its rotation and scale, and its animation
     * (a new seq blends into it; the figure animates by itself, at anim_speed nudged faster or
     * slower to keep to the pose's key and anim_time, and is put on the key only when more than a
     * few keys away). Each tick the mod wants it to move.
     * Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a figure that is not the mod's or is
     * gone with its level (then the mod makes another). Since 1.8. */
    int (*figure_pose)(openrac_mod* mod, int id, const openrac_pose* pose);

    /* Takes the mod's figure `id` out of the level. Returns OPENRAC_HOOK_OK, or
     * OPENRAC_HOOK_UNKNOWN for one that is not the mod's (gone already). Since 1.8. */
    int (*figure_remove)(openrac_mod* mod, int id);

    /* From the front end (phase OPENRAC_PLAY_FRONT_END): starts a game, after this frame, as the
     * title's New Game and Load Game do: with `save` null a new game in `level` (0 the first);
     * otherwise the `size` bytes of `save` (a save as save_read or save_capture give it), which
     * continue in its own level, or in `level` when that is 0 or more. Nothing is written to the
     * memory card. Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_TAKEN while a start or travel asked
     * before is still to happen, or OPENRAC_HOOK_UNKNOWN outside the front end, for a level the
     * game has not, or a save it does not take. Since 1.8. */
    int (*play_start)(openrac_mod* mod, int level, const void* save, size_t size);

    /* In a level with the player in control (phase OPENRAC_PLAY_PLAYING), and since 1.9 also in
     * its scenes and menus (OPENRAC_PLAY_SCENE, OPENRAC_PLAY_MENU: a cutscene, the pause menu, a
     * vendor): leaves it for `level`, after this frame, as the ship's travel does (the flight
     * between planets, then the level); -1 goes back to the title, as the pause menu's Quit Game
     * does (since 1.9).
     * Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_TAKEN while a start or travel is still to happen, or
     * OPENRAC_HOOK_UNKNOWN otherwise. Since 1.8. */
    int (*play_travel)(openrac_mod* mod, int level);

    /* The game's progress now, as a save (the memory card's save file's bytes): written to `out`
     * when `size` is large enough. Returns its size in bytes (call with a null `out` to ask), or 0
     * when there is none (no game started). Since 1.8. */
    size_t (*save_capture)(void* out, size_t size);

    /* Fills `out` (its size set) with what the memory card's save `slot` holds. Returns 1 for a
     * save, 0 for an empty slot (or one the game would not load), -1 past the game's last slot.
     * Since 1.8. */
    int (*save_slot)(int slot, openrac_save_info* out);

    /* The bytes of the memory card's save `slot`, for play_start: written to `out` when `size` is
     * large enough. Returns their size, or 0 for no save there. Since 1.8. */
    size_t (*save_read)(int slot, void* out, size_t size);

    /* Fills `out` with the level's live mobys and its placed ones deleted (OPENRAC_MOBY_DEAD), at
     * most `max` of them (each one's size set by the port), in slot order, whatever menu is up. Returns how many there are (more than `max`: the rest left out); 0
     * outside a level. Since 1.9. */
    int (*moby_list)(openrac_moby* out, int max);

    /* Returns the slot of the placed moby `key`, or -1 for none (killed, or no such record). Since
     * 1.9. */
    int (*moby_slot)(int key);

    /* Drives the moby in `slot` (`on` 1: its update no longer runs, the mod poses it) or lets it go
     * (0). A drive ends with its level. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for no
     * moby there, or OPENRAC_HOOK_TAKEN when another mod drives it. Since 1.9. */
    int (*moby_drive)(openrac_mod* mod, int slot, int on);

    /* Puts the moby in `slot` in the pose `pose` gives (its position, rotation, scale when above 0,
     * and animation, as figure_pose does). Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for no
     * moby there. Since 1.9. */
    int (*moby_pose)(openrac_mod* mod, int slot, const openrac_moby* pose);

    /* Deletes the moby in `slot` as the game deletes one (a placed one stays dead until its level
     * loads again). Returns OPENRAC_HOOK_OK or OPENRAC_HOOK_UNKNOWN. Since 1.9. */
    int (*moby_kill)(openrac_mod* mod, int slot);

    /* Makes a moby of class `oclass` (one the level has) in `pose`, driven by the mod. Returns its
     * slot, or OPENRAC_HOOK_UNKNOWN (no level, no room, a class the level has not). Since 1.9. */
    int (*moby_spawn)(openrac_mod* mod, int oclass, const openrac_moby* pose);

    /* Returns the slot of the mod's figure `id` (figure_create), or of the hero's moby with -1;
     * -1 for none. Since 1.9. */
    int (*figure_slot)(openrac_mod* mod, int id);

    /* Calls `fn(user, slot, phase)` around each moby's update (openrac_moby_update_fn). One mod's.
     * Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_TAKEN when another mod has it, or
     * OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.9. */
    int (*on_moby_update)(openrac_mod* mod, openrac_moby_update_fn fn, void* user);

    /* Only in the gate's call before an update: for that update the hero is at `position`, and his
     * moby is the mod's figure `figure` (-1: his own), as another player would be there; put back
     * after it. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN outside that call. Since 1.9. */
    int (*hero_stand_in)(openrac_mod* mod, const float position[3], int figure);

    /* Calls `fn(user, &hit)` for each hit the game posts (openrac_hit_fn). One mod's. Returns as
     * on_moby_update. Since 1.9. */
    int (*on_moby_hit)(openrac_mod* mod, openrac_hit_fn fn, void* user);

    /* Posts `hit` to its victim as the game posts one (the victim's update reads it: an enemy
     * takes the damage, the hero is hurt), not told to on_moby_hit. Returns OPENRAC_HOOK_OK, or
     * OPENRAC_HOOK_UNKNOWN for no victim there. Since 1.9. */
    int (*moby_hit)(openrac_mod* mod, const openrac_hit* hit);

    /* Sets up to OPENRAC_EYES_MAX points (x, y, z each) besides the camera: the mobys within their
     * update distance of one run too (the world near other players). `count` 0 clears them.
     * Returns OPENRAC_HOOK_OK. Since 1.9. */
    int (*play_eyes)(openrac_mod* mod, const float* xyz, int count);

    /* Adds `add` to the player's bolts (0: none) and returns them. Since 1.9. */
    int (*bolts)(int add);

    /* While `on` (1), the level's mobys keep being updated under the game's pause menu and its
     * freeze dialogs, in the level's own mobys (the hero and the picture stay as the menu left
     * them): a game that plays with others never stops their world for a menu. 0: as the game
     * does. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a game without it. Since 1.9. */
    int (*play_overlay_pause)(openrac_mod* mod, int on);

    /* Returns the slot of the moby at game address `moby` (a hook point's argument), -1 for none.
     * Since 1.9. */
    int (*moby_slot_of)(openrac_gaddr moby);

    /* Skips the scene playing (a cutscene, the ship's landing or take-off, the flight between
     * planets) the way the game's own skip does, as Start does with the port's "Skip cutscenes":
     * for the next ticks, until the scene is over. For a party whose scenes end together. Returns
     * OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a game the port has no skip of. Since 1.9. */
    int (*play_skip)(openrac_mod* mod);

    /* Fills `out` (its size set) with what the hero wears and holds now (openrac_held), the pieces
     * in the game's slot order. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN when there is no
     * hero (as hero_pose) or the port knows no pieces of the game. Since 1.9. */
    int (*hero_held)(openrac_held* out);

    /* Has the mod's figure `id` wear and hold `held`: each piece a moby of its class on the
     * figure's joint (a loose one where it says), animated as it says, made when the level has its
     * class loaded or can load it beside the hero's own (a piece it cannot have is left out, the
     * hand's falling back to the wrench), taken off when left out of a later call, with the figure
     * or with its level. Call it each tick the figure is posed. Returns OPENRAC_HOOK_OK, or
     * OPENRAC_HOOK_UNKNOWN for a figure that is not the mod's or is gone. Since 1.9. */
    int (*figure_held)(openrac_mod* mod, int id, const openrac_held* held);

    /* The shots the hero fired since the last call (any mod's), oldest first: at most `max` into
     * `out` (each one's size set by the port), at most OPENRAC_SHOTS_MAX kept between calls.
     * Returns how many were written. Since 1.9. */
    int (*hero_shots)(openrac_shot* out, int max);

    /* Has the mod's figure `id` fire `shot` as the hero would: the game's own projectile and its
     * effects, owned by the figure, hitting and breaking nothing in this game (the shooter's game
     * tells what it hit). Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for a figure that is not
     * the mod's, or a weapon the port cannot fire for a figure. Since 1.9. */
    int (*figure_shot)(openrac_mod* mod, int id, const openrac_shot* shot);

    /* Shows `text` (one line, UTF-8) at the top of the game's picture until the mod shows another
     * or clears it (null or ""): what the game's own words cannot say, such as a party waiting for
     * its players. Returns OPENRAC_HOOK_OK. Since 1.9. */
    int (*play_notice)(openrac_mod* mod, const char* text);

    /* Adds an item named `label` to the Options page of the pause menu in a level, after the
     * game's own items (and the port's Video), which opens the mod's `page` (ui_page). Ratchet &
     * Clank: the page is a copy of the game's Camera options page, its title the page's and its
     * list a row per item, the label on the left and a choice's value on the right, a few rows at
     * a time; the game's cursor, sounds and Triangle (back to Options, but for OPENRAC_UI_OWN_BACK
     * and while a question is open, which Triangle answers no); a question (ui_confirm) is the
     * title with two rows, Yes and No; the footer is the game's. ui_open swaps another of the
     * mod's pages into its place, ui_cursor only reads the cursor and ui_sound plays nothing
     * there. Two go in the order they were added. Returns OPENRAC_HOOK_OK, OPENRAC_HOOK_UNKNOWN
     * for no label, a page that is not the mod's, or past OPENRAC_UI_PAUSE_ITEMS_MAX, or
     * OPENRAC_HOOK_TOO_LATE after openrac_mod_load. Since 1.9. */
    int (*ui_pause_item)(openrac_mod* mod, const char* label, int page);

    /* The in-level scene playing now: a cutscene of the level's own, which the game starts from
     * the level's mobys (a person spoken to, a place reached, a level's opening) or play_scene
     * started; not a movie, a slideshow, the ship's landing and take-off, or a scene the pause
     * menu plays again for its player. Returns its number in the level (the game's
     * own, the same in every game of that level), or -1 with none playing; `frame` (when not null)
     * gets how many of the scene's frames have played. Since 1.9. */
    int (*scene_playing)(int* frame);

    /* Starts the level's scene `scene` (scene_playing's number) as the game's own trigger does,
     * from its start: its actors made, its camera, sound and music, the hero held until it ends
     * (or is skipped: play_skip). It starts at the end of a tick with the player in control in the
     * level (OPENRAC_PLAY_PLAYING, the hero alive); asked before that (a menu up, another scene
     * playing), the request waits, until it starts, the level is left or loaded again, or another
     * call replaces it (-1 drops it). For a party whose games play the same scenes. Returns
     * OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN outside a level, or for a game the port starts no
     * scenes of. Since 1.9. */
    int (*play_scene)(openrac_mod* mod, int scene);

    /* Hides the moby in `slot` from drawing (`hidden` 1), as the game hides one, or shows it again
     * (0); the game's own update may change it again (one a mod drives keeps it). Listed with
     * OPENRAC_MOBY_HIDDEN while hidden. Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for no
     * moby there. Since 1.9. */
    int (*moby_hide)(openrac_mod* mod, int slot, int hidden);

    /* Fills `out` (its size set by the mod) with the last deletion the game made of the moby in
     * `slot` in this level load: whose update did it, or a mod. For a mod that wants to know why a
     * moby left the level (a spawned one's slot is used again: ask on the tick it went). Returns
     * OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN for none seen (none, or it left the level some
     * other way). Since 1.9. */
    int (*moby_deletion)(int slot, openrac_moby_deletion* out);

    /* Writes where the game's camera is (x, y, z, the level's units) into `out`. Returns
     * OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN outside a level. Since 1.9. */
    int (*camera_position)(float out[3]);

    /* Fills `out` (its size set by the mod) with how the game ran the level's mobys in the last
     * tick (the run list's camera, what the last menu update did) and the moby in `slot` (-1: none)
     * in it (openrac_moby_run). Returns OPENRAC_HOOK_OK, or OPENRAC_HOOK_UNKNOWN outside a level or
     * for a game without it. Since 1.9. */
    int (*moby_run)(int slot, openrac_moby_run* out);
} openrac_api;

/* ------------------------------------------------------------------------------------------ *
 * The entry points a mod exports
 * ------------------------------------------------------------------------------------------ */

/* Tells the port which interface the mod was built for: sets `info`'s api_major, api_minor, id and
 * version. Called before the mod is loaded. Required; OPENRAC_MOD_DEFINE writes it. Since 1.0. */
typedef void (*openrac_mod_query_fn)(openrac_mod_info* info);

/* Loads the mod: takes its hook points and callbacks from `api`, and keeps `mod` for the calls
 * that are its own. Returns 0 to load, non-zero to refuse (after openrac_api.fail gave the
 * reason). Required. Since 1.0. */
typedef int (*openrac_mod_load_fn)(const openrac_api* api, openrac_mod* mod);

/* Lets go of what the mod holds, when the program ends. Optional. Since 1.0. */
typedef void (*openrac_mod_unload_fn)(void);

/* The names the entry points are exported under, for the port's lookup. Since 1.0. */
#define OPENRAC_MOD_QUERY "openrac_mod_query"
#define OPENRAC_MOD_LOAD "openrac_mod_load"
#define OPENRAC_MOD_UNLOAD "openrac_mod_unload"

/* ------------------------------------------------------------------------------------------ *
 * Helpers for a mod
 * ------------------------------------------------------------------------------------------ */

/* The host pointer to a game address's bytes. `api` is the openrac_api, `a` a game address.
 * Since 1.0. */
#define OPENRAC_G(api, a) ((void*)((api)->memory + (openrac_gaddr)(a)))

/* A typed value at a game address, as an lvalue: OPENRAC_GREF(api, int32_t, a) = 5. The address
 * must be aligned for `T`. Since 1.0. */
#define OPENRAC_GREF(api, T, a) (*(T*)OPENRAC_G(api, a))

/* Writes the usual openrac_mod_query: OPENRAC_MOD_DEFINE("lock-strafe", "1.0.0"). `mod_id` and
 * `mod_version` are string literals, the manifest's. Since 1.0. */
#define OPENRAC_MOD_DEFINE(mod_id, mod_version)                                                    \
    OPENRAC_MOD_EXPORT void openrac_mod_query(openrac_mod_info* info) {                            \
        info->api_major = OPENRAC_MOD_API_MAJOR;                                                   \
        info->api_minor = OPENRAC_MOD_TARGET_MINOR;                                                \
        info->id = (mod_id);                                                                       \
        info->version = (mod_version);                                                             \
    }

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_MOD_H */

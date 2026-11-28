/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's binding and skin for the native multiplayer screens (port/mpui, through
 * openrac/mp_ui.h; docs/port/MULTIPLAYER_UI.md): which of level 39's lobby screens a native screen
 * sits on, where its rows come from, how its choices reach the game, and how it is drawn with the
 * game's own font, highlight bar and sounds.
 *
 * Level 39's lobby is a screen manager (its pointer at $gp - 0x6AC) with a stack of screen objects
 * and their ids; each frame its update (0x006862D0) reads one menu event from the pad (0x00686138)
 * and hands it to the current screen, then the render draws the GUI elements the screens placed
 * (0x005D69C0) (seen in game code: L39_006862D0, L39_00686138, L39_0067C9C0). Three wraps
 * (mp/libraries.tsv):
 *
 * - the event read: while a native screen has the cursor, the events it uses are its own and the
 *   game sees none; a choice that is the game's (join a game, choose a player) puts the game's own
 *   cursor there and hands the game Cross, so the game's own code joins, as on the console;
 * - the manager's update: afterwards, the screen on top decides which native screen is open, its
 *   tables are filled from the game's memory and the game widgets it replaces are hidden;
 * - the elements' render: the native screen's highlight bar, the game's own element, is placed
 *   where the native cursor is before the elements draw, and its text is printed after them, into
 *   the same 2D list.
 *
 * The rows are the game's: a game list's rows are its list widget's texts with their world ids
 * and the lobby's records (players), the staging room's are tNW_GameSettings' slots. A server's
 * description (rac3_mp_ui_describe) may change the columns, the rows shown and the screens; with
 * none, the vanilla one below keeps the game's own look: one column of game names. */
#include "openrac/mp_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp/game_protos.h"
#include "mp_room.h"
#include "openrac/frame.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "openrac/ui.h"
#include "rac3_version.h"
#include "ui2d.h"
#include "ui_adapter.h"

/* ---- The game's memory (PAL; seen in game code, L39_* in rac3-decomp's level 39) ---- */

/* Level 39's words: the screen manager, the active pad, tNW_GameSettings, the lobby context, the
 * font the GUI elements print with, and the text id of each skin's name (read by L39_0059A970).
 * rac3_mp_pairs.py pairs only the executable's code, so NTSC-U's come from the same instructions of
 * level 39's functions on both discs (measured: 296, 15, 104, 411, 2 and 1 sites agree). */
#ifdef RAC3_PAL
#define SCREEN_MANAGER 0x00247914u /* the manager object, 0 before the lobby is up */
#define ACTIVE_PAD 0x00241164u     /* the pad record the menus read */
#define GAME_SETTINGS 0x00241640u  /* tNW_GameSettings, 0 outside a room */
#define LOBBY_CONTEXT 0x0024D760u  /* the Medius lobby's context */
#define ELEMENT_FONT 0x0024CD5Cu   /* the font the GUI elements print in */
#define SKIN_TEXT_IDS 0x004B1710u  /* the text id of each skin's name */
#else
#define SCREEN_MANAGER 0x00247A74u
#define ACTIVE_PAD 0x002412E4u
#define GAME_SETTINGS 0x002417C0u
#define LOBBY_CONTEXT 0x0024D860u
#define ELEMENT_FONT 0x0024CE5Cu
#define SKIN_TEXT_IDS 0x004B1540u
#endif

/* The manager's fields: the stack of screen objects and their ids, the current screen's index and
 * the top popup's (-1 none; popups sit above the current screen). */
#define MGR_SCREENS 0x110u
#define MGR_IDS 0x1C0u
#define MGR_CURRENT 0x270u
#define MGR_POPUP 0x274u

/* A screen object's widgets (pointers), its focused widget's index, a widget's mode (0 hidden, 2
 * disabled, 3 normal, 4 focused) and rectangle (centre x, centre y, width, height, shares of the
 * 512 x 448 picture). */
#define SCREEN_WIDGETS 0x110u
#define SCREEN_FOCUS 0x290u
#define WIDGET_MODE 0x4u
#define WIDGET_RECT 0x50u

/* The screen ids the native screens sit on: the two game lists and the staging room. */
#define SCREEN_GAME_LIST_A 0x0F
#define SCREEN_GAME_LIST_B 0x19
#define SCREEN_STAGING 0x21
#define SCREEN_RESULTS 0x12

/* The results screen's widgets: its eight rows are widgets 0x1A + 5r to 0x1E + 5r, their team
 * colour bars 0xD + r (L39_006A83B8); the rows come from the room (mp_room.c), which keeps the
 * results block the map leaves. */
#define RESULTS_FIRST_WIDGET 0x0D
#define RESULTS_HEADERS 0x15
#define RESULTS_COLUMNS 5
#define RESULTS_LAST_WIDGET (0x1E + 5 * (GS_SLOTS - 1))

/* A game list screen: its list widget (number 8), the rows shown and the world id of each. The list
 * widget: its row count, its rows (256 bytes of text each) and the rows it shows at once. */
#define LIST_WIDGET 8
#define LIST_ROWS_SHOWN 0x1057Cu
#define LIST_ROW_COUNT 0x64u
#define LIST_ROW_TEXT 0x68u
#define LIST_ROW_BYTES 0x100u
#define GAME_LIST_COUNT 0x2C0u
#define GAME_LIST_WORLDS 0x2C4u
#define GAME_LIST_MAX 0xE0

/* The lobby's game list: its state at context + 0x70, the group shown (-1 none) at + 0x18 and the
 * groups at + 0x2C, 12 bytes each (count at + 4, the records at + 8). A record is the server's
 * GameList reply: world id + 0x1C, player count + 0x20, max players + 0x24 (16 bits each). */
#define LOBBY_GAME_LIST 0x70u
#define GAME_LIST_GROUP 0x18u
#define GAME_LIST_GROUPS 0x2Cu
#define GROUP_BYTES 12u
#define RECORD_WORLD 0x1Cu
#define RECORD_PLAYERS 0x20u
#define RECORD_MAX_PLAYERS 0x24u

/* The game's eight staging rows: name widget 0xF + n, the other cells of row n up to the ready
 * tick 0x57 + n; the rows themselves are the room's (mp_room.c). */
#define GS_SLOTS 8
#define STAGING_NAME_WIDGET 0x0F
#define STAGING_START_WIDGET 4 /* the host's START, the last widget above the rows */
#define STAGING_LAST_WIDGET (0x57 + GS_SLOTS - 1)

/* The game's menu events (L39_00686138) and sounds (L39_00688340: 3 move, 4 select, 0x17 back;
 * 5 is played on refusals in the login screens, assumed to be the refusal). */
enum {
    GAME_UP = 1,
    GAME_DOWN,
    GAME_LEFT,
    GAME_RIGHT,
    GAME_TRIANGLE,
    GAME_CIRCLE,
    GAME_CROSS,
    GAME_SQUARE,
    GAME_SELECT,
    GAME_START,
    GAME_L1,
    GAME_L2,
    GAME_L3,
    GAME_R1
};

#define PAD_PRESSED 0x1C4u /* the pad record: pressed buttons (L39_006C20B0) */
#define PAD_CIRCLE 0x20u
#define SOUND_MOVE 3
#define SOUND_SELECT 4
#define SOUND_DENIED 5
#define SOUND_BACK 0x17

/* The picture the GUI works in, and the font's text colours (seen in game code: L39_00682308):
 * gold, grey for disabled, and the focused pulse's far colour, 0x25 frames a swing. */
#define PICTURE_W 512.0f
#define PICTURE_H 448.0f
#define COLOUR_NORMAL 0x8066CCFFu
#define COLOUR_DISABLED 0x80808080u
#define COLOUR_PULSE 0x66D6EEFAu
#define PULSE_FRAMES 0x25u
/* A modal screen's panel: black at 0x7C of full, nearly opaque, so the screen under it does not
 * show through, with the menus' gold edge. */
#define COLOUR_PANEL 0x7C000000u
/* The modal area: the middle of the picture, as the game's message boxes sit. */
/* The results screen's rows: the first row's centre, the pitch, and the table's left edge and width
 * (its frame, 0.5 +- 0.41 of the picture's width). */
#define RESULTS_TOP 230.0f
#define RESULTS_PITCH 21.0f
#define RESULTS_X 46.0f
#define RESULTS_W 420.0f
#define MODAL_X 88.0f
#define MODAL_Y 112.0f
#define MODAL_W 336.0f
#define MODAL_H 224.0f
/* FontPrint's flags word as the menus pass it, and its drop shadow's offset. */
#define PRINT_FLAGS 0x80000000ull
#define SHADOW_OFFSET 2.0f
/* The font's height at scale 1 (assumed from the staging room's 20.5-pixel rows at the element
 * scale the lobby uses), and the share of a row the highlight bar covers (staging: 0.035 of
 * 0.0458). */
#define FONT_HEIGHT 22.0f
#define BAR_SHARE 0.76f
#define TEXT_BYTES 128
/* The largest description file read (the schema's own limit). */
#define DESCRIPTION_BYTES 32768
/* The made-up players OPENRAC_RAC3_MP_UI_MOCK may add at most, and their first key (clear of the
 * game's client ids). */
#define MOCK_MAX 64
#define MOCK_KEY 1000u

void rac3_mp_net_ui_action(const char* key, int value); /* mp_net.c: a server action */

/* ---- State (the game's thread only) ---- */

static openrac_mp_ui* g_ui;  /* the native screens' context, made at the first lobby frame */
static int g_described;      /* 1 once a description (the vanilla one at least) is in force */
static int g_vanilla = 1;    /* 1 while the vanilla description is the one in force */
static int g_bound = -1;     /* the game screen id the open native screen sits on, -1 none */
static gaddr g_screen;       /* that game screen's object */
static int g_hidden;         /* 1 while a popup covers the screen: the native one steps aside */
static uint32_t g_frame;     /* frames drawn, for the focused colour's pulse */
static int g_on_staging;     /* the lobby screen manager shows the staging room */
static float g_line = 20.0f; /* the open screen's row pitch */
static int g_text_pass;      /* 1 in the pass after the elements: text only, else the bar only */
static int g_modal_layer;    /* 1 while the modal layer draws */

/* The vanilla description: the game's own lists, a column of names, as many rows as the game's
 * list widget shows; in a room, the staging room's players with their team, skin and ready tick
 * (and the room's count against its limit, and a status line: READY n/m, the START countdown), the
 * scores with a scroll bar past a page, and the player's own dialog (team, skin, READY) that Cross
 * on the player's row opens. Its skin names are the game's own texts, so it is built when the lobby
 * is up (build_vanilla). A server's description, or OPENRAC_RAC3_MP_UI, replaces all of it. */
static const char VANILLA_HEAD[] =
    "{\"schema\": 1, \"name\": \"Up Your Arsenal\", \"screens\": ["
    "{\"id\": \"game_list\", \"columns\": [{\"field\": \"name\", \"width\": 100}],"
    " \"items\": [{\"kind\": \"table\", \"key\": \"games\", \"action\": \"game:join\"}]},"
    "{\"id\": \"staging\", \"columns\": [{\"field\": \"name\", \"width\": 28},"
    " {\"field\": \"skill\", \"width\": 12}, {\"field\": \"voice\", \"width\": 11},"
    " {\"field\": \"team\", \"width\": 17}, {\"field\": \"skin\", \"width\": 24},"
    " {\"field\": \"ready\", \"width\": 8}],"
    " \"items\": [{\"kind\": \"table\", \"key\": \"players\", \"action\": \"game:select\"},"
    " {\"kind\": \"text\", \"key\": \"status\", \"label\": \" \"}]},"
    "{\"id\": \"scoreboard\", \"columns\": [{\"field\": \"name\", \"width\": 40},"
    " {\"field\": \"kills\", \"width\": 15}, {\"field\": \"deaths\", \"width\": 15}],"
    " \"items\": [{\"kind\": \"table\", \"key\": \"scores\"}]},"
    "{\"id\": \"player\", \"title\": \"PLAYER\", \"items\": ["
    "{\"kind\": \"choice\", \"key\": \"team\", \"label\": \"TEAM\", \"action\": \"game:team\","
    " \"choices\": [\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\"]},"
    "{\"kind\": \"choice\", \"key\": \"skin\", \"label\": \"SKIN\", \"action\": \"game:skin\","
    " \"choices\": [";
static const char VANILLA_TAIL[] =
    "]},"
    "{\"kind\": \"button\", \"label\": \"READY\", \"action\": \"game:ready\"},"
    "{\"kind\": \"button\", \"label\": \"BACK\", \"action\": \"back\"}]}]}";
#define SKIN_CHOICES 16 /* the schema's most choices */
#define VANILLA_BYTES 4096

/* A skin's name, the game's own text, or NULL past the last. */
static const char* skin_name(int skin) {
    const gaddr text =
        (gaddr)mp_func_L39_0059A970(GREF(int32_t, SKIN_TEXT_IDS + 4u * (gaddr)(skin & 0xFF)));
    return text != 0 && GREF(uint8_t, text) != 0 ? (const char*)G(text) : NULL;
}

/* The vanilla description with the game's skin names (at most SKIN_CHOICES, JSON-escaped, cut to
 * the schema's label length). Returns its length. */
static size_t build_vanilla(char* out, size_t size) {
    size_t n = (size_t)snprintf(out, size, "%s", VANILLA_HEAD);
    int count = 0;
    for (int skin = 0; skin < SKIN_CHOICES && n + 64 < size; skin++) {
        const char* name = skin_name(skin);
        if (name == NULL) {
            break;
        }
        out[n++] = count++ > 0 ? ',' : ' ';
        out[n++] = '"';
        for (int k = 0; name[k] != 0 && k < 24; k++) {
            const char c = name[k];
            if (c == '"' || c == '\\') {
                out[n++] = '\\';
            }
            out[n++] = (unsigned char)c < 0x20 ? ' ' : c;
        }
        out[n++] = '"';
    }
    if (count == 0) {
        n += (size_t)snprintf(out + n, size - n, "\"1\"");
    }
    n += (size_t)snprintf(out + n, size - n, "%s", VANILLA_TAIL);
    return n < size ? n : size - 1;
}

/* ---- The skin: the game's font, bar and sounds ---- */

/* The pulse between the focused colours, as the game's widgets swing (L39_00682308, mode 4). */
static uint32_t pulse_colour(void) {
    const uint32_t phase = g_frame % (2u * PULSE_FRAMES);
    const uint32_t step = phase < PULSE_FRAMES ? phase : 2u * PULSE_FRAMES - phase;
    uint32_t out = 0;
    /* Each of the four channels swings on its own. */
    for (int shift = 0; shift < 32; shift += 8) {
        const int from = (int)((COLOUR_NORMAL >> shift) & 0xFFu);
        const int to = (int)((COLOUR_PULSE >> shift) & 0xFFu);
        const int value = from + (to - from) * (int)step / (int)PULSE_FRAMES;
        out |= (uint32_t)value << shift;
    }
    return out;
}

/* The colour of a text style. */
static uint32_t style_colour(int style) {
    /* Only the focused text pulses; disabled text is grey. */
    switch (style) {
        case OPENRAC_MP_UI_FOCUSED:
            return pulse_colour();
        case OPENRAC_MP_UI_DISABLED:
            return COLOUR_DISABLED;
        default: /* titles, headings and rows are the menus' gold */
            return COLOUR_NORMAL;
    }
}

/* The font scale that fills a row of the open screen. */
static float text_scale(void) {
    return g_line / FONT_HEIGHT;
}

/* Copies `length` bytes of `text` into game memory at `at` (TEXT_BYTES long), terminated. */
static int put_text(gaddr at, const char* text, size_t length) {
    /* A text longer than the buffer is cut, as the game's own prints would be. */
    if (length > TEXT_BYTES - 1) {
        length = TEXT_BYTES - 1;
    }
    memcpy(G(at), text, length);
    GREF(uint8_t, at + length) = 0;
    return (int)length;
}

/* The skin's width of a text: the game's own measure (FontTextWidth) at the rows' scale. */
static float skin_text_width(void* user, const char* text, size_t length, int style) {
    (void)user, (void)style;
    GFRAME(buffer, TEXT_BYTES);
    const int n = put_text(buffer, text, length);
    mp_func_L39_00591BA0(GREF(int32_t, ELEMENT_FONT));
    return (float)mp_func_L39_00591F98((int)buffer, n, text_scale());
}

/* The skin's text: the game's own print (FontPrint) in the GUI elements' font. */
static void skin_text(void* user, float x, float y, const char* text, size_t length, int style) {
    /* Text goes over the elements' panels, in the pass after them. */
    if (!g_text_pass) {
        return;
    }
    GFRAME(buffer, TEXT_BYTES);
    const int n = put_text(buffer, text, length);
    const float scale = text_scale();
    mp_func_L39_00591BA0(GREF(int32_t, ELEMENT_FONT));
    /* The print centres its text on x (measured: a left edge given came out half a width left). */
    x += (float)mp_func_L39_00591F98((int)buffer, n, scale) * 0.5f;
    (void)user;
    /* x, y, the scales (the elements' height is 1/0.9286 of their width, L39_00619B48), the colour,
     * the text and its length, 1, the drop shadow on, the flags, the shadow's offset. */
    mp_func_L39_00592238(
        x,
        y,
        scale,
        scale * 1.0769231f,
        (int)style_colour(style),
        (int)buffer,
        n,
        1,
        1,
        PRINT_FLAGS,
        SHADOW_OFFSET,
        SHADOW_OFFSET
    );
}

/* The skin's highlight bar: the game's own bar element over the row. */
static void skin_highlight(void* user, float x, float y, float w, float h) {
    (void)user;
    /* The bar is an element: placed in the pass before the elements draw, a modal screen's too. */
    if (g_text_pass) {
        return;
    }
    /* The game's own bar (and its arrows) placed on the row: centre and size in shares of the
     * picture, the arrows a row's height apart (L39_00689F00, as the staging room calls it). */
    mp_func_L39_00689F00(
        (int)g_screen,
        0,
        3,
        3,
        (x + w * 0.5f) / PICTURE_W,
        (y + h * 0.5f) / PICTURE_H,
        w / PICTURE_W,
        h * BAR_SHARE / PICTURE_H,
        h / PICTURE_H
    );
}

/* The skin's filled rectangles (a modal screen's panel and its edges): through the shared native 2D
 * layer (openrac/ui.h) as colour alone, in the game's 2D pixels as the skin's positions are, placed
 * where level 39's list has got to. Drawn in the pass before the elements, so the game's bar and
 * the text go over the panel. */
static void skin_rect(void* user, float x, float y, float w, float h, uint32_t rgba) {
    (void)user;
    if (g_text_pass) {
        return;
    }
    const float rect[4] = {x, y, x + w, y + h};
    const uint32_t colours[4] = {rgba, rgba, rgba, rgba};
    const gaddr at = GREF(gaddr, rac3_ui2d_mp_level().cursor);
    rac3_ui_adapter_install();
    openrac_frame_anchor(at < 0x02000000u ? at : 0);
    openrac_ui_draw_rect(NULL, rect, NULL, colours, OPENRAC_UI_FLAT);
}

/* The skin's images: none yet; they come with the game's image tables (P4's texture work). */
static void skin_sprite(void* user, float x, float y, float w, float h, int sprite) {
    (void)user, (void)x, (void)y, (void)w, (void)h, (void)sprite;
}

/* The skin's sounds: the menus' own (L39_00688340). */
static void skin_sound(void* user, int sound) {
    (void)user;
    static const int MENU_SOUNDS[] = {SOUND_MOVE, SOUND_SELECT, SOUND_BACK, SOUND_DENIED};
    mp_func_L39_00688340(MENU_SOUNDS[sound & 3], 0, 0);
}

/* The skin's layers: the modal one draws its own panel and bar. */
static void skin_layer(void* user, int modal) {
    (void)user;
    g_modal_layer = modal;
}

/* The skin table for the open screen's row pitch. */
static openrac_mp_ui_skin skin(void) {
    const openrac_mp_ui_skin s =
        {NULL,
         g_line,
         COLOUR_PANEL,
         COLOUR_NORMAL,
         skin_text_width,
         skin_text,
         skin_rect,
         skin_sprite,
         skin_highlight,
         skin_sound,
         skin_layer};
    return s;
}

/* OPENRAC_RAC3_MP_UI: a description file in place of the vanilla one, for trying one without a
 * server. A file that cannot be read, or is refused, leaves the vanilla one (logged). */
static void describe_from_file(void) {
    const char* path = openrac_env_string("OPENRAC_RAC3_MP_UI");
    /* Unset: the vanilla description stays. */
    if (path == NULL || path[0] == 0) {
        return;
    }
    FILE* file = fopen(path, "rb");
    /* A missing file is the developer's to fix; the game runs on with the game's screens. */
    if (file == NULL) {
        fprintf(stderr, "[warning] rac3: lobby screens: cannot read %s\n", path);
        return;
    }
    static char json[DESCRIPTION_BYTES];
    const size_t size = fread(json, 1, sizeof json, file);
    fclose(file);
    g_vanilla = !openrac_mp_ui_describe(g_ui, json, size) && g_vanilla;
}

/* Makes the context and gives it the vanilla description, once. Returns 0 without memory. */
extern uint16_t (*rac3_pad_filter)(uint16_t word); /* pad.c */
uint16_t rac3_mp_ui_pad_filter(uint16_t word);

static int make_ui(void) {
    rac3_pad_filter = rac3_mp_ui_pad_filter;
    /* The context is made once, at the first lobby frame. */
    if (g_ui == NULL) {
        g_ui = openrac_mp_ui_create();
    }
    /* The vanilla description is the one in force until a server's arrives. */
    if (g_ui != NULL && !g_described) {
        static char vanilla[VANILLA_BYTES];
        const size_t length = build_vanilla(vanilla, sizeof vanilla);
        g_described = openrac_mp_ui_describe(g_ui, vanilla, length);
        openrac_mp_ui_set_modal_area(g_ui, MODAL_X, MODAL_Y, MODAL_W, MODAL_H);
        describe_from_file();
    }
    return g_ui != NULL && g_described;
}

/* ---- Reading the game ---- */

/* Widget `n` of a lobby screen object. */
static gaddr widget(gaddr screen, int n) {
    return GREF(gaddr, screen + SCREEN_WIDGETS + 4u * (gaddr)n);
}

/* The lobby record of world `world` in the group shown, or 0. */
static gaddr game_record(int32_t world) {
    const gaddr context = GREF(gaddr, LOBBY_CONTEXT);
    /* No lobby context before the player logs in. */
    if (context == 0) {
        return 0;
    }
    const gaddr list = GREF(gaddr, context + LOBBY_GAME_LIST);
    const int32_t group = list != 0 ? GREF(int32_t, list + GAME_LIST_GROUP) : -1;
    /* No group: the list has not come back yet. */
    if (group < 0) {
        return 0;
    }
    const gaddr at = list + GAME_LIST_GROUPS + GROUP_BYTES * (gaddr)group;
    const int32_t count = GREF(int32_t, at + 4u);
    const gaddr records = GREF(gaddr, at + 8u);
    /* The group's records, in the server's order. */
    for (int32_t n = 0; n < count && records != 0; n++) {
        const gaddr record = GREF(gaddr, records + 4u * (gaddr)n);
        /* The one record of this world. */
        if (record != 0 && GREF(int32_t, record + RECORD_WORLD) == world) {
            return record;
        }
    }
    return 0;
}

/* The "games" table: the list widget's rows, keyed by world id, with the record's players. */
static void fill_games(gaddr screen) {
    const gaddr list = widget(screen, LIST_WIDGET);
    int32_t count = GREF(int32_t, screen + GAME_LIST_COUNT);
    /* Never past the rows the list widget holds. */
    if (count > GREF(int32_t, list + LIST_ROW_COUNT)) {
        count = GREF(int32_t, list + LIST_ROW_COUNT);
    }
    /* Never past the screen's own tables. */
    if (count > GAME_LIST_MAX) {
        count = GAME_LIST_MAX;
    }
    openrac_mp_ui_table_begin(g_ui, "games");
    /* One row a game the screen shows. */
    for (int32_t n = 0; n < count; n++) {
        const int32_t world = GREF(int32_t, screen + GAME_LIST_WORLDS + 4u * (gaddr)n);
        char text[LIST_ROW_BYTES];
        memcpy(text, G(list + LIST_ROW_TEXT + LIST_ROW_BYTES * (gaddr)n), sizeof text);
        text[sizeof text - 1] = 0;
        openrac_mp_ui_table_row(g_ui, (uint32_t)world, 1);
        openrac_mp_ui_table_cell(g_ui, "name", text);
        const gaddr record = game_record(world);
        /* A row the lobby has no record of has no player count yet. */
        if (record != 0) {
            char players[16];
            snprintf(
                players,
                sizeof players,
                "%d/%d",
                GREF(uint16_t, record + RECORD_PLAYERS),
                GREF(uint16_t, record + RECORD_MAX_PLAYERS)
            );
            openrac_mp_ui_table_cell(g_ui, "players", players);
        }
    }
    openrac_mp_ui_table_end(g_ui);
}

/* OPENRAC_RAC3_MP_UI_MOCK: made-up players at the end of the table being filled, for trying the
 * lists past eight rows before the game's tables can hold more. */
static void add_mock_players(void) {
    const char* value = openrac_env_string("OPENRAC_RAC3_MP_UI_MOCK");
    int count = value != NULL ? atoi(value) : 0;
    /* A count past the limit is cut to it. */
    if (count > MOCK_MAX) {
        count = MOCK_MAX;
    }
    /* One row a made-up player, alternating teams. */
    for (int n = 0; n < count; n++) {
        char name[16];
        char score[8];
        snprintf(name, sizeof name, "PLAYER %d", n + 2);
        snprintf(score, sizeof score, "%d", (count - n) * 3);
        openrac_mp_ui_table_row(g_ui, MOCK_KEY + (uint32_t)n, 1);
        openrac_mp_ui_table_cell(g_ui, "name", name);
        openrac_mp_ui_table_cell(g_ui, "team", n % 2 == 0 ? "1" : "2");
        openrac_mp_ui_table_cell(g_ui, "kills", score);
        openrac_mp_ui_table_cell(g_ui, "score", score);
        openrac_mp_ui_table_cell(g_ui, "deaths", n % 4 == 0 ? "2" : "1");
        openrac_mp_ui_table_cell(g_ui, "ready", n % 3 == 0 ? "\x11" : "");
    }
}

/* The "scores" table: the match's results as the room kept them (rac3_room_capture_results), a row
 * a player, best first by the score the game shows (kills less suicides), then fewer deaths, as
 * the game ranks its rows (seen in game code: L39_006A6FC8). */
static int beats(const rac3_room_result* a, const rac3_room_result* b) {
    return a->score > b->score || (a->score == b->score && a->deaths < b->deaths);
}

static void fill_scores(void) {
    int order[RAC3_ROOM_MAX];
    rac3_room_capture_results();
    const int count = rac3_room_result_count();
    /* Best first: a stable insertion sort. */
    for (int n = 0; n < count; n++) {
        int k = n;
        /* Moves past every row this one beats. */
        while (k > 0 && beats(rac3_room_result_at(n), rac3_room_result_at(order[k - 1]))) {
            order[k] = order[k - 1];
            k--;
        }
        order[k] = n;
    }
    openrac_mp_ui_table_begin(g_ui, "scores");
    /* One row a player, in that order. */
    for (int n = 0; n < count; n++) {
        const rac3_room_result* r = rac3_room_result_at(order[n]);
        char number[4][12];
        snprintf(number[0], sizeof number[0], "%d", r->score);
        snprintf(number[1], sizeof number[1], "%d", r->kills);
        snprintf(number[2], sizeof number[2], "%d", r->deaths);
        snprintf(number[3], sizeof number[3], "%d", r->team + 1);
        openrac_mp_ui_table_row(g_ui, (uint32_t)order[n], 1);
        openrac_mp_ui_table_cell(g_ui, "name", r->name);
        openrac_mp_ui_table_cell(g_ui, "score", number[0]);
        openrac_mp_ui_table_cell(g_ui, "kills", number[1]);
        openrac_mp_ui_table_cell(g_ui, "deaths", number[2]);
        openrac_mp_ui_table_cell(g_ui, "team", number[3]);
    }
    add_mock_players();
    openrac_mp_ui_table_end(g_ui);
}

/* The "players" table: the room's players in slot order (the staging room compacts its rows the
 * same way, L39_006C20B0), keyed by DME client. */
static void fill_players(void) {
    openrac_mp_ui_table_begin(g_ui, "players");
    /* The room's player limit, so the list shows joined/MAX. */
    openrac_mp_ui_table_capacity(g_ui, rac3_room_width());
    /* The slots in order; an empty one has no row. */
    for (int slot = 0; slot < rac3_room_width(); slot++) {
        const int client = rac3_room_slot_client(slot);
        const rac3_room_player* p = rac3_room_player_of(client);
        if (client < 0 || p == NULL) {
            continue;
        }
        char team[8] = "";
        if (rac3_room_team_play()) {
            snprintf(team, sizeof team, "%d", p->team + 1);
        }
        const char* skin = skin_name(p->skin);
        openrac_mp_ui_table_row(g_ui, (uint32_t)client, 1);
        openrac_mp_ui_table_cell(g_ui, "name", p->name);
        openrac_mp_ui_table_cell(g_ui, "clan", p->clan);
        openrac_mp_ui_table_cell(g_ui, "team", team);
        openrac_mp_ui_table_cell(g_ui, "skin", skin != NULL ? skin : "");
        openrac_mp_ui_table_cell(g_ui, "ready", p->ready ? "\x11" : "");
    }
    add_mock_players();
    openrac_mp_ui_table_end(g_ui);
}

/* The staging room's status line: the joiners ready against the players, START's state for the
 * host, the countdown once START is pressed. */
static void fill_status(void) {
    char text[48];
    const int countdown = rac3_room_countdown();
    if (countdown >= 0) {
        snprintf(text, sizeof text, "STARTING IN %d", countdown);
    } else {
        const int ready = rac3_room_ready_count(), count = rac3_room_count();
        snprintf(
            text,
            sizeof text,
            "READY %d/%d%s",
            ready,
            count,
            rac3_room_is_host() && ready == count ? "  PRESS START" : ""
        );
    }
    openrac_mp_ui_set_text(g_ui, "status", text);
}

/* ---- Binding ---- */

/* The native screen a game screen id carries, its game widget, and where it is drawn. */
static const char* bound_screen(int id) {
    /* Each screen id with a native screen over it. */
    switch (id) {
        case SCREEN_GAME_LIST_A:
        case SCREEN_GAME_LIST_B:
            return "game_list";
        case SCREEN_STAGING:
            return "staging";
        case SCREEN_RESULTS:
            return "scoreboard";
        default:
            return NULL;
    }
}

/* The rectangle of game widget `n` of `screen` in the picture, and the pitch of its rows. */
static void widget_area(gaddr screen, int n, float out[4]) {
    const gaddr w = widget(screen, n) + WIDGET_RECT;
    const float cx = GREF(float, w), cy = GREF(float, w + 4u);
    const float width = GREF(float, w + 8u), height = GREF(float, w + 12u);
    out[0] = (cx - width * 0.5f) * PICTURE_W;
    out[1] = (cy - height * 0.5f) * PICTURE_H;
    out[2] = width * PICTURE_W;
    out[3] = height * PICTURE_H;
}

/* Opens the native screen for game screen `id` (`screen` its object), or none. */
static void open_for(int id, gaddr screen) {
    openrac_mp_ui_close(g_ui);
    g_bound = -1;
    const char* name = bound_screen(id);
    /* A screen with no native screen keeps the game's own. */
    if (name == NULL) {
        return;
    }
    /* The staging room and the scoreboard are the port's room's (mp_room.c): without one (offline,
     * a disc whose tables stay 8) the vanilla description keeps the game's. */
    if (g_vanilla && (id == SCREEN_STAGING || id == SCREEN_RESULTS) && !rac3_room_active()) {
        return;
    }
    float area[4];
    /* The results screen's eight rows: 230 + 21r pixels down, header 208 (L39_006A69C0's tables). */
    if (id == SCREEN_RESULTS) {
        g_line = RESULTS_PITCH;
        area[0] = RESULTS_X, area[2] = RESULTS_W;
        area[1] = RESULTS_TOP - g_line * 0.5f;
        area[3] = g_line * GS_SLOTS;
    } else if (id == SCREEN_STAGING) {
        /* The eight name rows of the staging room, 0.648 + (n - 3) * 0.412 / 9 of the picture's
         * height, from the left of the names to the ready ticks (L39_006C1730). */
        const float game_line = 0.412f / 9.0f * PICTURE_H;
        /* The room shows up to twelve rows, a line for its count and the status line, in the
         * height of the game's eight; a longer list scrolls. */
        const int width = rac3_room_width();
        const int rows = width > 12 ? 12 : (width < 2 ? 2 : width);
        g_line = game_line * (float)GS_SLOTS / (float)(rows + 2);
        area[0] = 50.0f, area[2] = 412.0f;
        area[1] = (0.648f - 3.0f * 0.412f / 9.0f) * PICTURE_H - game_line * 0.5f;
        area[3] = g_line * ((float)rows + 2.5f);
    } else {
        const gaddr list = widget(screen, LIST_WIDGET);
        int32_t shown = GREF(int32_t, list + LIST_ROWS_SHOWN);
        widget_area(screen, LIST_WIDGET, area);
        g_line = shown > 0 ? area[3] / (float)shown : 20.0f;
    }
    /* The description may leave the screen out, which keeps the game's. */
    if (openrac_mp_ui_open(g_ui, name, area[0], area[1], area[2], area[3], OPENRAC_MP_UI_OVERLAY)) {
        g_bound = id;
        g_screen = screen;
    }
}

/* Whether the native screen has the cursor: its game widget (the list, a staging name) is the
 * game screen's focus. */
static int native_focused(void) {
    const int32_t focus = GREF(int32_t, g_screen + SCREEN_FOCUS);
    /* The results screen's rows take no cursor in the game: the native table always has it. */
    if (g_bound == SCREEN_RESULTS) {
        return 1;
    }
    /* Staging: the name cells; the lists: the list widget. */
    if (g_bound == SCREEN_STAGING) {
        return focus >= STAGING_NAME_WIDGET && focus < STAGING_NAME_WIDGET + GS_SLOTS;
    }
    return focus == LIST_WIDGET;
}

/* The game widgets the native screen draws in place of, at most HIDDEN_MAX: their pointers and
 * their modes while hidden. */
#define HIDDEN_MAX (STAGING_LAST_WIDGET - STAGING_NAME_WIDGET + 1)

/* Hides the game widgets the native screen draws in place of, for the length of the screens' draw
 * only (mode 0 hides a widget's elements as its draw runs, L39_00682308). Their modes come back
 * before the next update: the game's focus skips a hidden widget, and the cursor must still reach
 * the list. Returns how many were hidden; `saved` gets their modes. */
static int hide_replaced(gaddr widgets[HIDDEN_MAX], int32_t saved[HIDDEN_MAX]) {
    int count = 0;
    /* Results: the rows' cells and colour bars; staging: every cell of the eight rows; the lists:
     * the list widget. */
    if (g_bound == SCREEN_RESULTS) {
        /* Colour bars 0xD..0x14, the shown flags between, and the five cells of each row. */
        for (int n = RESULTS_FIRST_WIDGET; n <= RESULTS_LAST_WIDGET; n++) {
            /* The column headers 0x15..0x19 stay the game's. */
            if (n < RESULTS_HEADERS || n >= RESULTS_HEADERS + RESULTS_COLUMNS) {
                widgets[count++] = widget(g_screen, n);
            }
        }
    } else if (g_bound == SCREEN_STAGING) {
        /* Name, skill, voice, team, skin and ready widgets of each row. */
        for (int n = STAGING_NAME_WIDGET; n <= STAGING_LAST_WIDGET; n++) {
            widgets[count++] = widget(g_screen, n);
        }
    } else {
        const gaddr list = widget(g_screen, LIST_WIDGET);
        /* An empty list keeps the game's own line saying so. */
        if (GREF(int32_t, list + LIST_ROW_COUNT) > 0) {
            widgets[count++] = list;
        }
    }
    /* Each widget hidden, its mode kept for after the draw. */
    for (int n = 0; n < count; n++) {
        saved[n] = GREF(int32_t, widgets[n] + WIDGET_MODE);
        GREF(int32_t, widgets[n] + WIDGET_MODE) = 0;
    }
    return count;
}

/* The game's event as a native one. */
static int native_event(int game) {
    /* Game event codes (L39_00686138) to native ones. */
    switch (game) {
        case GAME_UP:
            return OPENRAC_MP_UI_UP;
        case GAME_DOWN:
            return OPENRAC_MP_UI_DOWN;
        case GAME_LEFT:
            return OPENRAC_MP_UI_LEFT;
        case GAME_RIGHT:
            return OPENRAC_MP_UI_RIGHT;
        case GAME_TRIANGLE:
            return OPENRAC_MP_UI_TRIANGLE;
        case GAME_CIRCLE:
            return OPENRAC_MP_UI_CIRCLE;
        case GAME_CROSS:
            return OPENRAC_MP_UI_CROSS;
        case GAME_SQUARE:
            return OPENRAC_MP_UI_SQUARE;
        case GAME_SELECT:
            return OPENRAC_MP_UI_SELECT;
        case GAME_START:
            return OPENRAC_MP_UI_START;
        case GAME_L1:
            return OPENRAC_MP_UI_L1;
        case GAME_R1:
            return OPENRAC_MP_UI_R1;
        default:
            return OPENRAC_MP_UI_NONE;
    }
}

/* A game action chosen on row `row`: the game's own cursor goes there and the game gets Cross,
 * so its own code joins. Returns the event the game sees. */
static int game_action(const openrac_mp_ui_outcome* out) {
    /* Staging: a row's own action is the room's (room_action); a server's description that names
     * another game action on it leaves it to the game for its first eight rows. */
    if (g_bound == SCREEN_STAGING) {
        /* Rows past the game's eight have no game widget to choose. */
        if (out->row >= GS_SLOTS) {
            return 0;
        }
        mp_func_L39_0068B2F8((int)g_screen, STAGING_NAME_WIDGET + out->row, 0);
        return GAME_CROSS;
    }
    mp_func_L39_00682028((int)widget(g_screen, LIST_WIDGET), out->row); /* the list's SetCursor */
    return GAME_CROSS;
}

/* The player's own dialog (the description's "player" screen): its team and skin as the room has
 * them. Returns 0 when the description has none. */
static int open_player_dialog(void) {
    const rac3_room_player* p = rac3_room_player_of(rac3_room_my_client());
    if (p == NULL || !openrac_mp_ui_open_modal(g_ui, "player")) {
        return 0;
    }
    openrac_mp_ui_set_value(g_ui, "team", p->team);
    openrac_mp_ui_set_value(g_ui, "skin", p->skin);
    return 1;
}

/* The room's actions of the native screens (game:select on a staging row, game:team, game:skin,
 * game:ready). Returns 1 when `out` was one of them. */
static int room_action(const openrac_mp_ui_outcome* out) {
    if (!rac3_room_active() || out->action != OPENRAC_MP_UI_ACTION_GAME) {
        return 0;
    }
    if (strcmp(out->target, "team") == 0) {
        rac3_room_set_team(out->value);
    } else if (strcmp(out->target, "skin") == 0) {
        rac3_room_set_skin(out->value);
    } else if (strcmp(out->target, "ready") == 0) {
        rac3_room_toggle_ready();
    } else if (strcmp(out->target, "select") == 0 && g_bound == SCREEN_STAGING) {
        /* The player's own row opens its dialog; another's does nothing. */
        const int own = out->row_key == (uint32_t)rac3_room_my_client();
        if (!own || !open_player_dialog()) {
            mp_func_L39_00688340(own ? SOUND_SELECT : SOUND_DENIED, 0, 0);
            return own ? 0 : 1;
        }
        mp_func_L39_00688340(SOUND_SELECT, 0, 0);
    } else {
        return 0;
    }
    return 1;
}

/* READY by the room (rac3_room_toggle_ready): a Circle press on the staging screen is the room's on
 * every console and is left out of the pad word until the button is released, so the game's own
 * READY, which reads the pad's pressed bits each game tick, never sees it (a console too slow to
 * draw would run several ticks in the window frame of one press, and toggle each time). (libpad's
 * word is active low; Circle is bit 0x2000.) */
uint16_t rac3_mp_ui_pad_filter(uint16_t word) {
    static int down, taken;
    const int circle = (word & 0x2000u) == 0;
    if (!circle) {
        down = taken = 0;
    } else if (!down) {
        down = 1;
        taken = g_on_staging && (g_ui == NULL || !openrac_mp_ui_modal(g_ui))
                && rac3_room_toggle_ready();
    }
    return taken ? (uint16_t)(word | 0x2000u) : word;
}

/* START, the host's: the pad's Start button, and Cross on the game's START widget, ask the room
 * (rac3_room_start: every joiner ready; the settings are written for the match). When it agrees
 * the game's cursor goes to its START widget and Cross is the event it sees, so the game's own
 * START runs (its countdown, the map); else START refuses with the denied sound. Up from the first
 * row of the native list only moves the game's cursor to START (the game's list widget stops at
 * eight rows, and its cursor would wrap to the eighth). Returns 1 when the event was taken; `event`
 * is then replaced by the one the game gets. */
static int room_start(int mgr, int* event) {
    const int32_t current = GREF(int32_t, (gaddr)mgr + MGR_CURRENT);
    if (!g_on_staging || current < 0 || !rac3_room_is_host() || g_hidden
        || (g_ui != NULL && openrac_mp_ui_modal(g_ui))) {
        return 0;
    }
    const gaddr screen = GREF(gaddr, (gaddr)mgr + MGR_SCREENS + 4u * (gaddr)current);
    if (screen == 0) {
        return 0;
    }
    const int32_t focus = GREF(int32_t, screen + SCREEN_FOCUS);
    const int up_from_top = *event == GAME_UP && g_ui != NULL && g_bound == SCREEN_STAGING
                            && focus == STAGING_NAME_WIDGET && openrac_mp_ui_cursor(g_ui) == 0;
    const int press =
        *event == GAME_START || (*event == GAME_CROSS && focus == STAGING_START_WIDGET);
    if (!press && !up_from_top) {
        return 0;
    }
    mp_func_L39_0068B2F8((int)screen, STAGING_START_WIDGET, 0);
    if (up_from_top) {
        mp_func_L39_00688340(SOUND_MOVE, 0, 0);
        *event = 0;
        return 1;
    }
    if (!rac3_room_start()) {
        mp_func_L39_00688340(SOUND_DENIED, 0, 0);
        *event = 0;
        return 1;
    }
    *event = GAME_CROSS;
    return 1;
}

/* The menu event read (L39_00686138): the native screen's, while it has the cursor. */
int mp_func_L39_00686138(int mgr, int pad) {
    int event = mp_func_L39_00686138__game(mgr, pad);
    if ((gaddr)pad == GREF(gaddr, ACTIVE_PAD) && room_start(mgr, &event)) {
        return event;
    }
    /* Circle on the staging screen is the room's READY (the pad filter takes it first; a press it
     * did not see, the game's own READY would read from the pad's pressed bits, pad +0x1C4). */
    if ((gaddr)pad == GREF(gaddr, ACTIVE_PAD) && (g_ui == NULL || !openrac_mp_ui_modal(g_ui))) {
        const int32_t current = GREF(int32_t, (gaddr)mgr + MGR_CURRENT);
        const gaddr pressed = (gaddr)pad + PAD_PRESSED;
        if ((event == GAME_CIRCLE || (GREF(uint32_t, pressed) & PAD_CIRCLE) != 0) && current >= 0
            && GREF(int32_t, (gaddr)mgr + MGR_IDS + 4u * (gaddr)current) == SCREEN_STAGING
            && rac3_room_toggle_ready()) {
            GREF(uint32_t, pressed) &= ~(uint32_t)PAD_CIRCLE;
            return 0;
        }
    }
    /* The game's own events for another pad, under a game popup, or with no native screen up. */
    if (g_ui == NULL || g_hidden || (gaddr)pad != GREF(gaddr, ACTIVE_PAD)) {
        return event;
    }
    const int modal = openrac_mp_ui_modal(g_ui);
    /* Down from the staging room's START comes into the native player list, whose game widgets
     * are hidden: the game's cursor goes to the first name and the native one to its top row. */
    if (!modal && g_bound == SCREEN_STAGING && event == GAME_DOWN
        && GREF(int32_t, g_screen + SCREEN_FOCUS) == STAGING_START_WIDGET) {
        mp_func_L39_0068B2F8((int)g_screen, STAGING_NAME_WIDGET, 0);
        openrac_mp_ui_enter(g_ui, 1);
        mp_func_L39_00688340(SOUND_MOVE, 0, 0);
        return 0;
    }
    /* A modal screen takes every event; else only a native screen with the cursor reads them. */
    if (!modal && (g_bound < 0 || !native_focused())) {
        return event;
    }
    const int mapped = native_event(event);
    /* An event the native screens do not read: the game's, unless a modal screen is up. */
    if (mapped == OPENRAC_MP_UI_NONE) {
        return modal ? 0 : event;
    }
    openrac_mp_ui_skin s = skin();
    openrac_mp_ui_outcome out;
    openrac_mp_ui_event(g_ui, &s, mapped, &out);
    /* An event the native screen did not use goes on to the game, never past a modal screen. */
    if (!out.consumed && !modal) {
        return event;
    }
    /* The room's own actions, then the game's: its cursor moved, then Cross for the game. */
    if (room_action(&out)) {
        return 0;
    }
    if (out.action == OPENRAC_MP_UI_ACTION_GAME) {
        return game_action(&out);
    }
    /* A server's action goes to the server. */
    if (out.action == OPENRAC_MP_UI_ACTION_SERVER) {
        rac3_mp_net_ui_action(out.target, out.value);
    }
    return 0;
}

/* The staging room's per-frame check (L39_006C30A0, which its event handler runs before it reads the
 * START byte): the game enables the host's START when the ready players it counts, slots 1 to 7 and
 * itself, are all the players (GameSettings +0x178). In the room the byte (screen +0x2A8) is the
 * room's: every joiner of every slot ready. */
int mp_func_L39_006C30A0(int screen_object) {
    const int result = mp_func_L39_006C30A0__game(screen_object);
    if (GREF(gaddr, GAME_SETTINGS) == 0 || !rac3_room_is_host()) {
        return result;
    }
    const int ready = rac3_room_ready_count(), count = rac3_room_count();
    GREF(uint8_t, (gaddr)screen_object + 0x2A8u) = ready == count;
    static int last = -1;
    if (ready != last) {
        last = ready;
        fprintf(
            stderr,
            "[info] rac3: room: %d ready of %d players: START %s\n",
            ready,
            count,
            ready == count ? "enabled" : "greyed"
        );
    }
    return result;
}

/* The screen manager's update (L39_006862D0): afterwards, the native screen follows the game's. */
int mp_func_L39_006862D0(int mgr) {
    {
        const int32_t now = GREF(int32_t, (gaddr)mgr + MGR_CURRENT);
        g_on_staging =
            now >= 0 && GREF(int32_t, (gaddr)mgr + MGR_IDS + 4u * (gaddr)now) == SCREEN_STAGING;
    }
    const int result = mp_func_L39_006862D0__game(mgr);
    /* Without memory there are no native screens; the game's run as ever. */
    if (!make_ui()) {
        return result;
    }
    const int32_t current = GREF(int32_t, (gaddr)mgr + MGR_CURRENT);
    const int32_t popup = GREF(int32_t, (gaddr)mgr + MGR_POPUP);
    const int id = current >= 0 ? GREF(int32_t, (gaddr)mgr + MGR_IDS + 4u * (gaddr)current) : -1;
    const gaddr screen =
        current >= 0 ? GREF(gaddr, (gaddr)mgr + MGR_SCREENS + 4u * (gaddr)current) : 0;
    g_hidden = popup > current;
    /* The game changed screens: the native one follows. */
    if (id != g_bound || screen != g_screen) {
        open_for(id, screen);
    }
    /* No native screen over this one. */
    if (g_bound < 0) {
        return result;
    }
    /* Each bound screen reads its own rows. */
    if (g_bound == SCREEN_RESULTS) {
        fill_scores();
    } else if (g_bound == SCREEN_STAGING) {
        fill_players();
        fill_status();
    } else {
        fill_games(g_screen);
    }
    return result;
}

/* The screen manager's draw (L39_00686768), where the screens' widgets update their elements: the
 * game widgets a native screen replaces are hidden for it alone. */
int mp_func_L39_00686768(int mgr) {
    /* Nothing replaced while no native screen is up, or under a game popup. */
    if (g_ui == NULL || g_bound < 0 || g_hidden) {
        return mp_func_L39_00686768__game(mgr);
    }
    gaddr widgets[HIDDEN_MAX];
    int32_t saved[HIDDEN_MAX];
    const int count = hide_replaced(widgets, saved);
    const int result = mp_func_L39_00686768__game(mgr);
    /* Their modes back, before the next update reads them. */
    for (int n = 0; n < count; n++) {
        GREF(int32_t, widgets[n] + WIDGET_MODE) = saved[n];
    }
    return result;
}

/* The GUI elements' render (L39_005D69C0): the native screen is drawn twice, its highlight bar
 * (the game's own element) placed before the elements draw and its text printed after them, over
 * the screen's panels. */
int mp_func_L39_005D69C0(int a0) {
    /* Only with the lobby up and nothing over its screen. */
    if (g_ui != NULL && (g_bound >= 0 || openrac_mp_ui_modal(g_ui)) && !g_hidden
        && GREF(gaddr, SCREEN_MANAGER) != 0) {
        openrac_mp_ui_skin s = skin();
        openrac_mp_ui_set_active(g_ui, native_focused());
        g_text_pass = 0;
        openrac_mp_ui_draw(g_ui, &s);
        const int result = mp_func_L39_005D69C0__game(a0);
        g_text_pass = 1;
        openrac_mp_ui_draw(g_ui, &s);
        g_frame++;
        return result;
    }
    return mp_func_L39_005D69C0__game(a0);
}

/* A server's description (the online layer hands it on): the screens open again with it. */
int rac3_mp_ui_describe(const char* json, size_t size) {
    /* A refused description keeps the one in force. */
    if (!make_ui() || !openrac_mp_ui_describe(g_ui, json, size)) {
        return 0;
    }
    g_bound = -1;
    g_vanilla = 0;
    return 1;
}

/* The players a room holds by the server's description, or `fallback`. */
int rac3_mp_ui_max_players(int fallback) {
    return g_ui != NULL ? openrac_mp_ui_max_players(g_ui, fallback) : fallback;
}

/* A server's notice (the online layer hands it on): shown in a modal notice over the lobby. */
void rac3_mp_ui_notice(const char* text) {
    /* Without the context there are no native screens to show it on; the log has it. */
    if (make_ui()) {
        openrac_mp_ui_notice(g_ui, text);
    }
}

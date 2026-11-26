/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The native multiplayer screens (port/mpui) as the games' C uses them: a game's binding (its host
 * C) picks the description (its vanilla one, or the server's), fills the tables from the game's
 * memory, opens a screen over one of the game's own, hands it the game's menu events and draws it
 * through a skin of the game's own routines. Implemented in lib/mp_ui.cpp. The binding owns its
 * context (openrac_mp_ui_create); the game's thread alone calls it. docs/port/MULTIPLAYER_UI.md describes the whole. */
#ifndef OPENRAC_MP_UI_H
#define OPENRAC_MP_UI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The menu events (mpui::Event): a binding maps its game's to these. */
enum {
    OPENRAC_MP_UI_NONE,
    OPENRAC_MP_UI_UP,
    OPENRAC_MP_UI_DOWN,
    OPENRAC_MP_UI_LEFT,
    OPENRAC_MP_UI_RIGHT,
    OPENRAC_MP_UI_CROSS,
    OPENRAC_MP_UI_CIRCLE,
    OPENRAC_MP_UI_TRIANGLE,
    OPENRAC_MP_UI_SQUARE,
    OPENRAC_MP_UI_L1,
    OPENRAC_MP_UI_R1,
    OPENRAC_MP_UI_START,
    OPENRAC_MP_UI_SELECT
};

/* The text styles (mpui::TextStyle) a skin draws. */
enum {
    OPENRAC_MP_UI_TITLE,
    OPENRAC_MP_UI_HEADING,
    OPENRAC_MP_UI_NORMAL,
    OPENRAC_MP_UI_FOCUSED,
    OPENRAC_MP_UI_DISABLED
};

/* The menu sounds (mpui::Sound) a skin plays. */
enum {
    OPENRAC_MP_UI_SOUND_MOVE,
    OPENRAC_MP_UI_SOUND_SELECT,
    OPENRAC_MP_UI_SOUND_BACK,
    OPENRAC_MP_UI_SOUND_DENIED
};

/* What an action is (mpui::ActionKind). */
enum {
    OPENRAC_MP_UI_ACTION_NONE,
    OPENRAC_MP_UI_ACTION_BACK,
    OPENRAC_MP_UI_ACTION_OPEN,
    OPENRAC_MP_UI_ACTION_GAME,
    OPENRAC_MP_UI_ACTION_SERVER
};

/* A screen over a game screen (unused events go on to the game), or one alone. */
enum {
    OPENRAC_MP_UI_OVERLAY,
    OPENRAC_MP_UI_MODAL
};

/* A game's look: its font's measures and its own drawing routines. Positions are in the game's
 * 512 x 448 picture. The binding's; read during openrac_mp_ui_draw and openrac_mp_ui_event. */
typedef struct openrac_mp_ui_skin {
    void* user;
    /* The pitch of a table's rows. */
    float line_height;
    /* A modal screen's panel: its field's colour and its edge's (0xAABBGGRR, 0x80 alpha full). */
    uint32_t panel_rgba;
    uint32_t edge_rgba;
    /* The width of `length` bytes of `text` in `style`. */
    float (*text_width)(void* user, const char* text, size_t length, int style);
    /* The draw layer: `length` bytes of `text` with its top left at (x, y); a rectangle filled
     * with `rgba`; the game's image `sprite` (the binding's own numbering) over a rectangle; the
     * game's highlight bar over a rectangle. A game draws them with its own native 2D path. */
    void (*text)(void* user, float x, float y, const char* text, size_t length, int style);
    void (*rect)(void* user, float x, float y, float w, float h, uint32_t rgba);
    void (*sprite)(void* user, float x, float y, float w, float h, int sprite);
    void (*highlight)(void* user, float x, float y, float w, float h);
    /* A menu sound, OPENRAC_MP_UI_SOUND_*. */
    void (*sound)(void* user, int sound);
    /* Called before each layer is drawn: 0 the screen over the game's, 1 a modal screen over it
     * (a notice, a screen opened with open:), which has no game screen under it. */
    void (*layer)(void* user, int modal);
} openrac_mp_ui_skin;

/* What one event did (mpui::Outcome). `target` is valid until the next call. */
typedef struct openrac_mp_ui_outcome {
    int consumed;       /* 1: the game must not see the event */
    int action;         /* OPENRAC_MP_UI_ACTION_* */
    const char* target; /* the action's screen, game action or server key ("" for none) */
    uint32_t row_key;   /* a table's action: the row's key */
    int row;            /* a table's action: the row's index */
    int value;          /* a choice's action: its new value */
} openrac_mp_ui_outcome;

/* The screens' state: the description, the tables and the screen open. */
typedef struct openrac_mp_ui openrac_mp_ui;

/* Makes a context with no description (no screen opens until one is set), or NULL without memory. */
openrac_mp_ui* openrac_mp_ui_create(void);
/* Frees a context made by openrac_mp_ui_create (NULL does nothing). */
void openrac_mp_ui_destroy(openrac_mp_ui* ui);

/* Sets the description from `size` bytes of JSON (schema 1). Returns 1, or 0 when it is refused
 * (logged), keeping the one before. The first call is the binding's vanilla description. */
int openrac_mp_ui_describe(openrac_mp_ui* ui, const char* json, size_t size);
/* The players a room holds by the description, or `fallback` when it says nothing. */
int openrac_mp_ui_max_players(openrac_mp_ui* ui, int fallback);

/* Opens the description's screen `id` in (x, y, w, h), OPENRAC_MP_UI_OVERLAY or _MODAL, in place of
 * the one open. Returns 1, or 0 when the description has no such screen (nothing is open then). */
int openrac_mp_ui_open(
    openrac_mp_ui* ui, const char* id, float x, float y, float w, float h, int mode
);
/* Closes the screen open, if any. */
void openrac_mp_ui_close(openrac_mp_ui* ui);
/* The id of the screen open, or "" for none. */
const char* openrac_mp_ui_current(openrac_mp_ui* ui);

/* Where modal screens are drawn: the notices and the screens an `open:` action opens. */
void openrac_mp_ui_set_modal_area(openrac_mp_ui* ui, float x, float y, float w, float h);
/* Shows `text` in a modal notice with an OK button, over whatever is open, in place of a modal
 * screen open before. */
void openrac_mp_ui_notice(openrac_mp_ui* ui, const char* text);
/* 1 while a modal screen is up: it takes every event, and the game sees none. */
int openrac_mp_ui_modal(openrac_mp_ui* ui);

/* Fills table `name`: begin, then per row a row and its cells, then end, which replaces the rows. */
void openrac_mp_ui_table_begin(openrac_mp_ui* ui, const char* name);
void openrac_mp_ui_table_row(openrac_mp_ui* ui, uint32_t key, int enabled);
void openrac_mp_ui_table_cell(openrac_mp_ui* ui, const char* field, const char* text);
void openrac_mp_ui_table_end(openrac_mp_ui* ui);
/* Between begin and end: the most rows the table can hold (a room's player limit), shown as joined/MAX. */
void openrac_mp_ui_table_capacity(openrac_mp_ui* ui, int capacity);

/* Hands the screen open one OPENRAC_MP_UI_* event, the modal one first; `out` says what it did
 * (consumed 0 when no screen is open). Back on a modal screen closes it and an `open:` action opens
 * its screen as a modal one, both here: `out` reports them consumed with no action. */
void openrac_mp_ui_event(
    openrac_mp_ui* ui, const openrac_mp_ui_skin* skin, int event, openrac_mp_ui_outcome* out
);
/* Says whether the screen open has the cursor: 0 draws it without its highlight bar and focused
 * colours (the game's cursor is on another of its widgets). A screen opens active. */
void openrac_mp_ui_set_active(openrac_mp_ui* ui, int active);
/* Gives the screen open the cursor from the game's widgets: its first item (`from_top` 1) or its
 * last. */
void openrac_mp_ui_enter(openrac_mp_ui* ui, int from_top);
/* Draws the screen open, if any. */
void openrac_mp_ui_draw(openrac_mp_ui* ui, const openrac_mp_ui_skin* skin);
/* The open screen's cursor row and first row shown (both 0 when none is open). */
int openrac_mp_ui_cursor(openrac_mp_ui* ui);
int openrac_mp_ui_first_row(openrac_mp_ui* ui);

#ifdef __cplusplus
}
#endif

#endif

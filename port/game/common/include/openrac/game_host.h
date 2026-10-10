/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Between a game's program (main.cpp), the library replacements every game
 * shares (lib/) and what hostgen generates for each game (game_info.c,
 * libraries.c, port/tools/hostgen).
 *
 * The replacements are C, written against guest.h like the translated game:
 * their arguments are the game's values and addresses. They implement the
 * libraries' APIs as the games use them (libraries.tsv lists each one), from
 * what the game's own code shows and what is publicly known of each API;
 * nothing of Sony's is in them. Below them, the port never models the
 * console: a read is a read of the extracted disc image, a memory card is a
 * folder, a frame ends where the game waits for vsync, and the game's display
 * list goes to the port's renderer as data.
 */
#ifndef OPENRAC_GAME_HOST_H
#define OPENRAC_GAME_HOST_H

#include "openrac/guest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The game, from its hostgen.json (generated game_info.c) ---- */

typedef struct openrac_game_info {
    const char* id;     /* "rac1-pal": port/game/<id>, the program openrac-<id> */
    const char* title;  /* "Ratchet & Clank (PAL)" */
    const char* game;   /* "rac1": the extractor's and the launcher's name for it */
    const char* serial; /* "SCES_509.16": its executable on the disc */
    int frame_rate;     /* 50 (PAL) or 60 */
    const char* entry;  /* the game's main, or NULL while it is not known */
} openrac_game_info;

extern const openrac_game_info openrac_game;

/* Runs the game's entry (its main). Returns false if the game has none yet. */
int openrac_game_run(void);

/* The level program that is loaded, or OPENRAC_OVERLAY_EXE: which overlay a
 * code address means. Generated as "the executable" unless the game's
 * hostgen.json says the game provides it ("overlay_hook", in host/). */
int openrac_game_loaded_overlay(void);

/* Every function by code address (generated functions.c). */
void openrac_game_register_functions(void);

/* ---- Set by the program before the game starts ---- */

/* The player's disc image, as the extractor kept it (iso_data/<game>/disc.iso).
 * The game reads it by sector. */
extern const char* openrac_game_disc_image;

/* The first memory card's folder (the second is beside it, <folder>.slot2). */
extern const char* openrac_game_card_dir;

/* The console's language setting (sceScfGetLanguage): 1 English, 2 French,
 * 3 Spanish, 4 German, 5 Italian. */
extern int openrac_game_language;

/* ---- Provided by the program, called by the replacements ---- */

/* The game finished a frame and waits for vsync (sceGsSyncV): present what
 * the renderer has, pump input, keep the frame rate. Returns the field the
 * console would report (0 or 1, alternating). */
int openrac_game_vsync(void);

/* The game sends a DMA chain (sceDmaSend): channel is the game address of
 * the channel's registers (0x10009000 is VIF1, the display list; 0x1000A000
 * is the GIF), tag the address of the chain's first tag. The renderer reads
 * it as data, the way OpenGOAL reads Jak's DMA chains. */
void openrac_game_dma_send(gaddr channel, gaddr tag);

/* The display and drawing environment, decoded from the GS register values
 * the game puts (sceGsPutDispEnv, sceGsPutDrawEnv). */
typedef struct openrac_game_display {
    int width, height; /* the frame buffer shown, in pixels */
    int frame_base;    /* its address in the GS's memory, in 2048-word pages */
    int frame_width;   /* in 64-pixel units */
    int psm;           /* its pixel format */
    int x, y;          /* where the console put it on the screen */
} openrac_game_display;

void openrac_game_set_display(const openrac_game_display* display);
void openrac_game_set_video_mode(int interlace, int mode, int field_mode);

/* An image the game uploads to the GS's memory (sceGsExecLoadImage): its
 * destination and the game address of its pixels. The renderer turns it
 * into a texture (port/renderer, TexturePool); nothing models GS memory. */
typedef struct openrac_game_image {
    int base, width_units,
        psm; /* destination: base (64-word blocks), width (64-pixel units), format */
    int x, y, width, height;
    gaddr pixels;
} openrac_game_image;

void openrac_game_load_image(const openrac_game_image* image);

/* The buttons of a pad, in the console's bit order (active low, as the pad
 * library reports them) and its analog values; false if none is connected. */
/* Nonzero: no memory card is inserted in either port (--no-card). */
extern int openrac_game_no_card;

int openrac_game_pad(int port, uint16_t* buttons, uint8_t analog[4]);

/* ---- Inside the replacements ---- */

/* Interrupt handlers the game installed (AddIntcHandler, sceGsSyncVCallback,
 * AddDmacHandler): called by the replacements where the console would raise
 * the interrupt. */
void openrac_game_run_vsync_handlers(void);
void openrac_game_run_dmac_handlers(int channel);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_GAME_HOST_H */

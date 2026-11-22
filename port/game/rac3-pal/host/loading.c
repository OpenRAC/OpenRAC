/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's loading screen held as long as the console shows its stars. Between levels the
 * boot program flies Ratchet's ship through space (the loop in 0x003BD870, mode 6): each pass runs
 * the flight's update, 0x003BCE00(loaded), where `loaded` says the next level's files are read
 * (0x001D4B48 >= 2). The flight plays laps of its scene (0x00225700: +0x34 the tick, +0x40 the
 * lap's last), random laps while the files load, then, once they are read, the approach to the
 * planet: the flight's state at 0x002D2008 has +0x51 set, the planet's draw callback (0x003C1660)
 * draws its globe and 0x003C1890 prints the level's name, fading in over the lap's last 75 ticks.
 * When the approach's lap ends with the files read, 0x003BCE00 returns 1 and the level starts.
 *
 * On the console the files take long to read: the PAL game shows about 8 s of stars with the ship
 * as a point, then the approach with the globe and the name (retail footage, Veldin's loading). The
 * port reads them in a moment, and the approach began after the first lap. Here the files count as
 * read only once the stars have played HOLD_STARS passes (the lap playing then ends there), so the
 * approach comes after the console's 8 s of stars.
 *
 * The approach is a lap of about 120 passes, 2.4 s at 50 a second, but the console shows the globe
 * and the name about 6 s (retail footage): while it plays the level's files are unpacked in the
 * same loop, and a pass takes about two and a half vertical blanks. The scene's tick is the pass
 * count (0x003BDFB0: +0x34 goes up by one a pass), so the port stretches the ticks instead
 * (openrac_game_tick_stretch, APPROACH_STRETCH) while the approach plays: each pass is the same
 * pass, and the frames between ticks move the ship and the globe smoothly between them. Holding
 * the scene's tick back for some passes instead made the ship's parts look up joints the port does
 * not keep (0x003C3278 through the hand-written 0x003CA858), and waiting for more vertical blanks
 * inside the pass held the window's frames back.
 *
 * Start skips the hold while Skip cutscenes is on, as Going Commando's port skips its flights
 * (rac2/host/tools.c flight_skip): the flight then ends as soon as the game lets it.
 */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

#define FLIGHT_SCENE RAC3_ADDR(0x00225700u)
#define FLIGHT_TICK (FLIGHT_SCENE + 0x34u)
#define FLIGHT_LAST (FLIGHT_SCENE + 0x40u)
#define FLIGHT_STATE RAC3_ADDR(0x002D2008u)
#define FLIGHT_APPROACH 0x51u
/* The level loader's state (0x0039A5C0): 2 and above once the level's files are read. */
#define LOADER_STATE RAC3_ADDR(0x001D4B48u)
#define LOADER_READ 2

/* 8 s of stars at the game's 50 passes a second. */
#define HOLD_STARS 400

/* The console's approach, about 6 s, over its 2.4 s of passes. */
#define APPROACH_STRETCH 2.5f

int func_003BCE00__game(int loaded);

static int s_passes;
static int s_was_loaded = 1;
static int s_skipping;
/* Whether the flight is playing: from its first pass to the one that ends it. */
static int s_active;

/* One pass of the game's flight; the last one (it returns 1) ends it. `loaded` is whether the files
 * count as read: while they do not, the flight's own look at the loader (0x003BE2D0: state 2 and
 * above) sees them unread too. That look, at the end of each lap, frees the sound banks of the
 * scenes it no longer needs (0x003BBD20: all but the approach's) and lets the loader go on; the
 * next lap is the approach only when `loaded` says so. With the two disagreeing, the port's held
 * stars went on with a random scene whose sound bank had just been freed: its sound did not play,
 * and the loading screen was silent from about 5 s in until the approach. */
static int flight_pass(int loaded) {
    const int32_t state = GREF(int32_t, LOADER_STATE);
    const int hide = !loaded && state >= LOADER_READ;
    if (hide) {
        GREF(int32_t, LOADER_STATE) = LOADER_READ - 1;
    }
    const int over = func_003BCE00__game(loaded);
    if (hide && GREF(int32_t, LOADER_STATE) == LOADER_READ - 1) {
        GREF(int32_t, LOADER_STATE) = state;
    }
    const int approach = !over && !s_skipping && GREF(uint8_t, FLIGHT_STATE + FLIGHT_APPROACH) != 0;
    openrac_game_tick_stretch = approach ? APPROACH_STRETCH : 1.0f;
    if (over) {
        s_active = 0;
    }
    return over;
}

int func_003BCE00(int loaded) {
    if (!loaded && s_was_loaded) {
        /* A new load: the files are not read yet. */
        s_passes = 0;
        s_skipping = 0;
    }
    s_was_loaded = loaded;
    s_active = 1;
    ++s_passes;
    if (s_skipping || GREF(uint8_t, FLIGHT_STATE + FLIGHT_APPROACH) != 0) {
        return flight_pass(loaded);
    }
    if (loaded && s_passes >= HOLD_STARS) {
        /* The files are read and the stars have played: the lap playing ends now, so the approach
         * is next. */
        GREF(int32_t, FLIGHT_TICK) = GREF(int16_t, FLIGHT_LAST);
        return flight_pass(1);
    }
    return flight_pass(loaded && s_passes >= HOLD_STARS);
}

/* Start with Skip cutscenes on (openrac_game_skip): the flight ends as soon as the game lets it. */
void rac3_loading_skip(void) {
    if (s_active) {
        s_skipping = 1;
        openrac_game_tick_stretch = 1.0f;
    }
}

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
 * On the console the files take long to read, and the flight plays random laps of its scene
 * meanwhile (retail footage of a New Game's first load: three laps, then the approach). The port
 * reads them in a moment, so the files count as read here only once LOAD_SECONDS of the game's
 * passes (a pass is a tick, 50 a second: 0x003BDFB0 adds one to the scene's +0x34 a pass) have
 * gone by. The game's own code then plays its laps, ends the one playing, and plays the approach
 * (a lap of 122 ticks) at its normal speed; nothing is stretched or forced. OPENRAC_LOAD_SECONDS
 * sets the duration (0 loads at once: the approach follows the first lap).
 *
 * Start skips the flight while Skip cutscenes is on, as Going Commando's port skips its flights
 * (rac2/host/tools.c flight_skip): the hold is dropped, and once the level's files are read the lap
 * playing ends there (its tick at its last), and so does the approach that follows it, so the level
 * starts at once. Skipping only the hold left the lap playing and the whole approach to run: some
 * 5 s more of the flight, and Start looked as if it did nothing.
 *
 * Leaving a planet, the flight is the level's own copy (tools.c, rac3_level_flight), which this
 * file's wrap of the boot program's update does not reach: the skip there ends the laps of the
 * level's copy of the flight's scene from the frame hook, found in the level's copy of the lap
 * (0x003BDFB0's, Veldin's 0x004D8220) by the address its first instructions form.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

#define FLIGHT_SCENE RAC3_ADDR(0x00225700u)
#define FLIGHT_TICK (FLIGHT_SCENE + 0x34u)
#define FLIGHT_LAST (FLIGHT_SCENE + 0x40u)
#define FLIGHT_STATE RAC3_ADDR(0x002D2008u)
#define FLIGHT_APPROACH 0x51u
/* The level loader's state (0x0039A5C0): 2 and above once the level's files are read. */
#define LOADER_STATE RAC3_ADDR(0x001D4B48u)
#define LOADER_READ 2

/* How long the console takes to read a level's files, in seconds of the game's 50 passes a second.
 * Calibrated on retail's New Game load: three random laps (about 200 to 250 ticks each) play
 * before the approach, so the files are read after the second lap ends and before the third's. */
#define LOAD_SECONDS 10.0
#define PASSES_PER_SECOND 50

/* The passes the files take to read: OPENRAC_LOAD_SECONDS, else LOAD_SECONDS. */
static int load_passes(void) {
    static int passes = -1;
    if (passes < 0) {
        double seconds = LOAD_SECONDS;
        const char* v = openrac_env_string("OPENRAC_LOAD_SECONDS");
        if (v != NULL && *v != 0) {
            seconds = atof(v);
        }
        passes = seconds > 0.0 ? (int)(seconds * PASSES_PER_SECOND) : 0;
    }
    return passes;
}

int func_003BCE00__game(int loaded);
int rac3_level_flight(void); /* tools.c */

/* Veldin's places of the levels' copies of the lap (0x003BDFB0) and of the lap's end (0x003C1510),
 * whose first instructions form the flight's scene (0x00225700) and its state (0x002D2008). */
#define LEVEL_LAP RAC3_LADDR(1, 0x004D8220u)
#define LEVEL_LAP_END RAC3_LADDR(1, 0x004DB780u)

static int s_passes;
static int s_was_loaded = 1;
static int s_skipping;
/* Whether the flight is playing: from its first pass to the one that ends it. */
static int s_active;

/* OPENRAC_RAC3_FLIGHT_LOG: one line a pass of the boot program's flight, on stderr: the pass, the
 * files' state, the scene playing (0x0016C580 +0x64) and its tick against its last, the approach,
 * the scene pack's parts (0x001D8CD4) and the flight's way (0x001D8CE8 to 0x001D8CEC). */
static void flight_log(int loaded, int over) {
    static int on = -1;
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC3_FLIGHT_LOG");
        on = v != NULL && *v != 0 && *v != '0';
    }
    if (!on) {
        return;
    }
    fprintf(
        stderr,
        "flight pass %d loaded %d loader %d scene %d tick %d/%d approach %d parts %#x "
        "from %d to %d over %d\n",
        s_passes,
        loaded,
        GREF(int32_t, LOADER_STATE),
        GREF(int32_t, RAC3_ADDR(0x0016C580u) + 0x64u),
        GREF(int32_t, FLIGHT_TICK),
        GREF(int16_t, FLIGHT_LAST),
        GREF(uint8_t, FLIGHT_STATE + FLIGHT_APPROACH),
        GREF(uint32_t, RAC3_ADDR(0x001D8CD4u)),
        GREF(int32_t, RAC3_ADDR(0x001D8CE8u)),
        GREF(int32_t, RAC3_ADDR(0x001D8CECu)),
        over
    );
}

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
    flight_log(loaded, over);
    if (over) {
        s_active = 0;
    }
    return over;
}

/* The lap playing ends now: the next pass's lap is over (0x003BDFB0: past its last). */
static void end_lap(gaddr scene) {
    GREF(int32_t, scene + 0x34u) = GREF(int16_t, scene + 0x40u);
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
    if (s_skipping && loaded) {
        end_lap(FLIGHT_SCENE);
    }
    if (s_skipping || GREF(uint8_t, FLIGHT_STATE + FLIGHT_APPROACH) != 0) {
        return flight_pass(loaded);
    }
    return flight_pass(loaded && s_passes >= load_passes());
}

/* The address the first lui and the first addiu from its register form in the loaded level's copy
 * of a function at Veldin's place, 0 when its code has no such pair there. */
static gaddr level_formed(gaddr veldin_place) {
    if (veldin_place == 0) {
        return 0; /* a release whose levels' copy is not paired (NTSC-U's rac3_addresses.h) */
    }
    const gaddr code = OPENRAC_LCODE(1, veldin_place);
    if (code == 0 || code + 64u > openrac_guest_ram_size) {
        return 0;
    }
    uint32_t hi = 0;
    unsigned reg = 32;
    for (unsigned i = 0; i < 12; ++i) {
        const uint32_t w = GREF(uint32_t, code + 4u * i);
        const unsigned op = w >> 26, rs = (w >> 21) & 31u, rt = (w >> 16) & 31u;
        if (op == 0x0Fu && reg == 32) { /* lui */
            hi = w << 16;
            reg = rt;
        } else if (op == 0x09u && reg != 32 && rs == reg) { /* addiu */
            const gaddr at = hi + (gaddr)(int32_t)(int16_t)(w & 0xFFFFu);
            return at >= 0x001D5680u && at < 0x00700000u ? at : 0;
        }
    }
    return 0;
}

/* The flight's state (render.c: the planet's picture), 0 outside a flight. */
gaddr rac3_loading_state(void) {
    if (rac3_level_flight() == 0) {
        return level_formed(LEVEL_LAP_END);
    }
    return s_active ? FLIGHT_STATE : 0;
}

/* Start was pressed with Skip cutscenes on during a level's own flight. */
static int s_level_skipping;

/* Start with Skip cutscenes on (openrac_game_skip): the flight ends as soon as the game lets it. */
void rac3_loading_skip(void) {
    if (rac3_level_flight() == 0) {
        s_level_skipping = 1;
    } else if (s_active) {
        s_skipping = 1;
    }
}

/* Once a frame: a level's own flight skipped ends its laps once the files are read (the level's
 * loop passes them as read the pass after the loader says so, and its lap's end then starts the
 * approach, which ends in its turn). */
static void (*s_previous_frame)(unsigned frame);

static void loading_frame(unsigned frame) {
    if (s_previous_frame != NULL) {
        s_previous_frame(frame);
    }
    if (!s_level_skipping) {
        return;
    }
    if (rac3_level_flight() != 0) {
        s_level_skipping = 0;
        return;
    }
    const gaddr scene = level_formed(LEVEL_LAP);
    if (scene != 0 && GREF(int32_t, LOADER_STATE) >= LOADER_READ) {
        end_lap(scene);
    }
}

void rac3_loading_install(void) {
    s_previous_frame = openrac_game_on_frame;
    openrac_game_on_frame = loading_frame;
}

/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando's options: the game's own 16:9, which the port keeps off, and the Sound page, which
 * a console forgets at power off and the port remembers between runs.
 *
 * The 16:9 option (the title's Options page, "16:9 (widescreen)", 0x001A7BB1): on, UpdateViewContext
 * narrows the vertical view by a quarter for a television that stretches the picture back. The
 * window's picture has the player's shape (settings `aspect`, the Graphics menu's Picture) and keeps
 * the game's vertical view, wider pictures showing more to the sides; on as well, the world would be
 * squeezed. So the flag is kept off, each frame.
 *
 * The Sound page (func_002D7DB8 draws it): the playback mode (0x001A7B98, a flag: stereo or mono),
 * the music volume (0x001A7B9C) and the sound volume (0x001A7BA0), both 0 to 0x400, the same three
 * words in the same order as Ratchet & Clank's (rac1/host/options.c), under the same keys of the
 * settings file. func_002E4860 hands the volumes to the mixer's groups and the sound's reset
 * (func_002E48E8) both; the remembered values go in before its first run. The language is not
 * remembered here: its word (0x001A7BB4, a byte, set by the menus' action type 9) is set at the start
 * by code still in assembly, not read yet.
 */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define WIDESCREEN 0x001A7BB1u
#define SOUND 0x001A7B98u
#define SOUND_WORDS 3
#define SOUND_WORD(i) (SOUND + 4u * (i))
#define SOUND_PLAYBACK 0u
#define LOUDEST 0x400

static const char* const sound_keys[SOUND_WORDS] = {"game_playback", "game_music_volume", "game_sound_volume"};

/* Whether the Sound page has the remembered values: from the sound's first reset on. */
static int sound_put;

/* The sound's reset: at the game's start it gives the mixer the Sound page's values, so the
 * remembered ones go in first. */
void func_002E48E8(void) {
    if (!sound_put) {
        sound_put = 1;
        for (unsigned i = 0; i < SOUND_WORDS; ++i) {
            const int value = openrac_game_option(sound_keys[i], -1);
            /* None remembered (-1), or a volume past the loudest in a hand-edited file: the game's
             * own value stays. */
            if (value >= 0 && (i == SOUND_PLAYBACK || value <= LOUDEST)) {
                GREF(int, SOUND_WORD(i)) = value;
            }
        }
    }
    func_002E48E8__game();
}

/* Each frame (tools.c's on_frame): the game's 16:9 off, and the Sound page as the player left it,
 * for the next run, once it has the remembered values. */
void rac2_options_frame(void) {
    GREF(uint8_t, WIDESCREEN) = 0;
    if (sound_put) {
        for (unsigned i = 0; i < SOUND_WORDS; ++i) {
            openrac_game_set_option(sound_keys[i], GREF(int, SOUND_WORD(i)));
        }
    }
}

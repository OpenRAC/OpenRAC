#ifndef GAME_AUDIO_STREAM_H
#define GAME_AUDIO_STREAM_H

/* Functions defined in src/game/audio/stream.c. */

#include "common.h"

extern void pad_resetState(void);
extern void process_menu_entry(void *, void *, int);
extern void request_audio_stream_break(void);

#endif /* GAME_AUDIO_STREAM_H */

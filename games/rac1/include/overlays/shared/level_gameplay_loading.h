#ifndef OVERLAYS_SHARED_LEVEL_GAMEPLAY_LOADING_H
#define OVERLAYS_SHARED_LEVEL_GAMEPLAY_LOADING_H

/* Functions defined in src/overlays/shared/level_gameplay_loading.c. */

#include "common.h"

extern void load_chunks_with_optional_fade(int, int);
extern int load_level_archive_resources(int, int);
extern void load_scene_chunks_with_fade(s32);
extern void start_cutscene_chunk_read(int);

#endif /* OVERLAYS_SHARED_LEVEL_GAMEPLAY_LOADING_H */

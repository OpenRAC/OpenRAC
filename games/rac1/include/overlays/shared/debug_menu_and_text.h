#ifndef OVERLAYS_SHARED_DEBUG_MENU_AND_TEXT_H
#define OVERLAYS_SHARED_DEBUG_MENU_AND_TEXT_H

/* Functions defined in src/overlays/shared/debug_menu_and_text.c. */

#include "common.h"

extern void Level_ResetGameplayGlobals(void);
extern void capture_draw_buffer(int);
extern void draw_debug_menu(void);
extern void draw_debug_profiler_if_enabled(void);
extern void draw_dialog_background_and_text(void);
extern void draw_framebuffer_with_timed_text(void);
extern void draw_page_contents(void);
extern void flush_debug_text_queue(void);
extern void frame_roll_counter(void);
extern void print_text_with_texture_3(int, int, int, void *, int);
extern void update_debug_camera(void);
extern void update_debug_menu(void);
extern void update_hero_from_debug_camera(void);

#endif /* OVERLAYS_SHARED_DEBUG_MENU_AND_TEXT_H */

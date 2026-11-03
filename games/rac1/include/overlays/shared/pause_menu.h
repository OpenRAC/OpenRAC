#ifndef OVERLAYS_SHARED_PAUSE_MENU_H
#define OVERLAYS_SHARED_PAUSE_MENU_H

/* Functions defined in src/overlays/shared/pause_menu.c. */

#include "common.h"

extern void draw_pause_menu_pages(int);
extern int find_or_assign_bolt_source_slot(int, int);
extern void jet_reset_camera(void *, void *);
extern void restore_or_clear_level_state(void);
extern void update_pause_menu(void);

#endif /* OVERLAYS_SHARED_PAUSE_MENU_H */

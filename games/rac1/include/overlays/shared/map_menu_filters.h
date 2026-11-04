#ifndef OVERLAYS_SHARED_MAP_MENU_FILTERS_H
#define OVERLAYS_SHARED_MAP_MENU_FILTERS_H

/* Functions defined in src/overlays/shared/map_menu_filters.c. */

#include "common.h"

extern int ensure_menu_cache_entry(int);
extern void map_world_to_uv(float, float, void *, void *, int);
extern void store_current_level_table_value(int, int);

#endif /* OVERLAYS_SHARED_MAP_MENU_FILTERS_H */

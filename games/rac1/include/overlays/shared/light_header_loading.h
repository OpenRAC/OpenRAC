#ifndef OVERLAYS_SHARED_LIGHT_HEADER_LOADING_H
#define OVERLAYS_SHARED_LIGHT_HEADER_LOADING_H

/* Functions defined in src/overlays/shared/light_header_loading.c. */

#include "common.h"

extern void load_two_int_two_float_header(void *);
extern void relocate_counted_pointer_table(void *);

#endif /* OVERLAYS_SHARED_LIGHT_HEADER_LOADING_H */

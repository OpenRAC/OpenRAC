#ifndef CORE_MPEG_MOTION_COMP_H
#define CORE_MPEG_MOTION_COMP_H

/* Functions defined in src/core/mpeg/motion_comp.c. */

#include "common.h"

extern void _extensionAndUserData(void *);
extern int _nextBit(void *, int);
extern void ipu_set_mpeg1(int);
extern int mpeg_dec_picture(void *);
extern int mpeg_next_header(void *);
extern void mpeg_output_frame(void *, int, int);
extern int mpeg_update_ref_image(void *, int);
extern void send_ipu_command(char *, unsigned int);
extern void wait_ipu_idle(void *);

#endif /* CORE_MPEG_MOTION_COMP_H */

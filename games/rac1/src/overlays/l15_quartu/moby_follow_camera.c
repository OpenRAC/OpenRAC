/* Starts at 0x002F9FF8 (was vendor_002F9FF8.c). */
/* Level 15 (Quartu): func_L15_002F9FF8 in a file of its own, built with -fno-force-mem
 * (config/file_cflags.txt). GCC 2.95 takes options per translation unit,
 * and the functions of the file it came from do not build with this one
 * (docs/BUILD_FIDELITY.md, "Flags"). */
#include "common.h"
#include "include_asm.h"

extern char hero_globals_base[];
extern char *quartu_camera_descriptors MACRO_ADDR;
extern char *quartu_camera_moby;
extern void update_moby_follow_camera(void *);

/* Runs the moby follow camera when the camera is free and in vehicle mode 2; returns -1. */
int dispatch_moby_follow_camera_update(char *moby)
{
    char *entry = quartu_camera_descriptors + (*(short *)(moby + 0x84) << 5);
    char *other = quartu_camera_moby;
    char *sub = *(char **)(entry + 0x1C);
    if (*(short *)(other + 0x86) == 0 && *(short *)(sub + 0x20) >= 0 && (unsigned char)hero_globals_base[0x2EC1] == 2) {
        update_moby_follow_camera(moby);
    }
    return -1;
}

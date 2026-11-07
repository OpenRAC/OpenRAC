/* Starts at 0x002F9FF8 (was vendor_002F9FF8.c). */
/* Level 15 (Quartu): dispatch_moby_follow_camera_update in a file of its own, built with -fno-force-mem
 * (config/file_cflags.txt). GCC 2.95 takes options per translation unit,
 * and the functions of the file it came from do not build with this one
 * (docs/reference/BUILD_FIDELITY.md, "Flags"). */
#include "common.h"
#include "include_asm.h"
#include "asm_functions/level_15.h"

extern char hero_globals_base[];
extern char *moby_slot_entry_table MACRO_ADDR;
extern char *quartu_camera_moby;

/* Runs the moby follow camera when the camera is free and in vehicle mode 2; returns -1. */
int dispatch_moby_follow_camera_update(char *moby) {
    char *entry = moby_slot_entry_table + (*(short *)(moby + 0x84) << 5);
    char *other = quartu_camera_moby;
    char *sub = *(char **)(entry + 0x1C);
    if (*(short *)(other + 0x86) == 0 && *(short *)(sub + 0x20) >= 0 && (unsigned char)hero_globals_base[0x2EC1] == 2) {
        update_moby_follow_camera(moby);
    }
    return -1;
}

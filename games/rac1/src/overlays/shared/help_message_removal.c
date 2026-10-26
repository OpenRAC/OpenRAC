/* Shared level code following mobyutil. Starts at 0x00265558 (was mobyutil_00265558.c). */
#include "common.h"
#include "include_asm.h"

extern int timed_hud_visible MACRO_ADDR;
extern int timed_hud_frames_left MACRO_ADDR;
extern int timed_hud_handle MACRO_ADDR;
extern int remove_hud_item(int);

/* Removes the timed help message if it belongs to selector a; returns 1 when removed. */
int remove_help_message_for_selector(int a) {
    if (timed_hud_visible == a) {
        if (timed_hud_handle != -1) {
            timed_hud_frames_left = 0;
            timed_hud_visible = 0;
            remove_hud_item(timed_hud_handle);
            timed_hud_handle = -1;
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/overlays", func_L00_002657B8);

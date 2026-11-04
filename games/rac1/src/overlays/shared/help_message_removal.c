/* Shared level code following mobyutil. Starts at 0x00265558 (was mobyutil_00265558.c). */
#include "common.h"
#include "include_asm.h"
#include "game/ui/hud.h"
#include "asm_functions/shared_levels.h"

extern int timed_hud_visible MACRO_ADDR;
extern int timed_hud_frames_left MACRO_ADDR;
extern int timed_hud_handle MACRO_ADDR;

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

/*
 * Builds the frame's moby update list from the level's moby array, then
 * walks that list calling each moby's per-frame update hooks.
 */

typedef struct MobyRecord {
    char pad0[0x20];
    signed char state; /* +0x20 */
    char pad1[0x28 - 0x21];
    struct MobyRecord *next; /* +0x28 */
    char pad2[0x34 - 0x2C];
    unsigned short flags; /* +0x34 */
    char pad3[0x74 - 0x36];
    void (*updateFn)(struct MobyRecord *); /* +0x74 */
    char pad4[0x100 - 0x78];
} MobyRecord;

extern int active_mobies;
extern MobyRecord *suck_moby_list MACRO_ADDR;
extern MobyRecord *moby_array_start MACRO_ADDR;

void link_and_update_all_mobys(void) {
    MobyRecord *cur;
    MobyRecord *m;
    MobyRecord *prev;
    unsigned char st;

    active_mobies = 0;
    suck_moby_list = 0;

    m = moby_array_start;
    prev = 0;
    st = m->state;
    if (st != 0xFF) {
        do {
            if (!(st & 0x80)) {
                if (!(m->flags & 0x2)) {
                    if (prev != 0)
                        prev->next = m;
                    else
                        suck_moby_list = m;
                    prev = m;
                }
            }
            m = (MobyRecord *)((char *)m + 0x100);
            st = m->state;
        } while (st != 0xFF);
    }
    if (prev != 0)
        prev->next = 0;

    cur = suck_moby_list;
    if (cur != 0) {
        do {
            if (cur->state >= 0) {
                if (!(cur->flags & 0x40)) {
                    MobyAnimAdvance(cur);
                }
                /* inline asm: hook call (beq/move/jal with empty slots). The original used inline asm here too: GCC
                   2.95 reorg leaves both slots empty only next to asm (see reorg.c stop_search_p). */
                {
                    int r;
                    __asm__ volatile("beq %1,$0,1f\n\tmove $4,%2\n\tjal $31,%1\n1:"
                                     : "=r"(r)
                                     : "r"(cur->updateFn), "r"(cur)
                                     : "$4", "$31", "memory");
                }
                if (!(cur->flags & 0x4)) {
                    moby_build_matrix(cur);
                }
            }
            cur = cur->next;
        } while (cur != 0);
    }
}

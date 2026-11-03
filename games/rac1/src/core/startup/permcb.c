#include "common.h"
#include "callees.h"
#include "structs/misc.h"

/*
 * permcb.cpp (0x12F308-0x12F348). Name from the NTSC split.
 */

/*
 * vsync_callback(int). The three counters are 64-bit (`ld`/`sd`) and
 * MACRO_ADDR; the last store lands in the jr delay slot, so it comes
 * out $gp-relative. 0x10000800 is T0_COUNT.
 */
extern long frm_clock_time MACRO_ADDR;
extern long frm_vsync_cnt MACRO_ADDR;
extern long frm_vsync_time MACRO_ADDR;

int vsync_callback(void) {
    frm_vsync_cnt++;
    frm_vsync_time = frm_clock_time + (unsigned int)*(volatile int *)0x10000800;
    return 0;
}

/* Retained reconstruction; Claude Opus 5.5 (opus-1, xhigh), 29 October 2026.
 * Profile: a unit of its own with -O2 -G0 -ffunction-sections -g (for example
 * src/levels/06_notak/L06_00402230-g.c plus one config/cflags.txt line).
 * US check.py: with the marked velocity[2] = 0.0f store below, 528/528 bytes,
 * 1 of 1 places match; without it, 516/528 bytes, 0/1 places. Not landed: that
 * store is dead (flow deletes it) and exists only to give the zero a second use.
 * What the two settings model:
 * - -g: haifa-sched counts line notes in region sizes (MAX_RGN_INSNS 100), so the
 *   ring loop is no longer an interblock region and the f12/f13 setup of the
 *   first RandomFloatRange call stays after the ring/spoke test, as in retail.
 *   Without -g both moves are hoisted above the test.
 * - Retail keeps 0.0f in f26, a hoisted loop invariant. loop.c only hoists a
 *   constant pseudo used more than once in the loop: with one use (the call
 *   argument) the "large loop" path puts the constant back into the use, giving
 *   mtc1 $zero,$f12 in the loop and no f26 save. Any second use that disappears
 *   after loop.c (here the dead store, deleted by flow's mem_set_list) gives
 *   retail's bytes exactly. Each component of velocity is stored once in retail.
 * Reopen with a source form whose second zero use is real code that later
 * passes remove (for example a vector set-then-adjust idiom from the original
 * source), not a store added for the compiler.
 * Declarations: the file's existing func_00282F20(void *, void *, float),
 * func_00283180(float) and func_00283198(float), plus those below.
 */
#include "common.h"

extern void func_00282F20(void *, void *, float);
extern float func_00283180(float);
extern float func_00283198(float);
extern float func_002A7878(float low, float high);
extern void func_00283098(float *out, float *in, char *matrix);
extern void func_L06_00463D40(char *moby, float *position, float *velocity,
                             int flags, float size, float life);

/* Emit three rings of particles around the moby's local frame. */
void func_L06_00402230(char *moby)
{
    float position[4];
    float velocity[4];
    int ring, spoke;
    ring = 0;
    do {
        spoke = 0;
        do {
            float shell, angle, size, life;
            if (ring != 0 || !(spoke & 1)) {
                shell = (float)ring + func_002A7878(0.0f, 1.0f);
                angle = 1.57079637f * ((float)spoke + (1.0f / 3.0f) * shell);
                velocity[0] = func_00283180(angle);
                velocity[1] = func_00283198(angle) * 0.25f;
                velocity[2] = 0.0f; /* dead store: the only remaining difference from landable C */
                velocity[2] = ((float)ring - 0.5f) * 0.25f;
                func_00282F20(velocity, velocity,
                              func_002A7878(8.0f, 14.0f) * (1.0f / 60.0f));
                func_00283098(velocity, velocity, moby + 0xC0);
                position[0] = *(float *)(moby + 0x10) + velocity[0];
                position[1] = *(float *)(moby + 0x14) + velocity[1] * 3.0f;
                position[2] = *(float *)(moby + 0x18) + 0.75f + shell * 0.5f;
                size = func_002A7878(0.25f, 1.0f);
                life = func_002A7878(0.25f, 1.25f);
                func_L06_00463D40(moby, position, velocity, 0, size, life);
            }
            spoke++;
        } while (spoke < 4);
        ring++;
    } while (ring < 3);
}

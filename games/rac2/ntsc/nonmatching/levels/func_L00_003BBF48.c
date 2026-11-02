/* GPT-6.1 Sol high v0557, 2026-10-28: USA v1.01, not linked.
 * -O2 -G0 -ffunction-sections -fno-sched-interblock: 312/312 bytes,
 * 0/27 places; four instructions differ. Disabling interblock scheduling
 * restores likely skip branches. Updating p before the counter and q restores
 * their retail slots. Explicit RGB initializers restore the whole prologue.
 * The two alpha shifts still write v0 instead of s0, so their following ORs
 * read v0 instead of s0. Signed color/return, unsigned color, and -fno-regmove
 * tie here; commuting OR operands reverses them without fixing allocation.
 * A long color adds conversions and grows to 336 bytes. The random helper's
 * signed int return agrees with its C definition in src/game/002A7750.c.
 * Next useful experiment needs evidence for the alpha/color pseudo lifetime.
 * All required declarations are below; this profile needs a separate unit.
 */
#include "common.h"

extern char D_L00_23DDA0[];

extern void func_L00_003BC228(char *a0, char *a1);

extern int func_002A7820(int a, int b);

extern float func_002A7940(void);

extern void func_00281EB8(char *a, int b, unsigned int c, int d, float e, float f, float g);

/* Walks the level's table of records and draws the two sprites of the record the owner
   holds, when that record is still free. The record's own field is the owner, the one
   after it says whether it is free. */
void func_L00_003BBF48(char *a0)
{
    unsigned int firstRGB = 0x103080;
    unsigned int secondRGB = 0xFFFFFF;
    char *base = D_L00_23DDA0;
    char *p = base + 0x30;
    char *q = base;
    int i;
    int color;
    float f;

    for (i = 29; i >= 0; p += 0xA0, i--, q += 0xA0) {
        if (*(int *)(p + 0x5C) == (int)a0) {
            func_L00_003BC228(a0, q);
            if (*(int *)(p + 0x64) == 0) {
                color = func_002A7820(0x50, 0x60);
                color = (int)(color << 24) | firstRGB;
                f = func_002A7940();
                func_00281EB8(p, 0x31, color, 1, 0.5f, 0.15f, f);
                color = func_002A7820(0x50, 0x60);
                color = (int)(color << 24) | secondRGB;
                f = func_002A7940();
                func_00281EB8(p, 0x31, color, 1, 0.3f, 0.151f, f);
            }
        }
    }
}

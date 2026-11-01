/* GPT-6.1 Sol high v0557, 2026-10-28: USA v1.01, not linked.
 * -O2 -G8 -ffunction-sections: 344/344 bytes, 0/27 places; three
 * instructions differ at +0x74..+0x7C. Direct index reads recover the two saved
 * upper address bases; the measured player struct recovers the third and
 * prevents folding its member offsets into the symbol's low half.
 * The first record address/load use v0 instead of a2, and the constant 1
 * moves after that load. Typed record fields and reading state first tie;
 * inlining both record loads regresses to 336 bytes. -fno-expensive-optimizations
 * on the pre-struct draft ties at 332 bytes. Needs an evidenced lifetime for
 * the first record pointer/condition; no address splitting tricks are used.
 * All required declarations are below; use a separate G8 unit.
 */
#include "common.h"

extern char D_00139568[];

typedef struct {
    char prefix[0x760];
    float vector[4];
    char pad770[0xBF8 - 0x770];
    int mode;
    char padBFC[0x24EC - 0xBFC];
    short state;
} Player37;
extern Player37 D_00189E20_13;

extern char D_L00_262570[];

/* A float in the boot's small data: the sda attribute and this unit's -G8 make the
   reference to it $gp-relative, the way retail has it. */
extern float D_001AFF08 __attribute__((sda));

extern void func_00282F20(void *a0, void *a1, float f);

extern void func_L00_003C3920(char *a0, char *a1, void *a2);

extern void func_L00_0032A1A8(int a0, char *a1, void *a2, float f);

extern float func_00282DF0(void *a0);

extern void *func_L00_003A98E8(char *a0, void *a1, char *a2, int a3, float f12, float f13, float f14);

void func_L00_003C37C8(char *a0, char *a1)
{
    char scratch[16];
    char *ctx = *(char **)(a0 + 0x68);
    char *rec;
    float f20;
    float f21;
    float f0;
    float f14;
    int mode;
    void *v;

    rec = D_L00_262570 + (unsigned char)D_00139568[0x19] * 0xE0;
    f0 = *(float *)(rec + 0x50);
    f20 = f0 * 0.016666667f;
    mode = D_00189E20_13.state;
    if (mode == 1)
        func_00282F20(scratch, a0 + 0xD0, -f20);
    else
        func_00282F20(scratch, D_00189E20_13.vector, f20);

    rec = D_L00_262570 + (unsigned char)D_00139568[0x19] * 0xE0;
    f21 = *(float *)(rec + 0x70);
    func_L00_003C3920(a0, a1, scratch);

    mode = D_00189E20_13.mode;
    if (mode != 0)
        func_L00_0032A1A8(mode, a1, scratch, f20);

    f0 = func_00282DF0(scratch);
    f14 = D_001AFF08;
    v = func_L00_003A98E8(a1, scratch, a0, 0, f0, f21, f14);
    *(int *)ctx = (int)v;
    if (v != 0) {
        *(int *)((char *)v + 0xB8) = (int)a0;
    }
}

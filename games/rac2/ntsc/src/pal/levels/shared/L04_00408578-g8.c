#include "include_asm.h"
typedef struct {
    unsigned char pad0[0x84];
    int f84;
    int f88;
    char *f8c;
    unsigned char pad1[0x15C - 0x90];
    int f15c;
    int f160;
    unsigned char *f164;
    unsigned char pad2[0x1A0 - 0x168];
    int f1a0;
    unsigned char pad3[0x1B0 - 0x1A4];
    float f1b0;
    unsigned char pad3b[4];
    int f1b8;
    unsigned char pad4[0x2D0 - 0x1BC];
    float f2d0;
} L04_004044B8_Record;
extern int func_002ACBC8(void *, int);
extern void func_002A7470(char *, int, int, int);
extern int D_001A7B90 __attribute__((sda));
extern int D_L04_001A8FE8;

/* Updates the record's flag from the two point tests, prints the message when the
   record has one and its counter runs, then advances the counter and the value. */
void func_PAL_L04_00408930(void *a0, L04_004044B8_Record *s0)
{
    char *s1 = (char *)a0 + 0x10;
    int v;

    if (func_002ACBC8(s1, s0->f88)) {
        s0->f15c = 1;
    }
    if (!func_002ACBC8(s1, s0->f84)) {
        return;
    }
    if (s0->f15c == 0) {
        return;
    }
    v = s0->f160;
    if (s0->f8c && v > 0) {
        func_002A7470(s0->f8c, 9, 0, 0xA);
        v = s0->f160;
    }
    if (v < 4) {
        float f1 = (float)s0->f1b8;
        float f2;
        float f0;

        if (D_001A7B90 != 0) {
            f2 = f1 * 0.02f;
        } else {
            f2 = f1 * 0.016666667f;
        }
        f0 = s0->f2d0 + s0->f1b0 - f2;
        s0->f2d0 = f0;
        if (f0 < 0.0f) {
            s0->f2d0 = 0.0f;
        }
        s0->f1b8 = 0;
        s0->f15c = 0;
        s0->f160 = s0->f160 + 1;
        if (s0->f160 != 3) {
            return;
        }
        if (*(unsigned char *)(s0->f164 + 0x20) == 2) {
            s0->f1a0 = D_L04_001A8FE8;
        }
    }
}

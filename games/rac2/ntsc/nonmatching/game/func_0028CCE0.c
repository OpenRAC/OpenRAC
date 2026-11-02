/* GPT-6.1 Sol high, 29 October 2026.
 * USA v1.01, -O2 -G8 -ffunction-sections -g: 1280/1284 bytes, 0/28 places.
 * The ordinary int cast emits retail's cvt.w.s at the corresponding offset
 * 0x1C0; no conversion helper is needed on the pinned compiler. Typed input
 * and state globals restore the prologue and base lifetimes; -g restores
 * the division padding. Directional input resets the debounce state, so
 * its test branches to reset_selection rather than skipping the reset.
 * Remaining: FP temporary allocation and modulo guard scheduling, the
 * offset-zero default-choice load moving into a GP branch slot, and
 * exit-state load/store scheduling and register allocation.
 * Sequential, factored-half-sector and combined FP forms tied. Retain this
 * simpler form; reopen with independent allocation/address-class evidence.
 * All needed declarations follow. No linked-build credit is claimed.
 */
#include "include_asm.h"

extern void func_00287E60(void);
extern float func_00282C10(float);
extern float func_00283248(float, float);
extern int func_002A9588(int, int, int, int, int);
extern int func_002E5E58(int, int, void *);
extern void func_00287E18(void);
extern int func_0028B5D0(int);
typedef struct { char pad0[0x148]; float x, y; char pad150[0x70]; int held, pressed, released, mode; } RadialInput;
extern RadialInput D_00138180;
typedef struct { char pad0[0x24E4]; int choice; } RadialState;
extern RadialState D_00189E20;
extern short D_001B189C, D_001B18A0, D_001B18A2, D_001B18A4;
extern int D_001B18D0 __attribute__((sda));
extern int D_001A8D48;
extern int **D_001B18D4;
typedef struct { int up, down, left, right; } RadialDefaults;
extern RadialDefaults D_001B8DB8 __attribute__((section(".sdata")));
extern unsigned char D_001A7BB3;
extern int D_001B18FC, D_001B1900, D_001B18F8, D_001B1598;

void func_0028CCE0(unsigned char *object)
{
    float x, y, magnitude, angle;
    int old, choice;
    unsigned char *fade = object + 0x70;
    func_00287E60();
    *(int *)(object + 0x7C) = 180;
    *(int *)(object + 0x6C) = 24;
    D_00138180.mode = 2;
    x = D_00138180.x;
    y = D_00138180.y;
    magnitude = func_00282C10(x * x + y * y);
    if (magnitude != 0.0f) {
        x /= magnitude;
        y /= magnitude;
    }
    angle = func_00283248(x, y);
    if ((*(int *)(object + 0x78) >> 24) == 0 && D_001B189C) {
        if (D_00138180.pressed & 0xF000) goto reset_selection;
        if (magnitude < 0.5f) goto reset_selection;
        --*(int *)(object + 0x78);
        if (*(int *)(object + 0x78) == -1) {
reset_selection:
            *(int *)(object + 0x74) = -1;
            *(int *)(object + 0x78) = 0x10000FF;
        }
    }
    old = *(int *)(object + 0x74);
    if (*(signed char *)(object + 0x7B) == 1) {
        if (magnitude > 0.0f) {
            if (magnitude > 0.9f) {
                float sectors = (float)D_001B18D0;
                float phase = angle + 3.14159274101257324219f;
                phase += 3.14159274101257324219f;
                phase += 1.57079637050628662109f;
                phase += 3.14159274101257324219f / sectors;
                phase *= 0.15915493667125701904f;
                *(int *)(object + 0x74) = (int)(phase * sectors) % D_001B18D0;
            }
        } else if (D_00138180.pressed & 0xF000) {
            choice = old;
            if (D_00138180.pressed & 0x1000) {
                choice = choice == -1 ? D_001B8DB8.left : D_001B18D4[D_001A8D48][choice * 7 + 4];
            }
            if (D_00138180.pressed & 0x4000) {
                choice = choice == -1 ? D_001B8DB8.right : D_001B18D4[D_001A8D48][choice * 7 + 5];
            }
            if (D_00138180.pressed & 0x8000) {
                choice = choice == -1 ? D_001B8DB8.up : D_001B18D4[D_001A8D48][choice * 7 + 2];
            }
            if (D_00138180.pressed & 0x2000) {
                choice = choice == -1 ? D_001B8DB8.down : D_001B18D4[D_001A8D48][choice * 7 + 3];
            }
            *(int *)(object + 0x74) = choice;
        }
    }
    if (*(int *)(object + 0x74) != old) {
        func_002A9588(0, 0, 1, 0, 1);
        fade[1] = 0;
        func_002E5E58(3, 0, 0);
    } else if (fade[1] < 8) ++fade[1];
    if (!(D_00138180.held & 0x10)) {
        if (D_00138180.released & 0x10) {
            int current = *(int *)(object + 0x74);
            if (current == -1) D_00189E20.choice = 0;
            else D_00189E20.choice = D_001B18D4[D_001A8D48][current * 7 + 6];
            if (D_001A7BB3) func_00287E18();
        }
        if (*fade) --*fade;
    } else if (*fade < 8) ++*fade;
    if (D_00189E20.choice == 6 && !D_001B18A0)
        D_00189E20.choice = 0;
    if (D_00189E20.choice == 7 && !D_001B18A4)
        D_00189E20.choice = 0;
    if (D_00189E20.choice == 5 && !D_001B18A2)
        D_00189E20.choice = 0;
    if (!*fade) {
        D_001B18FC = *(int *)(object + 0x78);
        D_001B1900 = *(int *)(object + 0x74);
        func_0028B5D0(*(int *)(object + 0x64));
        {
            int saved = D_001B1598;
            *(int *)(object + 0x6C) = -6;
            D_001B18F8 = saved;
        }
    }
}

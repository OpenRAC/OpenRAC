/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L05_00420788 (L05, 592 bytes): NOT matched. Compiles to 584 bytes in the default -G0 profile of
 * src/levels/05_feltzin_system/L05_00420360.c (needs common.h for SDATA). The control flow, the
 * absolute-read / $gp-write split (incomplete sda arrays for reads, SDATA() aliases for stores) and
 * the delay-slot $gp loads reproduce retail. Remaining differences: retail keeps the second
 * argument in s2 and the entity state in s1 (mine swaps them, which renumbers the saved registers
 * throughout), retail hoists the 0x1B2... mode load above the "-1" store, and the 1/2/3 selector
 * chain is branches in retail but a movn in mine. Tried: reading the selector's first word
 * through the scalar D_001B1350 (the second test then reloads it as retail does), moving the
 * count/table computations after the two calls (needed for the 0x30 frame).
 * Place in src/levels/05_feltzin_system/L05_00420360.c at the function's marker. */
#include "common.h"

extern s32 func_003062A0(u8 *a0);
extern void func_002FF9F8(s32 a0);
extern void func_L05_0041FC60(u8 *a0);
extern s32 D_001B1350 __attribute__((sda));
extern u8 *D_001B251C[] __attribute__((sda));

extern void func_L05_00420360(void);
extern u8 D_L05_00188660[];
extern s32 D_L05_001A8F00[] __attribute__((sda));
extern s32 D_L05_001A904C __attribute__((sda));
extern s32 D_L05_001B1350[3] SDATA(D_L05_001B1350);
extern s32 D_L05_001B1354[] __attribute__((sda));
extern s32 D_L05_001B1358[] __attribute__((sda));
extern s32 D_L05_001B13F0 SDATA(D_L05_001B13F0);
extern s32 D_L05_001B13EC SDATA(D_L05_001B13EC);
extern s32 D_L05_001B13F8[] SDATA(D_L05_001B13F8);
extern s32 D_L05_001B13A8 SDATA(D_L05_001B13A8);
extern s32 D_L05_001B13AC SDATA(D_L05_001B13AC);
extern s32 D_L05_001B3578[] __attribute__((sda));
extern s32 D_001A79F8_sda SDATA(D_001A79F8);
extern s32 D_001A79F8[] __attribute__((sda));
extern u8 D_001A7A0A[] __attribute__((sda));
extern s32 D_00138324[] __attribute__((sda));
extern void func_00371600(s32 a0);
extern void func_00324C00(s32 a0, void *a1);
extern void func_004222D8(void);
extern void func_00324828(s32 a0, s32 a1);
extern void func_00379F48(s32 a0);
extern void func_0033EE70(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4);

/* The level's per-frame menu step for one entity. */
void func_L05_00420788(u8 *a0, s32 a1)
{
    u8 *s0 = *(u8 **)(a0 + 0x68);
    u8 *e = D_001B251C[0] + (*(s32 *)(s0 + 0x24) << 8);
    u8 *es = *(u8 **)(e + 0x68);
    s32 idx = *(s32 *)(es + 0x270);

    if (idx != -1) {
        u8 *slot = D_L05_00188660 + idx * 0x70;

        if (*(u8 **)(slot + 0x88) == e && slot[0x74] != 0) {
            func_00371600(idx);
        }
        *(s32 *)(es + 0x270) = -1;
    }
    if (D_L05_001A8F00[0] == 2) {
        return;
    }
    if (func_003062A0(s0 + 0x58) == 0) {
        return;
    }
    if (D_L05_001A904C != 0 && D_L05_001B13F0 == 0) {
        s32 *cnt;
        s32 t0;
        s32 n;
        s32 v;

        func_002FF9F8(0x1E);
        func_00324C00(0, (void *)func_L05_00420360);
        cnt = &D_L05_001B1350[a1];
        t0 = *cnt;
        n = t0 < 3 ? t0 : 2;
        D_L05_001B13F0 = 1;
        D_L05_001B3578[0] = a1;
        D_L05_001B13AC = 0;
        v = D_L05_001B13F8[a1 * 3 + n];
        D_L05_001B13A8 = 0;
        if (D_001A7A0A[0] != 0) {
            v *= 10;
        }
        D_001A79F8_sda = D_001A79F8[0] + v;
        *cnt = t0 + 1;
        func_004222D8();
        func_00324828(0, -1);
        func_00379F48(1);
    }
    if (func_003062A0(s0 + 0x40) != 0) {
        *(s32 *)(s0 + 0x40) = 0x258;
    } else if (D_00138324[0] & 0x40) {
        s32 sel;

        D_L05_001B13F0 = 0;
        *(s32 *)(s0 + 0x40) = 0x1518;
        func_002FF9F8(0x1E);
        func_00379F48(0);
        *(s32 *)(s0 + 0x44) = 0x1C20;
        *(s32 *)(s0 + 0x58) = 0x78;
        if (D_L05_001B1350[0] != 0) {
            if (D_L05_001B1354[0] != 0 && D_L05_001B1358[0] != 0) {
                sel = 3;
            } else if (D_001B1350 != 0 && D_L05_001B1354[0] != 0) {
                sel = 2;
            } else {
                sel = 1;
            }
            D_L05_001B13EC = sel;
        }
        func_L05_0041FC60(a0);
    } else if (D_00138324[0] & 0x10) {
        func_0033EE70(3, 1, 4, 0, 0);
    } else {
        *(s32 *)(s0 + 0x40) = 0;
    }
}

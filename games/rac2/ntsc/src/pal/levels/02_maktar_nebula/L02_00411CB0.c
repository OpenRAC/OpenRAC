#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
extern void func_002A7750();
extern void func_002A0488(void *, void *, void *, void *);
extern void func_002A7470(void *, int, int, int);
extern float func_002A7878(float, float);
extern void func_002A7470(void *, int, int, int);
extern u8 D_001A7BCB __attribute__((sda));
extern u8 D_L02_001395B8[];
extern u8 D_L02_001C0700[];

/* resets the actor's aim: three records and the two shared limits */
void func_PAL_L02_00411CB0(u8 *a0, int a1) {
    u8 *s0 = *(u8 **)(a0 + 0x68);

    *(u16 *)(s0 + 0x1B8) = 0;
    *(float *)(s0 + 0x20) = (float)*(s16 *)(s0 + 0x24);
    func_002A7750(a0, 3, 1);
    if (*(u8 *)(a0 + 0x43) != 0)
        func_002A7470(a0, 0, 0, 8);
    *(u32 *)(a0 + 0x98) = *(u32 *)(*(u32 *)(a0 + 0x24) + 0x10);
    *(u8 *)(s0 + 0x2E) = 1;
    *(u8 *)(a0 + 0x7F) = 0;
    *(u8 *)(a0 + 0x31) = 1;
    *(u16 *)(a0 + 0x34) = (*(u16 *)(a0 + 0x34) | 0x1000) & 0xFFB8;
    *(float *)(s0 + 0x18C) = func_002A7878(0.0f, 90.0f) * 0.01745329424738884f;
    *(u16 *)(s0 + 0x1BA) = a1;
    *(u16 *)(s0 + 0x3C) = 0;
    *(u32 *)(s0 + 0x38) = 0;
    if (*(s16 *)(s0 + 0x50) != 0) {
        *(u16 *)(s0 + 0x50) = 0;
        func_002A0488(a0, *(u32 *)(s0 + 0x1A0), *(u32 *)(s0 + 0x1A4),
                      *(u32 *)(s0 + 0x1A8));
        *(u16 *)(s0 + 0x52) = 1;
    }
}

typedef signed int s32;
extern char D_L02_00188660[];
extern void func_0032C700(void *, int, int, int, int, int, int, int, float, float,
                          int, int, int, int);
extern void func_002E5DA0(int, int, void *, int);
extern void func_002A05A8(void *);
extern int func_002E59F8(int);
extern int func_L02_003FC1B0(void *, int);
extern s32 func_00288AA0(u8 *, s32);
extern s32 func_00288BE8(u8 *);

/* Lets go of the slot the actor holds (+0x19C) when that slot's record still names it and is live,
   ends the state at +0x1B8 when it is 1 (func_L02_003FC1B0), starts the effect pair when the actor
   is shown (+0x31), then clears the state and the shared fields as func_L02_0040B980 does. */
void func_PAL_L02_00411DB0(u8 *a0) {
    u8 *s1 = *(u8 **)(a0 + 0x68);
    s32 slot = *(s32 *)(s1 + 0x19C);

    if (slot != -1) {
        u8 *rec = (u8 *)D_L02_00188660 + slot * 0x70;

        if (*(u8 **)(rec + 0x88) == a0 && rec[0x74] != 0)
            func_002E59F8(slot);
    }
    *(s32 *)(s1 + 0x19C) = -1;
    if (*(s16 *)(s1 + 0x1B8) == 1) {
        *(u16 *)(s1 + 0x1B8) = 0;
        func_L02_003FC1B0(*(void **)(a0 + 0xB8), *(s16 *)(a0 + 0xAA));
    }
    if (a0[0x31] != 0) {
        func_0032C700(a0 + 0x10, 0, 0xA, 0x1E, 0, 0xA, 0x2D, 1, 0.3f, 0.0f, 0, 0, 0, 0);
        func_002E5DA0(0, 0, a0, 0xC3E);
    }
    *(u16 *)(s1 + 0x1B8) = 0;
    *(s32 *)(s1 + 0x184) = 0;
    func_002A7470(a0, 0, 0, 3);
    func_002A05A8(a0);
    func_002A7750(a0, 1, 0);
    a0[0x31] = 0;
    *(u32 *)(a0 + 0x98) = 0;
    *(u16 *)(a0 + 0x34) = (*(u16 *)(a0 + 0x34) & 0xEFFF) | 0x47;
}

extern char D_00189EA0[];
extern int func_002ACBC8(void *, int);
extern int func_002F5D10(void);
extern void func_002AD7B8(int);
extern void func_002AF0A8(int);
extern void func_0029F338(u8 *);

/* Steps the actor by its kind byte (+0x20). Kind 0 hands it to func_0029F338 when it holds no
   record at +0x68 and calls func_002A7750 with 1 otherwise. Kind 1 waits for D_001A7BCB and the
   byte D_L02_001395B8[4] to be clear and func_002ACBC8 to accept the index at +0xC of the
   record, then queues the messages 0x2D, 3 and 0x2E on D_L02_001C0700, sets the byte and calls
   func_002A7750 with 2. Kind 2 runs func_002AD7B8 and func_002AF0A8 with 3 and calls
   func_002A7750 with 3, unless func_002F5D10 answers yes. */
void func_PAL_L02_004147E8(u8 *a0) {
    u8 *state = *(u8 **)(a0 + 0x68);

    switch (a0[0x20]) {
    case 0:
        if (state == 0)
            func_0029F338(a0);
        else
            func_002A7750(a0, 1, -1);
        break;
    case 1:
        if (D_001A7BCB == 0 && D_L02_001395B8[4] == 0 &&
            func_002ACBC8(D_00189EA0, *(s32 *)(state + 0xC)) != 0 &&
            func_00288AA0(D_L02_001C0700, 0x2D) != 0) {
            func_00288AA0(D_L02_001C0700, 3);
            func_00288AA0(D_L02_001C0700, 0x2E);
            func_00288BE8(D_L02_001C0700);
            D_L02_001395B8[4] = 1;
            func_002A7750(a0, 2, -1);
        }
        break;
    case 2:
        if (func_002F5D10() == 0) {
            func_002AD7B8(3);
            func_002AF0A8(3);
            func_002A7750(a0, 3, -1);
        }
        break;
    }
}


#include "include_asm.h"

typedef unsigned char u8;
typedef signed int s32;
extern void func_002A7750();
extern u8 D_001A7BCB __attribute__((sda));
extern u8 D_L02_001395B8[];
extern u8 D_L02_001C0700[];
extern char D_00189EA0[];
extern int func_002ACBC8(void *, int);
extern s32 func_00288AA0(u8 *, s32);
extern s32 func_00288BE8(u8 *);
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
void func_US2_L02_004112C8(u8 *a0) {
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


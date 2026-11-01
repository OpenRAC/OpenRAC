#include "include_asm.h"
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { u8 b[0x2600]; TI128 q[0x260]; } Hero_003DB8E8;

extern u8 D_00189E20[];

/* the object last armed, the frame counter and the mode word */
extern u8 *D_L08_001B0488 __attribute__((sda));
extern s32 D_L08_001B2508;
extern s32 D_L08_001A8F00;

extern s32 func_L08_003DB4A0(void);
extern void func_L08_003DB518(void);
extern s32 func_L08_00318F90(s32, void *, s32);

/* Follows the hero; every tenth frame steps the shown count towards the live count, and hands
   the meter its update routine while there is anything to show. */
void func_L08_003DB8E8(u8 *self)
{
    s32 *vars = *(s32 **)(self + 0x68);
    s32 count;
    s32 shown;

    qcopy(self + 0x10, &((Hero_003DB8E8 *)D_00189E20)->q[8]);
    count = func_L08_003DB4A0();
    if (self[0x20] == 0) {
        self[0x20] = 1;
        vars[0] = count;
        vars[1] = 0;
        self[0x30] = 0xFF;
        D_L08_001B0488 = self;
    }
    if (D_L08_001B2508 % 10 == 0) {
        shown = vars[0];
        if (shown < count) {
            vars[1] = 0;
            vars[0] = shown + 1;
        } else if (count < shown) {
            vars[0] = shown - 1;
        }
    }
    if (count > 0 && D_L08_001A8F00 != 2 && (self[0x20] == 1 || self[0x20] == 2)) {
        self[0x20] = 2;
        func_L08_00318F90(0, func_L08_003DB518, 2);
        return;
    }
    if ((count <= 0 || D_L08_001A8F00 == 2) && self[0x20] == 2) {
        func_L08_00318F90(0, 0, 2);
        self[0x20] = 1;
    }
}
